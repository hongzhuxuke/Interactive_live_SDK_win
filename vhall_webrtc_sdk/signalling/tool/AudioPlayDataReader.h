#pragma once
#include <vector>
#include "audio/audio_level.h"
#include "common_audio/resampler/include/push_resampler.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_processing/include/audio_processing.h"
#include "modules/audio_processing/typing_detection.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/critical_section.h"
#include "api/scoped_refptr.h"
#include "rtc_base/thread_annotations.h"
#include "3rdparty/vhall_webrtc_include/modules/audio_device/include/audio_device_defines.h"
#include "AudioSendFrame.h"
#include <mutex>

namespace webrtc {

  class AudioPlayDataReader : public AudioTransport {
  public:
    AudioPlayDataReader();
    virtual ~AudioPlayDataReader();
    bool InitInputFile(std::string filePath, const uint32_t samplesPerSec, const size_t nChannels);
    void Stop();
    void SetAudioListener(vhall::AudioSendFrame* listener);
    int32_t RecordedDataIsAvailable(const void* audioSamples,
      const size_t nSamples,
      const size_t nBytesPerSample,
      const size_t nChannels,
      const uint32_t samplesPerSec,
      const uint32_t totalDelayMS,
      const int32_t clockDrift,
      const uint32_t currentMicLevel,
      const bool keyPressed,
      uint32_t& newMicLevel) override;

    int32_t NeedMorePlayData(const size_t nSamples,
      const size_t nBytesPerSample,
      const size_t nChannels,
      const uint32_t samplesPerSec,
      void* audioSamples,
      size_t& nSamplesOut,
      int64_t* elapsed_time_ms,
      int64_t* ntp_time_ms) override;

    void PullRenderData(int bits_per_sample,
      int sample_rate,
      size_t number_of_channels,
      size_t number_of_frames,
      void* audio_data,
      int64_t* elapsed_time_ms,
      int64_t* ntp_time_ms) override;
    int Resample(const AudioFrame& frame, const int destination_sample_rate, PushResampler<int16_t>* resampler, int16_t* destination);
  private:
    FILE *   mAudioSource = nullptr;
    std::mutex mMtx;
    vhall::AudioSendFrame* mListener = nullptr;
    uint32_t mSamplesPerSec;
    size_t   mChannels;
    PushResampler<int16_t> mRenderResampler;
  };
}