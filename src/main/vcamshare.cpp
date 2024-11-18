#include "vcamshare.h"
#include "video_muxer.h"
#include <map>
#include <memory>
#include <iostream>

static int gHandler = 1;

static std::map<int, std::unique_ptr<vcamshare::VideoMuxer>> gVideoMuxers;

int createVideoMuxer(int w, int h, int videoFrameRate, const char* filePath) {
    auto p = std::unique_ptr<vcamshare::VideoMuxer>(new vcamshare::VideoMuxer(w, h, videoFrameRate, filePath));

    gVideoMuxers[gHandler] = std::move(p);
    auto hd = gHandler;
    gHandler ++;
    return hd;
}

int checkVideoMuxerError(int hd) {
    if(gVideoMuxers[hd]) {
        if(gVideoMuxers[hd]->hasError()) {
            return 1;
        }
    }
    return 0;
}


void closeVideoMuxer(int hd) {
    gVideoMuxers[hd] = nullptr;
}

int writeVideoFrames(int hd, uint8_t * const data, int len) {
    if(gVideoMuxers[hd]) {
        return gVideoMuxers[hd]->writeVideoFrames(data, len) ? 1 : 0;
    } else {
        std::cerr << "muxer not found!" << std::endl;
    }
    return 0;
}

int writeRawAudioFrames(int hd, float * const data, int len, bool isMute) {
    if(gVideoMuxers[hd]) {
        return gVideoMuxers[hd]->writeRawAudioFrames(data, len, isMute) ? 1 : 0;
    } else {
        std::cerr << "muxer not found!" << std::endl;
    }
    return 0;
}

void syncAudioDts(int hd) {
    if(gVideoMuxers[hd]) {
        return gVideoMuxers[hd]->syncAudioDts();
    } else {
        std::cerr << "muxer not found!" << std::endl;
    }
}

int writeAudioFrames(int hd, uint8_t * const data, int len) {
    if(gVideoMuxers[hd]) {
        return gVideoMuxers[hd]->writeAudioFrames(data, len) ? 1 : 0;
    } else {
        std::cerr << "muxer not found!" << std::endl;
    }
    return 0;
}

int videoMuxerIsOpen(int hd) {
    if(gVideoMuxers[hd]) {
        return gVideoMuxers[hd]->isOpen() ? 1 : 0;
    }
    return 0;
}

int videoMuxerGetAudioSampleRate(int hd) {
    if(gVideoMuxers[hd]) {
        return gVideoMuxers[hd]->audioSampleRate();
    }
    return 0;
}

void videoMuxerPause(int hd) {
    if(gVideoMuxers[hd]) {
        gVideoMuxers[hd]->pause();
    }
}

void videoMuxerResume(int hd) {
    if(gVideoMuxers[hd]) {
        gVideoMuxers[hd]->resume();
    }
}