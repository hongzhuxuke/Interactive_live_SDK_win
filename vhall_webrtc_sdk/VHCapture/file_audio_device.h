
#ifndef VHALL_AUDIO_DEVICE_FILE_AUDIO_DEVICE_H_
#define VHALL_AUDIO_DEVICE_FILE_AUDIO_DEVICE_H_

#include <cstdio>

#include <memory>
#include <string>

#include "3rdparty/media_reader/IMediaOutput.h"
#include "common/vhall_define.h"
#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/system/file_wrapper.h"

namespace rtc {
   class PlatformThread;
}  // namespace rtc

namespace webrtc {

  class EventWrapper;

  // This is a fake audio device which plays audio from a file as its microphone
  // and plays out into a file.
  class VHFileAudioDevice : public webrtc::AudioDeviceGeneric {
  public:
    // Constructs a file audio device with |id|. It will read audio from
    // |inputFilename| and record output audio to |outputFilename|.
    //
    // The input file should be a readable 48k stereo raw file, and the output
    // file should point to a writable location. The output format will also be
    // 48k stereo raw audio.
    VHFileAudioDevice(std::weak_ptr<IMediaOutput> &audio_output);
    virtual ~VHFileAudioDevice();

    // Retrieve the currently utilized audio layer
    int32_t ActiveAudioLayer(
      AudioDeviceModule::AudioLayer& audioLayer) const override;

    // Main initializaton and termination
    InitStatus Init() override;
    int32_t Terminate() override;
    bool Initialized() const override;

    // Device enumeration
    int16_t PlayoutDevices() override { return -1; };
    int16_t RecordingDevices() override { return 1; };
    int32_t PlayoutDeviceName(uint16_t index,
      char name[kAdmMaxDeviceNameSize],
      char guid[kAdmMaxGuidSize]) override {
      return -1;
    };
    int32_t RecordingDeviceName(uint16_t index,
      char name[kAdmMaxDeviceNameSize],
      char guid[kAdmMaxGuidSize]) override;

    // Device selection
    int32_t SetPlayoutDevice(uint16_t index) override { return 0; };
    int32_t SetPlayoutDevice(
      AudioDeviceModule::WindowsDeviceType device) override {
      return 0;
    };
    int32_t SetRecordingDevice(uint16_t index) override { return 0; };
    int32_t SetRecordingDevice(
      AudioDeviceModule::WindowsDeviceType device) override {
      return 0;
    };

    // Audio transport initialization
    int32_t PlayoutIsAvailable(bool& available) override { available = false; return -1; };
    int32_t InitPlayout() override { return -1; };
    bool PlayoutIsInitialized() const override { return false; };
    int32_t RecordingIsAvailable(bool& available) override;
    int32_t InitRecording() override;
    bool RecordingIsInitialized() const override;

    // Audio transport control
    int32_t StartPlayout() override { return -1; };
    int32_t StopPlayout() override { return -1; };
    bool Playing() const override { return false; };
    int32_t StartRecording() override;
    int32_t StopRecording() override;
    bool Recording() const override;

    // Audio mixer initialization
    int32_t InitSpeaker() override { return 0; };
    bool SpeakerIsInitialized() const override { return false; };
    int32_t InitMicrophone() override { return 0; };
    bool MicrophoneIsInitialized() const override { return false; };

    // Speaker volume controls
    int32_t SpeakerVolumeIsAvailable(bool& available) override { return 0; };
    int32_t SetSpeakerVolume(uint32_t volume) override { return 0; };
    int32_t SpeakerVolume(uint32_t& volume) const override { return 0; };
    int32_t MaxSpeakerVolume(uint32_t& maxVolume) const override { return 0; };
    int32_t MinSpeakerVolume(uint32_t& minVolume) const override { return 0; };

    // Microphone volume controls
    int32_t MicrophoneVolumeIsAvailable(bool& available) override { return 0; };
    int32_t SetMicrophoneVolume(uint32_t volume) override { return 0; };
    int32_t MicrophoneVolume(uint32_t& volume) const override { return 0; };
    int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const override { return 0; };
    int32_t MinMicrophoneVolume(uint32_t& minVolume) const override { return 0; };

    // Speaker mute control
    int32_t SpeakerMuteIsAvailable(bool& available) override { return 0; };
    int32_t SetSpeakerMute(bool enable) override { return 0; };
    int32_t SpeakerMute(bool& enabled) const override { return 0; };

    // Microphone mute control
    int32_t MicrophoneMuteIsAvailable(bool& available) override { return 0; };
    int32_t SetMicrophoneMute(bool enable) override { return 0; };
    int32_t MicrophoneMute(bool& enabled) const override { return 0; };

    // Stereo support
    int32_t StereoPlayoutIsAvailable(bool& available) override { return -1; };
    int32_t SetStereoPlayout(bool enable) override { return 0; };
    int32_t StereoPlayout(bool& enabled) const override { return -1; };
    int32_t StereoRecordingIsAvailable(bool& available) override { return -1; };
    int32_t SetStereoRecording(bool enable) override { return 0; };
    int32_t StereoRecording(bool& enabled) const override { return -1; };

    // Delay information and control
    int32_t PlayoutDelay(uint16_t& delayMS) const override { return -1; };

    void AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) override;

  private:
    static void RecThreadFunc(void*);
    bool RecThreadProcess();

    AudioDeviceBuffer* _ptrAudioBuffer;
    // TODO(pbos): Make plain members instead of pointers and stop resetting them.
    std::unique_ptr<rtc::PlatformThread> mThreadRecPtr;
    int64_t _lastCallRecordMillis;

    rtc::CriticalSection mCritSect_;
    bool mIsRecordingInitialized;
    std::weak_ptr<IMediaOutput> mAudioMediaOutput;
    unsigned int mAudioChannels;
    unsigned int mSamplesPerSec;
    unsigned int mBitsPerSample;
    unsigned int mRecAudioFrameSize;
    bool mIsRecording;

    int8_t* mRecordingBuffer;  // In bytes.  
    size_t mRecordingBufferFrameIndex;
    size_t mRecordingBufferFrameCount;
    size_t mRecordingBufferSizeIn10MS;
    size_t mRecordingFramesIn10MS;
  };
}
#endif  // AUDIO_DEVICE_FILE_AUDIO_DEVICE_H_
