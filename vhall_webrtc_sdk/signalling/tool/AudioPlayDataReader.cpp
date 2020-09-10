#include "AudioPlayDataReader.h"
#include "../../common/vhall_log.h"
#include "audio/audio_transport_impl.h"
#include <errno.h>

namespace webrtc {
  AudioPlayDataReader::AudioPlayDataReader() {
    LOGD("%s", __FUNCTION__);
  }
  AudioPlayDataReader::~AudioPlayDataReader() {
    LOGD("%s", __FUNCTION__);
  }

  bool AudioPlayDataReader::InitInputFile(std::string filePath, const uint32_t samplesPerSec, const size_t nChannels) {
    LOGD("%s", __FUNCTION__);
    int maxio = _getmaxstdio();
    mAudioSource = fopen(filePath.c_str(), "rb");
    if (!mAudioSource) {
      LOGE("openFile failed.");
      LOGE("open fail errno = %d", GetLastError());
      LOGE("open fail errno = %d", errno);

      return false;
    }
    mSamplesPerSec = samplesPerSec;
    mChannels = nChannels;
    return true;
  }

  void AudioPlayDataReader::Stop() {
    LOGD("%s", __FUNCTION__);
    if (mAudioSource) {
      fclose(mAudioSource);
      mAudioSource = nullptr;
    }
  }

  void AudioPlayDataReader::SetAudioListener(vhall::AudioSendFrame * listener)
  {
    std::unique_lock<std::mutex> lock(mMtx);
    mListener = listener;
  }

  int32_t AudioPlayDataReader::RecordedDataIsAvailable(const void * audioSamples, const size_t nSamples, const size_t nBytesPerSample, const size_t nChannels, const uint32_t samplesPerSec, const uint32_t totalDelayMS, const int32_t clockDrift, const uint32_t currentMicLevel, const bool keyPressed, uint32_t & newMicLevel) {
    return 0;
  }

  int32_t AudioPlayDataReader::NeedMorePlayData(const size_t nSamples, const size_t nBytesPerSample, const size_t nChannels, const uint32_t samplesPerSec, void * audioSamples, size_t & nSamplesOut, int64_t * elapsed_time_ms, int64_t * ntp_time_ms) {
    if (!mAudioSource) {
      return 0;
    }
    RTC_DCHECK_EQ(sizeof(int16_t) * nChannels, nBytesPerSample);
    RTC_DCHECK_GE(nChannels, 1);
    RTC_DCHECK_LE(nChannels, 2);
    RTC_DCHECK_GE(samplesPerSec, static_cast<uint32_t>(AudioProcessing::NativeRate::kSampleRate8kHz));

    // 100 = 1 second / data duration (10 ms).
    RTC_DCHECK_EQ(nSamples * 100, samplesPerSec);
    RTC_DCHECK_LE(nBytesPerSample * nSamples * nChannels,
      AudioFrame::kMaxDataSizeBytes);

    *elapsed_time_ms = 0;
    *ntp_time_ms = 0;

    //nSamplesOut = samplesPerSec / 100 * nChannels;

    /* 源数据默认为48000 双声道 */
    size_t numSampleIn10MS = mChannels * mSamplesPerSec / 100;
    std::unique_ptr < int16_t[] > src(new int16_t[numSampleIn10MS]);
    if (!src.get()) {
      LOGE("allocate buffer failed");
      return 0;
    }
    size_t readSamples = fread(src.get() , sizeof(int16_t), numSampleIn10MS, mAudioSource);
    if (readSamples != numSampleIn10MS) {
      /* file end */
      Stop();
      return 0;
    }
    if (nChannels != mChannels || samplesPerSec != mSamplesPerSec) {
      AudioFrame outFrame;
      outFrame.UpdateFrame((uint32_t)0, src.get(), numSampleIn10MS/mChannels, mSamplesPerSec, AudioFrame::SpeechType::kUndefined, AudioFrame::VADActivity::kVadUnknown, mChannels);
      nSamplesOut = Resample(outFrame, samplesPerSec, &mRenderResampler, static_cast<int16_t*>(audioSamples));
    }
    else {
      memcpy(audioSamples, src.get(), sizeof(int16_t)*readSamples);
      nSamplesOut = readSamples;
    }
    std::unique_lock<std::mutex> lock(mMtx);
    if (mListener) {
      mListener->SendAudioData(audioSamples, sizeof(int16_t), samplesPerSec, nChannels, nSamplesOut / nChannels);
    }
    RTC_DCHECK_EQ(nSamplesOut, nChannels * nSamples);
    return 0;
  }

  void AudioPlayDataReader::PullRenderData(int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_of_frames, void * audio_data, int64_t * elapsed_time_ms, int64_t * ntp_time_ms) {

  }

  // Resample audio in |frame| to given sample rate preserving the
  // channel count and place the result in |destination|.
  int AudioPlayDataReader::Resample(const AudioFrame& frame, const int destination_sample_rate, PushResampler<int16_t>* resampler, int16_t* destination) {
    const int number_of_channels = static_cast<int>(frame.num_channels_);
    const int target_number_of_samples_per_channel = destination_sample_rate / 100;
    resampler->InitializeIfNeeded(frame.sample_rate_hz_, destination_sample_rate,
      number_of_channels);

    // TODO(yujo): make resampler take an AudioFrame, and add special case
    // handling of muted frames.
    return resampler->Resample(frame.data(), frame.samples_per_channel_ * number_of_channels, destination, number_of_channels * target_number_of_samples_per_channel);
  }
}