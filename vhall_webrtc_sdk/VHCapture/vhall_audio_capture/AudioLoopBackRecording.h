#pragma once
#include <string>
#include <map>
#include <assert.h>
#include <mutex>

#include "IAudioCapture.h"
#include <atomic>

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

namespace vhall {
  typedef std::function<void(std::shared_ptr<float[]>& data, int sampleRate, int channel)> AudioCaptrueCB;
  class I_DShowDevieEvent;

  class AudioLoopBackRecording :public rtc::MessageHandler {
  public:
    AudioLoopBackRecording();
    ~AudioLoopBackRecording();
    virtual bool InitPlaybackSource(const std::wstring deviceId, float volume, const AudioCaptrueCB & callback = [](std::shared_ptr<float[]>& data, int sampleRate, int channel)->void {});
    bool Start();
    void Stop();
    void SetVolume(float volume);
    void SetAudioListening(IDataReceiver *dataListening);
    /* 设置设备事件监听 */
    typedef enum {
      REC_AUDIO_DATA = 0,
    }MsgType;
  private:
    virtual void OnMessage(rtc::Message* msg);
  private:
    IDataReceiver * mDataListening = nullptr;
    std::mutex      mMtx;
    DeviceInfo      mDeviceInfo;
    IAudioSource*   mPlaybackAudio = nullptr;
    std::atomic<float> mVolume;
    std::shared_ptr<rtc::Thread> mThread;
    AudioCaptrueCB PushFun = [](std::shared_ptr<float[]>& data, int sampleRate, int channel)->void {};
    UINT mSampleRateHz = 44100;
  };
}