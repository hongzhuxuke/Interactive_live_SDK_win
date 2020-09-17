#include "BaseStack.h"
#include "../vh_uplog.h"
#include "common/vhall_utility.h"
#include "json/json.h"
#include "../vh_tools.h"
#include "../tool/AudioDataFromCapture.h"
#include "BaseAudioDeviceModule.h"
#include "AdmConfig.h"
#include "audio/audio_transport_impl.h"
#include <evcode.h>
#include "rtc_base/ref_count.h"
#include "SnapShotCallBack.h"
#include "3rdparty/VHGPUImage/VHBeautifyRender.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "../../VHCapture/vhall_video_capture/picture_capture_impl.h"
#include "../../VHCapture/vhall_video_capture/VhallVideoCapture.h"
#include "../../VHCapture/vhall_video_capture/desktop_capture_impl.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "../../VHCapture/audio_device_custom.h"
#include "../../VHCapture/vhall_audio_device/vh_audio_device_impl.h"


NS_VH_BEGIN
using webrtc::SdpType;
/* all stream use common thread in baseStack, only file stream in uses independent peerconnectionfactory*/

BaseStack::BaseStack(std::shared_ptr<VHStream> p)
  : VideoDevID(""), DesktopID(0)
{
  LOGI("Create BaseStack");
  is_rendering_ = false;
  mVhStream = p;
  remoteDescriptionSet = false;
  mAudioFileDevice = NULL;
  audioMuted = false;
  videoMuted = false;
  mAudioInVol = 100;
  mAudioOutVol = 100;
  mPitchValue = 0.0;
  mCreatePeerConnectionObserver = nullptr;
  mSetLocalDescriptionObserver = NULL;
  mSetAnswerObserver = NULL;
  mCreateOfferObserver = NULL;
  mGetStatsObserver = NULL;
  mStreamId = "";
  mStopFlag = false;
  mEnableSnap = true;
  mDevTool = std::make_shared<VHTools>();
  mPublishVideo.reset(new SendVideoSsrc());
  task_queue_factory_ = webrtc::CreateDefaultTaskQueueFactory();
}

void BaseStack::Init() {
  LOGI("Create Init: %s", GetStreamID());
  mCreatePeerConnectionObserver = new CreatePeerConnectionObserver(shared_from_this());
  mSetAnswerObserver = new rtc::RefCountedObject<SetAnswerObserver>(shared_from_this());
  mSetLocalDescriptionObserver = new rtc::RefCountedObject<SetLocalDescriptionObserver>(shared_from_this());
  mCreateOfferObserver = new rtc::RefCountedObject<CreateOfferObserver>(shared_from_this());
  mGetStatsObserver = new rtc::RefCountedObject<GetStatsObserver>(shared_from_this());
  if (!mCreatePeerConnectionObserver || !mSetAnswerObserver || !mSetLocalDescriptionObserver || !mCreateOfferObserver || !mGetStatsObserver) {
    LOGE("Create WebRTC Observer classes failure, Make sure you have enough memory!");
    return;
  }
  snapshot_.reset(new Snapshot(shared_from_this()));
  /* allocat common thread if not set */
  mBaseAdm = AdmConfig::GetAdmInstance();
  if (!mBaseAdm) {
    LOGE("GetAdmInstance failed");
  }
  /* set local thread */
  mLocalThread = rtc::Thread::Create();
  if (mLocalThread) {
    mLocalThread->Start();
    if (mLocalThread) {
      mLocalThread->PostDelayed(RTC_FROM_HERE, 3000, this, GETSTREAMSTATS, nullptr);
    }
  }
}

BaseStack::~BaseStack() {
  LOGI("~BaseStack: %s", GetStreamID());
  Destroy();
  LOGI("end ~BaseStack");
}

void BaseStack::Destroy() {
  LOGD("%s", __FUNCTION__);
  mStopFlag = true;
  PostTask(STOP_STREAM, nullptr); // will not invoked actually
  if (mLocalThread) {
    /* clear tasks */
    mLocalThread->Stop();
    mLocalThread->Clear(this);
    mLocalThread->Start();
    /* Destroy */
    mLocalThread->Post(RTC_FROM_HERE, this, STOP_STREAM, nullptr);
    mLocalThread->Post(RTC_FROM_HERE, this, CLOSE_STREAM, nullptr);
    mLocalThread->Stop();
    mLocalThread.reset();
  }

  ClosePeerConnection();
  /* for file source */
  ClosePeerConnectionFactory();
  peer_connection_ = nullptr;
  mBaseAdm = nullptr;
  video_device = nullptr;
  video_track = nullptr;
  LOGD("end %s", __FUNCTION__);
}

void BaseStack::OnVideoNotify(long eventCode, long param1, long param2) {
  switch (eventCode) {
  case EC_COMPLETE:
  case EC_USERABORT:
  case EC_ERRORABORT:
    break;
  case EC_DEVICE_LOST:
    /* 摄像头移除 */
    if (0 == param2) {
      std::shared_ptr<VHLogReport> reportLog = GetReport();
      if (reportLog) {
        std::stringstream ss;
        ss << CAMERA_LOST;
        std::string msg = ss.str();
        reportLog->upLog(ulCamerLost, GetStreamID(), msg);
      }
      LOGE("Video Capturer Failure, camera lost!");
      BaseEvent event;
      event.mType = CAMERA_LOST;
      std::shared_ptr<VHStream> vhStream = mVhStream.lock();
      if (!vhStream) {
        LOGE("VHStream is reset!");
        return;
      }
      vhStream->DispatchEvent(event);
    }
    break;
  default:
    break;
  }
}
void BaseStack::OnVideoCaptureFail() {
  LOGE("Video Capturer Failure, video capture error stopped!");
  BaseEvent event;
  event.mType = VIDEO_CAPTURE_ERROR;
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  vhStream->DispatchEvent(event);
}
;

void BaseStack::OnSetFilterParam(LiveVideoFilterParam filterParam) {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  mFilterParam = filterParam;

  if ((vhStream->mStreamType == 3 || vhStream->mStreamType == 5 || vhStream->mStreamType == 6) && video_device && video_device->captureSource()) {
    DesktopCaptureImpl* deskCapture = nullptr;
    if (video_device) {
      webrtc::VideoCapturer* source = nullptr;
      source = video_device->captureSource();
      deskCapture = static_cast<DesktopCaptureImpl*>(source);
      if (deskCapture) {
        deskCapture->SetFilterParam(filterParam);
      }
    }
  } // window stream resolution
  if (vhStream->mStreamType == 1 || vhStream->mStreamType == 2) { // picture && camera stream

    VcmCapturer* vcm_cap = nullptr;
    if (video_device && video_device->captureSource()) {
      vcm_cap = dynamic_cast<VcmCapturer*>(video_device->captureSource());
      if (vcm_cap) {
      // 设置美颜
        vcm_cap->SetFilterParam(filterParam);
      }
    }
  }

  

}

