#pragma once

#include <stddef.h>
#include <memory>
#include "api/video/video_frame.h"
#include "api/video/video_source_interface.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "modules/video_capture/video_capture_defines.h"

namespace webrtc {
  class VideoPreProcessInterface {
  public:
    virtual bool VideoPreProcess(webrtc::VideoFrame& frame, int width, int height) = 0;
  };

class VideoCapturer : public rtc::VideoSourceInterface<webrtc::VideoFrame> {
 public:
  VideoCapturer();
  ~VideoCapturer() override;

  void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
  void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;

  virtual void RegisterPreProcess(webrtc::VideoPreProcessInterface* preProcess) {
    this->preProcess = preProcess;
  }

  virtual void SetCapability(webrtc::VideoCaptureCapability& cap) = 0;
 protected:
  virtual void OnFrame(const webrtc::VideoFrame& frame);
  rtc::VideoSinkWants GetSinkWants();

 private:
  void UpdateVideoAdapter();
  VideoPreProcessInterface* preProcess = nullptr;
  rtc::VideoBroadcaster broadcaster_;
  cricket::VideoAdapter video_adapter_;
};
}  // namespace vhall