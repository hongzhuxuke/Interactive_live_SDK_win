#pragma once
#include "api/scoped_refptr.h"
#include "modules/video_capture/video_capture.h"
#include "vhall_video_capturer.h"
#include "rtc_base/thread.h"
#include "3rdparty/media_reader/IMediaOutput.h"
#include "VhallRectUtil.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/logging.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "third_party/libyuv/include/libyuv.h"
#include <mutex>
#include "3rdparty/media_reader/MediaReader.h"
#include "common/vhall_log.h"

namespace vhall {

  class FileVideoCaptureImpl :public webrtc::VideoCapturer,
                              /*public rtc::VideoSinkInterface<webrtc::VideoFrame>,*/
                              public rtc::MessageHandler
  {
  public:
    class FileVideoFrame
    {
    public:
      FileVideoFrame();
      virtual ~FileVideoFrame();
      void InitBuffer(unsigned long long buffer_size);
      bool FormatToCapability(webrtc::VideoCaptureCapability &cap);
    public:
      unsigned long width = 0;
      unsigned long height = 0;
      long long frameDuration = 0;
      long long timeScale = 0;
      std::shared_ptr<char> buffer;
      unsigned long long bufferSize = 0;
      unsigned long long timestamp = 0;
    };

    static FileVideoCaptureImpl* Create(const webrtc::VideoCaptureCapability & capability) {
      std::unique_ptr<FileVideoCaptureImpl> fileCapImp(new FileVideoCaptureImpl());
      std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
      std::weak_ptr<IMediaOutput> media_output(media_reader->GetMediaOut());
      if (!fileCapImp->Init(media_output)) {
        LOGE("Init File Capture failure!");
        return nullptr;
      }
      fileCapImp->StartCapture(capability);
      return fileCapImp.release();
    };

    FileVideoCaptureImpl();
    virtual ~FileVideoCaptureImpl();

    bool Init(std::weak_ptr<IMediaOutput> &mediaOutput);

    // Start capture device
    int32_t StartCapture(const webrtc::VideoCaptureCapability& capability);

    int32_t StopCapture();

    virtual void SetCapability(webrtc::VideoCaptureCapability& cap);

    // Returns true if the capture device is running
    bool CaptureStarted();
  protected:
    virtual void OnFrame(const webrtc::VideoFrame& frame);
    int pushFrame(
      uint8_t* videoFrame,
      size_t videoFrameLength,
      const webrtc::VideoCaptureCapability& frameInfo,
      int64_t captureTime = 0);
    webrtc::VideoCaptureCapability _requestedCapability;
    std::mutex                   capMtx;
    rtc::CriticalSection _apiCs;
  private:
    enum {
      VIDEO_CAPTURE_LOOP
    };

    void OnMessage(rtc::Message* msg) override;

    void OnIncomingFrame();

    FileVideoFrame _frameInfo;

    std::unique_ptr<rtc::Thread> _videoCaptureThread;

    bool _isStart;

    std::mutex _mutex;

    std::weak_ptr<IMediaOutput> _mediaOutput;

    webrtc::VideoCaptureCapability _srcCapability;
  };

}