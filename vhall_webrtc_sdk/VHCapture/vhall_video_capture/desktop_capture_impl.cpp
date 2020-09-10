#include "desktop_capture_impl.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "VhallRectUtil.h"
#include "common/vhall_log.h"

namespace vhall {
  DesktopCaptureImpl * DesktopCaptureImpl::Create(int type, int width, int height, int fps) {
    webrtc::DesktopCaptureOptions options(webrtc::DesktopCaptureOptions::CreateDefault());
    options.set_allow_directx_capturer(true);

    std::unique_ptr<webrtc::DesktopCapturer> dc;
    if (6 == type) {
      dc = webrtc::DesktopCapturer::CreateScreenCapturer(options);
    }
    if (type == 3) {
      dc = webrtc::DesktopCapturer::CreateScreenCapturer(options);
      webrtc::DesktopCapturer::SourceList sources;
      dc->GetSourceList(&sources);
      if (sources.size() > 0) {
        dc->SelectSource(sources.at(0).id);
      }
    }
    else if (type == 5) {
      dc = webrtc::DesktopCapturer::CreateWindowCapturer(options);
    }
    else {
      LOGE("type:%d is not exists.", type);
    }

    std::unique_ptr<webrtc::DesktopCapturer> dcc = std::make_unique<webrtc::DesktopAndCursorComposer>(std::move(dc), options);
    std::unique_ptr<DesktopCaptureImpl> capture(new DesktopCaptureImpl());
    webrtc::VideoCaptureCapability op;
    op.width = width;
    op.height = height;
    op.maxFPS = fps;
    capture->SetCapability(op);
    if (!capture->Init(std::move(dcc))) {
      LOGE("Init Desktop Capture failure!");
      return nullptr;
    }
    LOGI("End CreateDesktopCapture");
    capture->StartCapture(op);
    return capture.release();
  }

  DesktopCaptureImpl::DesktopCaptureImpl()
    :_isStart(false) {
    _rect = (webrtc::DesktopRect::MakeLTRB(0, 0, 0, 0));
    _videoCaptureThread = rtc::Thread::Create();
  }

  DesktopCaptureImpl::~DesktopCaptureImpl() {
    if (!_videoCaptureThread->IsQuitting()) {
      _videoCaptureThread->Clear(this);
      _videoCaptureThread->Stop();
    }
  }

