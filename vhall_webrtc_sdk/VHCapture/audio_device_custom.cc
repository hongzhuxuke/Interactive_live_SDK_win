#include "audio_device_custom.h"
#include "modules/audio_device/audio_device_config.h"
#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/ref_counted_object.h"
#include "system_wrappers/include/metrics.h"

#include "file_audio_device.h"
#include "modules/audio_device/dummy/audio_device_dummy.h"
#include "modules/audio_device/dummy/file_audio_device.h"
#include "api/task_queue/default_task_queue_factory.h"

#define CHECKinitialized_() \
  {                         \
    if (!initialized_) {    \
      return -1;            \
    };                      \
  }
  
#define CHECKinitialized__BOOL() \
  {                              \
    if (!initialized_) {         \
      return false;              \
    };                           \
  }


namespace webrtc {

  rtc::scoped_refptr<AudioDeviceModule> AudioDeviceModuleCustom::Create(
    const AudioLayer2 audio_layer, std::weak_ptr<IMediaOutput> &audio_output) {
    RTC_LOG(INFO) << __FUNCTION__;
    // Create the generic reference counted (platform independent) implementation.
    rtc::scoped_refptr<AudioDeviceModuleCustom> audioDevice(
      new rtc::RefCountedObject<AudioDeviceModuleCustom>(audio_layer, audio_output));

    // Ensure that the current platform is supported.
    if (audioDevice->CheckPlatform() == -1) {
      return nullptr;
    }

    // Create the platform-dependent implementation.
    if (audioDevice->CreatePlatformSpecificObjects() == -1) {
      return nullptr;
    }

    // Ensure that the generic audio buffer can communicate with the platform
    // specific parts.
    if (audioDevice->AttachAudioBuffer() == -1) {
      return nullptr;
    }
    return  audioDevice;
  }


  AudioDeviceModuleCustom::AudioDeviceModuleCustom(const AudioLayer2 audioLayer, std::weak_ptr<IMediaOutput> &audio_output)
    : audio_layer_(audioLayer), 
    task_queue_factory_(webrtc::CreateDefaultTaskQueueFactory()), 
    audio_device_buffer_(task_queue_factory_.get()),
    mAudioMediaOutput(audio_output) {
    RTC_LOG(INFO) << __FUNCTION__;
  }

  int32_t AudioDeviceModuleCustom::CheckPlatform() {
    RTC_LOG(INFO) << __FUNCTION__;
    // Ensure that the current platform is supported
    PlatformType platform(kPlatformNotSupported);
#if defined(_WIN32)
    platform = kPlatformWin32;
    RTC_LOG(INFO) << "current platform is Win32";
#elif defined(WEBRTC_ANDROID)
    platform = kPlatformAndroid;
    RTC_LOG(INFO) << "current platform is Android";
#elif defined(WEBRTC_LINUX)
    platform = kPlatformLinux;
    RTC_LOG(INFO) << "current platform is Linux";
#elif defined(WEBRTC_IOS)
    platform = kPlatformIOS;
    RTC_LOG(INFO) << "current platform is IOS";
#elif defined(WEBRTC_MAC)
    platform = kPlatformMac;
    RTC_LOG(INFO) << "current platform is Mac";
#endif
    if (platform == kPlatformNotSupported) {
      RTC_LOG(LERROR)
        << "current platform is not supported => this module will self "
        "destruct!";
      return -1;
    }
    platform_type_ = platform;
    return 0;
  }

  int32_t AudioDeviceModuleCustom::CreatePlatformSpecificObjects() {
    RTC_LOG(INFO) << __FUNCTION__;
    // Dummy ADM implementations if build flags are set.

    audio_device_.reset(new VHFileAudioDevice(mAudioMediaOutput));
    if (audio_device_) {
      RTC_LOG(INFO) << "Will use file-playing dummy device.";
    }
    else {
      // Create a dummy device instead.
      audio_device_.reset(new AudioDeviceDummy());
      RTC_LOG(INFO) << "Dummy Audio APIs will be utilized";
    }

    if (!audio_device_) {
      RTC_LOG(LS_ERROR)
        << "Failed to create the platform specific ADM implementation.";
      return -1;
    }
    return 0;
  }

