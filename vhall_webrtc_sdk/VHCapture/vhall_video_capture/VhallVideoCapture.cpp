#include "VhallVideoCapture.h"
#include "common/vhall_log.h"

namespace webrtc {
  rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::CreateFileVideo(const webrtc::VideoCaptureCapability & capability) {
    std::unique_ptr<vhall::FileVideoCaptureImpl> capturer = nullptr;
    capturer = absl::WrapUnique(vhall::FileVideoCaptureImpl::Create(capability));
    if (capturer) {
      return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
    }
    return nullptr;
  }
  rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::CreateDeskCapture(
    int type, int width, int height, int fps) 
  {
    std::unique_ptr<vhall::DesktopCaptureImpl> capturer = nullptr;
    capturer = absl::WrapUnique(vhall::DesktopCaptureImpl::Create(type, width, height, fps));
    if (capturer) {
      return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
    }
    return nullptr;
  }

  rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::CreatePicCapture(
    std::shared_ptr<vhall::I420Data>& picture,
    const webrtc::VideoCaptureCapability & capability)
  {
    std::unique_ptr<vhall::PictureCaptureImpl> capturer = nullptr;
    capturer = absl::WrapUnique(vhall::PictureCaptureImpl::Create(picture, capability));
    if (capturer) {
      return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
    }
    return nullptr;
  }

  rtc::scoped_refptr<CapturerTrackSource> CapturerTrackSource::Create(
    webrtc::VideoCaptureCapability capability,
    webrtc::VideoCaptureCapability publish_capability,
    std::string device_id, 
    vhall::I_DShowDevieEvent* notify,
    vhall::LiveVideoFilterParam &filterParam)
  {
    std::unique_ptr<vhall::VcmCapturer> capturer = nullptr;
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
    if (!info) {
      return nullptr;
    }
    
    int num_devices = info->NumberOfDevices();
    if (num_devices <= 0) {
      LOGE("get video device info fail!");
      return nullptr;
    }

    for (int i = 0; i < num_devices; ++i) {
      const uint32_t kSize = 256;
      char devName[kSize] = { 0 };
      char devId[kSize] = { 0 };
      info->GetDeviceName(i, devName, kSize, devId, kSize);
      if (device_id == devId) {
        capturer = absl::WrapUnique(vhall::VcmCapturer::Create(capability, publish_capability, i, notify, filterParam));
        if (capturer) {
          return new rtc::RefCountedObject<CapturerTrackSource>(std::move(capturer));
        }
      }
    }
    return nullptr;
  }
}
