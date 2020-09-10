#ifndef VHALL_VCM_CAPTURER_H_
#define VHALL_VCM_CAPTURER_H_

#include <memory>
#include <atomic>
#include <vector>
#include "signalling/vh_stream.h"
#include "vhall_video_capturer.h"
#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/I_DshowDeviceEvent.h"
#include "rtc_base/thread.h"
#include "rtc_base/system/file_wrapper.h"
#include "video_process_filter.h"

namespace vhall {
  class VHBeautifyRender;

class VcmCapturer : public webrtc::VideoCapturer,
                    public rtc::VideoSinkInterface<webrtc::VideoFrame>,
                    public rtc::MessageHandler,
                    protected vhall::VideoProcessFilter {
 public:
  static VcmCapturer* Create(webrtc::VideoCaptureCapability capability,
                             webrtc::VideoCaptureCapability publish_capability,
                             size_t capture_device_index,
                             vhall::I_DShowDevieEvent* notify,
                             LiveVideoFilterParam & filterParam);
  virtual ~VcmCapturer();

  void OnFrame(const webrtc::VideoFrame& frame) override;

  void AdaptFrameResolution(webrtc::VideoFrame& frame);

  void video_pre_process(webrtc::VideoFrame& frame);

  bool BeautyFilter(webrtc::VideoFrame& frame);

  virtual void SetFilterParam(LiveVideoFilterParam & filterParam) override;

  void StopCapture();

  virtual void SetCapability(webrtc::VideoCaptureCapability& cap) {};
protected:
  virtual void ImageFilter(rtc::scoped_refptr<webrtc::I420Buffer> &video_i420, const std::string type) override {};
 private:
  VcmCapturer();
  bool Init(webrtc::VideoCaptureCapability capability,
            webrtc::VideoCaptureCapability publish_capability,
            size_t capture_device_index,
            vhall::I_DShowDevieEvent* notify,
            LiveVideoFilterParam & filterParam);
  rtc::scoped_refptr<webrtc::VideoCaptureModule> Create(
    const char* deviceUniqueIdUTF8, webrtc::VideoCaptureCapability capability, vhall::I_DShowDevieEvent* notify);
  void CheckCaptureState();
  bool KeepFrame(int64_t in_timestamp_us);
  virtual void OnMessage(rtc::Message* msg);
  enum {
    capture_check
  };

  std::shared_ptr<VHBeautifyRender> mBeautifyFilter;
  webrtc::FileWrapper yuv_data_;

  std::atomic_bool enable_capture;
  rtc::scoped_refptr<webrtc::VideoCaptureModule> vcm_;
  webrtc::VideoCaptureCapability capability_;
  vhall::I_DShowDevieEvent* vcm_notify_ = nullptr;
  std::atomic_ullong last_capture_ts;
  std::shared_ptr<rtc::Thread> thread_;

  // frameRate control
  webrtc::VideoCaptureCapability publish_capability_;
  int max_framerate_request_ = 30;
  int64_t next_frame_timestamp_us_ = 0;
};

}  // namespace vhall

#endif  // VHALL_VCM_CAPTURER_H_
