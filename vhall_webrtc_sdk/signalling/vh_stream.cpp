//
//  ec_stream.cpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//
#include <fstream>
#include <chrono>

#include "vh_stream.h"
#include "BaseStack/BaseStack.h"
#include "vh_room.h"
#include "common/message_type.h"
#include "common/vhall_log.h"
#include "vh_uplog.h"
#include "vh_tools.h"
#include "BaseStack/AdmConfig.h"
#include "signalling/BaseStack/GetStatsObserver.h"
#include "tool/VideoRenderReceiver.h"
#include "3rdparty/VHGPUImage/wglContext/VHGLContext.h"

NS_VH_BEGIN

using webrtc::SdpType;
std::mutex VHStream::mReportLogMtx;
std::shared_ptr<VHLogReport> VHStream::reportLog = nullptr;
std::atomic<uint32_t> VHStream::mAudioOutVol(100);
std::atomic<uint32_t> VHStream::mAudioInVol(100);

VHStream::VHStream(const StreamMsg &config) :
  VideoDevID(""), mStreamId("local")
{
  InitLog();
  //reportLog = nullptr;
  LOGI("Create VHStream");
  mAudio = config.mAudio;
  mVideo = config.mVideo;
  mScreen = config.mScreen;
  mMix = config.mMix;
  mData = config.mData;
  mStreamType = config.mStreamType;
  mUser = config.mUser;
  mLocal = config.mLocal;
  mId = config.mId;
  mStreamAttributes = config.mStreamAttributes;
  mVideoCodecPreference = config.mVideoCodecPreference;
  reconnect = config.reconnect;
  mNumSpatialLayers = config.mNumSpatialLayers;
  mMuteStreams = config.mMuteStreams;
  mDegradationPreference = config.mDegradationPreference;

  if (VIDEO_PROFILE_UNDEFINED != config.videoOpt.mProfileIndex) {
    videoOpt.mProfileIndex = config.videoOpt.mProfileIndex;
    /* parse video params */
    std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(videoOpt.mProfileIndex);
    if (profile && profile.get()) {
      videoOpt.maxWidth = profile->mMaxWidth;
      videoOpt.minWidth = profile->mMinWidth;
      videoOpt.maxHeight = profile->mMaxHeight;
      videoOpt.minHeight = profile->mMinHeight;
      videoOpt.maxFrameRate = profile->mMaxFrameRate;
      videoOpt.minFrameRate = profile->mMinFrameRate;
      videoOpt.maxVideoBW = profile->mMaxBitRate;
      videoOpt.minVideoBW = profile->mMinBitRate;
      videoOpt.startVideoBW = profile->mStartBitRate;
      videoOpt.ratioW = config.videoOpt.ratioW;
      videoOpt.ratioH = config.videoOpt.ratioH;
      videoOpt.lockRatio = config.videoOpt.lockRatio;
    }
  }
  else {
    videoOpt.maxWidth = config.videoOpt.maxWidth;
    videoOpt.minWidth = config.videoOpt.minWidth;
    videoOpt.maxHeight = config.videoOpt.maxHeight;
    videoOpt.minHeight = config.videoOpt.minHeight;
    videoOpt.maxFrameRate = config.videoOpt.maxFrameRate;
    videoOpt.minFrameRate = config.videoOpt.minFrameRate;
    videoOpt.maxVideoBW = config.videoOpt.maxVideoBW;
    videoOpt.minVideoBW = config.videoOpt.minVideoBW;
    videoOpt.ratioW = config.videoOpt.ratioW;
    videoOpt.ratioH = config.videoOpt.ratioH;
    videoOpt.lockRatio = config.videoOpt.lockRatio;
  }

  mBaseStack.reset();
  mMuteStreams.mAudio = false;
  mMuteStreams.mVideo = false;
  showing = false;
  attempted = 0;
  mState = VhallStreamState::Uninitialized;

  SetID(mId);
}