bool BaseStack::CreatePeerConnectionFactory() {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> file_adm;
  rtc::scoped_refptr<vhall::VHAudioDeviceModuleImpl> vh_adm;
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return false;
  }
  LOGI("CreatePeerConnectionFactory, id: %s", GetStreamID());
  /* global peerconnectionFactory lock */
  LOGD("baselock1");
  std::unique_lock<std::mutex> lock(mBaseAdm->mMtx);
  if (vhStream->mStreamType == 4 && vhStream->mLocal) {
    if (peer_connection_factory_file.get() != NULL) {
      peer_connection_factory_ = peer_connection_factory_file;
      file_adm = mAudioFileDevice;
      LOGD("peer_connection_factory_file Ready");
      lock.unlock();
      LOGD("unbaselock1");
      return true;
    }
  }
  else {
    if (mBaseAdm->peer_connection_factory_.get() != NULL) {
      peer_connection_factory_ = mBaseAdm->peer_connection_factory_;
      vh_adm = mBaseAdm->mAudioDevice;
      LOGD("baseAdm->peer_connection_factory_ Ready");
      return true;
    }
  }
  /* allocate audio device module */
  if (vhStream->mStreamType == 4 && vhStream->mLocal) { // file stream in
    if (!mAudioFileDevice) {
      std::weak_ptr<IMediaOutput> media_output(CreateMediaReader()->GetMediaOut());
      mAudioFileDevice = webrtc::AudioDeviceModuleCustom::Create(webrtc::AudioDeviceModuleCustom::AudioLayer2::kFileAudio, media_output);
      LOGD("Create File AudioDeviceModule");
      file_adm = mAudioFileDevice;
    }
  }
  else // micro phone audio
  {
    if (!mBaseAdm->mAudioDevice) {
      mBaseAdm->mAudioDevice = vhall::VHAudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kWindowsCoreAudio, task_queue_factory_.get());
    }
    vh_adm = mBaseAdm->mAudioDevice;
  }
  if (!file_adm.get() && !vh_adm.get()) {
    LOGE("Create PlatformDefaultAudio type of AudioDeviceModule Failure!");
    BaseEvent event;
    event.mType = AUDIO_DENIED;
    vhStream->DispatchEvent(event);
    return false;
  }

  /* 音频输入 输出设备选取 */
  std::vector<std::shared_ptr<AudioDevProperty>> audioIns;
  std::vector<std::shared_ptr<AudioDevProperty>> audioOuts;
  if (mDevTool) {
    audioIns = mDevTool->AudioRecordingDevicesList();
    audioOuts = mDevTool->AudioPlayoutDevicesList();
    UpLogSpeakerDevice(audioOuts);
    UpLogMircophoneDevie(audioIns);
  }
  std::shared_ptr<AudioDevProperty> audioCaptureDevice = nullptr;
  if (vhStream->mLocal && audioIns.size() > 0) {
    /* 如未指定麦克风设备则选择默认设备 */
    if (-1 == mBaseAdm->AudioInDevID) {
      for (size_t i = 0; i < audioIns.size(); i++) {
        audioCaptureDevice = audioIns.at(i);
        if (audioCaptureDevice->isDefaultDevice) {
          break;
        }
      }
    }
    /* 选取指定麦克风设备 */
    else if (-1 != mBaseAdm->AudioInDevID && audioIns.size() > mBaseAdm->AudioInDevID) {
      audioCaptureDevice = audioIns.at(mBaseAdm->AudioInDevID);
    }
  }
  else {
    /* 查找指定的扬声器 */
    if (mBaseAdm->AudioOutDevID != -1 && audioOuts.size() > mBaseAdm->AudioOutDevID) {
      auto dev = audioOuts.at(mBaseAdm->AudioOutDevID);
      UpLogSetSpeakerDevice(dev, "success", "crate peerconnectionfactory, adm init speaker");
    }
    /* 若未配置则使用默认扬声器设备 */
    else if (mBaseAdm->AudioOutDevID == -1) {
      for (size_t i = 0; i < audioOuts.size(); i++) {
        auto dev = audioOuts.at(i);
        if (dev->isDefaultDevice) {
          break;
        }
      }
    }
  }

  rtc::scoped_refptr<webrtc::AudioDeviceModule> audioDeviceModule;
  if (vh_adm.get()) {
    audioDeviceModule = vh_adm;
  }
  else {
    audioDeviceModule = file_adm;
  }
  audioDeviceModule->Init();
  /* set audio in dev */
  if (mBaseAdm->AudioInDevID != -1 && audioCaptureDevice) { // found dev
    UpLogSetMicroPhoneDevcie(audioCaptureDevice, mBaseAdm->mLoopBackDevice, mBaseAdm->mLoopBackVolume, "sucess", "create peerconnection factory, init adm");
    if (audioCaptureDevice->m_sDeviceType == TYPE_DSHOW_AUDIO) {
      char guid[256] = { 0 };
      int nLen = WideCharToMultiByte(CP_UTF8, 0, audioCaptureDevice->mDevGuid.c_str(), -1, NULL, 0, NULL, NULL);
      if (nLen >= 0) {
        WideCharToMultiByte(CP_UTF8, 0, audioCaptureDevice->mDevGuid.c_str(), -1, guid, nLen, NULL, NULL);
      }
      else { // error: null guid
        LOGE("AudioDeviceModule input device config error");
        BaseEvent event;
        event.mType = AUDIO_IN_ERROR;
        vhStream->DispatchEvent(event);
      }
      if (vh_adm.get()) {
        vh_adm->SetRecordingDevice(guid);
      }
    }
    else if (audioCaptureDevice->m_sDeviceType == TYPE_COREAUDIO) {
      audioDeviceModule->SetRecordingDevice(audioCaptureDevice->mIndex);
    }
  }
  else { // not found dev, use default config
    audioDeviceModule->SetRecordingDevice(webrtc::AudioDeviceModuleImpl::WindowsDeviceType::kDefaultDevice);
  }
  audioDeviceModule->InitMicrophone();
  audioDeviceModule->InitRecording();
  /* set audio play dev */
  if (mBaseAdm->AudioOutDevID != -1 && audioOuts.size() > mBaseAdm->AudioOutDevID) {
    audioDeviceModule->SetPlayoutDevice(audioOuts.at(mBaseAdm->AudioOutDevID)->mIndex);
  }
  else {
    audioDeviceModule->SetPlayoutDevice(webrtc::AudioDeviceModuleImpl::WindowsDeviceType::kDefaultDevice);
  }
  audioDeviceModule->InitSpeaker();
  audioDeviceModule->InitPlayout();
  bool stereo_playout_ = false;
  audioDeviceModule->StereoPlayoutIsAvailable(&stereo_playout_);
  audioDeviceModule->SetStereoPlayout(stereo_playout_);
  // create pcf
  peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
    mBaseAdm->m_network_thread.get(), 
    mBaseAdm->m_worker_thread.get(), 
    mBaseAdm->m_signaling_thread.get(), 
    audioDeviceModule.get(), 
    webrtc::CreateBuiltinAudioEncoderFactory(), 
    webrtc::CreateBuiltinAudioDecoderFactory(),
    webrtc::CreateBuiltinVideoEncoderFactory(),
    webrtc::CreateBuiltinVideoDecoderFactory(),
    nullptr, nullptr);
  if (peer_connection_factory_.get() == NULL) {
    LOGE("Create peer_connection_factory Failure!, id: %s", GetStreamID());
    return false;
  }
  else {
    if (vhStream->mStreamType == 4 && vhStream->mLocal) {
        peer_connection_factory_file = peer_connection_factory_;
    }
    else {
      mBaseAdm->peer_connection_factory_ = peer_connection_factory_;
        /* set capture boserver */
      mBaseAdm->mAudioCaptureObserver = std::make_shared<AudioDataFromCapture>();
      if (mBaseAdm->mAudioCaptureObserver) {
        mBaseAdm->mAudioDevice->RegisterAudioCaptureObserver(mBaseAdm->mAudioCaptureObserver.get());
      }
      if (mBaseAdm->mLoopBackDevice != L"") {
        mBaseAdm->mAudioDevice->StartLoopBack(mBaseAdm->mLoopBackDevice, mBaseAdm->mLoopBackVolume);
      }
    }
  }
  LOGD("Create Custom AudioDeviceModule type of peer_connection_factory, id: %s", GetStreamID());
  audioDeviceModule->SetMicrophoneVolume(mBaseAdm->mMicroPhoneVolume);
  audioDeviceModule->StartPlayout();
  audioDeviceModule->StartRecording();
  lock.unlock();
  LOGD("unbaselock1");
  /* 设置音量 */
  SetMicrophoneVolume(mBaseAdm->mMicroPhoneVolume);
  SetSpeakerVolume(mBaseAdm->mSpeakerVolume);

  LOGI("End CreatePeerConnectionFactory, id: %s", GetStreamID());
  return true;
}

bool BaseStack::CreatePeerConnection() {
  LOGI("CreatePeerConnection, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
     LOGE("VHStream is reset!");
     return false;
  }

  if (peer_connection_.get() != NULL) {
    LOGI("CreatePeerConnection is exist.");
    return true;
  }

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  webrtc::PeerConnectionInterface::IceServer server;
  std::string uri = "turn:39.106.126.43:3478";
  server.uri = uri;
  server.username = "lide";
  server.password = "dongsn";
  server.urls.push_back("turn:39.106.126.43:3478");
  config.servers.push_back(server);

  if (!vhStream->mLocal) {
    if (!CreatePeerConnectionFactory()) {
      LOGE("CreatePeerConnectionFactory fail, streamId:%s", vhStream->GetID());
      return false;
    }
  }

  if (!peer_connection_factory_) {
    LOGE("peer_connection_factory_ is nullptr");
    return false;
  }
  LOGD("baselock2");
  std::unique_lock<std::mutex> lock(mBaseAdm->mMtx);
  peer_connection_ = peer_connection_factory_->CreatePeerConnection(config, nullptr, nullptr, mCreatePeerConnectionObserver);
  lock.unlock();

  /* uplog */
  if (!vhStream->mLocal) {
    /* remote stream */
    std::shared_ptr<VHLogReport> reportLog = GetReport();
    if (reportLog) {
      reportLog->upLog(createRemotePeerconnection, GetStreamID(), peer_connection_.get() == nullptr ? "fail" : "sucessful");
    }
  }
  else {
    /* local stream */
    std::shared_ptr<VHLogReport> reportLog = GetReport();
    if (reportLog) {
      reportLog->upLog(createLocalPeerconnection, GetStreamID(), peer_connection_.get() == nullptr ? "fail" : "sucessful");
    }
    if (peer_connection_factory_.get() == NULL) {
      LOGE("peer_connection_factory_ is null.");
      return false;
    }
  }

  LOGD("unbaselock2");
  if (peer_connection_.get() == nullptr) {
    LOGE("CreatePeerConnection Failure!");
    return false;
  }

  if (!vhStream->mLocal) { // remote stream snapshot
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receiver = peer_connection_->GetReceivers();
    if (snapshot_ && receiver.size() > 0) {
      if (!stream_renderer_) {
        if (this->wnd) {
          stream_renderer_.reset(new VideoRenderer(this->wnd, shared_from_this()));
        }
        else if (this->receiver.lock()) {
          stream_renderer_.reset(new VideoRenderer(this->receiver.lock(), shared_from_this()));
        }
        else {
          LOGE("render fail, wnd & receiver is empty");
        }
      }
      if (stream_renderer_) {
        for (int i = 0; i < receiver.size(); i++) {
          auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)receiver[i]->track());
          if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            video_track->AddOrUpdateSink(snapshot_.get(), rtc::VideoSinkWants());
          }
        }
      }
    }
  }

  if (audio_track.get()) {
    peer_connection_->AddTrack(audio_track, { "audio_label" });
  }
  if (video_track.get()) {
    peer_connection_->AddTrack(video_track, { video_label_ });
  }

  SetBitRate();
  LOGI("End CreatePeerConnection, id: %s", GetStreamID());

  /* enable snapshot */
  if (snapshot_) {
    snapshot_->mEnable = true;
  }
  return true;
}

