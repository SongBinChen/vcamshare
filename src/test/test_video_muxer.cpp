#define BOOST_TEST_MODULE Test vCamShare Library
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

#include <boost/test/included/unit_test.hpp>
#include "../main/video_muxer.h"
#include "../main/utils.h"
#include "../main/vcamshare.h"


static void readH264File(std::string filePath, std::function<void(uint8_t*, int)> frameHandler) {
  std::ifstream source;
  source.open(filePath);
  if(source) {
    // std::cout << filePath << " opened" << std::endl;
    std::string line;

    while (std::getline(source, line)) {
      using namespace std;
      char space_char = ' ';
      vector<uint8_t> values {};

      stringstream sstream(line);
      string value;
      while (std::getline(sstream, value, space_char)){
          if(!value.empty()) {
            int v = stoi(value);
            if (v >= 0 && v <= 255) {
              values.push_back(v);
            } else {
              cerr << "value is out of range: " << v << endl;
            }
          }
          value.clear();
      }

      if(frameHandler) {
        frameHandler(values.data(), values.size());
      }
    }
  } else {
    std::cerr << "Failed to open " << filePath << std::endl;
  }
}

static void readAudioFile(std::string filePath, std::function<void(float*, int)> frameHandler) {
  std::ifstream source;
  source.open(filePath);
  if(source) {
    std::string line;

    while (std::getline(source, line)) {
      using namespace std;
      char space_char = ' ';
      vector<float> values {};

      stringstream sstream(line);
      string value;
      while (std::getline(sstream, value, space_char)){
          if(!value.empty()) {
            float v = stof(value);
            values.push_back(v);
          }
          value.clear();
      }

      if(frameHandler) {
        frameHandler(values.data(), values.size());
      }
    }
  } else {
    std::cerr << "Failed to open " << filePath << std::endl;
  }
}



BOOST_AUTO_TEST_SUITE(MuxerTest)