void VHStream::Init() {
  LOGI("reset SSRC Info");
  std::unique_lock<std::mutex> ssrclock(ssrcMtx);
  mPublishAudio.reset(new SendAudioSsrc());
  mPublishVideo.reset(new SendVideoSsrc());
  mSubscribeAudio.reset(new ReceiveAudioSsrc());
  mSubscribeVideo.reset(new ReceiveVideoSsrc());
  mOverallBwe.reset(new Bwe());
  ssrclock.unlock();

  LOGI("Init VHStream");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  mBaseStack = std::make_shared<BaseStack>(shared_from_this());
  if (!mBaseStack) {
    LOGE("Create BaseStack Failure!");
    return;
  }
  mBaseStack->Init();
  std::unique_lock<std::mutex> devLock(mDevMtx);
  mBaseStack->VideoDevID = VideoDevID;
  devLock.unlock();
  if (mLocal && (1 == mStreamType ||  2 == mStreamType)) {
    SetBestCaptureFormat();
  }
  mBaseStack->audioMuted = mMuteStreams.mAudio;
  mBaseStack->videoMuted = mMuteStreams.mVideo;
  mBaseStack->DesktopID = mDesktopId;
  mBaseStack->mStreamId = GetID();
  mBaseStack->SetReport(GetReport());
  SetStreamState(VhallStreamState::Initialized);
}

VHStream::~VHStream() {
  LOGI("~VHStream, local: %d, id:%s", mLocal, GetID());
  Destroy();
  LOGI("End ~VHStream");
}

void VHStream::Destroy() {
  LOGD("%s", __FUNCTION__);
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    if (mStreamType == 4 && mLocal) {
      mBaseStack->FileStop();
    }
    // sync
    mBaseStack->PostTask(STOP_STREAM, nullptr);
    mBaseStack->PostTask(CLOSE_STREAM, nullptr);
    showing = false;
    mBaseStack->Destroy();
  }
  mBaseStack.reset();
  lock.unlock();
  RemoveAllEventListener();

  mPublishAudio.reset();
  mPublishVideo.reset();
  mSubscribeAudio.reset();
  mSubscribeVideo.reset();
}

void VHStream::play(HWND &wnd) {
  LOGI("Stream play, id: %s", GetID());
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (!showing && mBaseStack) {
    mBaseStack->wnd = wnd;
    mBaseStack->PostTask(PLAY_STREAM, nullptr);
  }
  lock.unlock();
  showing = true;
  LOGI("Stream played");
}

void VHStream::play(std::shared_ptr<VideoRenderReceiveInterface> receiver) {
  LOGI("Stream play, id: %s", GetID());
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (!showing && mBaseStack) {
    mBaseStack->receiver = receiver;
    mBaseStack->PostTask(PLAY_STREAM, nullptr);
  }
  lock.unlock();
  showing = true;
  LOGI("Stream played");
}

void VHStream::stop() {
  LOGI("Stream stop, id: %s", GetID());
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack && showing) {
    if (mStreamType == 4 && mLocal) {
      mBaseStack->FileStop();
    }
    if (mBaseStack->mLocalThread) {
      // sync
      mBaseStack->PostTask(STOP_STREAM, nullptr);
    }
    mBaseStack->wnd = nullptr;
    mBaseStack->receiver = nullptr;
  }
  lock.unlock();
  showing = false;
}

void VHStream::close() {
  LOGI("Stream close, id: %s", GetID());
  RemoveAllEventListener();
  SetStreamState(VhallStreamState::Uninitialized);
  std::shared_ptr<VHRoom> room = GetRoom();
  if (mLocal && room) {
    std::shared_ptr<VHStream> stream = shared_from_this();
    room->UnPublish(stream, [](const RespMsg & resp)->void {}, 1000);
  }
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    mBaseStack->PostTask(CLOSE_STREAM, nullptr);
    mBaseStack->Destroy();
  }
  mBaseStack.reset();
  lock.unlock();
  LOGI("End Stream close");
}

void VHStream::Getlocalstream(std::shared_ptr<MediaCaptureInfo> mediaInfo) {
  LOGI("Getlocalstream");
  if (mediaInfo && mediaInfo->VideoDevID != "") {
    std::unique_lock<std::mutex> devLock(mDevMtx);
    VideoDevID = mediaInfo->VideoDevID;
    devLock.unlock();
  }
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    BaseMessage* baseMsg = new BaseMessage();
    if (baseMsg) {
      baseMsg->mCaptureInfo = mediaInfo;
    }
    mBaseStack->PostTask(GET_USER_MEDIA, baseMsg);
  }
  LOGI("end Getlocalstream");
}

