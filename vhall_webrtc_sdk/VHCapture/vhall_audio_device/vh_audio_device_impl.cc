/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vh_audio_device_impl.h"

#include <stddef.h>

#include "api/scoped_refptr.h"
#include "modules/audio_device/audio_device_config.h"  // IWYU pragma: keep
#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "system_wrappers/include/metrics.h"
#include "vh_audio_device_core_win.h"
#include "modules/audio_device/dummy/audio_device_dummy.h"

#define CHECKinitialized_() \
  {                         \
    if (!initialized_) {    \
      return -1;            \
    }                       \
  }

#define CHECKinitialized__BOOL() \
  {                              \
    if (!initialized_) {         \
      return false;              \
    }                            \
  }

namespace vhall {

  rtc::scoped_refptr<VHAudioDeviceModuleImpl> VHAudioDeviceModuleImpl::Create(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory) {
    RTC_LOG(INFO) << __FUNCTION__;
    return VHAudioDeviceModuleImpl::CreateForTest(audio_layer, task_queue_factory);
  }

  // static
  rtc::scoped_refptr<VHAudioDeviceModuleImpl> VHAudioDeviceModuleImpl::CreateForTest(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory) {
    RTC_LOG(INFO) << __FUNCTION__;

    // The "AudioDeviceModule::kWindowsCoreAudio2" audio layer has its own
    // dedicated factory method which should be used instead.
    if (audio_layer == AudioDeviceModule::kWindowsCoreAudio2) {
      RTC_LOG(LS_ERROR) << "Use the CreateWindowsCoreAudioAudioDeviceModule() "
        "factory method instead for this option.";
      return nullptr;
    }

    // Create the generic reference counted (platform independent) implementation.
    rtc::scoped_refptr<VHAudioDeviceModuleImpl> audioDevice(
      new rtc::RefCountedObject<VHAudioDeviceModuleImpl>(audio_layer,
        task_queue_factory));

    // Create the platform-dependent implementation.
    if (audioDevice->CreatePlatformSpecificObjects() == -1) {
      return nullptr;
    }

    // Ensure that the generic audio buffer can communicate with the platform
    // specific parts.
    if (audioDevice->AttachAudioBuffer() == -1) {
      return nullptr;
    }

    return audioDevice;
  }

  VHAudioDeviceModuleImpl::VHAudioDeviceModuleImpl(
    AudioLayer audio_layer,
    webrtc::TaskQueueFactory* task_queue_factory)
    : audio_layer_(audio_layer), audio_device_buffer_(task_queue_factory) {
    RTC_LOG(INFO) << __FUNCTION__;
  }

  int32_t VHAudioDeviceModuleImpl::CreatePlatformSpecificObjects() {
    RTC_LOG(INFO) << __FUNCTION__;
    // Dummy ADM implementations if build flags are set.
    AudioLayer audio_layer(PlatformAudioLayer());
    // Windows ADM implementation.
    if ((audio_layer == kWindowsCoreAudio) ||
      (audio_layer == kPlatformDefaultAudio) ||
      audio_layer == kWindowsDshowAudio) // dshow
    {
      RTC_LOG(INFO) << "Attempting to use the Windows Core Audio APIs...";
      if (webrtc::VHAudioDeviceWindowsCore::CoreAudioIsSupported()) {
        audio_device_.reset(new webrtc::VHAudioDeviceWindowsCore());
        audio_device_->mAudioLayer = audio_layer;
        RTC_LOG(INFO) << "Windows Core Audio APIs will be utilized";
      }
    }

    if (!audio_device_) {
      RTC_LOG(LS_ERROR)
        << "Failed to create the platform specific ADM implementation.";
      return -1;
    }
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::AttachAudioBuffer() {
    RTC_LOG(INFO) << __FUNCTION__;
    audio_device_->AttachAudioBuffer(&audio_device_buffer_);
    return 0;
  }

  VHAudioDeviceModuleImpl::~VHAudioDeviceModuleImpl() {
    RTC_LOG(INFO) << __FUNCTION__;
  }

  int32_t VHAudioDeviceModuleImpl::ActiveAudioLayer(AudioLayer* audioLayer) const {
    RTC_LOG(INFO) << __FUNCTION__;
    AudioLayer activeAudio;
    if (audio_device_->ActiveAudioLayer(activeAudio) == -1) {
      return -1;
    }
    *audioLayer = activeAudio;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::Init() {
    RTC_LOG(INFO) << __FUNCTION__;
    if (initialized_)
      return 0;
    RTC_CHECK(audio_device_);
    webrtc::AudioDeviceGeneric::InitStatus status = audio_device_->Init();
    RTC_HISTOGRAM_ENUMERATION(
      "WebRTC.Audio.InitializationResult", static_cast<int>(status),
      static_cast<int>(webrtc::AudioDeviceGeneric::InitStatus::NUM_STATUSES));
    if (status != webrtc::AudioDeviceGeneric::InitStatus::OK) {
      RTC_LOG(LS_ERROR) << "Audio device initialization failed.";
      return -1;
    }
    initialized_ = true;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::Terminate() {
    RTC_LOG(INFO) << __FUNCTION__;
    if (!initialized_)
      return 0;
    if (audio_device_->Terminate() == -1) {
      return -1;
    }
    initialized_ = false;
    return 0;
  }

  bool VHAudioDeviceModuleImpl::Initialized() const {
    RTC_LOG(INFO) << __FUNCTION__ << ": " << initialized_;
    return initialized_;
  }

  int32_t VHAudioDeviceModuleImpl::InitSpeaker() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->InitSpeaker();
  }

  int32_t VHAudioDeviceModuleImpl::InitMicrophone() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->InitMicrophone();
  }

  int32_t VHAudioDeviceModuleImpl::SpeakerVolumeIsAvailable(bool* available) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->SpeakerVolumeIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SetSpeakerVolume(uint32_t volume) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << volume << ")";
    CHECKinitialized_();
    return audio_device_->SetSpeakerVolume(volume);
  }

