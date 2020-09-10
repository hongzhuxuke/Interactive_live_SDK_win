#pragma once
#include <vector>
#include <mutex>
#include <unordered_map>
#include <iostream>

//#include "audio/audio_level.h"
#include "common/vhall_define.h"
//#include "common_audio/resampler/include/push_resampler.h"
//#include "modules/audio_device/include/audio_device.h"
//#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_device/include/audio_device_defines.h"
//#include "modules/audio_processing/typing_detection.h"
//#include "rtc_base/constructor_magic.h"
//#include "rtc_base/critical_section.h"
#include "api/scoped_refptr.h"
//#include "rtc_base/thread_annotations.h"

namespace vhall {
  class AudioSendFrame;

  /* AudioTransport From ADM */
  class /*__declspec(dllexport)*/ AudioDataFromCapture : public webrtc::AudioTransport {
  public:
    AudioDataFromCapture();
    virtual ~AudioDataFromCapture();
    virtual int32_t RecordedDataIsAvailable(
      const void* audioSamples,
      const size_t nSamples,
      const size_t nBytesPerSample,
      const size_t nChannels,
      const uint32_t samplesPerSec,
      const uint32_t totalDelayMS,
      const int32_t clockDrift,
      const uint32_t currentMicLevel,
      const bool keyPressed,
      uint32_t& newMicLevel) override;

    virtual int32_t NeedMorePlayData(
      const size_t nSamples,
      const size_t nBytesPerSample,
      const size_t nChannels,
      const uint32_t samplesPerSec,
      void* audioSamples,
      size_t& nSamplesOut,
      int64_t* elapsed_time_ms,
      int64_t* ntp_time_ms) override { return 0; };

    // Method to pull mixed render audio data from all active VoE channels.
    // The data will not be passed as reference for audio processing internally.
    virtual void PullRenderData(
      int bits_per_sample,
      int sample_rate,
      size_t number_of_channels,
      size_t number_of_frames,
      void* audio_data,
      int64_t* elapsed_time_ms,
      int64_t* ntp_time_ms) override {};
    void SetAudioCallback(AudioSendFrame* listener);
  public:
    AudioSendFrame * mListener = nullptr;
    std::mutex       mMutex;
  };
}