void VHStream::InitPeer() {
   LOGI("InitPeer");
   std::unique_lock<std::mutex> lock(mBaseMtx);
   if (mBaseStack && mBaseStack->mLocalThread) {
     mBaseStack->SetReport(GetReport());
     mBaseStack->PostTask(INITPEERCONNECTION, nullptr);
   }
}
void VHStream::DeInitPeer() {
   LOGI("DeInitPeer"); 
   std::unique_lock<std::mutex> lock(mBaseMtx);
   if (mBaseStack && mBaseStack->mLocalThread) {
     mBaseStack->SetReport(nullptr); // disable uplog
     mBaseStack->PostTask(DEINITPEERCONNECTION, nullptr);
   }
}

void VHStream::StreamSubscribed() {
  LOGI("StreamSubscribed");
  std::shared_ptr<VHRoom> room = GetRoom();
  if (room) {
    room->DispatchStreamSubscribed(shared_from_this());
  }
}

const char* VHStream::GetID() {
  LOGI("Get Stream ID");
  if (mLocal && mStreamId.empty()) {
    return "local";
  } else {
    return mStreamId.c_str();
  }
}

void VHStream::SetID(std::string id) {
  mStreamId = id;
  std::istringstream a(id);
  a >> mId;
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    mBaseStack->mStreamId = mStreamId;
  }
}

void VHStream::SetID(uint64_t id) {
  std::ostringstream o;
  o << id;
  mId = mStreamId = o.str();
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    mBaseStack->mStreamId = mStreamId;
  }
}

const char* VHStream::GetUserId() {
  LOGI("GetUserId");
  if (mLocal && mUser.mId.empty()) {
    return "local";
  } else {
    return mUser.mId.c_str();
  }
}

bool VHStream::HasAudio() {
  LOGI("HasAudio");
  return this->mAudio;
}

bool VHStream::HasVideo() {
  LOGI("HasVideo");
  return this->mVideo;
}

bool VHStream::HasData() {
  LOGI("HasData");
  return this->mData;
}

bool VHStream::HasScreen() {
  LOGI("HasScreen");
  return this->mScreen;
}

bool VHStream::HasMedia() {
  LOGI("HasMedia");
  return this->mAudio || this->mVideo || this->mScreen;
}

bool VHStream::isExternal() {
  LOGI("isExternal");
  return false;
}

bool VHStream::SetAudioInDevice(uint32_t index, std::wstring LoopBackDeivce) {
  LOGI("SetAudioInDevice");
  return BaseStack::SetAudioInDevice(index, LoopBackDeivce);
}

bool VHStream::SetLoopBackVolume(const uint32_t volume) {
  LOGI("");
  return BaseStack::SetLoopBackVolume(volume);
}

bool VHStream::SetMicrophoneVolume(const uint32_t volume) {
  LOGI("");
  return BaseStack::SetMicrophoneVolume(volume);
}

bool VHStream::SetSpeakerVolume(uint32_t volume) {
  return BaseStack::SetSpeakerVolume(volume);
}

bool VHStream::SetAudioOutDevice(uint32_t index) {
  LOGI("SetAudioOutDevice");
  return BaseStack::SetAudioOutDevice(index);
}

void VHStream::SetVideoDevice(std::string& devId) {
  LOGI("SetVideoDevice");
  std::unique_lock<std::mutex> devLock(mDevMtx);
  this->VideoDevID = devId;
  devLock.unlock();
}

bool VHStream::MuteVideo(bool isMuted) {
  LOGI("MuteVideo, id: %s, mute: %d", GetID(), isMuted);
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack && mBaseStack->mLocalThread) {
    mBaseStack->videoMuted = isMuted;
    mBaseStack->PostTask(MUTESTREAM, nullptr);
    mMuteStreams.mVideo = isMuted;
  } else {
    LOGE("mute video failed: stream not inited");
    return false;
  }
  return true;
}