BOOST_AUTO_TEST_CASE(mt_muxing_test)
{
  const std::string target = "/tmp/hdpro.mp4";
  const std::string source = "hdpro.h264";
  int hd = createVideoMuxer(1920, 1080, 30, target.c_str());

  readH264File(source, [hd] (uint8_t *data, int len) {
    int rs = writeVideoFrames(hd, data, len);
  });

  closeVideoMuxer(hd);
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(MuxerPauseTest)

BOOST_AUTO_TEST_CASE(mt_muxing_pause_test)
{
  const std::string target = "/tmp/drain.mp4";
  const std::string source = "drain.h264";
  int hd = createVideoMuxer(1920, 1080, 30, target.c_str());

  int frameIdx = 0;
  readH264File(source, [hd, &frameIdx] (uint8_t *data, int len) {
    if(frameIdx == 50) {
      videoMuxerPause(hd);
      std::cout << "Video paused !!!!!!!!!!!!!!!!!!" << std::endl;
    } else if(frameIdx == 120) {
      videoMuxerResume(hd);
      std::cout << "Video resumed !!!!!!!!!!!!!!!!!" << std::endl;
    }

    int rs = writeVideoFrames(hd, data, len);
    frameIdx ++;

    std::cout << "writeVideoFrame: " << rs << " Frame Index: " << frameIdx << " nalType: " << (data[4] & 0x1f) << std::endl;    
  });

  closeVideoMuxer(hd);
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(UtilsTest)

BOOST_AUTO_TEST_CASE(muxing_searchSpsPps_return_ok)
{
  using namespace vcamshare;
  VideoMuxer muxer {1920, 1080, 30, "/tmp/not_a_file"};
  uint8_t d1[] = {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 1, 9};
  uint8_t *next = muxer.fillSpsPps(d1, boost::range_detail::array_size(d1));

  BOOST_TEST(next - d1 == boost::range_detail::array_size(d1) - 5);
  BOOST_TEST(next[4] == 9);

  uint8_t d2[] = {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 1, 2, 3, 1};
  uint8_t *next2 = muxer.fillSpsPps(d2, boost::range_detail::array_size(d2));

  BOOST_TEST(next2 == nullptr);

  uint8_t d3[] = {0, 0, 0, 1, 3, 6, 2, 2, 2, 2, 2, 1, 2, 3, 1, 5};
  uint8_t *next3 = muxer.fillSpsPps(d3, boost::range_detail::array_size(d3));
  
  BOOST_TEST(nullptr != next3);
  BOOST_TEST(d3 == next3);

  uint8_t d4[] = {0, 0, 0, 1, 7, 6, 2, 0, 0, 0, 1, 8, 2, 3, 1, 5, 0, 0, 0, 1, 6, 2, 3, 1, 5, 0, 0, 0, 1, 9, 2, 3, 1, 5};
  uint8_t *next4 = muxer.fillSpsPps(d4, boost::range_detail::array_size(d4));
  BOOST_TEST(next4[4] == 9);
}

BOOST_AUTO_TEST_CASE(muxing_searchSpsPps_spspps_filled)
{
  using namespace vcamshare;
  VideoMuxer muxer {0, 0, 0, "/tmp/not_a_file"};
  uint8_t d1[] =    {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 1, 9};
  uint8_t d1Exp[] = {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2};

  muxer.fillSpsPps(d1, boost::range_detail::array_size(d1));  
  auto spspps = muxer.getSpsPps();
  std::vector<uint8_t> expected(d1Exp, d1Exp + boost::range_detail::array_size(d1Exp));
  
  BOOST_TEST(expected == spspps);
}

BOOST_AUTO_TEST_CASE(muxing_searchSpsPps_spspps_filled_2)
{
  using namespace vcamshare;
  VideoMuxer muxer {0, 0, 0, "/tmp/not_a_file"};
  uint8_t d1[] =    {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 1, 8, 2, 2, 0, 0, 0, 1, 6, 2, 3, 0, 0, 0, 1, 9};
  uint8_t d1Exp[] = {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 1, 8, 2, 2, 0, 0, 0, 1, 6, 2, 3};

  muxer.fillSpsPps(d1, boost::range_detail::array_size(d1));  
  auto spspps = muxer.getSpsPps();
  std::vector<uint8_t> expected(d1Exp, d1Exp + boost::range_detail::array_size(d1Exp));
  
  BOOST_TEST(expected == spspps);
}

BOOST_AUTO_TEST_CASE(muxing_searchSpsPps_spspps_filled_3)
{
  using namespace vcamshare;
  VideoMuxer muxer {0, 0, 0, "/tmp/not_a_file"};
  uint8_t d1[] =    {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 1, 8, 2, 2, 0, 2, 0, 1, 2};
  uint8_t d1Exp[] = {0, 0, 0, 1, 7, 6, 2, 2, 2, 2, 2, 0, 0, 0, 1, 8, 2, 2, 0, 2, 0, 1, 2};

  muxer.fillSpsPps(d1, boost::range_detail::array_size(d1));  
  auto spspps = muxer.getSpsPps();
  std::vector<uint8_t> expected(d1Exp, d1Exp + boost::range_detail::array_size(d1Exp));

  BOOST_TEST(expected == spspps);
}

BOOST_AUTO_TEST_CASE(searchH264Head)
{
    uint8_t data1[] = {0, 0, 0, 1, 2};
    uint8_t *rs = vcamshare::searchH264Head(data1, boost::range_detail::array_size(data1));
    BOOST_TEST(rs[4] == 2);

    uint8_t data2[] = {1, 4, 3, 0, 0, 0, 1, 2};
    uint8_t *rs2 = vcamshare::searchH264Head(data2, boost::range_detail::array_size(data2));
    BOOST_TEST(rs2 - data2 == 3);
}

BOOST_AUTO_TEST_CASE(searchH264Head_notFound)
{
  uint8_t data[] = {1, 4, 3, 0, 0, 1, 1, 2};
  uint8_t *rs = vcamshare::searchH264Head(data, boost::range_detail::array_size(data));
  BOOST_TEST(rs == nullptr);
}

BOOST_AUTO_TEST_CASE(nonIDR_work)
{
  uint8_t data[] = {0, 0, 0, 1, 1, 1, 1, 2};
  auto rs = vcamshare::isNonIDR(data);

  BOOST_TEST(rs == true);
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(vCamShareTest)

BOOST_AUTO_TEST_CASE(isOpen)
{
  int hd = createVideoMuxer(1920, 1080, 30, "/tmp/t.mp4");
  BOOST_TEST(hd > 0);

  BOOST_TEST(videoMuxerIsOpen(hd) == 0);

  uint8_t data[] = {0, 0, 0, 1, 7, 6, 2, 0, 0, 0, 1, 8, 2, 3, 1, 5};
  writeVideoFrames(hd, data, boost::range_detail::array_size(data));

  BOOST_TEST(videoMuxerIsOpen(hd) == 1);
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(AudioTest)

BOOST_AUTO_TEST_CASE(muxingAudio)
{
  const std::string target = "/tmp/audio2.mp4";
  const std::string audioSource = "android_audio.raw";
  const std::string videoSource = "mt.h264";
  int hd = createVideoMuxer(1920, 1080, 30, target.c_str());


  readH264File(videoSource, [hd] (uint8_t *data, int len) {
    int rs = writeVideoFrames(hd, data, len);
  });

  readAudioFile(audioSource, [hd] (float *data, int len) {
    int rs = writeRawAudioFrames(hd, data, len, false);
  });

  closeVideoMuxer(hd);
}


BOOST_AUTO_TEST_CASE(muxingAudioWithPause)
{
  const std::string target = "/tmp/audio2.mp4";
  const std::string audioSource = "android_audio.raw";
  const std::string videoSource = "mt.h264";
  int hd = createVideoMuxer(1920, 1080, 30, target.c_str());


  std::thread videoThread([hd, videoSource]() {
    int frameIdx = 0;
    readH264File(videoSource, [hd, &frameIdx] (uint8_t *data, int len) {
      if(frameIdx == 10) {
        videoMuxerPause(hd);
        std::cout << "Video Paused!!!!!!!!!!!!!!!!" << std::endl;
      } else if(frameIdx == 20) {
        videoMuxerResume(hd);
        std::cout << "Video Resumed!!!!!!!!!!!!!!!!" << std::endl;
      }

      int rs = writeVideoFrames(hd, data, len);
      frameIdx ++;

      // std::cout << "writeVideoFrames: " << rs << std::endl;
    });
  });

  std::thread audioThread([hd, audioSource]() {
    readAudioFile(audioSource, [hd] (float *data, int len) {
      int rs = writeRawAudioFrames(hd, data, len, false);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::cout << "writeRawAudioFrames: " << rs << std::endl;
    });
  });
  

  videoThread.join();
  audioThread.join();

  closeVideoMuxer(hd);
}

BOOST_AUTO_TEST_CASE(getAudioSampleRate)
{
  const std::string target = "/tmp/drain.mp4";
  const std::string source = "drain.h264";
  int hd = createVideoMuxer(1920, 1080, 30, target.c_str());

  readH264File(source, [hd] (uint8_t *data, int len) {
    if (videoMuxerIsOpen(hd)) {
      return;
    }
    writeVideoFrames(hd, data, len);
  });

  int sampleRate = videoMuxerGetAudioSampleRate(hd);
  closeVideoMuxer(hd);

  BOOST_TEST(sampleRate == 48000);
}

BOOST_AUTO_TEST_SUITE_END()
