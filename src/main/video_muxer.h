#ifndef VXMT_VCAM_SHARE_VIDEO_MUXER
#define VXMT_VCAM_SHARE_VIDEO_MUXER

#include <iostream>
#include <vector>
#include <queue>
#include <thread>

extern "C" {
#include <libavutil/timestamp.h>
#include <libavutil/mathematics.h>
#include <libavutil/frame.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace vcamshare {

    typedef struct OutputStream {
        AVStream *st;
        AVCodecContext *enc;

        AVFrame *frame;
        int dts;
    } OutputStream;

    class VideoMuxer {
    public:
        VideoMuxer(int w, int h, int videoFrameRate, std::string filePath);
        ~VideoMuxer();

        void pause();
        void resume();

        // expected data stream 00 00 00 01 xx xx xx xx
        bool writeVideoFrames(uint8_t * const data, int len);

        // expected data stram ADTS
        bool writeAudioFrames(uint8_t * const data, int len);
        bool writeRawAudioFrames(float * const data, int len, bool isMute);
        void syncAudioDts();

        bool hasError();

        void close();
        bool isOpen();
        int audioSampleRate();

        uint8_t *fillSpsPps(uint8_t * const data, int len);
        std::vector<uint8_t> getSpsPps();
    private:
        void logPacket(const AVFormatContext *fmt_ctx, const AVPacket *pkt);

        void open(uint8_t *extraData, int extraLen);
        bool writeVideoFramesToFile(uint8_t * const data, int len);
        bool writeRawAudioFramesToFile(float * const data, int len);

        bool addFrames(uint8_t * const data, int len, bool video);
        bool addFrames(AVPacket *pkt, bool video);
        bool addStream(OutputStream *ost, AVFormatContext *oc,
                            const AVCodec **codec,
                            enum AVCodecID codec_id,
                            uint8_t *extra,
                            int extra_len);
        bool openAudioEncoder(const AVCodec *codec);
        bool encodeAudio(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
                std::function<void(AVPacket *)> callback);
        void fillAudioBuffer(float * const data, int len, int batch, std::function<void(float*)>);
        AVFrame *allocAudioFrame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples);
        int64_t calculateAudioDtsFromVideoDts(int videoDts);

        AVFormatContext *outputCtx;
        const AVCodec *videoCodec;
        const AVCodec *audioCodec;
        OutputStream videoSt;
        OutputStream audioSt;

        int mWidth, mHeight;
        std::string mFilePath;

        std::vector<uint8_t> mSpsPps;
        std::vector<float> mAudioRawBuffer;

        std::queue<std::vector<uint8_t>> mVideoFramesQueue;
        std::queue<std::vector<float>> mAudioRawFramesQueue;
        std::thread mFrameReadThread;
        bool mStopReadingThread;
        std::condition_variable mCv;
        std::mutex mMutx;
        int64_t mLastAudioDts;
        bool mPaused, mHasIDR, mFrameWritten, mError, mSyncAudioDts;
        int mVideoFrameRate;

    };
}

#endif