bool VHStream::IsAudioMuted() {
  std::unique_lock<std::mutex> lock(mBaseMtx);
  return mMuteStreams.mAudio;
}

bool VHStream::IsVideoMuted() {
  std::unique_lock<std::mutex> lock(mBaseMtx);
  return mMuteStreams.mVideo;
}

bool VHStream::MuteAudio(bool isMuted) {
  LOGI("MuteAudio, id :%s, mute: %d", GetID(), isMuted);
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack && mBaseStack->mLocalThread) {
    mBaseStack->audioMuted = isMuted;
    mBaseStack->PostTask(MUTESTREAM, nullptr);
    mMuteStreams.mAudio = isMuted;
  } else {
    LOGE("mute audio failed: stream not inited");
    return false;
  }
  return true;
}

bool VHStream::SetFileVolume(uint32_t volume) {
  LOGI("");
  if (!mStreamType == 4 || !mLocal) {
    return false;
  }
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    if (volume > 100) {
      volume = 100;
    }
    mBaseStack->FileSetVolumn(volume);
    return true;
  }
  else {
    return false;
  }
}

uint32_t VHStream::GetFileVolume() {
  LOGI("");
  if (!mStreamType == 4 || !mLocal) {
    return -1;
  }
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FileGetVolumn();
  }
  else {
    return -1;
  }
}

uint32_t VHStream::GetAudioInVolume() {
  return mAudioInVol;
}

uint32_t VHStream::GetAudioOutVolume() {
  return mAudioOutVol;
}

bool VHStream::IsSupportBeauty() {
  std::shared_ptr<VHGLContext> glContext;
  glContext.reset(new VHGLContext());
  if (!glContext) {
    return false;
  }

  bool result = glContext->Initialize();
  glContext.reset();
  if (!result) {
    return false;
  }

  return true;
}

bool VHStream::SetFilterParam(LiveVideoFilterParam filterParam) {
  LOGI("");
  if (filterParam.beautyLevel > 0) {
    if (!IsSupportBeauty()) {
      return false;
    }
  }
 // return true;
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    mBaseStack->SetFilterParam(filterParam);
  }
  return true;
}

void VHStream::SetAudioListener(AudioSendFrame* listener) {
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack && mBaseStack->mLocalThread) {
    BaseMessage* baseMsg = new BaseMessage();
    if (baseMsg) {
      baseMsg->mListener = listener;
      mBaseStack->PostTask(SET_AUDIO_OUTPUT, baseMsg);
    }
    else {
      LOGE("new BaseMessage failed");
    }
  }
}

bool VHStream::ChangeVoiceType(VoiceChangeType newType) {
  std::shared_ptr<VHLogReport> reportLog = GetReport();
  if (reportLog) {
    std::stringstream ss;
    ss << "ChangeVoiceType:";
    ss << newType;
    reportLog->upLog(ulSetVoiceChange, "", ss.str());
  }
  return BaseStack::SetChangeVoiceType(newType);
}

VoiceChangeType VHStream::GetChangeType() {
  return (VoiceChangeType)BaseStack::GetChangeType();
}

bool VHStream::ChangePitchValue(double value) {
  std::shared_ptr<VHLogReport> reportLog = GetReport();
  if (reportLog) {
    std::stringstream ss;
    ss << "ChangePitchValue:";
    ss << value;
    reportLog->upLog(ulSetVoiceChange, "", ss.str());
  }
  return (VoiceChangeType)BaseStack::ChangePitchValue(value);
}

// Selects a source to be captured. Returns false in case of a failure (e.g.
// if there is no source with the specified type and id.)
bool VHStream::SelectSource(int64_t id) {
	std::unique_lock<std::mutex> lock(mBaseMtx);
	if (!mBaseStack) {
		return false;
	}
	if (mStreamType == VhallStreamScreen || mStreamType == VhallStreamWindow) {
		return mBaseStack->SelectSource(id);
	}
	return false;
}

