#pragma once
#include <iostream>
#include <string>
#include "api/video/i420_buffer.h"
#include "3rdparty/video_process/src/ImageEnhance.h"
#include "signalling/vh_stream.h"

namespace vhall {
  class VideoProcessFilter {
  public:
    virtual void SetFilterParam(LiveVideoFilterParam & filterParam);
  protected:
    virtual void ImageFilter(rtc::scoped_refptr<webrtc::I420Buffer> &video_i420, const std::string type);

    std::shared_ptr<ImageEnhance> imageFilter_;
    VideoFilterParam filterCfg_;
    std::shared_ptr<uint8_t[]> filterBufIn_;
    std::shared_ptr<uint8_t[]> filterBufOut_;

    LiveVideoFilterParam mFilterParam;
  };
}