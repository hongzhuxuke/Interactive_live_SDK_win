#include "picture_capture_impl.h"
#include "../../signalling/vh_stream.h"
#include "VhallRectUtil.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"
#include "api/task_queue/task_queue_factory.h" //m78
#include "rtc_base/thread.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "third_party/libyuv/include/libyuv.h"

namespace vhall {
  PictureCaptureImpl * PictureCaptureImpl::Create(std::shared_ptr<vhall::I420Data>& picture, const webrtc::VideoCaptureCapability & capability) {
    std::unique_ptr<PictureCaptureImpl> picCapImp(new PictureCaptureImpl(picture, capability));
    return picCapImp.release();
  }

  PictureCaptureImpl::PictureCaptureImpl(std::shared_ptr<vhall::I420Data>& picture, const webrtc::VideoCaptureCapability & capability) :_isStart(false) {
    _videoCaptureThread = rtc::Thread::Create();/*std::make_shared<rtc::Thread>();*/
    mSrcPic = picture;
    _requestedCapability = capability;
    StartCapture(capability);
  }

  PictureCaptureImpl::~PictureCaptureImpl() {
    if (!_videoCaptureThread->IsQuitting())
    {
      _videoCaptureThread->Clear(this);
      _videoCaptureThread->Stop();
    }
  }

  int32_t PictureCaptureImpl::StartCapture(const webrtc::VideoCaptureCapability & capability) {
    _requestedCapability = capability;
    _isStart = true;

    _videoCaptureThread->Start();
    _videoCaptureThread->Clear(this);
    _videoCaptureThread->Post(RTC_FROM_HERE_WITH_FUNCTION("vh"), this, CAPTURE_LOOP);
    return 0;
  }

  int32_t PictureCaptureImpl::StopCapture() {
    std::unique_lock<std::mutex> lock(_mutex);
    _isStart = false;
    if (!_videoCaptureThread->RunningForTest()) {
      _videoCaptureThread->Clear(this);
      _videoCaptureThread->Stop();
    }
    return 0;
  }

  void PictureCaptureImpl::SetCapability(webrtc::VideoCaptureCapability & cap) {
    std::unique_lock<std::mutex> lock(capMtx);
    _requestedCapability.width = cap.width;
    _requestedCapability.height = cap.height;
    _requestedCapability.maxFPS = cap.maxFPS;
    lock.unlock();
  }
  
  bool PictureCaptureImpl::CaptureStarted() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _isStart;
  }
  
  void PictureCaptureImpl::OnFrame(const webrtc::VideoFrame & frame) {
    webrtc::VideoCapturer::OnFrame(frame);
  }

  void PictureCaptureImpl::OnMessage(rtc::Message * msg) {
    switch (msg->message_id) {
    case CAPTURE_LOOP:
    {
      if (_isStart) {
        std::unique_lock<std::mutex> lock(capMtx);
        int cmsDelay = 1000.0 / _requestedCapability.maxFPS;
        lock.unlock();

        _videoCaptureThread->Clear(this);
        _videoCaptureThread->PostDelayed(RTC_FROM_HERE_WITH_FUNCTION("vh"), cmsDelay, this, CAPTURE_LOOP);
        OnIncomingFrame();
      }
    }
    break;
    default:
      break;
    }
  }

  void PictureCaptureImpl::OnIncomingFrame() {
    webrtc::VideoCaptureCapability cap;
    if (mSrcPic.get() && (mSrcPic->height * mSrcPic->width * 3 / 2) > 0) {
      cap.width = mSrcPic->width;
      cap.height = mSrcPic->height;

      std::unique_lock<std::mutex> lock(capMtx);
      cap.maxFPS = _requestedCapability.maxFPS;
      cap.videoType = _requestedCapability.videoType;
      lock.unlock();

      pushFrame(mSrcPic->mData.get(), mSrcPic->height * mSrcPic->width * 3 / 2, cap, rtc::TimeMillis());
    }
    else {
      pushFrame(NULL, 0, cap);
    }
  }

  int PictureCaptureImpl::pushFrame(uint8_t * videoFrame, size_t videoFrameLength, const webrtc::VideoCaptureCapability & frameInfo, int64_t captureTime) {
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

      if (target_width != send_width) {
        send_height = (target_height *send_width / target_width + 1) / 2 * 2;
      }

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
      rect = VhallRectUtil::adjustFitScreenFrame(VhallRectUtil::CGRect(0.0, 0.0, target_width, target_height), VhallRectUtil::CGRect(0.0, 0.0, send_width, send_height));
    }

    rtc::scoped_refptr<webrtc::I420Buffer> _sendBuffer = webrtc::I420Buffer::Create(send_width, send_height);
    webrtc::I420Buffer::SetBlack(_sendBuffer.get());

    if (rect.isValid()) {
      rect.toInt();
      rtc::scoped_refptr<webrtc::I420Buffer> _scaleBuffer = webrtc::I420Buffer::Create(rect.width, rect.height);
      webrtc::I420Buffer::SetBlack(_scaleBuffer.get());
      _scaleBuffer->ScaleFrom(*_srcBuffer.get());
      for (int i = 0; i < _scaleBuffer->height(); i++) {
        memcpy(_sendBuffer->MutableDataY() + _sendBuffer->StrideY()*(i + (int)rect.y) + (int)rect.x, _scaleBuffer->MutableDataY() + _scaleBuffer->StrideY()*i, _scaleBuffer->StrideY());
        if (i < _scaleBuffer->height() / 2) {
          memcpy(_sendBuffer->MutableDataU() + _sendBuffer->StrideU()*(i + (int)(rect.y / 2)) + (int)(rect.x / 2), _scaleBuffer->MutableDataU() + _scaleBuffer->StrideU()*i, _scaleBuffer->StrideU());
          memcpy(_sendBuffer->MutableDataV() + _sendBuffer->StrideV()*(i + (int)(rect.y / 2)) + (int)(rect.x / 2), _scaleBuffer->MutableDataV() + _scaleBuffer->StrideV()*i, _scaleBuffer->StrideV());
        }
      }
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