bool VHStream::ResetCapability(videoOptions & opt) {
  if (!mLocal) {
    LOGE("ResetCapability fail: local stream");
    return false;
  }

  if (opt.devId != "") {
    SetVideoDevice(opt.devId);
  }

  if (VIDEO_PROFILE_UNDEFINED != opt.mProfileIndex) {
    /* parse video params */
    std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(opt.mProfileIndex);
    if (profile && profile.get()) {
      opt.maxWidth = profile->mMaxWidth;
      opt.minWidth = profile->mMinWidth;
      opt.maxHeight = profile->mMaxHeight;
      opt.minHeight = profile->mMinHeight;
      opt.maxFrameRate = profile->mMaxFrameRate;
      opt.minFrameRate = profile->mMinFrameRate;
      opt.maxVideoBW = profile->mMaxBitRate;
      opt.minVideoBW = profile->mMinBitRate;
      opt.ratioW = opt.ratioW;
      opt.ratioH = opt.ratioH;
      opt.lockRatio = opt.lockRatio;
    }
  }
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (!mBaseStack) {
    return false;
  }

  mBaseStack->ResetCapability(opt);
  lock.unlock();
  return true;
}

bool VHStream::UpdateRect(int32_t left, int32_t top, int32_t right, int32_t bottom) {
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (!mBaseStack) {
    return false;
  }
  return mBaseStack->UpdateRect(left, top, right, bottom);
}

bool VHStream::GetAudioPublishStates(struct SendAudioSsrc *pSsrc) {
  LOGI("GetAudioPublishStates, id: %s", GetID());
  std::unique_lock<std::mutex> lock(ssrcMtx);
  if (!mPublishAudio || !pSsrc) {
    return false;
  }
  SsrcCalcData tmpData = pSsrc->CalcData;
  *pSsrc = *mPublishAudio;
  pSsrc->CalcData = tmpData;
  LOGI("done");
  return true;
}

bool VHStream::GetVideoPublishStates(struct SendVideoSsrc *pSsrc) {
  LOGI("GetVideoPublishStates, id: %s", GetID());
  std::unique_lock<std::mutex> lock(ssrcMtx);
  if (!mPublishVideo || !pSsrc) {
    return false;
  }
  SsrcCalcData tmpData = pSsrc->CalcData;
  *pSsrc = *mPublishVideo;
  pSsrc->CalcData = tmpData;
  LOGI("done");
  return true;
}

bool VHStream::GetAudioSubscribeStates(struct ReceiveAudioSsrc *pSsrc) {
  LOGI("GetAudioSubscribeStates, id: %s", GetID());
  std::unique_lock<std::mutex> lock(ssrcMtx);
  if (!mSubscribeAudio || !pSsrc) {
    return false;
  }
  SsrcCalcData tmpData = pSsrc->CalcData;
  *pSsrc = *mSubscribeAudio;
  pSsrc->CalcData = tmpData;
  LOGI("done");
  return true;
}

bool VHStream::GetVideoSubscribeStates(struct ReceiveVideoSsrc *pSsrc) {
  LOGI("GetVideoSubscribeStates, id: %s", GetID());
  std::unique_lock<std::mutex> lock(ssrcMtx);
  if (!mSubscribeVideo || !pSsrc) {
    return false;
  }
  SsrcCalcData tmpData = pSsrc->CalcData;
  *pSsrc = *mSubscribeVideo;
  pSsrc->CalcData = tmpData;
  LOGI("done");
  return true;
}

bool VHStream::GetOverallBwe(struct Bwe *pBwe) {
  LOGI("GetOverallBwe, id: %s", GetID());
  std::unique_lock<std::mutex> lock(ssrcMtx);
  if (!mOverallBwe || !pBwe) {
    return false;
  }
  *pBwe = *mOverallBwe;
  LOGI("done");
  return true;
}

void VHStream::RequestSnapShot(SnapShotCallBack * callBack, int width, int height, int quality) {
  LOGI("RequestSnapShot, id: %s", GetID());
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (!mBaseStack) {
    LOGE("RequestSnapShot failed, id: %s", GetID());
    return;
  }
  mBaseStack->RequestSnapShot(callBack, width, height, quality);
  LOGI("RequestSnapShot end, id: %s", GetID());
  return;
}

bool VHStream::OpenUpStats(std::string url) {
  return GetStatsObserver::OpenUpStats(url);
}

bool VHStream::CloseUpStats() {
  return GetStatsObserver::CloseUpStats();
}

