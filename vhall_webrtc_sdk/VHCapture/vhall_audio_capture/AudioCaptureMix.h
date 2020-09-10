#pragma once
#include <string>
#include <map>
#include <cassert>
#include <mutex>
#include <atomic>
#include <list>
//#include "talk/base/messagehandler.h"
#include "rtc_base/thread.h"
#include "rtc_base/message_queue.h"

namespace talk_base {
  class Thread;
}

namespace vhall {
  class NoiseCancelling;

  typedef std::function<void(std::shared_ptr<short[]>& data, int sampleRate, int channel, int recDelay)> AudioMixedCB;
  class AudioCaptureMix :public rtc::MessageHandler {
  public:
    class AudioData {
    public:
      AudioData(std::shared_ptr<float[]> data, int sampleRate, int channel, int64_t ts):mData(data), mSampleRate(sampleRate), mChannel(channel), mTs(ts) {};
      std::shared_ptr<float[]> mData = nullptr;
      int mSampleRate = 0;
      int mChannel = 0;
      int64_t mTs = 0; // millisecond
    };
    AudioCaptureMix();
    virtual ~AudioCaptureMix();
    bool Start(AudioMixedCB callback = [](std::shared_ptr<short[]>& data, int sampleRate, int channel, int recDelay)->void {});
    void Destroy();
    void PushCoreAudio(std::shared_ptr<AudioData>& data);
    void PushLoopBack(std::shared_ptr<AudioData>& data);
    void PushDshow(std::shared_ptr<AudioData>& data);
    void CoreAudioNs(std::shared_ptr<AudioData> &data);
    void OnNSCoreAudioData(const int8_t* audio_data, const int size);
    static float FloatToFloatS16(float v);
    static float FloatS16ToFloat(float v);
    static int16_t FloatS16ToS16(float v);
    static float S16ToFloat(int16_t v);
  private:
    enum {
      E_MixAudioData,
      E_DeliverAudioData,
    };
    virtual void OnMessage(rtc::Message* msg);
    void MixAudio();
    void DeliverAudio();

    std::shared_ptr<NoiseCancelling> mCoreNoiseCancelling = nullptr;
    std::list<std::shared_ptr<AudioData>> mCoreAuidoData;
    std::list<std::shared_ptr<AudioData>> mLoopBackData;
    std::list<std::shared_ptr<AudioData>> mDShowAudioData;
    std::list<std::shared_ptr<AudioData>> mMixedData;
    std::mutex mCoreMtx;
    std::mutex mLoopBackMtx;
    std::mutex mDshowMtx;
    std::mutex mDeliverMxt;
    std::shared_ptr<rtc::Thread> mThread;
    std::shared_ptr<rtc::Thread> mDeliverThread;
    AudioMixedCB MixedCb = [](std::shared_ptr<short[]>& data, int sampleRate, int channel, int recDelay)->void {};
    int64_t mStartTs = 0;
    int64_t mMixTimes = 0;
  };
}