#include "AudioPlayDeviceModule.h"

#include "3rdparty/vhall_webrtc_include/modules/audio_device/include/audio_device.h"
#include "api/media_stream_interface.h"
#include "api/create_peerconnection_factory.h"
#include "api/peer_connection_interface.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/video_capture/video_capture.h"
#include "media/base/video_common.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/audio_device_impl.h"
//#include "custom_capture/audio_capture/vc_audio_device_module_impl.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "../../common/vhall_log.h"
#include "../vh_tools.h"
#include "AudioPlayDataReader.h"
#include "../../VHCapture/vhall_audio_device/vh_audio_device_impl.h"

namespace vhall {
  static AudioPlayDeviceModule* audioDevice = nullptr;
  static std::mutex mutex;
  static rtc::scoped_refptr<webrtc::AudioDeviceModule> mAudioDeviceModule = nullptr;
  static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory = nullptr;
  static std::shared_ptr<webrtc::AudioPlayDataReader> mReader = nullptr;

  AudioPlayDeviceModule* AudioPlayDeviceModule::GetInstance() {
    LOGD("%s", __FUNCTION__);
    std::unique_lock<std::mutex> lock(mutex);
    if (!audioDevice) {
      audioDevice = new AudioPlayDeviceModule();
    }
    return audioDevice;
  }

  void AudioPlayDeviceModule::Release() {
    LOGD("%s", __FUNCTION__);
    std::unique_lock<std::mutex> lock(mutex);
    if (audioDevice) {
      delete audioDevice;
      audioDevice = nullptr;
    }
    if (mAudioDeviceModule) {
      mAudioDeviceModule->Release();
      mAudioDeviceModule.release();
    }
    if (mPeerConnectionFactory) {
      mPeerConnectionFactory->Release();
      mPeerConnectionFactory.release();
    }
    if (mReader) {
      mReader.reset();
    }
  }

  int AudioPlayDeviceModule::SetPlay(std::string audioFile, uint16_t playOutDeviceIndex, const uint32_t samplesPerSec, const size_t nChannels) {
    LOGD("%s", __FUNCTION__);
    mReader = std::make_shared<webrtc::AudioPlayDataReader>();
    if (!mReader || !mReader->InitInputFile(audioFile, samplesPerSec, nChannels)) {
      LOGE("open audio file failed");
      mReader.reset();
      return -1;
    }
    if (mListener) {
      mReader->SetAudioListener(mListener);
    }
    if (!mAudioDeviceModule.get()) {
      LOGE("mAudioDeviceModule is nullptr");
      return -2;
    }

    uint16_t deviceIndex = 0;
    /* obtain output device list */
    std::vector<std::shared_ptr<AudioDevProperty>> outputDevices;
    outputDevices = mDeviceTool->AudioPlayoutDevicesList();
    if (outputDevices.size() > playOutDeviceIndex) {
      std::shared_ptr<AudioDevProperty> audioCaptureDevice = outputDevices.at(playOutDeviceIndex);
      deviceIndex = audioCaptureDevice->mIndex;
    }
    else {
      LOGE("Could not find assigned playout device");
      return -3;
    }

    mAudioDeviceModule->StopPlayout();
    mAudioDeviceModule->SetPlayoutDevice(deviceIndex);
    mAudioDeviceModule->RegisterAudioCallback(mReader.get());
    initialized = true;

    return 0;
  }

  void AudioPlayDeviceModule::SetAudioListen(AudioSendFrame * listener) {
    mListener = listener;
    if (mReader) {
      mReader->SetAudioListener(listener);
    }
  }

  bool AudioPlayDeviceModule::Start() {
    LOGD("%s", __FUNCTION__);
    if (!mAudioDeviceModule.get() || !mReader) {
      LOGE("not inited, start failed");
      return false;
    }
    if (!initialized) {
      LOGE("not initialized");
      return false;
    }
    mAudioDeviceModule->InitPlayout();
    mAudioDeviceModule->StartPlayout();
    return false;
  }

  bool AudioPlayDeviceModule::Stop() {
    LOGD("%s", __FUNCTION__);
    if (!mAudioDeviceModule) {
      LOGE("failed: AudioDeviceModule null");
      return false;
    }
    mAudioDeviceModule->StopPlayout();
    if (mReader) {
       mReader->Stop();
    }
    initialized = false;
    return false;
  }

  AudioPlayDeviceModule::AudioPlayDeviceModule() {
    LOGD("%s", __FUNCTION__);
    task_queue_factory_ = webrtc::CreateDefaultTaskQueueFactory();
    CreatePeerConnectionFactory();
    mDeviceTool = std::make_shared<VHTools>();
    if (!mDeviceTool) {
      LOGE("create VHTools failed");
    }
  }

  AudioPlayDeviceModule::~AudioPlayDeviceModule() {

  }
  bool AudioPlayDeviceModule::CreatePeerConnectionFactory() {
    LOGD("%s", __FUNCTION__);
    mAudioDeviceModule = vhall::VHAudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kWindowsCoreAudio, task_queue_factory_.get());
    if (!mAudioDeviceModule) {
      LOGE("crate AudioDeviceModule failed");
      return false;
    }
    mAudioDeviceModule->Init();
    mAudioDeviceModule->SetPlayoutDevice(webrtc::AudioDeviceModuleImpl::WindowsDeviceType::kDefaultDevice);
    mPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
      nullptr,
      nullptr, 
      nullptr,
      mAudioDeviceModule.get(),
      webrtc::CreateBuiltinAudioEncoderFactory(), 
      webrtc::CreateBuiltinAudioDecoderFactory(),
      webrtc::CreateBuiltinVideoEncoderFactory(),
      webrtc::CreateBuiltinVideoDecoderFactory(),
      nullptr, nullptr);
    if (!mPeerConnectionFactory.get()) {
      LOGE("CreatePeerConnectionFactory failed");
      return false;
    }
    return true;
  }
}