void BaseStack::GetUserMedia(std::shared_ptr<MediaCaptureInfo> mediaInfo) {
  LOGI("GetUserMedia, id: %s", GetStreamID());

  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  std::shared_ptr<VHLogReport> reportLog = GetReport();

  BaseEvent Fevent;
  BaseEvent Sevent;
  Fevent.mType = ACCESS_DENIED;
  Sevent.mType = ACCESS_ACCEPTED;

  /* create pcf */
  if (!CreatePeerConnectionFactory()) {
     LOGE("CreatePeerConnectionFactory For User Media Failure!");
     /* uplog */
     if (reportLog) {
       reportLog->upLog(initLocalStreamFailed, GetStreamID(), "reason: CreatePeerConnectionFactory For User Media Failure!");
     }
     vhStream->DispatchEvent(Fevent);
     return;
  }

  // create peerconnection
  if (!CreatePeerConnection()) {
    if (reportLog) {
      reportLog->upLog(initLocalStreamFailed, GetStreamID(), std::string("reason: CreatePeerConnection failed, config:") + ListStreamParams());
    }
    vhStream->DispatchEvent(Fevent);
    return;
  }

  // Remove tracks
  if (peer_connection_) {
    is_rendering_ = false;
    // local stream
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    for (const auto& sender : senders) {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)sender->track());
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        if (stream_renderer_) {
          video_track->RemoveSink(stream_renderer_.get()); // remove sink
          stream_renderer_.reset();
          video_track->RemoveSink(snapshot_.get());
        }
      }
      else if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
        auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(track);
        if (mAudioExport.get()) { // stop export local captured audio
          audio_track->RemoveSink(mAudioExport.get()); // remove sink
          mAudioExport.reset();
        }
      }
      bool ret = peer_connection_->RemoveTrack(sender); // remove track
      printf("RemoveTrack, ret :%d\n", ret);
    }
    video_track = nullptr;
    stream_renderer_.reset();
    mEnableSnap = false;
  }

  // Create Audio Track to RTC Stream
  if (vhStream->mAudio) {
    webrtc::MediaConstraints constraints;
    cricket::AudioOptions options;
    /* 插播，关闭去噪和回音消除 */
    if (vhStream->mStreamType == 4) {
      options.echo_cancellation = false;
      options.noise_suppression = false;
      options.auto_gain_control = false;
    }
    else {
      options.echo_cancellation = true;
      options.noise_suppression = false;
      options.auto_gain_control = true;
      options.highpass_filter = true;
      options.experimental_agc = true;
      //options.extended_filter_aec = true;
      //options.delay_agnostic_aec = true;
      options.experimental_ns = true;
    }
    LOGD("baselock4");
    std::unique_lock<std::mutex> lock(mBaseAdm->mMtx);
    audio_track = ((peer_connection_factory_->CreateAudioTrack("audio_label", peer_connection_factory_->CreateAudioSource(options))));
    lock.unlock();
    LOGD("unbaselock4");
    if (audio_track.get()) {
      peer_connection_->AddTrack(audio_track, { "audio_label" });
    }
  }
  // Create Video Track to RTC Stream
  if (vhStream->mVideo) {
    //rtc::scoped_refptr<webrtc::CapturerTrackSource> video_device;
    video_device = nullptr;
    
    webrtc::VideoCaptureCapability capability;

    // desk video stream
    if (vhStream->mStreamType == 3 || vhStream->mStreamType == 5 || vhStream->mStreamType == 6) {
      video_label_ = "desk_video_label";
      video_device = webrtc::CapturerTrackSource::CreateDeskCapture(
        vhStream->mStreamType,
        vhStream->videoOpt.maxWidth,
        vhStream->videoOpt.maxHeight,
        vhStream->videoOpt.maxFrameRate
      );
    }
    else if (vhStream->mStreamType == 4) {
      capability.videoType = webrtc::VideoType::kI420;
      capability.width = vhStream->videoOpt.maxWidth;
      capability.height = vhStream->videoOpt.maxHeight;
      capability.maxFPS = vhStream->videoOpt.maxFrameRate;
      video_label_ = "file_video_label";
      video_device = webrtc::CapturerTrackSource::CreateFileVideo(capability);
    }
    // picture stream
    else if (nullptr != mediaInfo && nullptr != mediaInfo->mI420Data) {
      VideoProfileList mVideoProfiles;
      std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(vhStream->videoOpt.mProfileIndex);
      
      /* picture stream */
      if (mediaInfo->mI420Data->width >= mediaInfo->mI420Data->height) {
        capability.width = profile->mMaxWidth;
        float scaleRatio = 1.0f * profile->mMaxWidth / mediaInfo->mI420Data->width;
        capability.height = scaleRatio * mediaInfo->mI420Data->height;
      }
      else if (mediaInfo->mI420Data->width < mediaInfo->mI420Data->height) {
        capability.height = profile->mMaxHeight;
        float scaleRatio = 1.0f * profile->mMaxHeight / mediaInfo->mI420Data->height;
        capability.width = scaleRatio * mediaInfo->mI420Data->width;
      }

      /* set picture stream frameRate 10 at least */
      capability.maxFPS = std::max(profile->mMaxFrameRate, 10);
      vhStream->videoOpt.maxFrameRate = capability.maxFPS;
      vhStream->videoOpt.minFrameRate = 10;

      capability.videoType = webrtc::VideoType::kI420;
      
      video_label_ = "pic_cam_video_label";
      video_device = webrtc::CapturerTrackSource::CreatePicCapture(mediaInfo->mI420Data, capability);
      if (reportLog) {
        reportLog->upLog(ulSetVideoCapDev, GetStreamID(), "Init picture stream, detail:" + ListStreamParams());
      }
    }
    /* camera stream */
    else {
      if (nullptr != mediaInfo && "" != mediaInfo->VideoDevID) {
        VideoDevID = mediaInfo->VideoDevID;
      }
      
      int ret = vhStream->SetBestCaptureFormat(); /* apply resolution according by profile */
      if (ret < 0) {
        LOGE("init capture param error!");
        vhStream->DispatchEvent(Fevent);
        /* uplog */
        if (reportLog) {
          reportLog->upLog(initLocalStreamFailed, GetStreamID(), std::string("reason: cannot gian camera param, config:") + ListStreamParams());
        }
        return;
      }
      
      if (reportLog) {
        reportLog->upLog(ulSetVideoCapDev, GetStreamID(), "Init camera stream, detail:" + ListStreamParams());
      }
      // video config
      LOGD("Local video stream output Params maxWidth: %d, minWidth: %d, maxHeight: %d, minHeight: %d, maxFrameRate: %d, minFrameRate: %d, VideoDevID: %s",
        vhStream->videoOpt.maxWidth, vhStream->videoOpt.minWidth,
        vhStream->videoOpt.maxHeight, vhStream->videoOpt.minHeight,
        vhStream->videoOpt.maxFrameRate, vhStream->videoOpt.minFrameRate,
        VideoDevID.c_str());

      /* create video source track */
      video_label_ = "pic_cam_video_label";
      capability.width = vhStream->videoOpt.DshowFormat->width;
      capability.height = vhStream->videoOpt.DshowFormat->height;
      capability.maxFPS = vhStream->videoOpt.DshowFormat->maxFPS;
      capability.videoType = (webrtc::VideoType)vhStream->videoOpt.DshowFormat->videoType;
      LOGD("camera capture param: width: %d, height: %d, MaxFps: %d, videoType: %d", 
        capability.width, capability.height, capability.maxFPS, capability.videoType);

      webrtc::VideoCaptureCapability publish_capability;
      publish_capability.maxFPS = vhStream->videoOpt.publishFormat.maxFPS;
      publish_capability.width = vhStream->videoOpt.publishFormat.width;
      publish_capability.height = vhStream->videoOpt.publishFormat.height;

      video_device = webrtc::CapturerTrackSource::Create(capability, publish_capability, VideoDevID, this, mFilterParam);
      if (!video_device) {
        LOGE("OpenVideoCaptureDevice failed");
        vhStream->DispatchEvent(Fevent);
        /* uplog */
        if (reportLog) {
          reportLog->upLog(initLocalStreamFailed, GetStreamID(), std::string("reason: OpenVideoCaptureDevice failed, config:") + ListStreamParams());
        }
        return;
      }
    }
    LOGD("baselock5");
    std::unique_lock<std::mutex> lock(mBaseAdm->mMtx);
    video_track = nullptr;
    video_track = peer_connection_factory_->CreateVideoTrack(video_label_, video_device);
    lock.unlock();
    LOGD("unbaselock5");

    if (video_track.get()) {
      if (snapshot_) {
        mEnableSnap = true;
        video_track->AddOrUpdateSink(snapshot_.get(), rtc::VideoSinkWants());
      }
      else {
        LOGE("init Snapshot module failed.");
      }
      peer_connection_->AddTrack(video_track, { video_label_ });
    }
  }

  if (mListener) {
    SetAudioListener(mListener);
  }

  MuteStream();

  LOGD("Get User Media Success");
  vhStream->DispatchEvent(Sevent);

  /* uplog */
  if (reportLog) {
    reportLog->upLog(initLocalStream, GetStreamID(), ListStreamParams());
  }

  LOGI("End GetUserMedia, id: %s", GetStreamID());
}

