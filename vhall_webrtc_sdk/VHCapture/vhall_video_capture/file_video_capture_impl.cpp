#include "file_video_capture_impl.h"
namespace vhall {

  FileVideoCaptureImpl::FileVideoFrame::FileVideoFrame()
    :width(0),
    height(0),
    frameDuration(0),
    timeScale(0),
    buffer(nullptr),
    bufferSize(0) {
  }

  FileVideoCaptureImpl::FileVideoFrame::~FileVideoFrame() {

  }

  void FileVideoCaptureImpl::FileVideoFrame::InitBuffer(unsigned long long buffer_size) {
    if (!buffer) {
      std::shared_ptr<char> buf(new char[buffer_size]);
      buffer = buf;
    }
    else if (buffer_size != bufferSize) {
      buffer.reset(new char[buffer_size]);
    }
    bufferSize = buffer_size;
  }

  bool FileVideoCaptureImpl::FileVideoFrame::FormatToCapability(webrtc::VideoCaptureCapability &cap) {
    cap.width = width;
    cap.height = height;
    cap.maxFPS = (int32_t)(1000.0 / frameDuration);
    cap.videoType = webrtc::VideoType::kARGB;
    return true;
  }

  FileVideoCaptureImpl::FileVideoCaptureImpl() :_isStart(false) {
    _videoCaptureThread = rtc::Thread::Create();;
  }

  FileVideoCaptureImpl::~FileVideoCaptureImpl() {
    if (!_videoCaptureThread->IsQuitting()) {
      _videoCaptureThread->Clear(this);
      _videoCaptureThread->Stop();
    }
  }

  bool FileVideoCaptureImpl::Init(std::weak_ptr<IMediaOutput> &mediaOutput) {
    std::unique_lock<std::mutex> lock(_mutex);
    _mediaOutput = mediaOutput;
    return true;
  }

  int32_t FileVideoCaptureImpl::StartCapture(const webrtc::VideoCaptureCapability& capability) {
    std::unique_lock<std::mutex> lock(_mutex);
    std::shared_ptr<IMediaOutput> media_output(_mediaOutput.lock());
    if (media_output) {
      if (media_output->GetVideoParam(_frameInfo.width, _frameInfo.height, _frameInfo.frameDuration, _frameInfo.timeScale)) {
        _frameInfo.InitBuffer(_frameInfo.width*_frameInfo.height * 4);
      }
    }
    _requestedCapability = capability;
    _isStart = true;
    _videoCaptureThread->Start();
    _videoCaptureThread->Clear(this);
    _videoCaptureThread->Post(RTC_FROM_HERE_WITH_FUNCTION("vh"), this, VIDEO_CAPTURE_LOOP);
    return 0;
  }

  int32_t FileVideoCaptureImpl::StopCapture() {
    std::unique_lock<std::mutex> lock(_mutex);
    _isStart = false;
    if (!_videoCaptureThread->RunningForTest()) {
      _videoCaptureThread->Clear(this);
      _videoCaptureThread->Stop();
    }
    return 0;
  }

  void FileVideoCaptureImpl::SetCapability(webrtc::VideoCaptureCapability & cap) {
    std::unique_lock<std::mutex> lock(capMtx);
    _requestedCapability.width = cap.width;
    _requestedCapability.height = cap.height;
    _requestedCapability.maxFPS = cap.maxFPS;
    lock.unlock();
  }

  // Returns true if the capture device is running
  bool FileVideoCaptureImpl::CaptureStarted() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _isStart;
  }

  void FileVideoCaptureImpl::OnFrame(const webrtc::VideoFrame & frame) {
    webrtc::VideoCapturer::OnFrame(frame);
  }

  int FileVideoCaptureImpl::pushFrame(uint8_t * videoFrame, size_t videoFrameLength, const webrtc::VideoCaptureCapability & frameInfo, int64_t captureTime) {
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

  void FileVideoCaptureImpl::OnMessage(rtc::Message* msg) {
    switch (msg->message_id)
    {
    case VIDEO_CAPTURE_LOOP:
    {
      if (_isStart) {
        std::unique_lock<std::mutex> lock(capMtx);
        int cmsDelay = 1000.0 / _requestedCapability.maxFPS;
        lock.unlock();

        _videoCaptureThread->Clear(this);
        _videoCaptureThread->PostDelayed(RTC_FROM_HERE_WITH_FUNCTION("vh"), cmsDelay, this, VIDEO_CAPTURE_LOOP);
        OnIncomingFrame();
      }
    }
    break;
    default:
      break;
    }
  }

  void FileVideoCaptureImpl::OnIncomingFrame() {
    std::shared_ptr<IMediaOutput> media_output(_mediaOutput.lock());
    if (media_output) {
      unsigned long width = 0;
      unsigned long height = 0;
      if (media_output->GetVideoParam(width, height, _frameInfo.frameDuration, _frameInfo.timeScale)) {
        if (width == 0 || height == 0) {
          return;
        }
        if (width != _frameInfo.width || height != _frameInfo.height) {
          _frameInfo.width = width;
          _frameInfo.height = height;
          _frameInfo.InitBuffer(_frameInfo.width*_frameInfo.height * 4);
        }
      }
      if (media_output->GetNextVideoBuffer(_frameInfo.buffer.get(), _frameInfo.bufferSize, &_frameInfo.timestamp))
      {
        _frameInfo.FormatToCapability(_srcCapability);
        pushFrame((uint8_t*)_frameInfo.buffer.get(), _frameInfo.bufferSize, _srcCapability);
      }
    }
  }

}