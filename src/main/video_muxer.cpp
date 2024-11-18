
#include "video_muxer.h"
#include "utils.h"
#include <math.h>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavutil/timestamp.h>
#include <libavutil/mathematics.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

// #define STREAM_FRAME_RATE  30 /* 25 images/s */
#define STREAM_PIX_FMT     AV_PIX_FMT_YUV420P /* default pix_fmt */
#define AUDIO_BIT_RATE     128000
#define AUDIO_SAMPLE_RATE  44100

static constexpr int AUDIO_FRAME_SIZE = 1024;

static char *const get_error_text(const int error) {
    static char error_buffer[255];
    av_strerror(error, error_buffer, sizeof(error_buffer));
    return error_buffer;
}

namespace vcamshare {

    void VideoMuxer::logPacket(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {
        AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

        printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
            pkt->stream_index);
    }

    VideoMuxer::VideoMuxer(int w, int h, int videoFrameRate, std::string filePath) {
        mWidth = w;
        mHeight = h;
        mVideoFrameRate = videoFrameRate;
        mFilePath = filePath;
        outputCtx = nullptr;
        videoSt.enc = nullptr;
        videoSt.dts = 0;
        mLastAudioDts = 0;
        audioSt.enc = nullptr;
        audioSt.frame = nullptr;
        mPaused = false;
        mHasIDR = false;
        mFrameWritten = false;
        mError = false;
        mSyncAudioDts = false;

        mStopReadingThread = false;
        mFrameReadThread = std::thread([this] () {
            while(!mStopReadingThread) {
                std::vector<uint8_t> videoFrame;
                std::vector<float> audioFrame;
                {
                    std::unique_lock<std::mutex> l(mMutx);

                    if(!mVideoFramesQueue.empty()) {
                        videoFrame = std::move(mVideoFramesQueue.front());
                        mVideoFramesQueue.pop();
                    } else if (!mAudioRawFramesQueue.empty()) {
                        audioFrame = std::move(mAudioRawFramesQueue.front());
                        mAudioRawFramesQueue.pop();
                    } else {
                        mCv.wait(l);
                        continue;
                    }
                }

                if(!videoFrame.empty()) {
                    writeVideoFramesToFile(videoFrame.data(), videoFrame.size());
                }

                if(!audioFrame.empty()) {
                    writeRawAudioFramesToFile(audioFrame.data(), audioFrame.size());
                }
            }
        });
    }

    VideoMuxer::~VideoMuxer() {
        close();
    }

    void VideoMuxer::open(uint8_t *extraData, int extraLen) {
        if(outputCtx) return;

        int ret;

        std::cout << "video file: " << mFilePath << std::endl;
        mFrameWritten = false;
        mError = false;

        videoSt.dts = 0;
        audioSt.dts = 0;

        avformat_alloc_output_context2(&outputCtx, NULL, NULL, mFilePath.c_str());
        if (!outputCtx) {
            std::cerr << "Failed to open file: " << mFilePath << std::endl;
            goto end;
        }

        if(!addStream(&videoSt, outputCtx, &videoCodec, AV_CODEC_ID_H264, extraData, extraLen)) {
            goto end;
        }

        if(!addStream(&audioSt, outputCtx, &audioCodec, AV_CODEC_ID_AAC, nullptr, 0)) { //
            goto end;
        }

        // Support ADTS encoding
        if(!openAudioEncoder(audioCodec)) {
            goto end;
        }

        if(audioSt.enc) {
            mAudioRawBuffer.clear();
            mAudioRawBuffer.reserve(audioSt.enc->frame_size);
        } else {
            goto end;
        }

        av_dump_format(outputCtx, 0, mFilePath.c_str(), 1);

        if (!(outputCtx->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&outputCtx->pb, mFilePath.c_str(), AVIO_FLAG_WRITE);
            if (ret < 0) {
                std::cerr << "Could not open output context." << std::endl;
                goto end;
            }
        }

        ret = avformat_write_header(outputCtx, NULL);
        std::cout << "Header written: " << ret << std::endl;

        if (ret < 0) {
            char mess[256];
            av_strerror(ret, mess, 256);
            std::cerr << mess << std::endl;
            goto end;
        }

        return;

        end:
        std::cerr << "Something wrong in the end section." << std::endl;
        close();
    }
    
