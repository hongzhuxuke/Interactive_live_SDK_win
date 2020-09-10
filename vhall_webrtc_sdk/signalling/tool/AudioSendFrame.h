#pragma once
#include <iostream>
namespace vhall {
  class AudioSendFrame {
  public:
    AudioSendFrame() {};
    virtual ~AudioSendFrame() {};
    virtual void SendAudioData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel) = 0;

  };
}