  int32_t VHAudioDeviceModuleImpl::SpeakerVolume(uint32_t* volume) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint32_t level = 0;
    if (audio_device_->SpeakerVolume(level) == -1) {
      return -1;
    }
    *volume = level;
    RTC_LOG(INFO) << "output: " << *volume;
    return 0;
  }

  bool VHAudioDeviceModuleImpl::SpeakerIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isInitialized = audio_device_->SpeakerIsInitialized();
    RTC_LOG(INFO) << "output: " << isInitialized;
    return isInitialized;
  }

  bool VHAudioDeviceModuleImpl::MicrophoneIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isInitialized = audio_device_->MicrophoneIsInitialized();
    RTC_LOG(INFO) << "output: " << isInitialized;
    return isInitialized;
  }

  int32_t VHAudioDeviceModuleImpl::MaxSpeakerVolume(uint32_t* maxVolume) const {
    CHECKinitialized_();
    uint32_t maxVol = 0;
    if (audio_device_->MaxSpeakerVolume(maxVol) == -1) {
      return -1;
    }
    *maxVolume = maxVol;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::MinSpeakerVolume(uint32_t* minVolume) const {
    CHECKinitialized_();
    uint32_t minVol = 0;
    if (audio_device_->MinSpeakerVolume(minVol) == -1) {
      return -1;
    }
    *minVolume = minVol;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SpeakerMuteIsAvailable(bool* available) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->SpeakerMuteIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SetSpeakerMute(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    return audio_device_->SetSpeakerMute(enable);
  }

  int32_t VHAudioDeviceModuleImpl::SpeakerMute(bool* enabled) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool muted = false;
    if (audio_device_->SpeakerMute(muted) == -1) {
      return -1;
    }
    *enabled = muted;
    RTC_LOG(INFO) << "output: " << muted;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::MicrophoneMuteIsAvailable(bool* available) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->MicrophoneMuteIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SetMicrophoneMute(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    return (audio_device_->SetMicrophoneMute(enable));
  }

  int32_t VHAudioDeviceModuleImpl::MicrophoneMute(bool* enabled) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool muted = false;
    if (audio_device_->MicrophoneMute(muted) == -1) {
      return -1;
    }
    *enabled = muted;
    RTC_LOG(INFO) << "output: " << muted;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::MicrophoneVolumeIsAvailable(bool* available) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->MicrophoneVolumeIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SetMicrophoneVolume(uint32_t volume) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << volume << ")";
    CHECKinitialized_();
    return (audio_device_->SetMicrophoneVolume(volume));
  }

  int32_t VHAudioDeviceModuleImpl::MicrophoneVolume(uint32_t* volume) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint32_t level = 0;
    if (audio_device_->MicrophoneVolume(level) == -1) {
      return -1;
    }
    *volume = level;
    RTC_LOG(INFO) << "output: " << *volume;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::StereoRecordingIsAvailable(
    bool* available) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->StereoRecordingIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SetStereoRecording(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    if (audio_device_->RecordingIsInitialized()) {
      RTC_LOG(LERROR)
        << "unable to set stereo mode after recording is initialized";
      return -1;
    }
    if (audio_device_->SetStereoRecording(enable) == -1) {
      if (enable) {
        RTC_LOG(WARNING) << "failed to enable stereo recording";
      }
      return -1;
    }
    int8_t nChannels(1);
    if (enable) {
      nChannels = 2;
    }
    audio_device_buffer_.SetRecordingChannels(nChannels);
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::StereoRecording(bool* enabled) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool stereo = false;
    if (audio_device_->StereoRecording(stereo) == -1) {
      return -1;
    }
    *enabled = stereo;
    RTC_LOG(INFO) << "output: " << stereo;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::StereoPlayoutIsAvailable(bool* available) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->StereoPlayoutIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::SetStereoPlayout(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    if (audio_device_->PlayoutIsInitialized()) {
      RTC_LOG(LERROR)
        << "unable to set stereo mode while playing side is initialized";
      return -1;
    }
    if (audio_device_->SetStereoPlayout(enable)) {
      RTC_LOG(WARNING) << "stereo playout is not supported";
      return -1;
    }
    int8_t nChannels(1);
    if (enable) {
      nChannels = 2;
    }
    audio_device_buffer_.SetPlayoutChannels(nChannels);
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::StereoPlayout(bool* enabled) const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool stereo = false;
    if (audio_device_->StereoPlayout(stereo) == -1) {
      return -1;
    }
    *enabled = stereo;
    RTC_LOG(INFO) << "output: " << stereo;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::PlayoutIsAvailable(bool* available) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->PlayoutIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::RecordingIsAvailable(bool* available) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    bool isAvailable = false;
    if (audio_device_->RecordingIsAvailable(isAvailable) == -1) {
      return -1;
    }
    *available = isAvailable;
    RTC_LOG(INFO) << "output: " << isAvailable;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::MaxMicrophoneVolume(uint32_t* maxVolume) const {
    CHECKinitialized_();
    uint32_t maxVol(0);
    if (audio_device_->MaxMicrophoneVolume(maxVol) == -1) {
      return -1;
    }
    *maxVolume = maxVol;
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::MinMicrophoneVolume(uint32_t* minVolume) const {
    CHECKinitialized_();
    uint32_t minVol(0);
    if (audio_device_->MinMicrophoneVolume(minVol) == -1) {
      return -1;
    }
    *minVolume = minVol;
    return 0;
  }

  int16_t VHAudioDeviceModuleImpl::PlayoutDevices() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint16_t nPlayoutDevices = audio_device_->PlayoutDevices();
    RTC_LOG(INFO) << "output: " << nPlayoutDevices;
    return (int16_t)(nPlayoutDevices);
  }

  int32_t VHAudioDeviceModuleImpl::SetPlayoutDevice(uint16_t index) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << index << ")";
    CHECKinitialized_();
    return audio_device_->SetPlayoutDevice(index);
  }

  int32_t VHAudioDeviceModuleImpl::SetPlayoutDevice(WindowsDeviceType device) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetPlayoutDevice(device);
  }

  int32_t VHAudioDeviceModuleImpl::PlayoutDeviceName(
    uint16_t index,
    char name[webrtc::kAdmMaxDeviceNameSize],
    char guid[webrtc::kAdmMaxGuidSize]) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << index << ", ...)";
    CHECKinitialized_();
    if (name == NULL) {
      return -1;
    }
    if (audio_device_->PlayoutDeviceName(index, name, guid) == -1) {
      return -1;
    }
    if (name != NULL) {
      RTC_LOG(INFO) << "output: name = " << name;
    }
    if (guid != NULL) {
      RTC_LOG(INFO) << "output: guid = " << guid;
    }
    return 0;
  }

  int32_t VHAudioDeviceModuleImpl::RecordingDeviceName(
    uint16_t index,
    char name[webrtc::kAdmMaxDeviceNameSize],
    char guid[webrtc::kAdmMaxGuidSize]) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << index << ", ...)";
    CHECKinitialized_();
    if (name == NULL) {
      return -1;
    }
    if (audio_device_->RecordingDeviceName(index, name, guid) == -1) {
      return -1;
    }
    if (name != NULL) {
      RTC_LOG(INFO) << "output: name = " << name;
    }
    if (guid != NULL) {
      RTC_LOG(INFO) << "output: guid = " << guid;
    }
    return 0;
  }

  int16_t VHAudioDeviceModuleImpl::RecordingDevices() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint16_t nRecordingDevices = audio_device_->RecordingDevices();
    RTC_LOG(INFO) << "output: " << nRecordingDevices;
    return (int16_t)nRecordingDevices;
  }

  int32_t VHAudioDeviceModuleImpl::SetRecordingDevice(uint16_t index) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << index << ")";
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(index);
  }

  int32_t VHAudioDeviceModuleImpl::SetRecordingDevice(WindowsDeviceType device) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(device);
  }

  int32_t VHAudioDeviceModuleImpl::InitPlayout() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (PlayoutIsInitialized()) {
      return 0;
    }
    int32_t result = audio_device_->InitPlayout();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.InitPlayoutSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  int32_t VHAudioDeviceModuleImpl::InitRecording() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (RecordingIsInitialized()) {
      return 0;
    }
    int32_t result = audio_device_->InitRecording();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.InitRecordingSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  bool VHAudioDeviceModuleImpl::PlayoutIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->PlayoutIsInitialized();
  }

  bool VHAudioDeviceModuleImpl::RecordingIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->RecordingIsInitialized();
  }

  int32_t VHAudioDeviceModuleImpl::StartPlayout() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (Playing()) {
      return 0;
    }
    audio_device_buffer_.StartPlayout();
    int32_t result = audio_device_->StartPlayout();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StartPlayoutSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  int32_t VHAudioDeviceModuleImpl::StopPlayout() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t result = audio_device_->StopPlayout();
    audio_device_buffer_.StopPlayout();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopPlayoutSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  bool VHAudioDeviceModuleImpl::Playing() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->Playing();
  }

  int32_t VHAudioDeviceModuleImpl::StartRecording() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    if (Recording()) {
      return 0;
    }
    audio_device_buffer_.StartRecording();
    int32_t result = audio_device_->StartRecording();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StartRecordingSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  int32_t VHAudioDeviceModuleImpl::StopRecording() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t result = audio_device_->StopRecording();
    audio_device_buffer_.StopRecording();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopRecordingSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  bool VHAudioDeviceModuleImpl::Recording() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->Recording();
  }

  int32_t VHAudioDeviceModuleImpl::RegisterAudioCallback(
    webrtc::AudioTransport* audioCallback) {
    RTC_LOG(INFO) << __FUNCTION__;
    return audio_device_buffer_.RegisterAudioCallback(audioCallback);
  }

  int32_t VHAudioDeviceModuleImpl::PlayoutDelay(uint16_t* delayMS) const {
    CHECKinitialized_();
    uint16_t delay = 0;
    if (audio_device_->PlayoutDelay(delay) == -1) {
      RTC_LOG(LERROR) << "failed to retrieve the playout delay";
      return -1;
    }
    *delayMS = delay;
    return 0;
  }

  bool VHAudioDeviceModuleImpl::BuiltInAECIsAvailable() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInAECIsAvailable();
    RTC_LOG(INFO) << "output: " << isAvailable;
    return isAvailable;
  }

  int32_t VHAudioDeviceModuleImpl::EnableBuiltInAEC(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    //int32_t ok = audio_device_->EnableBuiltInAEC(enable); //todo
    //RTC_LOG(INFO) << "output: " << ok;
    //return ok;
    return 0;
  }

  bool VHAudioDeviceModuleImpl::BuiltInAGCIsAvailable() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInAGCIsAvailable();
    RTC_LOG(INFO) << "output: " << isAvailable;
    return isAvailable;
  }

  int32_t VHAudioDeviceModuleImpl::EnableBuiltInAGC(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInAGC(enable);
    RTC_LOG(INFO) << "output: " << ok;
    return ok;
  }

  bool VHAudioDeviceModuleImpl::BuiltInNSIsAvailable() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInNSIsAvailable();
    RTC_LOG(INFO) << "output: " << isAvailable;
    return isAvailable;
  }

  int32_t VHAudioDeviceModuleImpl::EnableBuiltInNS(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInNS(enable);
    RTC_LOG(INFO) << "output: " << ok;
    return ok;
  }

  int32_t VHAudioDeviceModuleImpl::GetPlayoutUnderrunCount() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t underrunCount = audio_device_->GetPlayoutUnderrunCount();
    RTC_LOG(INFO) << "output: " << underrunCount;
    return underrunCount;
  }

  int32_t VHAudioDeviceModuleImpl::SetRecordingDevice(std::string device) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(device);
  }

  int32_t VHAudioDeviceModuleImpl::StartLoopBack(std::wstring device, uint32_t volume) {
    return audio_device_->StartLoopBack(device, volume);
  }

  int32_t VHAudioDeviceModuleImpl::StopLoopBack() {
    return audio_device_->StopLoopBack();
  }

  void VHAudioDeviceModuleImpl::SetLoopBackVolume(uint32_t volume) {
    if (audio_device_) {
      audio_device_->SetLoopBackVolume(volume);
    }
  }

  int32_t VHAudioDeviceModuleImpl::RegisterAudioCaptureObserver(webrtc::AudioTransport* audio_observer) {
    RTC_LOG(INFO) << __FUNCTION__;
    if (audio_device_) {
      return audio_device_->RegisterAudioCaptureObserver(audio_observer);
    }
    return 0;
  }

  VHAudioDeviceModuleImpl::PlatformType VHAudioDeviceModuleImpl::Platform() const {
    RTC_LOG(INFO) << __FUNCTION__;
    return platform_type_;
  }

  webrtc::AudioDeviceModule::AudioLayer VHAudioDeviceModuleImpl::PlatformAudioLayer()
    const {
    RTC_LOG(INFO) << __FUNCTION__;
    return audio_layer_;
  }

}  // namespace webrtc
