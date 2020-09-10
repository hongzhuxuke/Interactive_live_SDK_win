#include "AudioCaptureMix.h"
#include "../../common/vhall_log.h"
//#include "talk/base/thread.h"
#include "vhall_live_core/vhall_media_core/3rdparty/audiomixing/AudioMixing.h"
#include "vhall_webrtc_include/common_audio/include/audio_util.h"
#include "vhall_webrtc_include/audio/remix_resample.h"
#include "vhall_webrtc_include/audio/audio_transport_impl.h"
#include "vhall_webrtc_include/audio/remix_resample.h"
#include "vhall_webrtc_include/audio/utility/audio_frame_operations.h"
//#include "../AudioProcess/NoiseSuppression.h"
#include "vhall_webrtc_include/../vhall_live_core/vhall_media_core/audioprocess/noise_cancelling.h"
#include <chrono>
#include <time.h>
#include <limits.h>
#include <algorithm>
#include <iomanip>
#include <ostream>
#include <vector>
#include <mmsystem.h>

#define CacheBuffCount 3000
#define MaxDelayTime   30  // ms
#define MinBuffCount   3

namespace vhall {
  AudioCaptureMix::AudioCaptureMix() {
  }

  AudioCaptureMix::~AudioCaptureMix() {
    Destroy();
  }

  bool AudioCaptureMix::Start(AudioMixedCB callback) {
    if (!mThread) {
      mThread = rtc::Thread::Create();
      if (!mThread) {
        LOGE("Init thread failed");
        return false;
      }
      else {
        mThread->Restart();
        mThread->Start();
        mThread->Post(RTC_FROM_HERE, this, E_MixAudioData, nullptr);
      }
    }
    if (!mDeliverThread) {
      mDeliverThread = rtc::Thread::Create();
      if (!mDeliverThread) {
        LOGE("Init mPushThread failed");
        return false;
      }
      else {
        mDeliverThread->Restart();
        mDeliverThread->Start();
        mDeliverThread->Post(RTC_FROM_HERE, this, E_DeliverAudioData, nullptr);
      }
    }

    MixedCb = callback;
    return true;
  }

  void AudioCaptureMix::Destroy() {
    if (mCoreNoiseCancelling) {
      mCoreNoiseCancelling->Stop();
      mCoreNoiseCancelling.reset();
    }
    if (mThread) {
      mThread->Clear(this);
      mThread->Stop();
      mThread.reset();
    }
    if (mDeliverThread) {
      mDeliverThread->Stop();
      mDeliverThread->Stop();
      mDeliverThread.reset();
    }
  }

  void AudioCaptureMix::PushCoreAudio(std::shared_ptr<AudioData>& data) {
    CoreAudioNs(data);
  }

  void AudioCaptureMix::PushLoopBack(std::shared_ptr<AudioData>& data) {
    std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
    int64_t curTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;
    data->mTs = curTimeInMs;
    std::unique_lock<std::mutex> lock(mLoopBackMtx);
    if (mLoopBackData.size() >= CacheBuffCount) {
      mLoopBackData.pop_front();
    }
    mLoopBackData.push_back(data);
    lock.unlock();
  }

  void AudioCaptureMix::PushDshow(std::shared_ptr<AudioData>& data) {
    CoreAudioNs(data);
  }

  void AudioCaptureMix::CoreAudioNs(std::shared_ptr<AudioData> &data) {
    if (!mCoreNoiseCancelling) {
      mCoreNoiseCancelling.reset(new NoiseCancelling());
      if (!mCoreNoiseCancelling) {
        return;
      }
      else {
        mCoreNoiseCancelling->SetOutputDataDelegate(VH_CALLBACK_2(AudioCaptureMix::OnNSCoreAudioData, this));
        mCoreNoiseCancelling->Init(44100, 3, 2, VH_AV_SAMPLE_FMT_FLT);
        mCoreNoiseCancelling->Start();
      }
    }
    mCoreNoiseCancelling->NoiseCancellingProcess((int8_t*)data->mData.get(), data->mChannel*data->mSampleRate / 100 * sizeof(float));
  }