void BaseStack::CreateOfferToErizo() {
  LOGI("CreateOfferToErizo, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  if (!CreatePeerConnection()) {
    std::string errMsg = "CreatePeerConnection" + std::string(vhStream->GetID());
    vhStream->OnPeerConnectionErr(errMsg);
    return;
  }

  webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offerOpt;
  offerOpt.offer_to_receive_audio = true;
  offerOpt.offer_to_receive_video = true;

  peer_connection_->CreateOffer(mCreateOfferObserver, offerOpt);
  LOGI("End CreateOfferToErizo, id: %s", GetStreamID());
}

void BaseStack::ReceiveAnswer(const SignalingMessageVhallRespMsg &ErizoMsg) {
  LOGI("ReceiveAnswer, id: %s", GetStreamID());
  ptrAnswer = std::make_shared<SignalingMessageVhallRespMsg>(ErizoMsg);
  LOGI("End ReceiveAnswer, id: %s", GetStreamID());
}

void BaseStack::ResetCapability(videoOptions & opt) {
  BaseMessage* baseMsg = new BaseMessage();
  baseMsg->mOpt = opt;
  PostTask(MsgResetCap, baseMsg);
  return;
}

void BaseStack::OnResetCapability(videoOptions& opt) {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  webrtc::VideoCaptureCapability op;
  op.height = opt.maxHeight;
  op.width = opt.maxWidth;
  op.maxFPS = opt.maxFrameRate;

  vhStream->videoOpt = opt;

  if (vhStream->mStreamType == 4 && video_device && video_device->captureSource()) {
    video_device->captureSource()->SetCapability(op);
  } // file stream resolution
  if ((vhStream->mStreamType == 3 || vhStream->mStreamType == 5 || vhStream->mStreamType == 6) && video_device && video_device->captureSource()) {
    video_device->captureSource()->SetCapability(op);
  } // window stream resolution
  if(vhStream->mStreamType == 1 || vhStream->mStreamType == 2) { // picture && camera stream
    
    PictureCaptureImpl* pic_cap = nullptr;
    if (video_device && video_device->captureSource()) {
      pic_cap = dynamic_cast<PictureCaptureImpl*>(video_device->captureSource());
    }

    if (pic_cap) { // picture stream
      pic_cap->SetCapability(op);
    }
    else { // camera stream
      vhStream->videoOpt = opt;
      VideoDevID = opt.devId;
      GetUserMedia();
    }
  }

  SetBitRate();
  LOGD("Done");
  return;
}

void BaseStack::SetBitRate() {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  if (vhStream->mLocal && peer_connection_) { // set local stream BitRate
    webrtc::PeerConnectionInterface::BitrateParameters setbit;
    setbit.current_bitrate_bps = (vhStream->mNumSpatialLayers == 2) ? vhStream->videoOpt.startVideoBW * 1.25 * 1000 : vhStream->videoOpt.startVideoBW * 1000;
    /* if enable simulcast increase maxBitRate */
    setbit.max_bitrate_bps = (vhStream->mNumSpatialLayers == 2) ? vhStream->videoOpt.maxVideoBW * 1.25 * 1000 : vhStream->videoOpt.maxVideoBW * 1000;
    setbit.min_bitrate_bps = (vhStream->mNumSpatialLayers == 2) ? vhStream->videoOpt.minVideoBW * 1000 * 1.25 : vhStream->videoOpt.minVideoBW * 1000;
    webrtc::RTCError ret = peer_connection_->SetBitrate(setbit);

    LOGI("setBitRate result: %d, current_bitrate_bps: %d, max_bitrate_bps: %d, min_bitrate_bps:%d",
      ret.ok(),
      vhStream->videoOpt.maxVideoBW * 1000,
      vhStream->videoOpt.maxVideoBW * 1000,
      vhStream->videoOpt.minVideoBW * 1000);
  }
}

std::string BaseStack::ListStreamParams() {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return "";
  }

  std::stringstream ss;
  ss << "streamId " << vhStream->mId;
  ss << ", streamType " << vhStream->mStreamType;
  ss << ", deviceId " << VideoDevID;
  ss << ", width " << vhStream->videoOpt.maxWidth;
  ss << ", height " << vhStream->videoOpt.maxHeight;
  ss << ", ratio " << 1.0f * vhStream->videoOpt.maxWidth / vhStream->videoOpt.maxHeight;
  ss << ", frameRate " << vhStream->videoOpt.maxFrameRate;
  ss << ", enable video " << vhStream->mVideo;
  ss << ", enable audio " << vhStream->mAudio;

  return ss.str();
}

void BaseStack::processAnswer() {
  LOGI("processAnswer, id: %s", GetStreamID());
  if (!peer_connection_) {
    LOGE("null peer_connection");
    return;
  }
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  std::string type_str = ptrAnswer->mMess.mType;
  absl::optional<webrtc::SdpType> type_maybe = webrtc::SdpTypeFromString(type_str);
  if (!type_maybe) {
    LOGE("Unknown SDP type: %s", type_str.c_str());
    return;
  }
  SdpType type = *type_maybe;
  std::string sdp = ptrAnswer->mMess.mSdp;
  LOGD("Answer SDP: %s", sdp.c_str());

  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(type, sdp, &error);
  if (!session_description) {
    LOGE("Can't parse received session description message. SdpParseError was: %s", error.description.c_str());
    return;
  }
  /* uplog */
  if (vhStream->mLocal) {
    std::shared_ptr<VHLogReport> reportLog = GetReport();
    if (reportLog) {
      reportLog->upLog(ProcessAnswer, GetStreamID(), std::string("local stream answer sdp:") + sdp);
    }
  }
  else {
    std::shared_ptr<VHLogReport> reportLog = GetReport();
    if (reportLog) {
      reportLog->upLog(processRemoteAnswer, GetStreamID(), std::string("remote stream answer sdp") + sdp);
    }
  }

  peer_connection_->SetRemoteDescription(mSetAnswerObserver, session_description.release());
  remoteDescriptionSet = true;

  //for (SignalingMessageMsg Candidate : listIceCandidate) {
  //  vhStream->SendData(Candidate.ToJsonStr());
  //}
  LOGI("End processAnswer, id: %s", GetStreamID());
}

void BaseStack::ReceiveRemoteStream() {
  LOGI("ReceiveRemoteStream, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  if (!peer_connection_) {
    LOGE("peer_connection_ is null!");
    return;
  }

  if (mListener) {
    SetAudioListener(mListener);
  }

  MuteStream();
  /*if (mLocalThread) {
    mLocalThread->PostDelayed(RTC_FROM_HERE, 3000, this, GETSTREAMSTATS, nullptr);
  }*/

  /* CreatePeerConnectionObserver init mediaStream for remote stream */
  std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receiver = peer_connection_->GetReceivers();
  if (snapshot_ && receiver.size() > 0) {
    for (int i = 0; i < receiver.size(); i++) {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)receiver[i]->track());
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        video_track->AddOrUpdateSink(snapshot_.get(), rtc::VideoSinkWants());
      }
      else if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
      }
    }
  }

  if (!vhStream->mLocal) {
    vhStream->StreamSubscribed();
  }
  LOGI("End ReceiveRemoteStream, id: %s", GetStreamID());
}

