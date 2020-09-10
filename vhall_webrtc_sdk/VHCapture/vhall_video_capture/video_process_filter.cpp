#include "video_process_filter.h"
#include "common/vhall_log.h"

void vhall::VideoProcessFilter::ImageFilter(rtc::scoped_refptr<webrtc::I420Buffer>& video_i420, const std::string type) {
  // reset filter
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

  int pixel_count = video_i420->height() * video_i420->width();
  memcpy(filterBufIn_.get(), video_i420->DataY(), pixel_count);
  memcpy(filterBufIn_.get() + pixel_count, video_i420->DataU(), pixel_count / 4);
  memcpy(filterBufIn_.get() + pixel_count + pixel_count / 4, video_i420->DataV(), pixel_count / 4);

  if (type == "screen") {
    imageFilter_->EdgeEnhance(filterBufIn_.get(), filterBufOut_.get(), PixelFmt_YV12);
  }
  else if (type == "camera") {
    imageFilter_->Denoise(filterBufIn_.get(), filterBufOut_.get(), PixelFmt_YV12);
    memcpy(filterBufIn_.get(), filterBufOut_.get(), pixel_count * 3 / 2);
    imageFilter_->Brighter(filterBufIn_.get(), filterBufOut_.get(), PixelFmt_YV12);
  }

  memcpy(video_i420->MutableDataY(), filterBufOut_.get(), pixel_count);
  memcpy(video_i420->MutableDataU(), filterBufOut_.get() + pixel_count, pixel_count / 4);
  memcpy(video_i420->MutableDataV(), filterBufOut_.get() + pixel_count + pixel_count / 4, pixel_count / 4);

  return;
}

void vhall::VideoProcessFilter::SetFilterParam(LiveVideoFilterParam & filterParam){
  mFilterParam = filterParam;
}