void VHStream::SetStreamState(VhallStreamState state) {
   std::unique_lock<std::mutex> lock(mStateMtx);
   mState = state;
   lock.unlock();
}
VhallStreamState VHStream::GetStreamState() {
   std::unique_lock<std::mutex> lock(mStateMtx);
   VhallStreamState state = mState;
   lock.unlock();
   return state;
}
void VHStream::OnMessageFromErizo(const SignalingMessageVhallRespMsg &ErizoMsg) {
  LOGI("OnMessageFromErizo: %s, ID:%s", ErizoMsg.mMess.mType.c_str(), GetID());

  if (ErizoMsg.mMess.mType == "initializing") {
   
  } else if (ErizoMsg.mMess.mType == "started") {
    std::unique_lock<std::mutex> lock(mBaseMtx);
    if (mBaseStack && mBaseStack->mLocalThread && GetStreamState() != VhallStreamState::Uninitialized) {
      mBaseStack->PostTask(CREATE_OFFER, nullptr);
    }
  } else if (ErizoMsg.mMess.mType == "answer") {
    std::unique_lock<std::mutex> lock(mBaseMtx);
    if (mBaseStack && mBaseStack->mLocalThread && GetStreamState() != VhallStreamState::Uninitialized) {
      mBaseStack->ReceiveAnswer(ErizoMsg);
      mBaseStack->PostTask(PROCESS_ANSWER, nullptr);
    }
  } else if (ErizoMsg.mMess.mType == "ready") {
    if (mLocal) {
      std::shared_ptr<VHLogReport> reportLog = GetReport();
      if (reportLog) {
        reportLog->upLog(localStreamReady, GetID(), std::string("SignalingMessageVhallRespMsg:") +ErizoMsg.mJson_str);
        if (mLocal) {
          reportLog->upInitStreamParam(GetID(), "publish ready, Init params");
        }
      }
    }
    else {
      std::shared_ptr<VHLogReport> reportLog = GetReport();
      if (reportLog) {
        reportLog->upLog(RemoteStreamReady, GetID(), std::string("SignalingMessageVhallRespMsg:") + ErizoMsg.mJson_str);
      }
    }
  }
}

uint64_t VHStream::GetSteadyTimeStamp() {
  LOGD("GetSteadyTimeStamp");
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> time_span = t1.time_since_epoch();
  return uint64_t(time_span.count() * 1000);
}

void VHStream::SetRoom(std::shared_ptr<VHRoom> room) {
  std::unique_lock<std::mutex> l(mRoomMtx);
  mRoom = room;
}

std::shared_ptr<VHRoom> VHStream::GetRoom()
{
  std::unique_lock<std::mutex> l(mRoomMtx);
  return mRoom;
}

void VHStream::SetReport(std::shared_ptr<VHLogReport> report) {
  std::unique_lock<std::mutex> lock(mReportLogMtx);
  reportLog = report;
}

std::shared_ptr<VHLogReport> VHStream::GetReport() {
  std::unique_lock<std::mutex> lock(mReportLogMtx);
  return reportLog;
}