void BaseStack::RenderView() {
  LOGI("RenderView, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  if (!peer_connection_) {
    LOGE("peer_connection_ is empty!");
    return;
  }
  if (stream_renderer_ || is_rendering_) {
    LOGE("render has already started, return");
    return;
  }
  is_rendering_ = true;

  // local stream
  if (vhStream->mLocal) {
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    if (snapshot_ && senders.size() > 0) {
      if (!stream_renderer_) {
        if (this->wnd) {
          stream_renderer_.reset(new VideoRenderer(this->wnd, shared_from_this()));
        }
        else if (this->receiver.lock()) {
          stream_renderer_.reset(new VideoRenderer(this->receiver.lock(), shared_from_this()));
        }
        else {
          LOGE("render fail, wnd & receiver is empty");
        }
      }
      if (stream_renderer_) {
        for (int i = 0; i < senders.size(); i++) {
          auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)senders[i]->track());
          if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            video_track->AddOrUpdateSink(stream_renderer_.get(), rtc::VideoSinkWants());
          }
        }
      }
    }
  }
  else if (!vhStream->mLocal) { // remote stream
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receiver = peer_connection_->GetReceivers();
    if (snapshot_ && receiver.size() > 0) {
      if (!stream_renderer_) {
        if (this->wnd) {
          stream_renderer_.reset(new VideoRenderer(this->wnd, shared_from_this()));
        }
        else if (this->receiver.lock()) {
          stream_renderer_.reset(new VideoRenderer(this->receiver.lock(), shared_from_this()));
        }
        else {
          LOGE("render fail, wnd & receiver is empty");
        }
      }
      if (stream_renderer_) {
        for (int i = 0; i < receiver.size(); i++) {
          auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)receiver[i]->track());
          if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            video_track->AddOrUpdateSink(stream_renderer_.get(), rtc::VideoSinkWants());
          }
        }
      }
    }
  }

  std::shared_ptr<VHLogReport> reportLog = GetReport();
  if (reportLog) {
    reportLog->upLog(playStream, GetStreamID(), "reason: default");
  }
  LOGI("End RenderView, id: %s", GetStreamID());
}

void BaseStack::SetAudioListener(AudioSendFrame* listener) {
  LOGI("%s, id: %s", __FUNCTION__, GetStreamID());
  mListener = listener;
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  /* Set remote stream audio listener */
  if (!vhStream->mLocal) {
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    if (snapshot_ && senders.size() > 0) {
      if (!mAudioExport) {
        mAudioExport.reset(new AudioExport());
        if (!mAudioExport) {
          LOGE("create AudioExport failed");
          return;
        }
        for (int i = 0; i < senders.size(); i++) {
          if (senders[i]->stream_ids()[0] == "audio_label") {
            auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)senders[i]->track());
            if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
              auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(track);
              audio_track->AddSink(mAudioExport.get());
              mAudioExport->SetAudioCallback(listener);
            }
          }
        }
      }
    }
  }
  /* Set Local stream auido listener except file stream */
  else if (vhStream->mLocal && vhStream->mStreamType != 4) {
    std::unique_lock<std::mutex> lock(mBaseAdm->mMtx);
    if (mBaseAdm->mAudioCaptureObserver) {
      mBaseAdm->mAudioCaptureObserver->SetAudioCallback(listener);
    }
    lock.unlock();
  }
  else {
    LOGE("stream type not support audio export");
  }
  LOGD("done");
}

void BaseStack::stopStream() {
  LOGI("stopStream, id: %s", GetStreamID());
  if (!peer_connection_ || !stream_renderer_ || !is_rendering_) {
    LOGE("peer_connection_ or stream_renderer_ is empty!");
    return;
  }
  is_rendering_ = false;

  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  // local stream
  if (vhStream->mLocal) {
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    for (const auto& sender : senders) {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)sender->track());
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
        video_track->RemoveSink(stream_renderer_.get()); // remove sink
        stream_renderer_.reset();
      }
      else if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
        auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(track);
        // RemoveSink
      }
    }
  }
  // remote stream
  if (!vhStream->mLocal) {
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers = peer_connection_->GetReceivers();
    if (snapshot_ && receivers.size() > 0) {
      for (const auto& receiver : receivers) {
        auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)receiver->track());
        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
          auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
          video_track->RemoveSink(stream_renderer_.get()); // remove sink
          stream_renderer_.reset();
        }
      }
    }
  }

  std::shared_ptr<VHLogReport> reportLog = GetReport();
  if (reportLog) {
    reportLog->upLog(stopPlayStream, GetStreamID(), "reason: default");
  }
  LOGD("about to Mutex, id: %s", GetStreamID());
  std::lock_guard<std::mutex> locker(mRenderingMutex);
  LOGD("in Mutex, id: %s", GetStreamID());
  LOGI("End stopStream, id: %s", GetStreamID());
}

void BaseStack::OnMessage(rtc::Message * msg) {
  BaseMessage * msgData = (static_cast<BaseMessage*>(msg->pdata));
  LOGI("%s, streamId:%s, taskId:%d,", __FUNCTION__, GetStreamID(), msg->message_id);
  switch (msg->message_id) {
  case GET_USER_MEDIA:
  {
    std::shared_ptr<MediaCaptureInfo> mCaptureInfo = nullptr;
    if (msgData) {
      mCaptureInfo = msgData->mCaptureInfo;
    }
    GetUserMedia(mCaptureInfo);
  }
    break;
  case CREATE_OFFER:
    CreateOfferToErizo();
    break;
  case PROCESS_ANSWER:
    processAnswer();
    break;
  case PLAY_STREAM: {
    this->wnd = msgData->wnd;
    this->receiver = msgData->receiver;
    RenderView();
    break; 
  }
  case STOP_STREAM:
    stopStream();
    break;
  case CLOSE_STREAM:
    closeStream();
    break;
  case RECEIVESTREAM:
    ReceiveRemoteStream();
    break;
  case MUTESTREAM:
    MuteStream();
    break;
  case GETSTREAMSTATS:
    GetTrackStatsTimer();
    break;
  case IceConnectionCompleted: {
    uplogIceCompleted = true;
    GetTrackStats();
    break;
  }
  case WM_QUIT:
    LOGD("Exit BaseStack Message Loop:");
    break;
  case INITPEERCONNECTION:
    CreatePeerConnection();
    break;
  case DEINITPEERCONNECTION:
    ClosePeerConnection();
    break;
  case CREATEOFFERSUCCESS:
    if (mCreateOfferObserver && peer_connection_) {
      mCreateOfferObserver->OnSuccess();
    }
    break;
  case SETANSWERSUCCESS:
    if (mSetAnswerObserver && peer_connection_) {
      mSetAnswerObserver->OnSuccess_();
    }
    break;
  case SETAUDIOPLAYOUTDEVICE:
    if (msgData) {
      SetAudioOutDevice(msgData->AudioDevID);
    }
    break;
  case SETAUDIOINDEVICE:
    if (msgData) {
      SetAudioInDevice(msgData->AudioDevID, msgData->LoopBackDeivce);
    }
    break;
  case SET_AUDIO_OUTPUT:
    if (msgData) {
      SetAudioListener(msgData->mListener);
    }
    break;
  case IceGatheringComplete:
    if (mCreatePeerConnectionObserver && peer_connection_) {
      mCreatePeerConnectionObserver->OnIceCandidateComplete();
    }
    break;
  case IceConnectionFailed: {
    /* uplog */
    std::shared_ptr<VHStream> vhStream = mVhStream.lock();
    if (vhStream->mLocal) {
      std::shared_ptr<VHLogReport> reportLog = GetReport();
      if (reportLog) {
        reportLog->upLog(localPeerConnectionFailed, GetStreamID(), "reason: local stream IceConnectionState::kIceConnectionFailed");
        reportLog->upLog(ulStreamFailed, GetStreamID(), "reason: local stream IceConnectionState::kIceConnectionFailed");
      }
    }
    else {
      std::shared_ptr<VHLogReport> reportLog = GetReport();
      if (reportLog) {
        reportLog->upLog(RemotePeerConnectionFailed, GetStreamID(), "reason: remote stream IceConnectionState kIceConnectionFailed");
        reportLog->upLog(ulStreamFailed, GetStreamID(), "reason: remote stream IceConnectionState kIceConnectionFailed");
      }
    }
    vhStream.reset();

    if (mCreatePeerConnectionObserver && peer_connection_) {
      mCreatePeerConnectionObserver->OnIceConnectionFailed();
    }
    break;
  }
  case MsgCreateSDPFail:
  {
    std::shared_ptr<VHStream> vhStream = mVhStream.lock();
    if (vhStream) {
      if (vhStream->GetStreamState() == VhallStreamState::Publish || vhStream->GetStreamState() == VhallStreamState::Subscribe) {
        std::string msg("MsgCreateSDPFail");
        vhStream->OnPeerConnectionErr(msg);
      }
    }
  }
    break;
  case SetLocalDescriptionFailed:
    if (mSetLocalDescriptionObserver && peer_connection_) {
      mSetLocalDescriptionObserver->OnFailure();
    }
    break;
  case SetAnswerFailed:
    if (mSetAnswerObserver && peer_connection_) {
      mSetAnswerObserver->OnFailure();
    }
  case MsgRequestSnapShot:
    if (msgData) {
      OnRequestSnapShot(msgData->mSnapShotListener, msgData->mSnapWidth, msgData->mSnapHeight, msgData->mSnapQuality);
    }
    break;
  case MsgUpdateRect:
    OnUpdateRect(msgData->mRect);
    break;
  case MsgResetCap:
    OnResetCapability(msgData->mOpt);
    break;
  case MsgVideoFilterParam:
    OnSetFilterParam(msgData->mFilterParam);
    break;
  default:
    LOGE("%s :undefined message", __func__);
    break;
  }
  if (msgData) {
    delete msgData;
    msgData = nullptr;
  }
}


