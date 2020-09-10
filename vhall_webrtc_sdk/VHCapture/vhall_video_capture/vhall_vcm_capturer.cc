#include "vhall_vcm_capturer.h"

#include <stdint.h>
#include <memory>

#include "api/video/i420_buffer.h"
#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "3rdparty/VHGPUImage/VHBeautifyRender.h"
#include "../video_capture_dshow/video_capture_ds.h"
#include "common/vhall_log.h"

namespace vhall {

  VcmCapturer::VcmCapturer() : vcm_(nullptr) {
    enable_capture = true;
    last_capture_ts = 0;
  }

  bool VcmCapturer::Init(
    webrtc::VideoCaptureCapability capability,
    webrtc::VideoCaptureCapability publish_capability,
    size_t capture_device_index,
    vhall::I_DShowDevieEvent* notify,
    LiveVideoFilterParam & filterParam) {
    capability_ = capability;
    max_framerate_request_ = publish_capability.maxFPS;
    this->publish_capability_ = publish_capability;

    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());

    char device_name[256];
    char unique_name[256];
    if (device_info->GetDeviceName(static_cast<uint32_t>(capture_device_index),
      device_name, sizeof(device_name), unique_name,
      sizeof(unique_name)) != 0) {
      StopCapture();
      return false;
    }

    vcm_ = Create(unique_name, capability, notify);
    if (!vcm_) {
      return false;
    }
    vcm_->RegisterCaptureDataCallback(this);

    /* 校正宽高比，如果实际采样和目标采样不一致，则更正采样，可能首屏花屏 */
    if (vcm_->StartCapture(capability_) != 0) {
      StopCapture();
      return false;
    }

    // for debug
    //yuv_data_ = webrtc::FileWrapper::OpenWriteOnly("./test.yuv");

    mFilterParam = filterParam;
    mBeautifyFilter.reset(new VHBeautifyRender(mFilterParam.beautyLevel, [this](std::shared_ptr<unsigned char[]> data, int length, int width, int height, int64_t ts) ->void {
      rtc::scoped_refptr<webrtc::I420Buffer> srcBuffer = webrtc::I420Buffer::Create(width, height);
      libyuv::RGB24ToI420(
        data.get(), srcBuffer->width() * 3,
        srcBuffer->MutableDataY(), srcBuffer->StrideY(),
        srcBuffer->MutableDataU(), srcBuffer->StrideU(),
        srcBuffer->MutableDataV(), srcBuffer->StrideV(),
        srcBuffer->width(), srcBuffer->height());

      data.reset(); // release rgb buffer

      if (yuv_data_.is_open()) {
        yuv_data_.Write(srcBuffer->MutableDataY(), width * height * 3 / 2);
      }

      webrtc::VideoFrame frame(webrtc::VideoFrame::Builder()
        .set_video_frame_buffer(srcBuffer)
        .set_timestamp_us(ts)
        .build());

      VideoCapturer::OnFrame(frame);
    }));

