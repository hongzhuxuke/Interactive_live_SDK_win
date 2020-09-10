#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "vhall_video_capturer.h"
#include "rtc_base/thread.h"
#include "../../signalling/vh_stream.h"

namespace vhall {
  class I420Data;

  class PictureCaptureImpl :public webrtc::VideoCapturer,
                            /*public rtc::VideoSinkInterface<webrtc::VideoFrame>,*/
                            public rtc::MessageHandler {
  public:
    static PictureCaptureImpl* Create(
      std::shared_ptr<vhall::I420Data>& picture,
      const webrtc::VideoCaptureCapability & capability);

    PictureCaptureImpl(std::shared_ptr<vhall::I420Data>& picture, const webrtc::VideoCaptureCapability & capability);
    virtual ~PictureCaptureImpl();

    // Start capture device
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability);

    int32_t StopCapture();

    virtual void SetCapability(webrtc::VideoCaptureCapability& cap);

    // Returns true if the capture device is running
    bool CaptureStarted();
  protected:
    virtual void OnFrame(const webrtc::VideoFrame& frame);
  private:
    enum {
      CAPTURE_LOOP
    };
    void OnMessage(rtc::Message* msg) override;
    void OnIncomingFrame();
    int pushFrame(
      uint8_t* videoFrame,
      size_t videoFrameLength,
      const webrtc::VideoCaptureCapability& frameInfo,
      int64_t captureTime = 0);
    rtc::CriticalSection _apiCs;
  private:
    std::shared_ptr<rtc::Thread> _videoCaptureThread;
    bool                         _isStart;
    std::mutex                   _mutex;
    std::mutex                   capMtx;
    webrtc::VideoCaptureCapability _requestedCapability;
    std::shared_ptr<vhall::I420Data> mSrcPic = nullptr;
  };
}