int VHStream::SetBestCaptureFormat() {
  std::shared_ptr<VHTools> DevTool = std::make_shared<VHTools>();
  if (!DevTool) {
    LOGE("init video check failed.");
    return -1;
  }

  /* ReInit params from profile */
  if (VIDEO_PROFILE_UNDEFINED != videoOpt.mProfileIndex) {
    videoOpt.mProfileIndex = videoOpt.mProfileIndex;
    /* parse video params */
    std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(videoOpt.mProfileIndex);
    if (profile && profile.get()) {
      videoOpt.maxWidth = profile->mMaxWidth;
      videoOpt.minWidth = profile->mMinWidth;
      videoOpt.maxHeight = profile->mMaxHeight;
      videoOpt.minHeight = profile->mMinHeight;
      videoOpt.maxFrameRate = profile->mMaxFrameRate;
      videoOpt.minFrameRate = profile->mMinFrameRate;
      videoOpt.maxVideoBW = profile->mMaxBitRate;
      videoOpt.minVideoBW = profile->mMinBitRate;
    }
  }

  // video config, choose best camera config
  int maxWidth = videoOpt.maxWidth;
  int minWidth = videoOpt.minWidth;
  int maxHeight = videoOpt.maxHeight;
  int minHeight = videoOpt.minHeight;
  int maxFrameRate = videoOpt.maxFrameRate;
  int minFrameRate = videoOpt.minFrameRate;
  std::shared_ptr<VideoDevProperty> prop = nullptr;
  std::vector<std::shared_ptr<VideoDevProperty>> videoDev;
  videoDev = DevTool->VideoInDeviceList();
  std::unique_lock<std::mutex> devLock(mDevMtx);
  std::string devId = VideoDevID;
  devLock.unlock();
  for (size_t i = 0; i < videoDev.size(); i++) {
    if (videoDev.at(i) && !devId.compare(videoDev.at(i)->mDevId)) {
      prop = videoDev.at(i);
    }
  }
  if (!prop || prop->mDevFormatList.size() <= 0) {
    LOGE("cannot find video device info");
    return -2;
  }
  
  /* 过滤宽高比 */
  std::vector<std::shared_ptr<VideoFormat>> formats;
  if (videoOpt.lockRatio) {
    std::vector<std::shared_ptr<VideoFormat>> formats_Ratio;
    for (size_t k = 0; k < prop->mDevFormatList.size(); k++) {
      std::shared_ptr<VideoFormat> format = prop->mDevFormatList.at(k);
      int tolerance = 5; // tolerace = %5, range %0 ~ %100
      if((videoOpt.ratioW * (100 - tolerance) * format->height < videoOpt.ratioH * 100 * format->width) &&
        (videoOpt.ratioH * 100 * format->width < videoOpt.ratioW * (100 + tolerance) * format->height)) {
        formats.push_back(format);
      }
    }
  }
  if (formats.size() == 0) {
    LOGW("filter video Ratio fail");
    formats = prop->mDevFormatList;
  }

  auto bestMax = formats.at(0);
  auto bestMin = formats.at(0);
  int targetMaxArea = maxWidth * maxHeight;
  int targetMinArea = minWidth * minHeight;
  int BestDiffMaxArea = std::abs(maxWidth * maxHeight - bestMax->width * bestMax->height);
  int BestDiffMinArea = std::abs(minWidth * minHeight - bestMin->width * bestMin->height);
  INT32 BestdiffMaxFps = (maxFrameRate - bestMax->maxFPS);
  for (size_t k = 0; k < formats.size(); k++) {
    std::shared_ptr<VideoFormat> format = formats.at(k);
    INT32 diffMaxArea = std::abs(targetMaxArea - format->width * format->height);
    INT32 diffMinArea = std::abs(targetMinArea - format->width * format->height);
    INT32 diffMaxFps = (maxFrameRate - format->maxFPS);
    INT32 diffMinFps = std::abs(format->maxFPS - minFrameRate);
    if (diffMaxArea < BestDiffMaxArea || (diffMaxArea == BestDiffMaxArea && diffMaxFps < BestdiffMaxFps)) {
      bestMax = format;
      BestDiffMaxArea = diffMaxArea;
      BestdiffMaxFps = diffMaxFps;
    }
  }

  videoOpt.maxFrameRate = bestMax->maxFPS > maxFrameRate ? maxFrameRate : bestMax->maxFPS;
  videoOpt.minFrameRate = bestMin->maxFPS > minFrameRate ? minFrameRate : bestMin->maxFPS;

  videoOpt.DshowFormat = bestMax;
  /* 根据采集属性、请求属性计算推流属性 */
  videoOpt.publishFormat.maxFPS = bestMax->maxFPS > maxFrameRate ? maxFrameRate : bestMax->maxFPS;

  if (bestMax->width * bestMax->height > videoOpt.maxWidth * videoOpt.maxHeight) { /* 采样分辨率大于配置分辨率 */
    if (bestMax->width >= bestMax->height) {
      videoOpt.publishFormat.width = videoOpt.maxWidth;
      float scaleRatio = 1.0f * videoOpt.maxWidth / bestMax->width;
      videoOpt.publishFormat.height = scaleRatio * bestMax->height;
    }
    else if (bestMax->width < bestMax->height) {
      videoOpt.publishFormat.height = videoOpt.maxHeight;
      float scaleRatio = 1.0f * videoOpt.maxHeight / bestMax->height;
      videoOpt.publishFormat.width = scaleRatio * bestMax->width;
    }

    videoOpt.publishFormat.width = (videoOpt.publishFormat.width + 3) / 4 * 4;
    videoOpt.publishFormat.height = (videoOpt.publishFormat.height + 3) / 4 * 4;
  }

  /* 根据实际推流分辨率、帧率配置码率 */
  int32_t targetPixelCount = videoOpt.publishFormat.width * videoOpt.publishFormat.height;
  int32_t PixelcountDiff = 1920 * 1080;
  std::shared_ptr<VideoProfile> FinalProfile = nullptr;
  for (int index = RTC_VIDEO_PROFILE_1080P_4x3_L; index < RTC_VIDEO_PROFILE_90P_16x9_H; index++) {
    std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(static_cast<VideoProfileIndex>(index));
    if (!profile) {
      continue;
    }

    int curPixelCountDiff = profile->mMaxHeight * profile->mMaxWidth - targetPixelCount;
    if (profile->mMaxFrameRate >= videoOpt.publishFormat.maxFPS && curPixelCountDiff >= 0 && curPixelCountDiff < PixelcountDiff) {
      PixelcountDiff = abs(profile->mMaxHeight * profile->mMaxWidth - targetPixelCount);
      FinalProfile = profile;
    }
  }
  if (FinalProfile) {
    videoOpt.maxVideoBW = FinalProfile->mMaxBitRate;
    videoOpt.minVideoBW = FinalProfile->mMinBitRate;
    videoOpt.startVideoBW = FinalProfile->mStartBitRate;
  }

  return 0;
}

