/***** audio resampler based on ffmpeg *****/
#pragma once
#include <iostream>
#include "AudioRSUtil.h"

extern "C" {
#include "libavformat/avformat.h"
}

struct SwrContext;

namespace vhall {

  class resample_info final {
  public:
    uint32_t            samples_per_sec = 0;
    enum audio_format   format = AUDIO_FORMAT_UNKNOWN;
    enum speaker_layout speakers = SPEAKERS_UNKNOWN;

    bool operator==(resample_info& info) {
      return samples_per_sec == info.samples_per_sec && format == info.format && speakers == info.speakers;
    };
    bool operator!=(resample_info& info) {
      return samples_per_sec != info.samples_per_sec || format != info.format || speakers != info.speakers;
    };
  };

  __declspec(dllexport) class AudioResampler final {
  public:
    AudioResampler(const resample_info &dst, const resample_info &src);
    ~AudioResampler();

    bool Resample(uint8_t *output[], uint32_t *out_frames, uint64_t *ts_offset,
                  const uint8_t *const input[], uint32_t in_frames);
    void Destroy();
  private:
    struct SwrContext   *context = nullptr;
    bool                opened = false;

    uint32_t            input_freq = 0;
    uint64_t            input_layout = 0;
    enum AVSampleFormat input_format;

    uint8_t             *output_buffer[MAX_AV_PLANES] = {};
    uint64_t            output_layout = 0;
    enum AVSampleFormat output_format;
    int                 output_size = 0;
    uint32_t            output_ch = 0;
    uint32_t            output_freq = 0;
    uint32_t            output_planes = 0;
  };



};