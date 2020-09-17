#pragma once

#include <iostream>
#include <string>
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <shlobj.h>
#include <intrin.h>
#include <psapi.h>

#include "util_uint128.h"
extern "C" {
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
}

#define EXPORT __declspec(dllexport)
#define MAX_AUDIO_CHANNELS  8

#define MAX_AV_PLANES 8

namespace vhall {

  enum audio_format {
    AUDIO_FORMAT_UNKNOWN,

    AUDIO_FORMAT_U8BIT,
    AUDIO_FORMAT_16BIT,
    AUDIO_FORMAT_32BIT,
    AUDIO_FORMAT_FLOAT,

    AUDIO_FORMAT_U8BIT_PLANAR,
    AUDIO_FORMAT_16BIT_PLANAR,
    AUDIO_FORMAT_32BIT_PLANAR,
    AUDIO_FORMAT_FLOAT_PLANAR,
  };

  /**
   * The speaker layout describes where the speakers are located in the room.
   * For OBS it dictates:
   *  *  how many channels are available and
   *  *  which channels are used for which speakers.
   *
   * Standard channel layouts where retrieved from ffmpeg documentation at:
   *     https://trac.ffmpeg.org/wiki/AudioChannelManipulation
   */
  enum speaker_layout {
    SPEAKERS_UNKNOWN,   /**< Unknown setting, fallback is stereo. */
    SPEAKERS_MONO,      /**< Channels: MONO */
    SPEAKERS_STEREO,    /**< Channels: FL, FR */
    SPEAKERS_2POINT1,   /**< Channels: FL, FR, LFE */
    SPEAKERS_2_1,
    SPEAKERS_SURROUND,
    SPEAKERS_3POINT1,
    SPEAKERS_4POINT0,   /**< Channels: FL, FR, FC, RC */
    SPEAKERS_4POINT1,   /**< Channels: FL, FR, FC, LFE, RC */
    SPEAKERS_2_2,
    SPEAKERS_QUAD,
    SPEAKERS_5POINT0,
    SPEAKERS_5POINT1,   /**< Channels: FL, FR, FC, LFE, RL, RR */
    SPEAKERS_5POINT0_BACK,
    SPEAKERS_5POINT1_BACK,
    SPEAKERS_6POINT0,
    SPEAKERS_6POINT0_FRONT,
    SPEAKERS_HEXAGONAL,
    SPEAKERS_6POINT1,
    SPEAKERS_6POINT1_BACK,
    SPEAKERS_6POINT1_FRONT,
    SPEAKERS_7POINT0,
    SPEAKERS_7POINT0_FRONT,
    SPEAKERS_7POINT1, /**< Channels: FL, FR, FC, LFE, RL, RR, SL, SR */
    SPEAKERS_7POINT1_WIDE,
    SPEAKERS_7POINT1_WIDE_BACK,
    SPEAKERS_OCTAGONAL,
    SPEAKERS_HEXADECAGONAL,
    SPEAKERS_STEREO_DOWNMIX,
  };
  