  int32_t AudioDeviceModuleCustom::AttachAudioBuffer() {
    RTC_LOG(INFO) << __FUNCTION__;
    audio_device_->AttachAudioBuffer(&audio_device_buffer_);
    return 0;
  }

  AudioDeviceModuleCustom::~AudioDeviceModuleCustom() {
    RTC_LOG(INFO) << __FUNCTION__;
  }

  int32_t AudioDeviceModuleCustom::ActiveAudioLayer(AudioLayer* audioLayer) const {
    RTC_LOG(INFO) << __FUNCTION__;
    AudioLayer activeAudio;
    if (audio_device_->ActiveAudioLayer(activeAudio) == -1) {
      return -1;
    }
    *audioLayer = activeAudio;
    return 0;
  }

  int32_t AudioDeviceModuleCustom::Init() {
    RTC_LOG(INFO) << __FUNCTION__;
    if (initialized_)
      return 0;
    RTC_CHECK(audio_device_);
    AudioDeviceGeneric::InitStatus status = audio_device_->Init();
    RTC_HISTOGRAM_ENUMERATION(
      "WebRTC.Audio.InitializationResult", static_cast<int>(status),
      static_cast<int>(AudioDeviceGeneric::InitStatus::NUM_STATUSES));
    if (status != AudioDeviceGeneric::InitStatus::OK) {
      RTC_LOG(LS_ERROR) << "Audio device initialization failed.";
      return -1;
    }
    initialized_ = true;
    return 0;
  }

  int32_t AudioDeviceModuleCustom::Terminate() {
    RTC_LOG(INFO) << __FUNCTION__;
    if (!initialized_)
      return 0;
    if (audio_device_->Terminate() == -1) {
      return -1;
    }
    initialized_ = false;
    return 0;
  }

  bool AudioDeviceModuleCustom::Initialized() const {
    RTC_LOG(INFO) << __FUNCTION__ << ": " << initialized_;
    return initialized_;
  }

