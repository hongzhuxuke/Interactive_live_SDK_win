#include "Utility.h"
#include "common/Utility/Defs.h"
#include "Logging.h"
#include "DshowAudioCapture.h" /* fixed header order */

#include "DeckLinkAudioSource.h"
#include "DShowAudioSource.h"
#include "CoreAudioSource.h"
#include "DShowPlugin.h"
#include "../../common/vhall_log.h"
//#include "talk/base/thread.h"

/* for InitDShowPlugin */
static bool plugInited = false;
bool DShowFileOpen(char *filename) { return false; }
bool DSHowFileClear() { return true; }
bool DShowFileWrite(DeviceInfo deviceInfo, UINT width, UINT height, int frameInternal, DeinterlacingType type, VideoFormat format) { return true; }
bool DShowFileRead(DeviceInfo &deviceInfo, UINT &width, UINT &height, int &frameInternal, DeinterlacingType &type, VideoFormat &format) { return true; }
bool DShowFileClose() { return true;}

namespace vhall {
  DShowAudioCapture::DShowAudioCapture() {
    /* Init DShow plug */
    //InitDeckLinkDeviceManager();
    if (!plugInited) {
      InitDShowPlugin(DShowFileOpen, DSHowFileClear, DShowFileRead, DShowFileWrite, DShowFileClose);
      plugInited = true;
    }
  }

  DShowAudioCapture::~DShowAudioCapture() {
    Stop();
  }
  
  bool DShowAudioCapture::SetMicDeviceInfo(DeviceInfo deviceInfo, bool isNoise, int closeThreshold, int openThreshold, UINT iAudioSampleRate, vhall::I_DShowDevieEvent* notify) {
    mDeviceInfo = deviceInfo;
    Stop();
    IAudioSource *source = NULL;
    switch (deviceInfo.m_sDeviceType) {
    case TYPE_DECKLINK:
    {
      DeckLinkAudioSource *decklinkAudioSource = new DeckLinkAudioSource(deviceInfo.m_sDeviceName, iAudioSampleRate);
      if (!decklinkAudioSource) {
        LOGE("create DeckLinkAudioSource failed");
        return false;
      }
      decklinkAudioSource->Initialize();
      source = decklinkAudioSource;
      break;
    }
    case TYPE_DSHOW_AUDIO:
    {
      DShowAudioSource *dshowAudioSource = new DShowAudioSource(deviceInfo.m_sDeviceName,
        deviceInfo.m_sDeviceID, iAudioSampleRate);
      if (!dshowAudioSource) {
        LOGE("create DShowAudioSource failed");
        return false;
      }
      dshowAudioSource->SetDhowDeviceNotify(notify);
      bool ret = dshowAudioSource->Initialize();
      if (!ret) {
        LOGE("DShowAudioSource init failed");
        return false;
      }
      source = dshowAudioSource;
      /* obtain audio config */
      std::shared_ptr<DShow::AudioConfig> audioConfig = dshowAudioSource->GetAudioConfig();
      if (audioConfig) {
        mAudioConfig = std::make_shared<AudioConfig>();
        if (mAudioConfig) {
          mAudioConfig->channels = audioConfig->channels;
          mAudioConfig->sampleRate = audioConfig->sampleRate;
          mAudioConfig->format = (int)audioConfig->format;
        }
      }
      break;
    }
    default:
      LOGE("error audio type");
      return false;
      break;
    } /* end of switch */
    mCaptureDevice = source;
    
    return true;
  }

  bool DShowAudioCapture::Start() {
    if (mCaptureDevice) {
      mCaptureDevice->StartCapture();
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
        return false;
      }
      else {
        mThread->Restart();
        mThread->Start();
        mThread->Post(RTC_FROM_HERE, this, REC_AUDIO_DATA, nullptr);
      }
    }
    return true;
  }
  
  void DShowAudioCapture::Stop() {
    if (mThread) {
      mThread->Clear(this);
      mThread->Stop();
      mThread.reset();
    }
    if (mCaptureDevice) {
      mCaptureDevice->StopCapture();
      delete mCaptureDevice;
      mCaptureDevice = nullptr;
    }
  }

  void DShowAudioCapture::SetAudioListening(IDataReceiver * dataListening) {
    std::unique_lock<std::mutex> lock(mMtx);
    mDataListening = dataListening;
  }

  std::shared_ptr<AudioConfig> DShowAudioCapture::GetAudioConfig()
  {
    return mAudioConfig;
  }

  void DShowAudioCapture::OnMessage(rtc::Message * msg) {
    switch (msg->message_id) {
    case REC_AUDIO_DATA:
    {
      float *buffer = nullptr;
      UINT numFrames = 0;
      QWORD timestamp = 0;
      bool floatFormat = 0;
      UINT bitPerSample = 0;
      UINT channels = 0;
      if (!mCaptureDevice) {
        LOGE("DShow AudioSource is null");
      }
      bool bGotSomeAudio = false;
      if (mCaptureDevice != NULL) {
        if (mCaptureDevice->QueryAudio2(1, true, 0) != NoAudioAvailable) {
          bGotSomeAudio = true;
        }
      }
      if (bGotSomeAudio && mCaptureDevice && mCaptureDevice->GetBuffer(&buffer, timestamp)) {
        if (buffer) {
          std::shared_ptr<float[]> data(new float[44100 / 100 * 2]);
          if (data) {
            memcpy(data.get(), buffer, sizeof(float) * 44100 / 100 * 2);
            mDataListening->PushAudioSegment((float *)buffer, numFrames, timestamp);
          }
        }
      }
      /*if (mCaptureDevice && mCaptureDevice->GetNextOriBuffer(buffer, numFrames, timestamp, floatFormat, bitPerSample, channels)) {
        if (mDataListening) {
          mDataListening->PushAudioSegment(buffer, numFrames, timestamp);
        }
      }*/
      if (mThread) {
        mThread->PostDelayed(RTC_FROM_HERE, 5, this, REC_AUDIO_DATA, nullptr);
      }
    }
      break;
    default:
      LOGE("undefine message");
      break;
    }
  }

}