void BaseStack::closeStream() {
  LOGI("closeStream, id: %s", GetStreamID());

  std::shared_ptr<VHLogReport> reportLog = GetReport();
  if (reportLog) {
    reportLog->upLog(ulCloseStream, GetStreamID(), "default");
  }

  ClosePeerConnection();
  ClosePeerConnectionFactory();
  LOGI("closeStream, release Rtc Stream id: %s", GetStreamID());
  if (peer_connection_) {
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    for (const auto& sender : senders) {
      peer_connection_->RemoveTrack(sender);
    }
  }
  video_device = nullptr;
  
  LOGI("closeStream, release Audio Device id: %s", GetStreamID());
  if (mAudioFileDevice) {
    LOGD("mAudioFileDevice release, id: %d", GetStreamID());
    const auto status = mAudioFileDevice->Release();
    mAudioFileDevice.release();
    LOGD("mAudioFileDevice release status: %d, id: %d", (int)status, GetStreamID());
  }
  LOGI("closeStream, delete Observer id: %s", GetStreamID());
  if (mCreatePeerConnectionObserver) {
    delete mCreatePeerConnectionObserver;
    mCreatePeerConnectionObserver = nullptr;
    LOGD("release mCreatePeerConnectionObserver success, id: %s", GetStreamID());
  }
  if (mSetAnswerObserver) {
    mSetAnswerObserver->Release();
    mSetAnswerObserver.release();
    LOGD("release mSetAnswerObserver success, id: %s", GetStreamID());
  }
  if (mSetLocalDescriptionObserver) {
    mSetLocalDescriptionObserver->Release();
    mSetLocalDescriptionObserver.release();
    LOGD("release mSetLocalDescriptionObserver success, id: %s", GetStreamID());
  }
  if (mCreateOfferObserver) {
    mCreateOfferObserver->Release();
    mCreateOfferObserver.release();
    LOGD("release mCreateOfferObserver success, id: %s", GetStreamID());
  }
  if (mGetStatsObserver) {
    mGetStatsObserver->Release();
    mGetStatsObserver.release();
    LOGD("release mGetStatsObserver success, id: %s", GetStreamID());
  }
  
  LOGI("End closeStream, id: %s", GetStreamID());
}

void BaseStack::ClosePeerConnection() {
  LOGI("ClosePeerConnection, id: %s", GetStreamID());

  /* stop snapshot */
  if (snapshot_) {
    snapshot_->mEnable = false;
  }
  if (peer_connection_) {
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    for (const auto& sender : senders) {
      peer_connection_->RemoveTrack(sender);
    }
    peer_connection_->Close();
    peer_connection_->Release();
    peer_connection_.release();
    peer_connection_ = NULL;
  }
  LOGI("End ClosePeerConnection, id: %s", GetStreamID());
}

void BaseStack::ClosePeerConnectionFactory() {
  LOGI("ClosePeerConnectionFactory, id: %s", GetStreamID());
  if (peer_connection_factory_file) {
    peer_connection_factory_file->Release();
    peer_connection_factory_file.release();
    peer_connection_factory_file = nullptr;
  }
  
  LOGI("End ClosePeerConnectionFactory, id: %s", GetStreamID());
}

void BaseStack::GetTrackStatsTimer() {
  if (nullptr == peer_connection_) {
    mLocalThread->PostDelayed(RTC_FROM_HERE, 3000, this, GETSTREAMSTATS, nullptr);
    return;
  }
  //LOGD("GetTrackStatsTimer, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  {
    //LOGI("Post GETSTREAMSTATS task");
    mLocalThread->PostDelayed(RTC_FROM_HERE, 1000, this, GETSTREAMSTATS, nullptr);
    //LOGI("end Post GETSTREAMSTATS task");
    GetTrackStats();
  }

  //LOGI("End GetTrackStatsTimer, id: %s", GetStreamID());
}

void BaseStack::GetTrackStats() {
  //LOGI("GetTrackStats, id: %s", GetStreamID());
  if (peer_connection_.get()) {
    peer_connection_->GetStats(mGetStatsObserver, NULL, webrtc::PeerConnectionInterface::kStatsOutputLevelStandard);
  }
  //LOGI("End GetTrackStats, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  return;
}

void BaseStack::MuteStream() {
  LOGI("Set MuteStream, id: %s", GetStreamID());
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  if (!peer_connection_) {
    LOGE("peer_connection_ is null!");
    return;
  }
  std::shared_ptr<VHLogReport> reportLog = GetReport();
  // local stream
  if (vhStream->mLocal) {
    std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> senders = peer_connection_->GetSenders();
    for(const auto& sender:senders) {
      auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)sender->track());
      
      if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
        sender->track()->set_enabled(!videoMuted);
        if (reportLog) {
          reportLog->upLog(invokeMuteVideo, GetStreamID(), std::string("invokeMuteVideo, set video mute:") + Utility::ToString(videoMuted));
        }
      }
      else if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
        sender->track()->set_enabled(!audioMuted);
        if (reportLog) {
          reportLog->upLog(invokeMuteAudio, GetStreamID(), std::string("invokeMuteAudio, set audio mute:") + Utility::ToString(audioMuted) );
        }
      }
    }
  }
  else { // remote stream
    std::vector<rtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers = peer_connection_->GetReceivers();
    if (snapshot_ && receivers.size() > 0) {
      for (const auto& receiver : receivers) {
        auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>((void*)receiver->track());
        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
          receiver->track()->set_enabled(!videoMuted);
          if (reportLog) {
            reportLog->upLog(invokeMuteVideo, GetStreamID(), "invokeMuteVideo");
          }
        }
        if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
          receiver->track()->set_enabled(!audioMuted);
          if (reportLog) {
            reportLog->upLog(invokeMuteAudio, GetStreamID(), "invokeMuteAudio");
          }
        }
      }
    }
  }

  // Send Mute Message to Server
  LOGD("send mute message: %s", GetStreamID());
  MuteStreamMsg muteMsg;
  muteMsg.mIsLocal = vhStream->mLocal;
  muteMsg.mStreamId = vhStream->GetID();
  muteMsg.mMuteStreams.mAudio = audioMuted;
  muteMsg.mMuteStreams.mVideo = videoMuted;
  std::string muteId = GetStreamID();
  vhStream->SendData(muteMsg.ToJsonStr(), [&, muteId](const RespMsg &msg) -> void {
    LOGI("id: %s Set Mute Stream to Server response: %s", muteId.c_str(), msg.mResult.c_str());
    }, 5000);
  LOGI("End Set MuteStream, id: %s", GetStreamID());
}

bool BaseStack::SetAudioOutDevice(uint32_t index) {
  std::shared_ptr<VHTools> devTool = std::make_shared<VHTools>();
  if (!devTool) {
    LOGE("create VHTools failed");
    return false;
  }
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  std::vector<std::shared_ptr<AudioDevProperty>> audioOuts;
  /* check device */
  bool devOutOk(false);
  std::shared_ptr<AudioDevProperty> audioDevice = nullptr;
  audioOuts = devTool->AudioPlayoutDevicesList();
  UpLogSpeakerDevice(audioOuts);
  /* 通过设备列表索引获选所选设备 */
  if (audioOuts.size() > index) {
    audioDevice = audioOuts.at(index);
    if (devTool->IsSupported(audioDevice)) {
      devOutOk = true;
    }
  }
  if (!audioDevice) {
    LOGE("cannot find deivce index");
    return false;
  }

  if (!devOutOk) {
    LOGE("AudioDeviceModule out device config error");
    UpLogSetSpeakerDevice(audioDevice, "fail", "AudioDeviceModule out device config error");
    return false;
  }

  LOGD("baselock7");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  baseAdm->AudioOutDevID = index;
  baseAdm->mAudioOutProp = audioDevice;
  if (baseAdm->peer_connection_factory_ && baseAdm->mAudioDevice) {
    baseAdm->mAudioDevice->StopPlayout();
    baseAdm->mAudioDevice->SetPlayoutDevice(audioDevice->mIndex);
    baseAdm->mAudioDevice->InitPlayout();
    baseAdm->mAudioDevice->StartPlayout();
  }
  else {
    LOGW("AudioDevice not initialized");
  }
  lock.unlock();
  LOGD("unbaselock7");

  UpLogSetSpeakerDevice(audioDevice, "sucess", "SetAudioOutDevice done");
  LOGD("done");
  return true;
}

