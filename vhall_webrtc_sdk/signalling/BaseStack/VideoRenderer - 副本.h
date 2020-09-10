#ifndef VH_VIDEORENDERER_H
#define VH_VIDEORENDERER_H

#include "common/vhall_define.h"

#include "api/peer_connection_interface.h"
#include "../tool/VideoRenderReceiver.h"

NS_VH_BEGIN

class BaseStack;
class VHStream;

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
  VideoRenderer(HWND &wnd, std::shared_ptr<BaseStack> p);
  VideoRenderer(std::shared_ptr<VideoRenderReceiveInterface> receiver, std::shared_ptr<BaseStack> p);
  ~VideoRenderer();

  void Lock() {
    ::EnterCriticalSection(&buffer_lock_);
  }

  void Unlock() {
    ::LeaveCriticalSection(&buffer_lock_);
  }

  // VideoSinkInterface implementation
  void OnFrame(const webrtc::VideoFrame& video_frame);
  void OnPaint();

  const BITMAPINFO& bmi() const { return bmi_; }
  const uint8_t* image() const { return image_.get(); }

  //    HWND wnd_;
protected:
  void SetSize(int width, int height);

  enum {
    SET_SIZE,
    RENDER_FRAME,
  };

  HWND wnd_ = nullptr;
  std::shared_ptr<VideoRenderReceiveInterface> receiver = nullptr;
  BITMAPINFO bmi_;
  std::unique_ptr<uint8_t[]> image_;
  CRITICAL_SECTION buffer_lock_;

  std::weak_ptr<BaseStack> mPtrBaseStack;
  std::weak_ptr<VHStream> mVhStream;
};

// A little helper class to make sure we always to proper locking and
// unlocking when working with VideoRenderer buffers.
template <typename T>
class AutoLock {
public:
  explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
  ~AutoLock() { obj_->Unlock(); }
protected:
  T * obj_;
};

NS_VH_END

#endif /* ec_stream_h */