    bool VideoMuxer::isOpen() {
        return outputCtx != nullptr;
    }

    bool VideoMuxer::hasError() {
        return mError;
    }

    void VideoMuxer::close() {
        mStopReadingThread = true;
        {
            std::unique_lock<std::mutex> l(mMutx);
            mCv.notify_all();
        }

        if(mFrameReadThread.joinable()) {
            mFrameReadThread.join();    
        }

        if(outputCtx && mFrameWritten && !mError) {
            int rs = av_write_trailer(outputCtx);
            std::cout << "trailer written: " << rs << std::endl;
        }

        /* Close each codec. */
        if(videoSt.enc) {
            avcodec_free_context(&videoSt.enc);
            videoSt.enc = nullptr;
        }
        
        if(audioSt.enc) {
            avcodec_free_context(&audioSt.enc);
            audioSt.enc = nullptr;
        }

        if(audioSt.frame) {
            av_frame_free(&audioSt.frame);
            audioSt.frame = nullptr;
        }

        /* close output */
        if (outputCtx && !(outputCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&outputCtx->pb);
        }

        if(outputCtx) {
            avformat_free_context(outputCtx);
            outputCtx = nullptr;
        }
    }

    void VideoMuxer::pause() {
        mPaused = true;
        mHasIDR = false;
    }

    void VideoMuxer::resume() {
        mPaused = false;
    }

    int VideoMuxer::audioSampleRate() {
        if (audioSt.enc) {
            return audioSt.enc->sample_rate;
        }
        return 0;
    }

    std::vector<uint8_t> VideoMuxer::getSpsPps() {
        return mSpsPps;
    }

    uint8_t *VideoMuxer::fillSpsPps(uint8_t * const data, int len) {
        int nalType = data[4] & 0x1f;
        if (nalType != 7 && nalType != 8 && nalType != 6) {
            return data;
        }
        int max = len;
        uint8_t *start = data;
        uint8_t *head = nullptr;
        
        do {
            head = searchH264Head(start + 1, max - 1);
            if (head) {
                nalType = head[4] & 0x1f;

                max -= (head - start);
                start = head;
            } else {
                nalType = -1;
                break;
            }
        } while (nalType == 7 || nalType == 8 || nalType == 6);

        // Now the head points to the next I/P nal or null.
        int orgNalType = data[4] & 0x1f;
        if (orgNalType == 7 || orgNalType == 6 || orgNalType == 8) {
            int spsPpsLen = head == nullptr ? len : (head - data);
            mSpsPps.clear();
            for (int i = 0; i < spsPpsLen; i ++) {
                mSpsPps.push_back(data[i]);
            }
        }
        return head;
    }

    bool VideoMuxer::writeVideoFrames(uint8_t * const data, int len) {
        if(mPaused) return false;

        if(!mHasIDR) {
            mHasIDR = !vcamshare::isNonIDR(data);
            if(!mHasIDR) return false;
        }

        std::vector<uint8_t> d(data, data + len);

        {
            std::unique_lock<std::mutex> l(mMutx);
            mVideoFramesQueue.push(std::move(d));
            mCv.notify_all();
        }

        return true;
    }

    void VideoMuxer::syncAudioDts() {
        mSyncAudioDts = true;
    }

    bool VideoMuxer::writeRawAudioFrames(float * const rawData, int len, bool isMute) {
        if(mPaused) return false;
        if(!mHasIDR) return false;

        auto expectAudioDts = calculateAudioDtsFromVideoDts(videoSt.dts);
        if(audioSt.dts > expectAudioDts && isMute) {
            return false;
        }

        std::vector<float> d(rawData, rawData + len);

        {
            std::unique_lock<std::mutex> l(mMutx);
            mAudioRawFramesQueue.push(std::move(d));
            mCv.notify_all();
        }

        return true;        
    }

    bool VideoMuxer::writeAudioFrames(uint8_t * const data, int len) {
        if(mPaused) return false;
        if(!mHasIDR) return false;

        if(isOpen()) {
            return addFrames(data, len, false);
        }
        return false;
    }