  void AudioCaptureMix::OnNSCoreAudioData(const int8_t * audio_data, const int size) {
    std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
    int64_t curTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;
    std::shared_ptr<float[]> audioData(new float[44100 / 100 * 2]);
    if (!audioData) {
      return;
    }
    memcpy(audioData.get(), audio_data, size);
    std::shared_ptr<AudioData> mixData(new AudioData(audioData, 44100, 2, curTimeInMs));
    if (!mixData) {
      return;
    }
    std::unique_lock<std::mutex> lock(mCoreMtx);
    if (mCoreAuidoData.size() >= CacheBuffCount) {
      mCoreAuidoData.pop_front();
    }
    mCoreAuidoData.push_back(mixData);
    lock.unlock();
  }

  float AudioCaptureMix::FloatToFloatS16(float v)
  {
    return webrtc::FloatToFloatS16(v);
  }

  float AudioCaptureMix::FloatS16ToFloat(float v)
  {
    return webrtc::FloatS16ToFloat(v);
  }

  int16_t AudioCaptureMix::FloatS16ToS16(float v)
  {
    return webrtc::FloatS16ToS16(v);
  }

  float AudioCaptureMix::S16ToFloat(int16_t v) {
    return webrtc::S16ToFloat(v);
  }

  void AudioCaptureMix::OnMessage(rtc::Message * msg) {
    switch (msg->message_id) {
    case E_MixAudioData:
      MixAudio();
      mMixTimes++;
      if (mThread) {
        std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
        int64_t curTime = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count();
        if (mStartTs == 0) {
          mStartTs = curTime;
          mThread->PostDelayed(RTC_FROM_HERE, 10, this, E_MixAudioData, nullptr);
        }
        else {
          int64_t cmsDelay = 0;
          /* 计算延迟任务时长 */
          int64_t expectTime = mMixTimes * 10 * 1000 + mStartTs;
          int64_t delayInMs = (expectTime - curTime) / 1000;
          if ((delayInMs + MaxDelayTime) < 0) {
            mMixTimes += abs(delayInMs / 10) -1;
            cmsDelay = abs(delayInMs) % 10 + 1;
          }
          else {
            cmsDelay = delayInMs > 0 ? delayInMs : 5;
          }
          if (cmsDelay > 0) {
            if (cmsDelay > 10) {
              cmsDelay = 10;
            }
            mThread->PostDelayed(RTC_FROM_HERE, cmsDelay, this, E_MixAudioData, nullptr);
          }
          else {
            mThread->PostDelayed(RTC_FROM_HERE, 5, this, E_MixAudioData, nullptr);
          }
        }
      }
      break;
    case E_DeliverAudioData:
      DeliverAudio();
      break;
    }
  }

