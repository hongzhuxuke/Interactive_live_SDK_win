#pragma once
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <utility>
#include <vector>
#include "absl/memory/memory.h"
#include "absl/types/optional.h"
#include "api/create_peerconnection_factory.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "pc/video_track_source.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/rtc_certificate_generator.h"
#include "rtc_base/strings/json.h"
#include "api/scoped_refptr.h"
#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "vhall_vcm_capturer.h"
#include "picture_capture_impl.h"
#include "desktop_capture_impl.h"
#include "file_video_capture_impl.h"

namespace webrtc {
  class CapturerTrackSource : public webrtc::VideoTrackSource {
  public:
    static rtc::scoped_refptr<CapturerTrackSource> CreateFileVideo(
      const webrtc::VideoCaptureCapability & capability);

    static rtc::scoped_refptr<CapturerTrackSource> CreateDeskCapture(int type, 
                                                                     int width, 
                                                                     int height, 
                                                                     int fps);
    
    static rtc::scoped_refptr<CapturerTrackSource> CreatePicCapture(
      std::shared_ptr<vhall::I420Data>& picture,
      const webrtc::VideoCaptureCapability & capability);

    static rtc::scoped_refptr<CapturerTrackSource> Create(
      webrtc::VideoCaptureCapability capability,
      webrtc::VideoCaptureCapability publish_capability,
      std::string device_id,
      vhall::I_DShowDevieEvent* notify,
      vhall::LiveVideoFilterParam &filterParam);

    VideoCapturer* captureSource() {
      if (capturer_) {
        return capturer_.get();
      }
      if (capturer_picture) {
        return capturer_picture.get();
      }
      if (desk_capture) {
        return desk_capture.get();
      }
      if (file_capture) {
        return file_capture.get();
      }
      return nullptr;
    }

    void StopCapture() {
      if (capturer_) {
        capturer_->StopCapture();
      }
      if (capturer_picture) {
        capturer_picture->StopCapture();
      }
      if (desk_capture) {
        desk_capture->StopCapture();
      }
      if (file_capture) {
        file_capture->StopCapture();
      }
    }
  protected:
    explicit CapturerTrackSource(std::unique_ptr<vhall::FileVideoCaptureImpl> capturer)
      : VideoTrackSource(/*remote=*/false), file_capture(std::move(capturer)) {}

    explicit CapturerTrackSource(std::unique_ptr<vhall::VcmCapturer> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

    explicit CapturerTrackSource(std::unique_ptr<vhall::PictureCaptureImpl> capturer)
      : VideoTrackSource(/*remote=*/false), capturer_picture(std::move(capturer)) {}

    explicit CapturerTrackSource(std::unique_ptr<vhall::DesktopCaptureImpl> capturer)
      : VideoTrackSource(/*remote=*/false), desk_capture(std::move(capturer)) {}
  private:
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
      if (capturer_.get()) {
        return capturer_.get();
      }
      if (capturer_picture.get()) {
        return capturer_picture.get();
      }
      if (desk_capture.get()) {
        return desk_capture.get();
      }
      if (file_capture) {
        return file_capture.get();
      }
    }

    std::unique_ptr<vhall::FileVideoCaptureImpl> file_capture;
    std::unique_ptr<vhall::PictureCaptureImpl> capturer_picture;
    std::unique_ptr<vhall::VcmCapturer> capturer_;
    std::unique_ptr<vhall::DesktopCaptureImpl> desk_capture;
  };
}  // namespace vhall

namespace webrtc {
  class VcmCapturer : public rtc::VideoSinkInterface<VideoFrame>, public rtc::VideoSourceInterface<VideoFrame> {
  public:
    static VcmCapturer* Create(
      size_t width,
      size_t height,
      size_t target_fps,
      size_t capture_device_index);
    virtual ~VcmCapturer();

    //void OnFrame(const VideoFrame& frame) override;
    void AddOrUpdateSink(rtc::VideoSinkInterface<VideoFrame>* sink,
      const rtc::VideoSinkWants& wants) override;
    void RemoveSink(rtc::VideoSinkInterface<VideoFrame>* sink) override;

  protected:
    void OnFrame(const VideoFrame& frame);
    rtc::VideoSinkWants GetSinkWants();
  private:
    VcmCapturer();
    bool Init(
      size_t width,
      size_t height,
      size_t target_fps,
      size_t capture_device_index);
    void Destroy();

    rtc::scoped_refptr<VideoCaptureModule> vcm_;
    VideoCaptureCapability capability_;
  };

}  // namespace webrtc