  bool DesktopCaptureImpl::Init(std::unique_ptr<webrtc::DesktopCapturer> capturer) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (!capturer) {
      RTC_LOG(LS_ERROR) << "The capturer is empty.";
      return false;
    }
    _capturer = std::move(capturer);
    return true;
  }

  webrtc::DesktopCapturer::SourceList DesktopCaptureImpl::GetSourceList() {
    webrtc::DesktopCapturer::SourceList screens;
    if (_capturer) {
      _capturer->GetSourceList(&screens);
    }
    return screens;
  }

  bool DesktopCaptureImpl::SelectSource(int64_t id) {
    std::unique_lock<std::mutex> lock(_mutex);
    bool ret = false;
    if (_capturer) {
      ret = _capturer->SelectSource(id);
      ret = _capturer->FocusOnSelectedSource();
    }
    return ret;
  }

  int32_t DesktopCaptureImpl::StartCapture(const webrtc::VideoCaptureCapability& capability) {
    std::unique_lock<std::mutex> lock(_mutex);
    _requestedCapability = capability;
    _requestedCapability.videoType = webrtc::VideoType::kARGB;
    _isStart = true;
    if (_capturer) {
      _capturer->Start(this);
    }
    else {
      return -1;
    }
    _videoCaptureThread->Start();
    _videoCaptureThread->Clear(this);
    _videoCaptureThread->Post(RTC_FROM_HERE_WITH_FUNCTION("vh"), this, DESKTOP_CAPTURE_LOOP);
    return 0;
  }

  int32_t DesktopCaptureImpl::StopCapture() {
    std::unique_lock<std::mutex> lock(_mutex);
    _isStart = false;
    if (!_videoCaptureThread->RunningForTest()) {
      _videoCaptureThread->Clear(this);
      _videoCaptureThread->Stop();
    }
    return 0;
  }

  bool DesktopCaptureImpl::CaptureStarted() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _isStart;
  }

  void DesktopCaptureImpl::SetCapability(webrtc::VideoCaptureCapability & cap) {
    std::unique_lock<std::mutex> lock(capMtx);
    _requestedCapability.width = cap.width;
    _requestedCapability.height = cap.height;
    _requestedCapability.maxFPS = cap.maxFPS;
    lock.unlock();
  }

  void DesktopCaptureImpl::OnFrame(const webrtc::VideoFrame & frame) {
    webrtc::VideoCapturer::OnFrame(frame);
  }

  void DesktopCaptureImpl::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame) {
    if (result == webrtc::DesktopCapturer::Result::SUCCESS) {
      std::unique_lock<std::mutex> lock(_mutex);
      webrtc::DesktopRect rect = _rect;
      lock.unlock();
      if (rect.is_empty()) {
        _frame = std::move(frame);
      }
      else { // crop
        std::unique_ptr<webrtc::DesktopFrame> cropedFrame;
        //DesktopSize cropedSize(rect->height(), rect->width());
        if (rect.width() > frame->size().width() || rect.height() > frame->size().height()) {
          RTC_LOG(LS_WARNING) << "DesktopCapturer Result ERROR, invalid crop size w: " << rect.width() << ", h:" << rect.height();
          return;
        }
        cropedFrame.reset(new webrtc::BasicDesktopFrame(webrtc::DesktopSize(rect.width(), rect.height())));
        uint8_t* StartSrcPtr = frame->data() + rect.top_left().y()*frame->stride() + rect.top_left().x() * 4; // 4bytes per pixel
        // mem crop
        int bytesPerLine = 4 * rect.width();
        for (int i = 0; i < rect.height(); i++) {
          memcpy(cropedFrame->data() + i * bytesPerLine, StartSrcPtr + i * frame->stride(), bytesPerLine);
        }
        _frame = std::move(cropedFrame);
        frame.reset();
      }

    }
    else {
      RTC_LOG(LS_WARNING) << "DesktopCapturer Result ERROR.";
    }
  }

  void DesktopCaptureImpl::UpdateRect(webrtc::DesktopRect rect) {
    std::unique_lock<std::mutex> lock(_mutex);
    _rect = rect;
  }

  void DesktopCaptureImpl::OnMessage(rtc::Message* msg) {
    switch (msg->message_id)
    {
    case DESKTOP_CAPTURE_LOOP: {
      if (_isStart) {
        std::unique_lock<std::mutex> lock(capMtx);
        int cmsDelay = 1000.0 / _requestedCapability.maxFPS;
        lock.unlock();

        _videoCaptureThread->Clear(this);
        _videoCaptureThread->PostDelayed(RTC_FROM_HERE_WITH_FUNCTION("vh"), cmsDelay, this, DESKTOP_CAPTURE_LOOP);
        OnIncomingFrame();
      }
    }
      break;
    default:
      break;
    }
  }

  void DesktopCaptureImpl::OnIncomingFrame() {
    webrtc::VideoCaptureCapability cap;
    if (_frame) {
      cap.width = _frame->size().width();
      cap.height = _frame->size().height();

      std::unique_lock<std::mutex> lock(capMtx);
      cap.maxFPS = _requestedCapability.maxFPS;
      cap.videoType = _requestedCapability.videoType;
      lock.unlock();

      pushFrame(_frame->data(), _frame->stride()*_frame->size().height(), cap);
    }
    else {
      pushFrame(NULL, 0, cap);
    }
    _capturer->CaptureFrame();
  }

  int DesktopCaptureImpl::pushFrame(uint8_t * videoFrame, size_t videoFrameLength, const webrtc::VideoCaptureCapability & frameInfo, int64_t captureTime) {
    rtc::CritScope cs(&_apiCs);

    // SetApplyRotation doesn't take any lock. Make a local copy here.
    VhallRectUtil::CGRect rect;

    rtc::scoped_refptr<webrtc::I420Buffer> _srcBuffer;

    std::unique_lock<std::mutex> lock(capMtx);
    int send_width = _requestedCapability.width;
    int send_height = _requestedCapability.height;
    lock.unlock();

    if (videoFrame) {
      const int32_t width = frameInfo.width;
      const int32_t height = frameInfo.height;

      // Not encoded, convert to I420.
      if (webrtc::VideoType::kI420 == frameInfo.videoType) {
        if (width * height * 3 / 2 != videoFrameLength) { // libyuv length cacl error sometimes
          RTC_LOG(LS_ERROR) << "Wrong incoming frame length.";
          return -1;
        }
      }
      else if (frameInfo.videoType != webrtc::VideoType::kMJPEG &&
        CalcBufferSize(frameInfo.videoType, width, abs(height)) != videoFrameLength) {
        RTC_LOG(LS_ERROR) << "Wrong incoming frame length.";
        return -1;
      }

      int target_width = width;
      int target_height = height;

      if (target_width >= target_height) {
        if (target_width != send_width) {
          send_height = target_height *send_width / target_width;
        }
      }
      else if (target_width < target_height) {
        if (target_height != send_height) {
          send_width = target_width *send_height / target_height;
        }
      }

      send_width = (send_width + 1) / 2 * 2;
      send_height = (send_height + 1) / 2 * 2;

      _srcBuffer = webrtc::I420Buffer::Create(target_width, abs(target_height));

      const int conversionResult = libyuv::ConvertToI420(
        videoFrame, videoFrameLength, _srcBuffer->MutableDataY(),
        _srcBuffer->StrideY(), _srcBuffer->MutableDataU(),
        _srcBuffer->StrideU(), _srcBuffer->MutableDataV(),
        _srcBuffer->StrideV(), 0, 0,  // No Cropping
        width, height, target_width, target_height, libyuv::kRotate0,
        ConvertVideoType(frameInfo.videoType));
      if (conversionResult < 0) {
        RTC_LOG(LS_ERROR) << "Failed to convert capture frame from type "
          << static_cast<int>(frameInfo.videoType) << "to I420.";
        return -1;
      }
    }
    else { // empty frame
      return -1;
    }

    rtc::scoped_refptr<webrtc::I420Buffer> _sendBuffer = webrtc::I420Buffer::Create(send_width, send_height);
    webrtc::I420Buffer::SetBlack(_sendBuffer.get());
    _sendBuffer->ScaleFrom(*_srcBuffer.get());

    // image enhance
    if (mFilterParam.enableEdgeEnhance) {
      ImageFilter(_sendBuffer, "screen");
    }

    webrtc::VideoFrame captureFrame =
      webrtc::VideoFrame::Builder()
      .set_video_frame_buffer(_sendBuffer)
      .set_timestamp_rtp(0)
      .set_timestamp_ms(rtc::TimeMillis())
      .set_rotation(webrtc::kVideoRotation_0)
      .build();

    OnFrame(captureFrame);
    return 0;
  }
}