  inline uint64_t convert_speaker_layout(enum speaker_layout layout)
  {
    switch (layout) {
    case SPEAKERS_UNKNOWN:          return 0;
    case SPEAKERS_MONO:             return AV_CH_LAYOUT_MONO;
    case SPEAKERS_STEREO:           return AV_CH_LAYOUT_STEREO;
    case SPEAKERS_2POINT1:          return AV_CH_LAYOUT_SURROUND;
    case SPEAKERS_2_1:              return AV_CH_LAYOUT_2_1;
    case SPEAKERS_SURROUND:         return AV_CH_LAYOUT_SURROUND;
    case SPEAKERS_3POINT1:          return AV_CH_LAYOUT_3POINT1;
    case SPEAKERS_4POINT0:          return AV_CH_LAYOUT_4POINT0;
    case SPEAKERS_4POINT1:          return AV_CH_LAYOUT_4POINT1;
    case SPEAKERS_2_2:              return AV_CH_LAYOUT_2_2;
    case SPEAKERS_QUAD:             return AV_CH_LAYOUT_QUAD;
    case SPEAKERS_5POINT0:          return AV_CH_LAYOUT_5POINT0;
    case SPEAKERS_5POINT1:          return AV_CH_LAYOUT_5POINT1_BACK;
    case SPEAKERS_5POINT0_BACK:     return AV_CH_LAYOUT_5POINT0_BACK;
    case SPEAKERS_5POINT1_BACK:     return AV_CH_LAYOUT_5POINT1_BACK;
    case SPEAKERS_6POINT0:          return AV_CH_LAYOUT_6POINT0;
    case SPEAKERS_6POINT0_FRONT:    return AV_CH_LAYOUT_6POINT0_FRONT;
    case SPEAKERS_HEXAGONAL:        return AV_CH_LAYOUT_HEXAGONAL;
    case SPEAKERS_6POINT1:          return AV_CH_LAYOUT_6POINT1;
    case SPEAKERS_6POINT1_BACK:     return AV_CH_LAYOUT_6POINT1_BACK;
    case SPEAKERS_6POINT1_FRONT:    return AV_CH_LAYOUT_6POINT1_FRONT;
    case SPEAKERS_7POINT0:          return AV_CH_LAYOUT_7POINT0;
    case SPEAKERS_7POINT0_FRONT:    return AV_CH_LAYOUT_7POINT0_FRONT;
    case SPEAKERS_7POINT1:          return AV_CH_LAYOUT_7POINT1;
    case SPEAKERS_7POINT1_WIDE:     return AV_CH_LAYOUT_7POINT1_WIDE;
    case SPEAKERS_7POINT1_WIDE_BACK:return AV_CH_LAYOUT_7POINT1_WIDE_BACK;
    case SPEAKERS_OCTAGONAL:        return AV_CH_LAYOUT_OCTAGONAL;
    case SPEAKERS_HEXADECAGONAL:    return AV_CH_LAYOUT_HEXADECAGONAL;
    case SPEAKERS_STEREO_DOWNMIX:   return AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    /* shouldn't get here */
    return 0;
  }

  static inline uint32_t get_audio_channels(enum speaker_layout speakers) {
    uint64_t channel_layout = convert_speaker_layout(speakers);
    if (channel_layout > 0) {
      return av_get_channel_layout_nb_channels(convert_speaker_layout(speakers));
    }

    return 0;
  }

  static inline size_t get_audio_bytes_per_channel(enum audio_format format) {
    switch (format) {
    case AUDIO_FORMAT_U8BIT:
    case AUDIO_FORMAT_U8BIT_PLANAR:
      return 1;

    case AUDIO_FORMAT_16BIT:
    case AUDIO_FORMAT_16BIT_PLANAR:
      return 2;

    case AUDIO_FORMAT_FLOAT:
    case AUDIO_FORMAT_FLOAT_PLANAR:
    case AUDIO_FORMAT_32BIT:
    case AUDIO_FORMAT_32BIT_PLANAR:
      return 4;

    case AUDIO_FORMAT_UNKNOWN:
      return 0;
    }

    return 0;
  }

  static inline bool is_audio_planar(enum audio_format format) {
    switch (format) {
    case AUDIO_FORMAT_U8BIT:
    case AUDIO_FORMAT_16BIT:
    case AUDIO_FORMAT_32BIT:
    case AUDIO_FORMAT_FLOAT:
      return false;

    case AUDIO_FORMAT_U8BIT_PLANAR:
    case AUDIO_FORMAT_FLOAT_PLANAR:
    case AUDIO_FORMAT_16BIT_PLANAR:
    case AUDIO_FORMAT_32BIT_PLANAR:
      return true;

    case AUDIO_FORMAT_UNKNOWN:
      return false;
    }

    return false;
  }

  static inline size_t get_audio_planes(enum audio_format format, enum speaker_layout speakers) {
    return (is_audio_planar(format) ? get_audio_channels(speakers) : 1);
  }

  static inline size_t get_audio_size(enum audio_format format, enum speaker_layout speakers, uint32_t frames) {
    bool planar = is_audio_planar(format);

    return (planar ? 1 : get_audio_channels(speakers)) *
      get_audio_bytes_per_channel(format) *
      frames;
  }

  static inline uint64_t audio_frames_to_ns(size_t sample_rate, uint64_t frames) {
    util_uint128_t val;
    val = util_mul64_64(frames, 1000000000ULL);
    val = util_div128_32(val, (uint32_t)sample_rate);
    return val.low;
  }

  static inline uint64_t ns_to_audio_frames(size_t sample_rate, uint64_t frames) {
    util_uint128_t val;
    val = util_mul64_64(frames, sample_rate);
    val = util_div128_32(val, 1000000000);
    return val.low;
  }

  static bool have_clockfreq = false;
  static LARGE_INTEGER clock_freq;
  static uint32_t winver = 0;
  static inline uint64_t get_clockfreq(void) {
    if (!have_clockfreq) {
      QueryPerformanceFrequency(&clock_freq);
      have_clockfreq = true;
    }

    return clock_freq.QuadPart;
  }

  inline uint64_t os_gettime_ns(void) {
    LARGE_INTEGER current_time;
    double time_val;

    QueryPerformanceCounter(&current_time);
    time_val = (double)current_time.QuadPart;
    time_val *= 1000000000.0;
    time_val /= (double)get_clockfreq();

    return (uint64_t)time_val;
  }

}