void BaseStack::UpLogSpeakerDevice(std::vector<std::shared_ptr<AudioDevProperty>>& vecAuidoDevice) {
  int devNum = vecAuidoDevice.size();
  if (devNum <= 0) {
    return;
  }
  Json::Value dictdata(Json::ValueType::arrayValue);
  for (int i = 0; i < devNum; i++) {
    std::shared_ptr<AudioDevProperty> devProp = vecAuidoDevice.at(i);
    Json::Value dictDev(Json::ValueType::objectValue);
    char devName[512] = { 0 };
    int nLen = WideCharToMultiByte(CP_UTF8, 0, devProp->mDevName.c_str(), -1, NULL, 0, NULL, NULL);
    if (nLen >= 0) {
      WideCharToMultiByte(CP_UTF8, 0, devProp->mDevName.c_str(), -1, devName, nLen, NULL, NULL);
    }
    dictDev["deviceName"] = Json::Value(devName);
    dictDev["channel"] = Json::Value(devProp->mFormat.nChannels);
    dictDev["sampleRate"] = Json::Value((int)devProp->mFormat.nSamplesPerSec);
    dictDev["bitsPerSample"] = Json::Value(devProp->mFormat.wBitsPerSample);

    dictdata[i] = dictDev;
  }
  Json::FastWriter root;
  std::string msg = root.write(dictdata);
  std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
  if (reportLog) {
    //LOGE("SpeakerDevice:%s", msg.c_str());
    reportLog->upLogAudioDevice(ulSpeakerList, "", msg, std::string("SpeakerDevice:") + msg);
  }
}

void BaseStack::UpLogMircophoneDevie(std::vector<std::shared_ptr<AudioDevProperty>>& vecAuidoDevice) {
  int devNum = vecAuidoDevice.size();
  if (devNum <= 0) {
    return;
  }
  Json::Value dictdata(Json::ValueType::arrayValue);
  for (int i = 0; i < devNum; i++) {
    std::shared_ptr<AudioDevProperty> devProp = vecAuidoDevice.at(i);
    Json::Value dictDev(Json::ValueType::objectValue);
    char devName[512] = { 0 };
    int nLen = WideCharToMultiByte(CP_UTF8, 0, devProp->mDevName.c_str(), -1, NULL, 0, NULL, NULL);
    if (nLen >= 0) {
      WideCharToMultiByte(CP_UTF8, 0, devProp->mDevName.c_str(), -1, devName, nLen, NULL, NULL);
    }
    dictDev["deviceName"] = Json::Value(devName);
    dictDev["channel"] = Json::Value(devProp->mFormat.nChannels);
    dictDev["sampleRate"] = Json::Value((int)devProp->mFormat.nSamplesPerSec);
    dictDev["bitsPerSample"] = Json::Value(devProp->mFormat.wBitsPerSample);

    dictdata[i] = dictDev;
  }
  Json::FastWriter root;
  std::string msg = root.write(dictdata);
  std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
  if (reportLog) {
    //LOGE("microphone:%s", msg.c_str());
    reportLog->upLogAudioDevice(ulMicroPhoneList, "", msg, "microphone list");
  }
}

void BaseStack::UpLogSetMicroPhoneDevcie(std::shared_ptr<AudioDevProperty> dev, std::wstring LoopBackDeivce, float volume, std::string result, std::string message) {
  Json::Value dictDev(Json::ValueType::objectValue);
  char deskDevice[512] = { 0 };
  if (LoopBackDeivce != L"") {
    int nLen = WideCharToMultiByte(CP_UTF8, 0, LoopBackDeivce.c_str(), -1, NULL, 0, NULL, NULL);
    if (nLen >= 0) {
      WideCharToMultiByte(CP_UTF8, 0, LoopBackDeivce.c_str(), -1, deskDevice, nLen, NULL, NULL);
    }
  }
  dictDev["deskDevice"] = Json::Value(deskDevice);

  if (!dev) {
    LOGE("no audio device");
    return;
  }
  else {
    char devName[512] = { 0 };
    int nLen = WideCharToMultiByte(CP_UTF8, 0, dev->mDevName.c_str(), -1, NULL, 0, NULL, NULL);
    if (nLen >= 0) {
      WideCharToMultiByte(CP_UTF8, 0, dev->mDevName.c_str(), -1, devName, nLen, NULL, NULL);
    }
    dictDev["deviceType"] = Json::Value(dev->m_sDeviceType);
    dictDev["deviceName"] = Json::Value(devName);
    dictDev["channel"] = Json::Value(dev->mFormat.nChannels);
    dictDev["sampleRate"] = Json::Value((int)dev->mFormat.nSamplesPerSec);
    dictDev["bitsPerSample"] = Json::Value(dev->mFormat.wBitsPerSample);
  }
  std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
  if (reportLog) {
    dictDev["result"] = Json::Value(result);
    Json::Value dictdata(Json::ValueType::arrayValue);
    dictdata[0] = dictDev;
    Json::FastWriter root;
    reportLog->upLogAudioDevice(ulSetMicroPhone, "", root.write(dictdata), message + root.write(dictdata));
  }
}

void BaseStack::UpLogSetSpeakerDevice(std::shared_ptr<AudioDevProperty> dev, std::string result, std::string message) {
  if (!dev) {
    LOGE("cant found audio device");
    return;
  }
  char devName[512] = { 0 };
  int nLen = WideCharToMultiByte(CP_UTF8, 0, dev->mDevName.c_str(), -1, NULL, 0, NULL, NULL);
  if (nLen >= 0) {
    WideCharToMultiByte(CP_UTF8, 0, dev->mDevName.c_str(), -1, devName, nLen, NULL, NULL);
  }
  Json::Value dictDev(Json::ValueType::objectValue);
  dictDev["deviceType"] = Json::Value(dev->m_sDeviceType);
  dictDev["deviceName"] = Json::Value(devName);
  dictDev["channel"] = Json::Value(dev->mFormat.nChannels);
  dictDev["sampleRate"] = Json::Value((int)dev->mFormat.nSamplesPerSec);
  dictDev["bitsPerSample"] = Json::Value(dev->mFormat.wBitsPerSample);

  std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
  if (reportLog) {
    dictDev["result"] = Json::Value(result);
    Json::Value dictdata(Json::ValueType::arrayValue);
    dictdata[0] = dictDev;
    Json::FastWriter root;
    reportLog->upLogAudioDevice(ulSetSpeaker, "", root.write(dictdata), message);
  }
}

bool BaseStack::SetAudioInDevice(uint32_t index, std::wstring LoopBackDeivce) {
  std::shared_ptr<VHTools> devTool = std::make_shared<VHTools>();
  if (!devTool) {
    LOGE("create VHTools failed");
    return false;
  }
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  std::shared_ptr<AudioDevProperty> audioDevice = nullptr;
  std::vector<std::shared_ptr<AudioDevProperty>> audioIns;
  if (devTool) {
    /* 通过设备列表索引获选所选设备 */
    audioIns = devTool->AudioRecordingDevicesList();
    UpLogMircophoneDevie(audioIns);
    if (audioIns.size() > index) {
      audioDevice = audioIns.at(index);
    }
  }

  LOGD("baselock8");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  baseAdm->AudioInDevID = index;
  baseAdm->mAudioInProp = audioDevice;
  baseAdm->mLoopBackDevice = LoopBackDeivce;
  if (baseAdm->peer_connection_factory_ && baseAdm->mAudioDevice) {
    baseAdm->mAudioDevice->StopRecording();
    if (audioDevice && audioDevice->m_sDeviceType == TYPE_COREAUDIO) {
      baseAdm->mAudioDevice->SetRecordingDevice(audioDevice->mIndex);
    }
    if (audioDevice && audioDevice->m_sDeviceType == TYPE_DSHOW_AUDIO) {
      char guid[512] = { 0 };
      int nLen = WideCharToMultiByte(CP_UTF8, 0, audioDevice->mDevGuid.c_str(), -1, NULL, 0, NULL, NULL);
      if (nLen >= 0) {
        WideCharToMultiByte(CP_UTF8, 0, audioDevice->mDevGuid.c_str(), -1, guid, nLen, NULL, NULL);
        baseAdm->mAudioDevice->SetRecordingDevice(guid);
      }
      else { // error: null guid
        UpLogSetMicroPhoneDevcie(audioDevice, LoopBackDeivce, baseAdm->mLoopBackVolume, "fail", "AudioDeviceModule input device config error, guid is null");
        LOGE("AudioDeviceModule input device config error, guid is null");
        return false;
      }
    }
    baseAdm->mAudioDevice->InitMicrophone();
    baseAdm->mAudioDevice->InitRecording();
    baseAdm->mAudioDevice->StartRecording();
    baseAdm->mAudioDevice->StopLoopBack();
    if (LoopBackDeivce != L"") {
      baseAdm->mAudioDevice->StartLoopBack(LoopBackDeivce, baseAdm->mLoopBackVolume);
    }
  }
  else {
    LOGW("AudioDevice not initialized");
  }
  lock.unlock();
  LOGD("unbaselock8");

  UpLogSetMicroPhoneDevcie(audioDevice, LoopBackDeivce, baseAdm->mMicroPhoneVolume, "success", "SetAudioInDevice done");
  LOGD("done");
  return true;
}

