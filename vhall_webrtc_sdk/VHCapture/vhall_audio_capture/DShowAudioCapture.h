#ifndef __VHALL_DSHOW_AUDIO__CAPTURE__
#define __VHALL_DSHOW_AUDIO__CAPTURE__

#include <string>
#include <map>
#include <assert.h>
#include <mutex>

#include "IAudioCapture.h"
#include <atomic>

//#include "talk/base/messagehandler.h"
//#include "modules/video_capture/I_DshowDeviceEvent.h"
/* could not use thread of webrtc's thread here */
#include "rtc_base/thread.h"
#include "rtc_base/message_queue.h"
#include "api/task_queue/default_task_queue_factory.h"

class IAudioSource;

namespace talk_base {
  class Thread;
}
namespace DShow {
  class AudioConfig;
}

struct AudioCaptureConfig {
  DeviceInfo info;
  bool isNoise;
  int openThreshold;
  int closeThreshold;
  int iKbps;
  int iAudioSampleRate;
  bool bNoiseReduction;
  float fMicGain;
public:
  AudioCaptureConfig() {
    memset(this, 0, sizeof(AudioCaptureConfig));
  }
};

namespace vhall {
  class I_DShowDevieEvent;

  class AudioConfig {
  public:
    /* Desired sample rate */
    int sampleRate = 0;
    /* Desired channels */
    int channels = 0;
    /* Desired audio format */
    int format = 0;
  };

  class DShowAudioCapture :public rtc::MessageHandler {
  public:
    DShowAudioCapture();
    ~DShowAudioCapture();
    bool SetMicDeviceInfo(DeviceInfo device, bool isNoise, int closeThreshold, int openThreshold, UINT iAudioSampleRate, vhall::I_DShowDevieEvent* notify = nullptr);
    bool Start();
    void Stop();
    void SetAudioListening(IDataReceiver *dataListening);
    /* 设置设备事件监听 */
    typedef enum {
      REC_AUDIO_DATA = 0,
    }MsgType;
    std::shared_ptr<AudioConfig> GetAudioConfig();
  private:
    virtual void OnMessage(rtc::Message* msg);
  private:
    IDataReceiver*  mDataListening  = nullptr;
    std::mutex      mMtx;
    DeviceInfo      mDeviceInfo;
    IAudioSource*   mCaptureDevice = nullptr;
    std::shared_ptr<AudioConfig> mAudioConfig;
    std::shared_ptr<rtc::Thread> mThread;
  };
}
#endif /* __VHALL_DSHOW_AUDIO__CAPTURE__ */