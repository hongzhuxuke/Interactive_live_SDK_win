#pragma once
#include "AudioResampler.h"

namespace vhall {
  class SourceAudioData final {
  public:
    ~SourceAudioData() {};
    void CleanBuffer() {
      for (int i = 0; i < MAX_AV_PLANES; i++) {
        if (nullptr != data[i]) {
          data[i] = nullptr;
        }
      }

      planar_ = false;
      block_size_ = 0;
      channels_ = 0;
      planes_ = 0;
      size_ = 0;
      frames = 0;
    }

    std::shared_ptr<uint8_t[]> data[MAX_AV_PLANES];
    uint32_t            frames = 0;

    enum speaker_layout speakers = SPEAKERS_UNKNOWN;
    enum audio_format   format = AUDIO_FORMAT_UNKNOWN;
    uint32_t            samples_per_sec = 0;

    uint64_t            timestamp = 0;
    uint64_t            ts_resample_offset = 0;

    // extra info
    bool                planar_ = false;
    size_t              block_size_ = 0;
    size_t              channels_ = 0;
    size_t              planes_ = 0;
    size_t              size_ = 0;
  };

  class AudioSource final {
  public:
    AudioSource(resample_info &source_audio_info, resample_info &sink_audio_info);
    ~AudioSource();
    bool InsertAudioData(SourceAudioData &audio);
    bool ValidAudioParams(const resample_info &info);
    const SourceAudioData* GetAudioData();
  private:
    void ResetSourceFormat(resample_info &source_audio_info);
    void ProcessAudio(SourceAudioData &audio);
    void ResetResampler();
    void CopyData(uint8_t *data[], uint32_t frames, uint64_t ts);

    resample_info            source_audio_info_;
    resample_info            sink_audio_info_;
    std::shared_ptr<AudioResampler> resampler_;

    std::shared_ptr<SourceAudioData> audio_data_;
  };
}