#include "AudioDataFromCapture.h"
#include "common/vhall_log.h"
#include "AudioSendFrame.h"

namespace vhall {
  AudioDataFromCapture::AudioDataFromCapture() {

  }
  AudioDataFromCapture::~AudioDataFromCapture() {

  }

  int32_t AudioDataFromCapture::RecordedDataIsAvailable(const void * audioSamples, const size_t nSamples, const size_t nBytesPerSample, const size_t nChannels, const uint32_t samplesPerSec, const uint32_t totalDelayMS, const int32_t clockDrift, const uint32_t currentMicLevel, const bool keyPressed, uint32_t & newMicLevel) {
    std::unique_lock<std::mutex> lock(mMutex);
    if (mListener) {
        mListener->SendAudioData(audioSamples, nBytesPerSample * 8, samplesPerSec, nChannels, nSamples);
    }
    return 0;
  }

  void AudioDataFromCapture::SetAudioCallback(AudioSendFrame * listener) {
    std::unique_lock<std::mutex> lock(mMutex);
    mListener = listener;
  }
}