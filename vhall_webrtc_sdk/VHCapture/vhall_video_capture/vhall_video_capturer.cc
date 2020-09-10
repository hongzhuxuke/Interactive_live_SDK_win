#include "vhall_video_capturer.h"

#include <algorithm>
#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"

namespace webrtc {
  VideoCapturer::VideoCapturer() = default;
  VideoCapturer::~VideoCapturer() = default;

  void VideoCapturer::OnFrame(const webrtc::VideoFrame& frame) {
    int cropped_width = 0;
    int cropped_height = 0;
    int out_width = 0;
    int out_height = 0;

    webrtc::VideoFrame frameToDeliver(frame);
    /* Í¼ÏñÔ¤´¦Àí */
    if (preProcess) {
      bool ret = preProcess->VideoPreProcess(frameToDeliver, frame.width(), frame.height());
      if (!ret) {
        return;
      }
    }

    if (!video_adapter_.AdaptFrameResolution(
      frameToDeliver.width(), frameToDeliver.height(), frameToDeliver.timestamp_us() * 1000,
      &cropped_width, &cropped_height, &out_width, &out_height)) {
      // Drop frame in order to respect frame rate constraint.
      return;
    }

    out_height = (out_height + 3) / 4 * 4;
    out_width = (out_width + 3) / 4 * 4;

    if (out_height != frameToDeliver.height() || out_width != frameToDeliver.width()) {
      // Video adapter has requested a down-scale. Allocate a new buffer and
      // return scaled version.
      rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer = webrtc::I420Buffer::Create(out_width, out_height);
      scaled_buffer->ScaleFrom(*frameToDeliver.video_frame_buffer()->ToI420());
      broadcaster_.OnFrame(webrtc::VideoFrame::Builder()
        .set_video_frame_buffer(scaled_buffer)
        .set_rotation(webrtc::kVideoRotation_0)
        .set_timestamp_us(frameToDeliver.timestamp_us())
        .set_id(frameToDeliver.id())
        .build());
    }
    else {
      // No adaptations needed, just return the frame as is.
      broadcaster_.OnFrame(frameToDeliver);
    }
  }

  rtc::VideoSinkWants VideoCapturer::GetSinkWants() {
    return broadcaster_.wants();
  }

  void VideoCapturer::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
    broadcaster_.AddOrUpdateSink(sink, wants);
    UpdateVideoAdapter();
  }

  void VideoCapturer::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
    broadcaster_.RemoveSink(sink);
    UpdateVideoAdapter();
  }

  void VideoCapturer::UpdateVideoAdapter() {
    rtc::VideoSinkWants wants = broadcaster_.wants();
    video_adapter_.OnResolutionFramerateRequest(wants.target_pixel_count, wants.max_pixel_count, wants.max_framerate_fps);
  }

}  // namespace vhall