    RTC_CHECK(vcm_->CaptureStarted());
    if (thread_) {
       thread_->Stop();
       thread_->Clear(this);
       thread_.reset();
    }
    thread_ = rtc::Thread::Create();
    thread_->Start();
    return true;
  }

  VcmCapturer* VcmCapturer::Create(
    webrtc::VideoCaptureCapability capability,
    webrtc::VideoCaptureCapability publish_capability,
    size_t capture_device_index,
    vhall::I_DShowDevieEvent* notify,
    LiveVideoFilterParam & filterParam) {
    std::unique_ptr<VcmCapturer> vcm_capturer(new VcmCapturer());
    if (!vcm_capturer->Init(capability, publish_capability, capture_device_index, notify, filterParam)) {
      RTC_LOG(LS_WARNING) << "Failed to create VcmCapturer(w = " << capability.width
        << ", h = " << capability.height << ", fps = " << publish_capability.maxFPS
        << ")";
      return nullptr;
    }
    return vcm_capturer.release();
  }

  void VcmCapturer::StopCapture() {
    enable_capture = false;

    if (!vcm_)
      return;

    if (thread_) {
      thread_->Stop();
      thread_->Clear(this);
      thread_.reset();
    }

    vcm_->StopCapture();
    vcm_->DeRegisterCaptureDataCallback();
    // Release reference to VCM.
    vcm_ = nullptr;
  }

  rtc::scoped_refptr<webrtc::VideoCaptureModule> VcmCapturer::Create(const char * deviceUniqueIdUTF8, webrtc::VideoCaptureCapability capability, vhall::I_DShowDevieEvent* notify) {
    if (deviceUniqueIdUTF8 == nullptr)
      return nullptr;

    vcm_notify_ = notify;
    // TODO(tommi): Use Media Foundation implementation for Vista and up.
    rtc::scoped_refptr<VideoCaptureDS> capture(
      new rtc::RefCountedObject<VideoCaptureDS>());

    if (capture->Init(deviceUniqueIdUTF8, capability) != 0) {
      return nullptr;
    }

    capture->SetDhowDeviceNotify(notify);
    if (thread_) {
      thread_->Post(RTC_FROM_HERE, this, capture_check, 0);
    }
    return capture;
  }

  void VcmCapturer::CheckCaptureState() {
    std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
    int64_t cur_ts = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;

    if (0 == last_capture_ts) {
      last_capture_ts = cur_ts;
    }
    else if ((cur_ts - last_capture_ts > 5 * 1000) && vcm_notify_) { // max internal: 5s
      vcm_notify_->OnVideoCaptureFail();
    }

    return;
  }

  bool VcmCapturer::KeepFrame(int64_t in_timestamp_us) {
    int64_t frame_interval_us = rtc::kNumMicrosecsPerSec / max_framerate_request_;
    if (frame_interval_us <= 0) {
      // Frame rate throttling not enabled.
      return true;
    }

    if (next_frame_timestamp_us_) {
      // Time until next frame should be outputted.
      const int64_t time_until_next_frame_ns =
        (next_frame_timestamp_us_ - in_timestamp_us);

      // Continue if timestamp is within expected range.
      if (std::abs(time_until_next_frame_ns) < 2 * frame_interval_us) {
        // Drop if a frame shouldn't be outputted yet.
        if (time_until_next_frame_ns > 0)
          return false;
        // Time to output new frame.
        next_frame_timestamp_us_ += frame_interval_us;
        return true;
      }
    }

    // First timestamp received or timestamp is way outside expected range, so
    // reset. Set first timestamp target to just half the interval to prefer
    // keeping frames in case of jitter.
    next_frame_timestamp_us_ = in_timestamp_us + frame_interval_us / 2;
    return true;
  }

  void VcmCapturer::OnMessage(rtc::Message * msg) {
    if (!msg) {
      return;
    }
    switch (msg->message_id) {
    case capture_check:
      CheckCaptureState();
      if (thread_) {
        thread_->PostDelayed(RTC_FROM_HERE, 5000, this, capture_check, 0);
      }
      break;
    default:
      LOGE("unknown event");
      break;
    }
    return;
  }

  VcmCapturer::~VcmCapturer() {
    StopCapture();
  }

  void VcmCapturer::OnFrame(const webrtc::VideoFrame& frame) {
    std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
    last_capture_ts = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;

    if (enable_capture) {
      if (KeepFrame(frame.timestamp_us())) {
        webrtc::VideoFrame in_frame(frame);
        AdaptFrameResolution(in_frame);
        video_pre_process(in_frame);
        if (mFilterParam.beautyLevel > 0) {
          BeautyFilter(in_frame);
        }
        else {
          VideoCapturer::OnFrame(in_frame);
        }
      }
    }
  }

  void VcmCapturer::AdaptFrameResolution(webrtc::VideoFrame & frame) {
    if (publish_capability_.height <= 0 || publish_capability_.width <= 0) {
      return;
    }

    if (publish_capability_.height != frame.height() || publish_capability_.width != frame.width()) {
      rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer = webrtc::I420Buffer::Create(publish_capability_.width, publish_capability_.height);
      scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
      frame = webrtc::VideoFrame::Builder()
        .set_video_frame_buffer(scaled_buffer)
        .set_rotation(frame.rotation())
        .set_timestamp_us(frame.timestamp_us())
        .set_id(frame.id())
        .build();
    }
  }

  void VcmCapturer::video_pre_process(webrtc::VideoFrame & frame) {
    rtc::scoped_refptr<webrtc::I420Buffer> video_i420 = webrtc::I420Buffer::Create(frame.width(), frame.height());
    video_i420->ScaleFrom(*frame.video_frame_buffer()->ToI420());

    if (video_i420->width() != filterCfg_.width || video_i420->height() != filterCfg_.height || !imageFilter_) {
      filterCfg_.width = video_i420->width();
      filterCfg_.height = video_i420->height();

      filterBufIn_.reset(new uint8_t[filterCfg_.width * filterCfg_.height * 3 / 2]);
      filterBufOut_.reset(new uint8_t[filterCfg_.width * filterCfg_.height * 3 / 2]);
      imageFilter_.reset(new ImageEnhance());
      bool ret = imageFilter_->Init(&filterCfg_);
      if (!ret) {
        LOGE("imageFilter_ init fail");
        imageFilter_.reset();
        return;
      }
    }
    
    if (mFilterParam.enableDenoise || mFilterParam.enableBrighter) {
      int pixel_count = video_i420->height() * video_i420->width();
      memcpy(filterBufIn_.get(), video_i420->DataY(), pixel_count);
      memcpy(filterBufIn_.get() + pixel_count, video_i420->DataU(), pixel_count / 4);
      memcpy(filterBufIn_.get() + pixel_count + pixel_count / 4, video_i420->DataV(), pixel_count / 4);

      if (mFilterParam.enableDenoise) {
        imageFilter_->Denoise(filterBufIn_.get(), filterBufOut_.get(), PixelFmt_YV12); /* 降噪 */
        memcpy(filterBufIn_.get(), filterBufOut_.get(), pixel_count * 3 / 2); /* 更新输入缓存 */
      }
      
      if (mFilterParam.enableBrighter) {
        imageFilter_->Brighter(filterBufIn_.get(), filterBufOut_.get(), PixelFmt_YV12);
      }

      memcpy(video_i420->MutableDataY(), filterBufOut_.get(), pixel_count);
      memcpy(video_i420->MutableDataU(), filterBufOut_.get() + pixel_count, pixel_count / 4);
      memcpy(video_i420->MutableDataV(), filterBufOut_.get() + pixel_count + pixel_count / 4, pixel_count / 4);

      frame = webrtc::VideoFrame::Builder()
        .set_video_frame_buffer(video_i420)
        .set_rotation(frame.rotation())
        .set_timestamp_us(frame.timestamp_us())
        .set_id(frame.id())
        .build();
    }
    return;
  }

  bool VcmCapturer::BeautyFilter(webrtc::VideoFrame & frame) {
    std::vector<webrtc::VideoFrame> frames;
    if (!mBeautifyFilter || mFilterParam.beautyLevel < 1) {
      LOGE("mBeautifyFilter: %d, beautyLevel:%d", nullptr != mBeautifyFilter, mFilterParam.beautyLevel);
      return false;
    }
    else {
      // allocate rgb buffer
      std::shared_ptr<unsigned char[]> rgbDataBuffer(new unsigned char[frame.width()*frame.height() * 3]);
      rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(frame.video_frame_buffer()->ToI420());
      libyuv::I420ToRGB24(
        buffer->DataY(), buffer->StrideY(),
        buffer->DataU(), buffer->StrideU(),
        buffer->DataV(), buffer->StrideV(),
        rgbDataBuffer.get(),
        buffer->width() * 3,
        buffer->width(), buffer->height());
      mBeautifyFilter->loadFrame(rgbDataBuffer, frame.width() * frame.height() * 3, frame.width(), frame.height(), frame.timestamp_us()); 
    }

    return true;
  }

  void VcmCapturer::SetFilterParam(LiveVideoFilterParam & filterParam) {
    if (mBeautifyFilter && filterParam.beautyLevel != mFilterParam.beautyLevel && filterParam.beautyLevel != 0) {
      mBeautifyFilter->SetBeautifyLevel(filterParam.beautyLevel);
    }

    mFilterParam = filterParam;
  }

}  // namespace vhall
