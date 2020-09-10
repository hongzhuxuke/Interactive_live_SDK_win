#pragma once
#include "3rdparty/vhall_webrtc_include/api/media_stream_interface.h"
#include <mutex>
#include <list>
#include "common/vhall_define.h"
#include "../tool/AudioSendFrame.h"

namespace vhall {
  class AudioExport :public webrtc::AudioTrackSinkInterface {
  public:
    AudioExport();
    virtual ~AudioExport();
    void OnData(const void* audio_data,
                int bits_per_sample,
                int sample_rate,
                size_t number_of_channels,
                size_t number_of_frames) override;
    void SetAudioCallback(AudioSendFrame* listener);
  public:
    std::mutex                               mMutex;
    AudioSendFrame*                          mListener;
  };
}