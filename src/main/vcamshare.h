#ifndef VXMT_VCAM_SHARE_PUBLIC
#define VXMT_VCAM_SHARE_PUBLIC
#include "stdint.h"

#ifdef __cplusplus
extern "C"
#endif
int createVideoMuxer(int w, int h, int videoFrameRate, const char* filePath);

#ifdef __cplusplus
extern "C"
#endif
void closeVideoMuxer(int hd);

#ifdef __cplusplus
extern "C"
#endif
int checkVideoMuxerError(int hd);

#ifdef __cplusplus
extern "C"
#endif
int writeVideoFrames(int hd, uint8_t * const data, int len);

#ifdef __cplusplus
extern "C"
#endif
int writeAudioFrames(int hd, uint8_t * const data, int len);

#ifdef __cplusplus
extern "C"
#endif
int writeRawAudioFrames(int hd, float * const data, int len, bool isMute);

#ifdef __cplusplus
extern "C"
#endif
void syncAudioDts(int hd);

#ifdef __cplusplus
extern "C"
#endif
int videoMuxerIsOpen(int hd);

#ifdef __cplusplus
extern "C"
#endif
int videoMuxerGetAudioSampleRate(int hd);

#ifdef __cplusplus
extern "C"
#endif
void videoMuxerPause(int hd);

#ifdef __cplusplus
extern "C"
#endif
void videoMuxerResume(int hd);


#endif