    int64_t VideoMuxer::calculateAudioDtsFromVideoDts(int videoDts) {
        if(!isOpen()) return 0;
        float dts = 0.0;
        if (audioSt.enc != NULL && videoSt.enc != NULL ){
            dts = audioSt.enc->time_base.den * (float(videoDts) / videoSt.enc->time_base.den);
        }
        return dts;
    }

    bool VideoMuxer::writeVideoFramesToFile(uint8_t * const data, int len) {
        uint8_t *frame = fillSpsPps(data, len);

        if(!isOpen()) {
            if (mSpsPps.empty()) {
                return false;
            }
            open(mSpsPps.data(), mSpsPps.size());

            if(!isOpen()) {
                std::cerr << "Failed to open Muxer!" << std::endl;
            }
        }

        if(isOpen() && frame) {            
            // if (nalType == 5) {
            //     addFrames(mSpsPps.data(), mSpsPps.size(), true);    
            // }
            // addFrames(frame, len - (frame - data), true);
            
            return addFrames(data, len, true);
        }

        return false;
    }

    void VideoMuxer::fillAudioBuffer(float * const data, int len, int batchLen, std::function<void(float*)> cb) {
        float *p = data;
        int remain = len;
        
        while(remain > 0) {
            mAudioRawBuffer.push_back(*p);
            p ++;
            remain --;

            if (mAudioRawBuffer.size() == batchLen) {
                cb(mAudioRawBuffer.data());
                mAudioRawBuffer.clear();
            }
        }
    }

    bool VideoMuxer::writeRawAudioFramesToFile(float * const rawData, int len) {
        if(!isOpen()) return false;

        int frameSize = audioSt.enc->frame_size;
        int channels = audioSt.enc->channels;
        // std::cout << "frameSize: " << frameSize << std::endl;

        std::function<void(float* batchData)> cb = [this, frameSize, channels] (float* batchData) {
            AVFrame *frame = audioSt.frame;
            if (frame) {
                int ret = av_frame_make_writable(frame);
                // if (ret < 0) return;

                for(int c = 0; c < channels; c ++) {
                    float *frameData = (float *)frame->data[c];
                    memcpy(frameData, batchData, frameSize * sizeof(float));
                }

                AVPacket pkt = { 0 };
                encodeAudio(audioSt.enc, frame, &pkt, [this] (AVPacket *pkt) {
                    addFrames(pkt, false);
                });
                av_packet_unref(&pkt);
            }
        };

        fillAudioBuffer(rawData, len, frameSize, cb);
        return true;
    }

    bool VideoMuxer::addFrames(AVPacket *pkt, bool video) {
        if(mError) {
            return false;
        }

        OutputStream *stream = video ? &videoSt : &audioSt;

        if (video) {
            pkt->dts = pkt->pts = stream->dts;
        } else {
            if(mSyncAudioDts) {
                stream->dts = calculateAudioDtsFromVideoDts(videoSt.dts);

                mSyncAudioDts = false;
            }
            // 
            // auto drift = expectedDts - stream->dts;
            // auto delaySecond = float(drift) / audioSt.enc->time_base.den;
            // if(delaySecond > 1) {
            //     stream->dts = expectedDts;
            // }
            // std::cout << "audio dts drift: " << drift << " delay second:" << delaySecond << std::endl;
            pkt->dts = pkt->pts = stream->dts;
        }
        
        pkt->duration = 1;
        pkt->pos = -1;        

        if (video) {
            int nalType = pkt->data[4] & 0x1f;
            if (nalType != 1) {
                pkt->flags |= AV_PKT_FLAG_KEY;
            }
        }

        av_packet_rescale_ts(pkt, stream->enc->time_base, stream->st->time_base);
        pkt->stream_index = stream->st->index;

        // logPacket(outputCtx, pkt);

        int ret = av_interleaved_write_frame(outputCtx, pkt);
        if(ret == 0) {
            mFrameWritten = true;
        } else {
            mError = true;
        }

        stream->dts ++;
        return ret == 0;
    }