bool BaseStack::SetLoopBackVolume(const uint32_t volume) {
  LOGI("SetLoopBackVolume: %u", volume);
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  LOGD("baselock9");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  if (volume > 100) {
    baseAdm->mLoopBackVolume = 100;
  }
  else {
    baseAdm->mLoopBackVolume = volume;
  }
  if (baseAdm->mAudioDevice) {
    baseAdm->mAudioDevice->SetLoopBackVolume(baseAdm->mLoopBackVolume);
  }
  lock.unlock();
  LOGD("unbaselock9");
  LOGI("SetLoopBackVolume done");
  return true;
}

bool BaseStack::SetMicrophoneVolume(const uint32_t volume) {
  LOGI("SetMicrophoneVolume: %u", volume);
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  LOGD("baselock10");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  if (volume > 100) {
    baseAdm->mMicroPhoneVolume = 100;
  }
  else {
    baseAdm->mMicroPhoneVolume = volume;
  }
  if (baseAdm->mAudioDevice) {
    baseAdm->mAudioDevice->SetMicrophoneVolume(baseAdm->mMicroPhoneVolume);
  }
  lock.unlock();
  LOGD("unbaselock10");
  LOGI("SetMicrophoneVolume done");
  return true;
}

bool BaseStack::PostTask(E_BASE_TASK task, BaseMessage * data, int delay)
{
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return false;
  }
  LOGI("%s, tastId:%d, streamId:%s, local:%d", __FUNCTION__, (int)(task), vhStream->GetID(), vhStream->mLocal);
  if (mStopFlag) {
    if (data) {
      delete data;
      data = nullptr;
    }
    LOGW("task thread stopped, will not execute");
    return false;
  }
  std::shared_ptr<rtc::Thread> thread_ = mLocalThread;
  if (thread_) {
    if (delay > 0) {
      thread_->PostDelayed(RTC_FROM_HERE, delay, this, task, data);
    }else {
      thread_->Post(RTC_FROM_HERE, this, task, data);
    }

  }
  else {
    if (data) {
      delete data;
      data = nullptr;
    }
    return false;
  }
  return true;
}

void BaseStack::SetReport(std::shared_ptr<VHLogReport> report){
  LOGD("%s", __FUNCTION__);
  std::unique_lock<std::mutex> lock(mLogMtx);
  mReportLog = report;
}

std::shared_ptr<VHLogReport> BaseStack::GetReport()
{
  std::unique_lock<std::mutex> lock(mLogMtx);
  return mReportLog;
}

bool BaseStack::SetSpeakerVolume(uint32_t volume) {
  LOGD("SetSpeakerVolume");
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  LOGD("baselock11");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  if (volume > 100) {
    baseAdm->mSpeakerVolume = 100;
  }
  else {
    baseAdm->mSpeakerVolume = volume;
  }
  if (!baseAdm->mAudioDevice) {
    LOGD("Audio Device is empty!");
    return false;
  }
  bool available = false;
  baseAdm->mAudioDevice->SpeakerVolumeIsAvailable(&available);
  if (!available) {
    LOGE("Speaker Volume is not Available!");
    return false;
  }
  uint32_t MaxVol;
  uint32_t MinVol;
  baseAdm->mAudioDevice->MaxSpeakerVolume(&MaxVol);
  baseAdm->mAudioDevice->MinSpeakerVolume(&MinVol);
  uint32_t RealVolume = uint32_t((float(MaxVol - MinVol) / 100 * baseAdm->mSpeakerVolume) + MinVol);
  baseAdm->mAudioDevice->SetSpeakerVolume(RealVolume);
  lock.unlock();
  LOGD("unbaselock11");
  LOGD("End SetAudioOutVolume");
  return true;
}

// Selects a source to be captured. Returns false in case of a failure (e.g.
// if there is no source with the specified type and id.)
bool BaseStack::SelectSource(int64_t id) {
  DesktopCaptureImpl* deskCapture = nullptr;
  if (video_device) {
    webrtc::VideoCapturer* source = nullptr;
    source = video_device->captureSource();
    deskCapture = static_cast<DesktopCaptureImpl*>(source);
  }
  if (!deskCapture) {
    return false;
  }
  deskCapture->SelectSource(id);
  return true;
}

bool BaseStack::UpdateRect(int32_t left, int32_t top, int32_t right, int32_t bottom){
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return false;
  }
  if (vhStream->mStreamType != 6) {
    LOGE("vhStream->mStreamType:%d", vhStream->mStreamType);
    return false;
  }

  BaseMessage* baseMsg = new BaseMessage();
  baseMsg->mRect = webrtc::DesktopRect::MakeLTRB(left, top, right, bottom);
  PostTask(MsgUpdateRect, baseMsg);
  return true;
}

bool BaseStack::OnUpdateRect(webrtc::DesktopRect& _rect) {
  DesktopCaptureImpl* deskCapture = nullptr;
  if (video_device) {
    webrtc::VideoCapturer* source = nullptr;
    source = video_device->captureSource();
    deskCapture = static_cast<DesktopCaptureImpl*>(source);
  }
  if (!deskCapture) {
    return false;
  }
  deskCapture->UpdateRect(_rect);
  return true;
}

bool BaseStack::SetChangeVoiceType(int newType) {
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  LOGD("baselock12");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  baseAdm->mVCC.mType = newType;
  if (!baseAdm->peer_connection_factory_) {
    LOGE("pcf not inited, SetChangeVoiceType failed");
    return false;
  }
  bool ret = false; 
  ret = baseAdm->peer_connection_factory_->SetVoiceChange(&baseAdm->mVCC);
  lock.unlock();
  LOGD("unbaselock12");
  return ret;
}

int BaseStack::GetChangeType() {
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return kVoiceTypeNone;
  }
  LOGD("baselock13");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  int type = baseAdm->mVCC.mType;
  lock.unlock();
  LOGD("unbaselock13");
  return type;
}

bool BaseStack::ChangePitchValue(double value) {
  std::shared_ptr<BaseAudioDeviceModule> baseAdm = AdmConfig::GetAdmInstance();
  if (!baseAdm) {
    LOGE("GetAdmInstance failed");
    return false;
  }
  LOGD("baselock14");
  std::unique_lock<std::mutex> lock(baseAdm->mMtx);
  baseAdm->mVCC.mPitchChange = value;
  if (!baseAdm->peer_connection_factory_) {
    LOGE("pcf not inited, ChangePitchValue faild");
    return false;
  }
  bool ret = baseAdm->peer_connection_factory_->SetVoiceChange(&baseAdm->mVCC);
  lock.unlock();
  LOGD("unbaselock14");
  return ret;
}

void BaseStack::SetFilterParam(LiveVideoFilterParam filterParam) {
  BaseMessage* baseMsg = new BaseMessage();
  if (nullptr == baseMsg) {
    LOGE("SetBeautifyLevel failed");
    return;
  }
  baseMsg->mFilterParam = filterParam;

  PostTask(MsgVideoFilterParam, baseMsg);
}

bool BaseStack::FilePlay(const char *file) {
  LOGI("FilePlay: %s", file);
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallPlay((char *)file);
}

void BaseStack::FilePause() {
  LOGI("FilePause");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallPause();
}

void BaseStack::FileResume() {
  LOGI("FileResume");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallResume();
}

void BaseStack::FileStop() {
  LOGI("FileStop");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallStop();
}

void BaseStack::FileSeek(const unsigned long long& seekTimeInMs) {
  LOGD("FileSeek");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallSeek(seekTimeInMs);
}

const long long BaseStack::FileGetMaxDuration() {
  LOGD("FileGetMaxDuration");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallGetMaxDuration();
}

const long long BaseStack::FileGetCurrentDuration() {
  LOGD("FileGetCurrentDuration");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallGetCurrentDuration();
}

void BaseStack::FileSetVolumn(unsigned int volumn) {
  LOGD("FileSetVolumn");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallSetVolumn(volumn);
}

int BaseStack::FileGetVolumn() {
  LOGD("FileGetVolumn");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallGetVolumn();
}

int BaseStack::GetPlayerState() {
  LOGD("GetPlayerState");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->GetPlayerState();
}

void BaseStack::SetEventCallBack(FileEventCallBack cb, void *param) {
  LOGD("SetEventCallBack");
  std::shared_ptr<IMediaReader> media_reader = CreateMediaReader();
  return media_reader->VhallSetEventCallBack(cb, param);
}

void BaseStack::RequestSnapShot(SnapShotCallBack * callBack, int width, int height, int quality) {
  BaseMessage* baseMsg = new BaseMessage();
  if (nullptr == baseMsg) {
    LOGE("RequestSnapShot failed");
    return;
  }
  baseMsg->mSnapShotListener = callBack;
  baseMsg->mSnapHeight = height;
  baseMsg->mSnapWidth = width;
  baseMsg->mSnapQuality = quality;
  PostTask(MsgRequestSnapShot, baseMsg);
}

void BaseStack::OnRequestSnapShot(SnapShotCallBack * callBack, int width, int height, int quality) {
  if (snapshot_) {
    snapshot_->RequestSnapShot(callBack, width, height, quality);
  }
}

const char* BaseStack::GetStreamID() {
  if (mStreamId.empty()) {
    return "";
  } else {
    return mStreamId.c_str();
  }
}
NS_VH_END