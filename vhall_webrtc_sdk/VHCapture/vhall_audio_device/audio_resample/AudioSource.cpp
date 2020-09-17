#include "AudioSource.h"

namespace vhall {
  AudioSource::AudioSource(resample_info &source_audio_info, resample_info & sink_audio_info) {
    source_audio_info_ = source_audio_info;
    sink_audio_info_ = sink_audio_info;

    if (ValidAudioParams(sink_audio_info_) && ValidAudioParams(source_audio_info_)) {
      ResetResampler();
    }

    audio_data_.reset(new SourceAudioData());
    audio_data_->speakers = sink_audio_info.speakers;
    audio_data_->format = sink_audio_info.format;
  }

  AudioSource::~AudioSource() {
    resampler_.reset();
  }

  bool AudioSource::InsertAudioData(SourceAudioData & audio) {
    if (source_audio_info_.samples_per_sec != audio.samples_per_sec ||
      source_audio_info_.format != audio.format ||
      source_audio_info_.speakers != audio.speakers)
    {
      source_audio_info_.format = audio.format;
      source_audio_info_.samples_per_sec = audio.samples_per_sec;
      source_audio_info_.speakers = audio.speakers;
      ResetResampler();
    }

    ProcessAudio(audio);
    
    return true;
  }

  bool AudioSource::ValidAudioParams(const resample_info & info) {
    return info.samples_per_sec > 0 && info.format != AUDIO_FORMAT_UNKNOWN && info.speakers != SPEAKERS_UNKNOWN;
  }

  const SourceAudioData * AudioSource::GetAudioData() {
    if (!audio_data_ || !audio_data_->data[0] || audio_data_->frames == 0) {
      return nullptr;
    }

    return audio_data_.get();
  }

  void AudioSource::ResetSourceFormat(resample_info & source_audio_info) {

  }

  void AudioSource::ProcessAudio(SourceAudioData & audio) {
    uint32_t frames = audio.frames;
    uint8_t *intput[MAX_AV_PLANES] = { audio.data[0].get(), audio.data[1].get(), audio.data[3].get(),
                                       audio.data[3].get(), audio.data[4].get(), audio.data[5].get(),
                                       audio.data[6].get(),audio.data[7].get() };
    if (resampler_) {
      uint8_t *output[MAX_AV_PLANES] = { 0 };
      resampler_->Resample(output, &frames, &audio.ts_resample_offset, intput, audio.frames);
      CopyData(output, frames, audio.timestamp); // output's memory managed by resampler
    }
    else {
      CopyData(intput, audio.frames, audio.timestamp);
    }
  }

  void AudioSource::ResetResampler() {
    resampler_.reset();
    
    if (audio_data_) {
      audio_data_->CleanBuffer();
    }

    if (source_audio_info_ != sink_audio_info_) {
      resampler_.reset(new AudioResampler(sink_audio_info_, source_audio_info_));
    }
  }

  void AudioSource::CopyData(uint8_t *data[], uint32_t frames, uint64_t ts) {
    bool planar = is_audio_planar(sink_audio_info_.format);
    audio_data_->channels_ = get_audio_channels(sink_audio_info_.speakers);
    audio_data_->planes_ = get_audio_planes(sink_audio_info_.format, sink_audio_info_.speakers); /* 交错格式音频plane都为1 */
    audio_data_->block_size_ = (planar ? 1 : audio_data_->channels_) *
      get_audio_bytes_per_channel(sink_audio_info_.format); /* 通道数 x 采样点字节数per通道 */
    size_t data_size = (size_t)frames * audio_data_->block_size_;
    bool bResize = data_size != audio_data_->size_;

    audio_data_->frames = frames;
    audio_data_->timestamp = ts;
    for (size_t i = 0; i < audio_data_->planes_; i++) {
      /* ensure audio storage capacity */
      if (bResize) {
        if (audio_data_->data[i]) {
          audio_data_->data[i].reset();
        }

        audio_data_->size_ = data_size;
        std::shared_ptr<uint8_t[]> tmp(new uint8_t[audio_data_->size_]);
        memset(tmp.get(), 0, audio_data_->size_);
        audio_data_->data[i] = tmp;
      }

      memcpy(audio_data_->data[i].get(), data[i], audio_data_->size_);
    }

  }
}