  int32_t AudioDeviceModuleCustom::InitSpeaker() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->InitSpeaker();
  }

  int32_t AudioDeviceModuleCustom::InitMicrophone() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->InitMicrophone();
  }

  int32_t AudioDeviceModuleCustom::SpeakerVolumeIsAvailable(bool* available) {
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

  int32_t AudioDeviceModuleCustom::SetSpeakerVolume(uint32_t volume) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << volume << ")";
    CHECKinitialized_();
    return audio_device_->SetSpeakerVolume(volume);
  }

  int32_t AudioDeviceModuleCustom::SpeakerVolume(uint32_t* volume) const {
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

  bool AudioDeviceModuleCustom::SpeakerIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isInitialized = audio_device_->SpeakerIsInitialized();
    RTC_LOG(INFO) << "output: " << isInitialized;
    return isInitialized;
  }

  bool AudioDeviceModuleCustom::MicrophoneIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isInitialized = audio_device_->MicrophoneIsInitialized();
    RTC_LOG(INFO) << "output: " << isInitialized;
    return isInitialized;
  }

  int32_t AudioDeviceModuleCustom::MaxSpeakerVolume(uint32_t* maxVolume) const {
    CHECKinitialized_();
    uint32_t maxVol = 0;
    if (audio_device_->MaxSpeakerVolume(maxVol) == -1) {
      return -1;
    }
    *maxVolume = maxVol;
    return 0;
  }

  int32_t AudioDeviceModuleCustom::MinSpeakerVolume(uint32_t* minVolume) const {
    CHECKinitialized_();
    uint32_t minVol = 0;
    if (audio_device_->MinSpeakerVolume(minVol) == -1) {
      return -1;
    }
    *minVolume = minVol;
    return 0;
  }

  int32_t AudioDeviceModuleCustom::SpeakerMuteIsAvailable(bool* available) {
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

  int32_t AudioDeviceModuleCustom::SetSpeakerMute(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    return audio_device_->SetSpeakerMute(enable);
  }

  int32_t AudioDeviceModuleCustom::SpeakerMute(bool* enabled) const {
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

  int32_t AudioDeviceModuleCustom::MicrophoneMuteIsAvailable(bool* available) {
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

  int32_t AudioDeviceModuleCustom::SetMicrophoneMute(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    return (audio_device_->SetMicrophoneMute(enable));
  }

  int32_t AudioDeviceModuleCustom::MicrophoneMute(bool* enabled) const {
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

  int32_t AudioDeviceModuleCustom::MicrophoneVolumeIsAvailable(bool* available) {
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

  int32_t AudioDeviceModuleCustom::SetMicrophoneVolume(uint32_t volume) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << volume << ")";
    CHECKinitialized_();
    return (audio_device_->SetMicrophoneVolume(volume));
  }

  int32_t AudioDeviceModuleCustom::MicrophoneVolume(uint32_t* volume) const {
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

  int32_t AudioDeviceModuleCustom::StereoRecordingIsAvailable(
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

  int32_t AudioDeviceModuleCustom::SetStereoRecording(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    if (audio_device_->RecordingIsInitialized()) {
      RTC_LOG(WARNING) << "recording in stereo is not supported";
      return -1;
    }
    if (audio_device_->SetStereoRecording(enable) == -1) {
      RTC_LOG(WARNING) << "failed to change stereo recording";
      return -1;
    }
    int8_t nChannels(1);
    if (enable) {
      nChannels = 2;
    }
    audio_device_buffer_.SetRecordingChannels(nChannels);
    return 0;
  }

  int32_t AudioDeviceModuleCustom::StereoRecording(bool* enabled) const {
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

  int32_t AudioDeviceModuleCustom::StereoPlayoutIsAvailable(bool* available) const {
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

  int32_t AudioDeviceModuleCustom::SetStereoPlayout(bool enable) {
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

  int32_t AudioDeviceModuleCustom::StereoPlayout(bool* enabled) const {
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

  int32_t AudioDeviceModuleCustom::PlayoutIsAvailable(bool* available) {
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

  int32_t AudioDeviceModuleCustom::RecordingIsAvailable(bool* available) {
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

  int32_t AudioDeviceModuleCustom::MaxMicrophoneVolume(uint32_t* maxVolume) const {
    CHECKinitialized_();
    uint32_t maxVol(0);
    if (audio_device_->MaxMicrophoneVolume(maxVol) == -1) {
      return -1;
    }
    *maxVolume = maxVol;
    return 0;
  }

  int32_t AudioDeviceModuleCustom::MinMicrophoneVolume(uint32_t* minVolume) const {
    CHECKinitialized_();
    uint32_t minVol(0);
    if (audio_device_->MinMicrophoneVolume(minVol) == -1) {
      return -1;
    }
    *minVolume = minVol;
    return 0;
  }

  int16_t AudioDeviceModuleCustom::PlayoutDevices() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint16_t nPlayoutDevices = audio_device_->PlayoutDevices();
    RTC_LOG(INFO) << "output: " << nPlayoutDevices;
    return (int16_t)(nPlayoutDevices);
  }

  int32_t AudioDeviceModuleCustom::SetPlayoutDevice(uint16_t index) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << index << ")";
    CHECKinitialized_();
    return audio_device_->SetPlayoutDevice(index);
  }

  int32_t AudioDeviceModuleCustom::SetPlayoutDevice(WindowsDeviceType device) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetPlayoutDevice(device);
  }

  int32_t AudioDeviceModuleCustom::PlayoutDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
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

  int32_t AudioDeviceModuleCustom::RecordingDeviceName(
    uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
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

  int16_t AudioDeviceModuleCustom::RecordingDevices() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    uint16_t nRecordingDevices = audio_device_->RecordingDevices();
    RTC_LOG(INFO) << "output: " << nRecordingDevices;
    return (int16_t)nRecordingDevices;
  }

  int32_t AudioDeviceModuleCustom::SetRecordingDevice(uint16_t index) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << index << ")";
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(index);
  }

  int32_t AudioDeviceModuleCustom::SetRecordingDevice(WindowsDeviceType device) {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    return audio_device_->SetRecordingDevice(device);
  }

  int32_t AudioDeviceModuleCustom::InitPlayout() {
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

  int32_t AudioDeviceModuleCustom::InitRecording() {
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

  bool AudioDeviceModuleCustom::PlayoutIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->PlayoutIsInitialized();
  }

  bool AudioDeviceModuleCustom::RecordingIsInitialized() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->RecordingIsInitialized();
  }

  int32_t AudioDeviceModuleCustom::StartPlayout() {
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

  int32_t AudioDeviceModuleCustom::StopPlayout() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t result = audio_device_->StopPlayout();
    audio_device_buffer_.StopPlayout();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopPlayoutSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  bool AudioDeviceModuleCustom::Playing() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->Playing();
  }

  int32_t AudioDeviceModuleCustom::StartRecording() {
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

  int32_t AudioDeviceModuleCustom::StopRecording() {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized_();
    int32_t result = audio_device_->StopRecording();
    audio_device_buffer_.StopRecording();
    RTC_LOG(INFO) << "output: " << result;
    RTC_HISTOGRAM_BOOLEAN("WebRTC.Audio.StopRecordingSuccess",
      static_cast<int>(result == 0));
    return result;
  }

  bool AudioDeviceModuleCustom::Recording() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    return audio_device_->Recording();
  }

  int32_t AudioDeviceModuleCustom::RegisterAudioCallback(
    AudioTransport* audioCallback) {
    RTC_LOG(INFO) << __FUNCTION__;
    return audio_device_buffer_.RegisterAudioCallback(audioCallback);
  }

  int32_t AudioDeviceModuleCustom::PlayoutDelay(uint16_t* delayMS) const {
    CHECKinitialized_();
    uint16_t delay = 0;
    if (audio_device_->PlayoutDelay(delay) == -1) {
      RTC_LOG(LERROR) << "failed to retrieve the playout delay";
      return -1;
    }
    *delayMS = delay;
    return 0;
  }

  bool AudioDeviceModuleCustom::BuiltInAECIsAvailable() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInAECIsAvailable();
    RTC_LOG(INFO) << "output: " << isAvailable;
    return isAvailable;
  }

  int32_t AudioDeviceModuleCustom::EnableBuiltInAEC(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInAEC(enable);
    RTC_LOG(INFO) << "output: " << ok;
    return ok;
  }

  bool AudioDeviceModuleCustom::BuiltInAGCIsAvailable() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInAGCIsAvailable();
    RTC_LOG(INFO) << "output: " << isAvailable;
    return isAvailable;
  }

  int32_t AudioDeviceModuleCustom::EnableBuiltInAGC(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInAGC(enable);
    RTC_LOG(INFO) << "output: " << ok;
    return ok;
  }

  bool AudioDeviceModuleCustom::BuiltInNSIsAvailable() const {
    RTC_LOG(INFO) << __FUNCTION__;
    CHECKinitialized__BOOL();
    bool isAvailable = audio_device_->BuiltInNSIsAvailable();
    RTC_LOG(INFO) << "output: " << isAvailable;
    return isAvailable;
  }

  int32_t AudioDeviceModuleCustom::EnableBuiltInNS(bool enable) {
    RTC_LOG(INFO) << __FUNCTION__ << "(" << enable << ")";
    CHECKinitialized_();
    int32_t ok = audio_device_->EnableBuiltInNS(enable);
    RTC_LOG(INFO) << "output: " << ok;
    return ok;
  }



  AudioDeviceModuleCustom::PlatformType AudioDeviceModuleCustom::Platform() const {
    RTC_LOG(INFO) << __FUNCTION__;
    return platform_type_;
  }

  AudioDeviceModuleCustom::AudioLayer2 AudioDeviceModuleCustom::PlatformAudioLayer()
    const {
    RTC_LOG(INFO) << __FUNCTION__;
    return audio_layer_;
  }
}
