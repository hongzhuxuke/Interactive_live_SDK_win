#pragma once

#include <mutex>
#include <atomic>
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_geometry.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "rtc_base/thread.h"
#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_defines.h"
#include "vhall_video_capturer.h"
#include "rtc_base/thread.h"
#include "api/video/i420_buffer.h"
#include "video_process_filter.h"

namespace vhall {

  class DesktopCaptureImpl :public webrtc::VideoCapturer,
                            /*public rtc::VideoSinkInterface<webrtc::VideoFrame>,*/
                            public rtc::MessageHandler,
                            public webrtc::DesktopCapturer::Callback,
                            public vhall::VideoProcessFilter
  {
  public:
    static DesktopCaptureImpl* Create(int type, int width, int height, int fps);

    DesktopCaptureImpl();
    virtual ~DesktopCaptureImpl();
    bool Init(std::unique_ptr<webrtc::DesktopCapturer> capturer);
    webrtc::DesktopCapturer::SourceList GetSourceList();
    // Selects a source to be captured. Returns false in case of a failure (e.g.
    // if there is no source with the specified type and id.)
    bool SelectSource(int64_t id);
    // Start capture device
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability);
    int32_t StopCapture();
    // Returns true if the capture device is running
    bool CaptureStarted();
    // DesktopCapturer::Callback interface
    void OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame);
    void UpdateRect(webrtc::DesktopRect _rect);
    virtual void SetCapability(webrtc::VideoCaptureCapability& cap);
  protected:
    virtual void OnFrame(const webrtc::VideoFrame & frame);
    int pushFrame(
      uint8_t* videoFrame,
      size_t videoFrameLength,
      const webrtc::VideoCaptureCapability& frameInfo,
      int64_t captureTime = 0);

    std::mutex                   capMtx;
    webrtc::VideoCaptureCapability _requestedCapability;
    rtc::CriticalSection _apiCs;
  private:
    enum {
      DESKTOP_CAPTURE_LOOP
    };

    void OnMessage(rtc::Message* msg) override;
    void OnIncomingFrame();
   
    std::shared_ptr<rtc::Thread> _videoCaptureThread;
    bool _isStart;
    std::unique_ptr<webrtc::DesktopCapturer> _capturer;
    std::unique_ptr<webrtc::DesktopFrame> _frame;
    std::mutex _mutex;
    webrtc::DesktopRect _rect;
  };
}