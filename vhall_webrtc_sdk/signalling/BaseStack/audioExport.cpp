#include "audioExport.h"
#include "common/vhall_log.h"

namespace vhall {
  
  AudioExport::AudioExport() {
    mListener = nullptr;
  }
  AudioExport::~AudioExport() {
    LOGD("");
  }
  void AudioExport::OnData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames) {
    std::unique_lock<std::mutex> lock(mMutex);
    if (mListener) {
      mListener->SendAudioData(audio_data, bits_per_sample, sample_rate, number_of_channels, number_of_frames);
    }
    lock.unlock();
    return;
  }
  void AudioExport::SetAudioCallback(AudioSendFrame* listener) {
    LOGD("%s", __FUNCTION__);
    std::unique_lock<std::mutex> lock(mMutex);
    mListener = listener;
    lock.unlock();
    LOGD("end %s", __FUNCTION__);
  }
}