    bool VideoMuxer::addFrames(uint8_t * const data, int len, bool video) {
        int ret = -1;

        int append = video ? AV_INPUT_BUFFER_PADDING_SIZE : 0;
        uint8_t *avdata = static_cast<uint8_t *>(av_malloc(len + append));
        memset(avdata + len, 0, append);
        memcpy(avdata, data, len);

        AVPacket pkt = { 0 };
        int rs = av_packet_from_data(&pkt, (uint8_t *)avdata, len + AV_INPUT_BUFFER_PADDING_SIZE);
        if(rs == 0) {
            addFrames(&pkt, video);
        } else {
            std::cerr << "Failed to create AVPacket" << std::endl;
        }

        av_packet_unref(&pkt);
        return rs == 0;
    }

    bool VideoMuxer::addStream(OutputStream *ost, 
                                AVFormatContext *oc,
                                const AVCodec **codec,
                                enum AVCodecID codec_id,
                                uint8_t *extra,
                                int extra_len) {
        AVCodecContext *c;
        int i;

        /* find the encoder */
        // the muxer doesn't support h264 encoding.
        if (codec_id == AV_CODEC_ID_H264) {
            *codec = avcodec_find_decoder(codec_id);
        } else {
            *codec = avcodec_find_encoder(codec_id);
        }
        
        if (!(*codec)) {
            fprintf(stderr, "Could not find encoder for '%s'\n",
                    avcodec_get_name(codec_id));
            return false;
        }

        ost->st = avformat_new_stream(oc, NULL); //*codec
        if (!ost->st) {
            fprintf(stderr, "Could not allocate stream\n");
            return false;
        }
        ost->st->id = oc->nb_streams-1;
        c = avcodec_alloc_context3(*codec);

        if (!c) {
            fprintf(stderr, "Could not alloc an encoding context\n");
            return false;
        }
        ost->enc = c;

        switch ((*codec)->type) {
            case AVMEDIA_TYPE_AUDIO:
                std::cout << "add audio stream " << std::endl;
                c->bit_rate    = AUDIO_BIT_RATE;

                // The sample format must be float.
                c->sample_fmt  = (*codec)->sample_fmts ?
                                (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                c->sample_fmt = AV_SAMPLE_FMT_FLTP;

                // Find the supported sample rate
                c->sample_rate = 0;
                if ((*codec)->supported_samplerates) {
                    c->sample_rate = (*codec)->supported_samplerates[0];
                    for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                        if ((*codec)->supported_samplerates[i] == AUDIO_SAMPLE_RATE) {
                            c->sample_rate = AUDIO_SAMPLE_RATE;
                        }
                    }
                }
                if (c->sample_rate == 0) {
                    std::cerr << "Failed to get audio sample rate." << std::endl;
                    return false;
                }

                // Default channels.
                c->channel_layout = AV_CH_LAYOUT_STEREO;
                c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

                // Find supported channels.
                if ((*codec)->channel_layouts) {
                    c->channel_layout = (*codec)->channel_layouts[0];
                    for (i = 0; (*codec)->channel_layouts[i]; i++) {
                        if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_MONO)
                            c->channel_layout = AV_CH_LAYOUT_MONO;
                    }
                }
                c->channels = av_get_channel_layout_nb_channels(c->channel_layout);

                

                c->profile = FF_PROFILE_AAC_LOW; //FF_PROFILE_AAC_MAIN; //FF_PROFILE_AAC_LOW;
                c->thread_count = 2; // multi-core encoding.
                c->thread_type = FF_THREAD_SLICE; //FF_THREAD_FRAME;

                c->time_base = ost->st->time_base;
                break;

            case AVMEDIA_TYPE_VIDEO:
                std::cout << "add video stream " << std::endl;
                // avcodec_get_context_defaults3(c, *codec);
                c->codec_id = codec_id;

                c->bit_rate = 8000000;
                // c->sample_rate = 30;
                /* Resolution must be a multiple of two. */
                c->width    = mWidth;
                c->height   = mHeight;
                /* timebase: This is the fundamental unit of time (in seconds) in terms
                * of which frame timestamps are represented. For fixed-fps content,
                * timebase should be 1/framerate and timestamp increments should be
                * identical to 1. */
                ost->st->time_base = (AVRational){ 1, mVideoFrameRate };
                c->time_base       = ost->st->time_base;

                c->gop_size      = 30; /* emit one intra frame every twelve frames at most */
                c->pix_fmt       = STREAM_PIX_FMT;
                c->profile = FF_PROFILE_H264_BASELINE;
                if(extra) {
                    uint8_t *avextra = (uint8_t *)av_mallocz(extra_len + AV_INPUT_BUFFER_PADDING_SIZE);
                    memset(avextra, 0, extra_len + AV_INPUT_BUFFER_PADDING_SIZE);
                    memcpy(avextra, extra, extra_len);
                    c->extradata = avextra;
                    c->extradata_size = extra_len;
                }

                if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                    /* just for testing, we also add B-frames */
                    c->max_b_frames = 2;
                }
                if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                    /* Needed to avoid using macroblocks in which some coeffs overflow.
                    * This does not happen with normal video, it just happens here as
                    * the motion of the chroma plane does not match the luma plane. */
                    c->mb_decision = 2;
                }
                c->time_base       = ost->st->time_base;
                break;

            default:
                break;
        }

        /* Some formats want stream headers to be separate. */
        if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        int ret = avcodec_parameters_from_context(ost->st->codecpar, c);

        if (ret < 0) {
            fprintf(stderr, "Could not copy the stream parameters\n");
            return false;
        }

        return true;
    }

    bool VideoMuxer::openAudioEncoder(const AVCodec *codec) {
        AVCodecContext *c;
        int nb_samples;
        int ret;
        AVDictionary *opt = NULL;

        c = audioSt.enc;
        /* open it */
        av_dict_copy(&opt, NULL, 0);
        // av_dict_set(&opt, "-b:a", "128K", 0);
        
        ret = avcodec_open2(c, codec, &opt);        

        av_dict_free(&opt);
        if (ret < 0) {
            std::cerr << "Could not open audio codec" << std::endl;
            
            return false;
        }

        if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
            nb_samples = 10000;
        else
            nb_samples = c->frame_size;

        c->frame_size = nb_samples;
        c->time_base = (AVRational){ 1, c->sample_rate / c->frame_size };
        audioSt.st->time_base = audioSt.enc->time_base;

        audioSt.frame     = allocAudioFrame(c->sample_fmt, c->channel_layout,
                                        c->sample_rate, nb_samples);
        if(audioSt.frame == nullptr) {
            return false;
        }

        /* copy the stream parameters to the muxer */
        ret = avcodec_parameters_from_context(audioSt.st->codecpar, c);

        if (ret < 0) {
            std::cerr << "Could not copy the stream parameters" << std::endl;
            return false;
        }

        return true;
    }

    AVFrame *VideoMuxer::allocAudioFrame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples) {
        AVFrame *frame = av_frame_alloc();
        int ret;

        if (!frame) {
            fprintf(stderr, "Error allocating an audio frame\n");
            return nullptr;
        }

        frame->format = sample_fmt;
        frame->channel_layout = channel_layout;
        frame->sample_rate = sample_rate;
        frame->nb_samples = nb_samples;

        // std::cout << "sample_fmt:" << sample_fmt << std::endl;
        // std::cout << "channel_layout:" << channel_layout << std::endl;
        // std::cout << "sample_rate:" << sample_rate << std::endl;
        // std::cout << "nb_samples:" << nb_samples << std::endl;

        if (nb_samples) {
            ret = av_frame_get_buffer(frame, 0);
            if (ret < 0) {
                fprintf(stderr, "Error allocating an audio buffer\n");
                return nullptr;
            }
        }

        return frame;
    }

    bool VideoMuxer::encodeAudio(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
                std::function<void(AVPacket*)> callback) {
        int ret;

        /* send the frame for encoding */
        ret = avcodec_send_frame(ctx, frame);
        if (ret < 0) {
            char * errorMess = get_error_text(ret);
            std::cerr << "Error encoding Audio:" << errorMess << std::endl;
            return false;
        }

        /* read all the available output packets (in general there may be any
        * number of them */
        while (ret >= 0) {
            ret = avcodec_receive_packet(ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return true;
            else if (ret < 0) {
                std::cerr << "Error encoding audio frame." << std::endl;
                return false;
            }
            callback(pkt);
            av_packet_unref(pkt);
        }
        return true;
    }
}
