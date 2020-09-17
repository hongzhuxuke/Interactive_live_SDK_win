
#include "AudioResampler.h"
#include "AudioRSUtil.h"
#include "common/vhall_log.h"

extern "C" {
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

namespace vhall {
  static inline enum AVSampleFormat convert_audio_format(enum audio_format format)
  {
    switch (format) {
    case AUDIO_FORMAT_UNKNOWN:      return AV_SAMPLE_FMT_S16;
    case AUDIO_FORMAT_U8BIT:        return AV_SAMPLE_FMT_U8;
    case AUDIO_FORMAT_16BIT:        return AV_SAMPLE_FMT_S16;
    case AUDIO_FORMAT_32BIT:        return AV_SAMPLE_FMT_S32;
    case AUDIO_FORMAT_FLOAT:        return AV_SAMPLE_FMT_FLT;
    case AUDIO_FORMAT_U8BIT_PLANAR: return AV_SAMPLE_FMT_U8P;
    case AUDIO_FORMAT_16BIT_PLANAR: return AV_SAMPLE_FMT_S16P;
    case AUDIO_FORMAT_32BIT_PLANAR: return AV_SAMPLE_FMT_S32P;
    case AUDIO_FORMAT_FLOAT_PLANAR: return AV_SAMPLE_FMT_FLTP;
    }

    /* shouldn't get here */
    return AV_SAMPLE_FMT_S16;
  }

  AudioResampler::AudioResampler(const resample_info &dst, const resample_info &src) {
    int errcode = 0;
    opened = false;
    input_freq = src.samples_per_sec;
    input_layout = convert_speaker_layout(src.speakers);
    input_format = convert_audio_format(src.format);
    output_size = 0;
    output_ch = get_audio_channels(dst.speakers);
    output_freq = dst.samples_per_sec;
    output_layout = convert_speaker_layout(dst.speakers);
    output_format = convert_audio_format(dst.format);
    output_planes = is_audio_planar(dst.format) ? output_ch : 1;

    context = swr_alloc_set_opts(NULL,
      output_layout, output_format, dst.samples_per_sec,
      input_layout, input_format, src.samples_per_sec,
      0, NULL);

    if (!context) {
      //LOGE("swr_alloc_set_opts failed");
      Destroy();
      goto END;
    }

    errcode = swr_init(context);
    if (errcode != 0) {
      //LOGE("avresample_open failed: error code %d", errcode);
      Destroy();
    }
  END:
    printf("");
  }

  AudioResampler::~AudioResampler() {
    Destroy();
  }

  void AudioResampler::Destroy() {
    if (context) {
      swr_free(&context);
      context = nullptr;
    }
    if (output_buffer[0]) {
      av_freep(&output_buffer[0]);
      output_buffer[0] = nullptr;
    }
  }

  bool AudioResampler::Resample(
    uint8_t *output[], uint32_t *out_frames,
    uint64_t *ts_offset,
    const uint8_t *const input[], uint32_t in_frames)
  {
    int ret = 0;

    int64_t delay = swr_get_delay(context, input_freq);
    int estimated = (int)av_rescale_rnd(
      delay + static_cast<int64_t>(in_frames),
      static_cast<int64_t>(output_freq), static_cast<int64_t>(input_freq),
      AV_ROUND_UP);

    *ts_offset = (uint64_t)swr_get_delay(context, 1000000000);

    /* resize the buffer if bigger */
    if (estimated > output_size) {
      if (output_buffer[0]) {
        av_freep(&output_buffer[0]);
        output_buffer[0] = nullptr;
      }
      
      av_samples_alloc(output_buffer, NULL, output_ch, estimated, output_format, 0);
      output_size = estimated;
    }

    ret = swr_convert(context, output_buffer, output_size, (const uint8_t**)input, in_frames);
    if (ret < 0) {
      //LOGE("swr_convert failed: %d", ret);
      return false;
    }

    for (uint32_t i = 0; i < output_planes; i++) {
      output[i] = output_buffer[i];
    }

    *out_frames = (uint32_t)ret;
    return true;
  }
}