  void AudioCaptureMix::MixAudio() {
    int channelNum = 0;
    int offSet = 0;
    int loopbackDelay = -1;
    std::shared_ptr<float[]> srcData(new float[44100 / 100 * 2 * 3]);
    std::shared_ptr<float[]> dstData(new float[44100 / 100 * 2]);
    if (!srcData || !dstData) {
      return;
    }
    memset(srcData.get(), 44100 / 100 * 2 * 3 * sizeof(float), 0);
    memset(dstData.get(), 44100 / 100 * 2 * sizeof(float), 0);
    float* pIn = srcData.get();
    std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
    int64_t curTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;
    std::unique_lock<std::mutex> lock_core(mCoreMtx);
    while (mCoreAuidoData.size() > 0) {
      std::shared_ptr<AudioData> coreAudioData = mCoreAuidoData.front();
      mCoreAuidoData.pop_front();
      if (coreAudioData->mTs < (curTimeInMs - MaxDelayTime) && mCoreAuidoData.size() > 5) {
        static int coreAudioLost = 0;
        coreAudioLost++;
        LOGW("coreAudio Audio Data Delay time : %ld ms, total discard: %d", curTimeInMs - coreAudioData->mTs, coreAudioLost);
        continue;
      }
      channelNum++;
      int length = coreAudioData->mSampleRate / 100 * coreAudioData->mChannel;
      memcpy(pIn + offSet, coreAudioData->mData.get(), length * sizeof(float));
      offSet += length;
      break;
    }
    lock_core.unlock();

    std::unique_lock<std::mutex> lock_LoopBack(mLoopBackMtx);
    while (mLoopBackData.size() > 0) {
      std::shared_ptr<AudioData> loopBackData = mLoopBackData.front();
      mLoopBackData.pop_front();
      loopbackDelay = mLoopBackData.size();
      if (loopBackData->mTs < (curTimeInMs - MaxDelayTime) && mLoopBackData.size() > 5) {
        static int loopBackLost = 0;
        loopBackLost++;
        LOGW("loopBack Audio Data Delay time : %ld ms, total discard: %d", curTimeInMs - loopBackData->mTs, loopBackLost);
        continue;
      }
      channelNum++;
      int length = loopBackData->mSampleRate / 100 * loopBackData->mChannel;
      memcpy(pIn + offSet, loopBackData->mData.get(), length * sizeof(float));
      offSet += length;
      break;
    }
    lock_LoopBack.unlock();

    std::unique_lock<std::mutex> lock_Dshow(mDshowMtx);
    while (mDShowAudioData.size() > 0) {
      std::shared_ptr<AudioData> DshowAudioData = mDShowAudioData.front();
      mDShowAudioData.pop_front();
      if (DshowAudioData->mTs < (curTimeInMs - MaxDelayTime) && mDShowAudioData.size() > 5) {
        static int DshowAudioLost = 0;
        DshowAudioLost++;
        LOGW("Dshow Audio Data Delay time : %ld ms, total discard: %d", curTimeInMs - DshowAudioData->mTs,  DshowAudioLost);
        continue;
      }
      channelNum++;
      int length = DshowAudioData->mSampleRate / 100 * DshowAudioData->mChannel;
      memcpy(pIn + offSet, DshowAudioData->mData.get(), length * sizeof(float));
      offSet += length;
      break;
    }
    lock_Dshow.unlock();
    
    if (channelNum) {
      VhallAudioMixing(srcData.get(), dstData.get(), channelNum, 44100, 1, 1);
      std::shared_ptr<short[]> data(new short[44100 / 100 * 2]);
      if (!data) {
        return;
      }
      float* src = dstData.get();
      short* dst = data.get();
      int sampleNum = 44100 / 100 * 2;
      while (sampleNum) {
        (dst[44100 / 100 * 2 - sampleNum]) = webrtc::FloatS16ToS16(webrtc::FloatToFloatS16((src[44100 / 100 * 2- sampleNum])));
        sampleNum--;
      }
      std::shared_ptr<AudioData> mixData(new AudioData(dstData, 44100, 2, 0));
      if (!mixData) {
        return;
      }
      std::unique_lock<std::mutex> lock(mDeliverMxt);
      mMixedData.push_back(mixData);
      lock.unlock();
    }
  }

  void AudioCaptureMix::DeliverAudio() {
    std::shared_ptr<AudioData> mixDataSeg = nullptr;
    std::unique_lock<std::mutex> lock(mDeliverMxt);
    int buffCount = mMixedData.size();
    if (buffCount > 0) {
      mixDataSeg = mMixedData.front();
      mMixedData.pop_front();
      buffCount--;
    }
    lock.unlock();
    if (!mixDataSeg) {
      mDeliverThread->PostDelayed(RTC_FROM_HERE, 10, this, E_DeliverAudioData, nullptr);
      return;
    }
    std::shared_ptr<short[]> deliverDataSeg(new short[44100 / 100 * 2]);
    if (!deliverDataSeg) {
      return;
    }
    memset(deliverDataSeg.get(), 0, sizeof(short) * 44100 / 100 * 2);
    
    float* src = mixDataSeg->mData.get();
    short* dst = deliverDataSeg.get();
    int sampleNum = 44100 / 100 * 2;
    while (sampleNum) {
      (dst[44100 / 100 * 2 - sampleNum]) = webrtc::FloatS16ToS16(webrtc::FloatToFloatS16((src[44100 / 100 * 2 - sampleNum])));
      sampleNum--;
    }
    if (MixedCb) {
      MixedCb(deliverDataSeg, 44100, 2, 10 * buffCount - 10);
    }
    mDeliverThread->Post(RTC_FROM_HERE, this, E_DeliverAudioData, nullptr);
  }
}
