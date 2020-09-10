#pragma once
#include "api/peer_connection_interface.h"
#include <mutex>
#include <atomic>

namespace vhall {
  class SnapShotCallBack;
  class BaseStack;
  class VHStream;
  class Snapshot : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
  public:
    Snapshot(std::shared_ptr<BaseStack> p);
    virtual ~Snapshot();
    virtual void OnFrame(const webrtc::VideoFrame& frame);
    void SetEncodeParam(int width, int height, int quality = 50);
    void RequestSnapShot(SnapShotCallBack* callBack,int width = 0, int height = 0, int quality = 80);
  public:
    int mInterval; // sec
    bool mEnable;  // 推流开始后截图上报
  protected:
    std::mutex mMtx;
    std::weak_ptr<BaseStack> mPtrBaseStack;
    std::weak_ptr<VHStream> mVhStream;

    int mDstWidth;
    int mDstHeight;
    int mQuality;
    int64_t mLastFrameTs;
  private:
    /* usr request snapshot */
    void RequestFrame(const webrtc::VideoFrame& frame);
    SnapShotCallBack* mSnapShotListener;
    std::mutex        mListenMtx;
    std::atomic<int> mRequestCount;
    std::atomic<int> mRequestHeight;
    std::atomic<int> mRequestWidth;
    std::atomic<int> mRequestQuality;
  };

}
