/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_AUDIO_DEVICE_AUDIO_DEVICE_CUSTOM_H_
#define MODULES_AUDIO_DEVICE_AUDIO_DEVICE_CUSTOM_H_

#if defined(WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE)

#include <memory>
#include "common/vhall_define.h"
#include "3rdparty/media_reader/IMediaOutput.h"
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/checks.h"
#include "rtc_base/critical_section.h"

namespace webrtc {
  class AudioDeviceGeneric;

  class AudioDeviceModuleCustom : public webrtc::AudioDeviceModule {
  public:
    enum PlatformType {
      kPlatformNotSupported = 0,
      kPlatformWin32 = 1,
      kPlatformWinCe = 2,
      kPlatformLinux = 3,
      kPlatformMac = 4,
      kPlatformAndroid = 5,
      kPlatformIOS = 6
    };

    enum AudioLayer2 {
      kPlatformDefaultAudio = 0,
      kWindowsCoreAudio = 2,
      kLinuxAlsaAudio = 3,
      kLinuxPulseAudio = 4,
      kAndroidJavaAudio = 5,
      kAndroidOpenSLESAudio = 6,
      kAndroidJavaInputAndOpenSLESOutputAudio = 7,
      kDummyAudio = 8,
      kFileAudio = 8,
    };

  public:
    static rtc::scoped_refptr<AudioDeviceModule> Create(
      const AudioLayer2 audio_layer, std::weak_ptr<IMediaOutput> &audio_output);

    int32_t CheckPlatform();
    int32_t CreatePlatformSpecificObjects();
    int32_t AttachAudioBuffer();

    AudioDeviceModuleCustom(const AudioLayer2 audioLayer, std::weak_ptr<IMediaOutput> &audio_output);

    ~AudioDeviceModuleCustom() override;

    // Retrieve the currently utilized audio layer
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override;

    // Full-duplex transportation of PCM audio
    int32_t RegisterAudioCallback(AudioTransport* audioCallback) override;

    int32_t RegisterAudioCaptureObserver(AudioTransport* audio_observer) { return 0; };

    // Main initializaton and termination
    int32_t Init() override;
    int32_t Terminate() override;
    bool Initialized() const override;

    // Device enumeration
    int16_t PlayoutDevices() override;
    int16_t RecordingDevices() override;
    int32_t PlayoutDeviceName(uint16_t index,
      char name[kAdmMaxDeviceNameSize],
      char guid[kAdmMaxGuidSize]) override;
    int32_t RecordingDeviceName(uint16_t index,
      char name[kAdmMaxDeviceNameSize],
      char guid[kAdmMaxGuidSize]) override;

    // Device selection
    int32_t SetPlayoutDevice(uint16_t index) override;
    int32_t SetPlayoutDevice(WindowsDeviceType device) override;
    int32_t SetRecordingDevice(uint16_t index) override;
    int32_t SetRecordingDevice(WindowsDeviceType device) override;

    // Audio transport initialization
    int32_t PlayoutIsAvailable(bool* available) override;
    int32_t InitPlayout() override;
    bool PlayoutIsInitialized() const override;
    int32_t RecordingIsAvailable(bool* available) override;
    int32_t InitRecording() override;
    bool RecordingIsInitialized() const override;

    // Audio transport control
    int32_t StartPlayout() override;
    int32_t StopPlayout() override;
    bool Playing() const override;
    int32_t StartRecording() override;
    int32_t StopRecording() override;
    bool Recording() const override;

    // Audio mixer initialization
    int32_t InitSpeaker() override;
    bool SpeakerIsInitialized() const override;
    int32_t InitMicrophone() override;
    bool MicrophoneIsInitialized() const override;

    // Speaker volume controls
    int32_t SpeakerVolumeIsAvailable(bool* available) override;
    int32_t SetSpeakerVolume(uint32_t volume) override;
    int32_t SpeakerVolume(uint32_t* volume) const override;
    int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override;
    int32_t MinSpeakerVolume(uint32_t* minVolume) const override;

    // Microphone volume controls
    int32_t MicrophoneVolumeIsAvailable(bool* available) override;
    int32_t SetMicrophoneVolume(uint32_t volume) override;
    int32_t MicrophoneVolume(uint32_t* volume) const override;
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override;
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const override;

    // Speaker mute control
    int32_t SpeakerMuteIsAvailable(bool* available) override;
    int32_t SetSpeakerMute(bool enable) override;
    int32_t SpeakerMute(bool* enabled) const override;

    // Microphone mute control
    int32_t MicrophoneMuteIsAvailable(bool* available) override;
    int32_t SetMicrophoneMute(bool enable) override;
    int32_t MicrophoneMute(bool* enabled) const override;

    // Stereo support
    int32_t StereoPlayoutIsAvailable(bool* available) const override;
    int32_t SetStereoPlayout(bool enable) override;
    int32_t StereoPlayout(bool* enabled) const override;
    int32_t StereoRecordingIsAvailable(bool* available) const override;
    int32_t SetStereoRecording(bool enable) override;
    int32_t StereoRecording(bool* enabled) const override;

    // Delay information and control
    int32_t PlayoutDelay(uint16_t* delayMS) const override;

    bool BuiltInAECIsAvailable() const override;
    int32_t EnableBuiltInAEC(bool enable) override;
    bool BuiltInAGCIsAvailable() const override;
    int32_t EnableBuiltInAGC(bool enable) override;
    bool BuiltInNSIsAvailable() const override;
    int32_t EnableBuiltInNS(bool enable) override;

    AudioDeviceBuffer* GetAudioDeviceBuffer() { return &audio_device_buffer_; }

  private:
    PlatformType Platform() const;
    AudioLayer2 PlatformAudioLayer() const;

    AudioLayer2 audio_layer_;
    PlatformType platform_type_ = kPlatformNotSupported;
    bool initialized_ = false;

    std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;
    webrtc::AudioDeviceBuffer audio_device_buffer_;
    std::unique_ptr<webrtc::AudioDeviceGeneric> audio_device_;
    std::weak_ptr<IMediaOutput> mAudioMediaOutput;
  };

}

#endif  // defined(WEBRTC_INCLUDE_INTERNAL_AUDIO_DEVICE)

#endif  // MODULES_AUDIO_DEVICE_AUDIO_DEVICE_IMPL_H_