void VHStream::SendData(const std::string& data, const vhall::VHEventCB & callback, uint64_t timeOut) {
  LOGI("SendData: %s", data.c_str());
  std::shared_ptr<VHRoom> room = GetRoom();
  if (room) {
    std::shared_ptr<VHStream> stream = shared_from_this();
    room->SendSignalingMessage(data, [&, stream, callback](const RespMsg &msg)->void {
      if (callback) {
        callback(msg);
      }
      if (msg.mResult.compare("success")) {
        std::string msg = "signalling send failed";
        stream->OnPeerConnectionErr(msg);
      }
    }, 10000);
  }
}

void VHStream::OnPeerConnectionErr(std::string & msg) {
  std::shared_ptr<VHStream> stream = shared_from_this();
  std::shared_ptr<VHRoom> room = GetRoom();
  if (room) {
    room->StreamFailed(stream, msg, "ice failed");
  }

  std::shared_ptr<VHLogReport> reportLog = GetReport();
  if (reportLog) {
    reportLog->upLog(ulStreamFailed, mStreamId, msg);
  }
}

bool VHStream::FilePlay(const char *file) {
  LOGI("FilePlay: %s", file);
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FilePlay(file);
  }
  return false;
}

void VHStream::FilePause() {
  LOGI("FilePause");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FilePause();
  }
}

void VHStream::FileResume() {
  LOGI("FileResume");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FileResume();
  }
}

void VHStream::FileStop() {
  LOGI("FileStop");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FileStop();
  }
}

void VHStream::FileSeek(const unsigned long long& seekTimeInMs) {
  LOGD("FileSeek");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FileSeek(seekTimeInMs);
  }
}

const long long VHStream::FileGetMaxDuration() {
  LOGD("FileGetMaxDuration");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FileGetMaxDuration();
  }
  return 0;
}

const long long VHStream::FileGetCurrentDuration() {
  LOGD("FileGetCurrentDuration");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->FileGetCurrentDuration();
  }
  return 0;
}

int VHStream::GetPlayerState() {
  LOGD("GetPlayerState");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->GetPlayerState();
  }
  return -1;
}

void VHStream::SetEventCallBack(FileEventCallBack cb, void *param) {
  LOGI("SetEventCallBack");
  std::unique_lock<std::mutex> lock(mBaseMtx);
  if (mBaseStack) {
    return mBaseStack->SetEventCallBack(cb, param);
  }
}

NS_VH_END
