#include "Utility.h"
#include "Logging.h"
#include "AudioLoopBackRecording.h" /* fixed header order */
#include "CoreAudioSource.h"
#include "DShowPlugin.h"
#include "../../common/vhall_log.h"
//#include "talk/base/thread.h"

namespace vhall {
  AudioLoopBackRecording::AudioLoopBackRecording() {
    mVolume = 1;
  }

  AudioLoopBackRecording::~AudioLoopBackRecording() {
    Stop();
  }

  static QWORD GetVideoTime() {
    return 0;
  }

  bool AudioLoopBackRecording::InitPlaybackSource(const std::wstring deviceId, float volume, const AudioCaptrueCB & callback) {
    Stop();
    mVolume = volume;
    PushFun = callback;
    std::wstring strPlaybackDevice;
    if (deviceId == L"") {
      strPlaybackDevice = L"Default";
    }
    else {
      strPlaybackDevice = deviceId;
    }
    CoreAudioSource *source = new CoreAudioSource(false, GetVideoTime, mSampleRateHz);
    if (source->Initialize(false, strPlaybackDevice.c_str())) {
      IAudioSource* tmp = mPlaybackAudio;
      mPlaybackAudio = NULL;
      if (tmp) {
        tmp->StopCapture();
        delete tmp;
      }
      mPlaybackAudio = source;
      return true;
    }

    if (source) {
      delete source;
      source = nullptr;
    }
    return false;
  }

  bool AudioLoopBackRecording::Start() {
    if (mPlaybackAudio) {
      mPlaybackAudio->StartCapture();
    }
    else {
      LOGE("mCaptureDevice not initialized");
      return false;
    }
    /* create thread */
    if (!mThread) {
      mThread = rtc::Thread::Create();
      if (!mThread) {
        LOGE("Init thread failed");
      }
      else {
        mThread->Restart();
        mThread->Start();
        mThread->Post(RTC_FROM_HERE, this, REC_AUDIO_DATA, nullptr);
      }
    }
    return true;
  }

  void AudioLoopBackRecording::Stop() {
    if (mThread) {
      mThread->Clear(this);
      mThread->Stop();
      mThread.reset();
      mThread = nullptr;
    }
    if (mPlaybackAudio) {
      mPlaybackAudio->StopCapture();
      delete mPlaybackAudio;
      mPlaybackAudio = nullptr;
    }
  }

  void AudioLoopBackRecording::SetVolume(float volume) {
    if (volume < 0.0 || volume > 1.0) {
      return;
    }
    mVolume = volume;
  }

  void AudioLoopBackRecording::SetAudioListening(IDataReceiver * dataListening) {
    std::unique_lock<std::mutex> lock(mMtx);
    mDataListening = dataListening;
  }

  void AudioLoopBackRecording::OnMessage(rtc::Message * msg) {
    switch (msg->message_id) {
    case REC_AUDIO_DATA:
    {
      float* buffer = nullptr;
      UINT numFrames = 0;
      QWORD timestamp = 0;
      bool floatFormat = 0;
      UINT bitPerSample = 0;
      UINT channels = 0;
      if (!mPlaybackAudio) {
        LOGE("LoopBack AudioSource is null");
      }
      bool bGotSomeAudio = false;
      if (mPlaybackAudio != NULL) {
        if (mPlaybackAudio->QueryAudio2(mVolume, true, 0) != NoAudioAvailable) {
          bGotSomeAudio = true;
        }
      }
      std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
      int64_t curTime = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count();
      timestamp = curTime / 1000;
      if (bGotSomeAudio && mPlaybackAudio && mPlaybackAudio->GetBuffer(&buffer, timestamp)) {
        if (buffer) {
          std::shared_ptr<float[]> data(new float[mSampleRateHz/100*2]);
          if (data) {
            memcpy(data.get(), buffer, sizeof(float)*mSampleRateHz / 100 * 2);
            PushFun(data, mSampleRateHz, 2);
          }
        }
      }
      if (mThread) {
        mThread->PostDelayed(RTC_FROM_HERE, 3, this, REC_AUDIO_DATA, nullptr);
      }
    }
    break;
    default:
      LOGE("undefine message");
      break;
    }
  }
}
