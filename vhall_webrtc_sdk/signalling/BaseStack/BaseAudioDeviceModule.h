#pragma once

#include "signalling/vh_data_message.h"
#include "signalling/vh_events.h"
#include "signalling/vh_stream.h"
#include "signalling/vh_room.h"
#include "signalling/tool/vhall_dev_format.h"
#include "common/vhall_define.h"
#include "common/vhall_timer.h"
#include "common/message_type.h"

#include "CreatePeerConnectionObserver.h"
#include "CreateOfferObserver.h"
#include "SetLocalDescriptionObserver.h"
#include "SetAnswerObserver.h"
#include "VideoRenderer.h"
#include "GetStatsObserver.h"

// about webrtc api
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/video_capture/video_capture.h"
#include "media/base/video_common.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/audio_device_impl.h"
#include "rtc_base/strings/json.h"
#include "third_party/jsoncpp/source/include/json/json.h"

// voice change
#include "../VHCapture/vhall_audio_device/vh_audio_device_impl.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#include "rtc_base/arraysize.h"
//#include "../tool/AudioDataFromCapture.h"
#include "audio/audio_transport_impl.h"
#include "../vh_events.h"
#include <mutex>
#include <atomic>

namespace vhall {
  class AudioDataFromCapture;

  class __declspec(dllexport)BaseAudioDeviceModule: public EventDispatcher {
  private:
    BaseAudioDeviceModule(const BaseAudioDeviceModule&) {};
    BaseAudioDeviceModule& operator=(const BaseAudioDeviceModule&) { return *this; };
    friend class AdmConfig;
  public:
    BaseAudioDeviceModule();
    void Init();
    void Destroy();
    ~BaseAudioDeviceModule();
    void DispatchEvent(BaseEvent&event) override;
  public:
    std::shared_ptr<AudioDataFromCapture>                      mAudioCaptureObserver;
    std::shared_ptr<rtc::Thread>                               mThread = nullptr;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_ = nullptr;
    rtc::scoped_refptr<vhall::VHAudioDeviceModuleImpl>        mAudioDevice = nullptr;
    std::shared_ptr<AudioDevProperty>                          mAudioInProp = nullptr;
    std::shared_ptr<AudioDevProperty>                          mAudioOutProp = nullptr;
    uint16_t                                                   AudioInDevID = -1;
    uint16_t                                                   AudioOutDevID = -1;
    std::wstring                                               mLoopBackDevice = L"";
    std::atomic<uint32_t>                                      mLoopBackVolume;
    std::atomic<uint32_t>                                      mMicroPhoneVolume;
    std::atomic<uint32_t>                                      mSpeakerVolume;
    webrtc::AudioTransportImpl::VoiceChangeControl             mVCC;
    
    std::mutex                   mMtx;
    std::unique_ptr<rtc::Thread> m_network_thread;
    std::unique_ptr<rtc::Thread> m_worker_thread;
    std::unique_ptr<rtc::Thread> m_signaling_thread;

    std::shared_ptr<EventDispatcher> mDispatcher = nullptr;
  };
}