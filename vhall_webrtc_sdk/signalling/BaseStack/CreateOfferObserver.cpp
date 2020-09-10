#include "BaseStack.h"
#include "CreateOfferObserver.h"
#include "../../common/sdp_helpers.h"
#include "../vh_uplog.h"
NS_VH_BEGIN

// 创建offer相关回调
CreateOfferObserver::CreateOfferObserver(std::shared_ptr<BaseStack> p) {
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

CreateOfferObserver::~CreateOfferObserver() {
  LOGI("~CreateOfferObserver");
  m_session_description.release();
  m_session_description.reset();
}

void CreateOfferObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  LOGI("Create Offer Success");
  mDesc = desc;

  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  ptrBaseStack->PostTask(CREATEOFFERSUCCESS, nullptr);
}

void CreateOfferObserver::OnFailure(const std::string& error) {
  std::string msg = "Create Offer Failure: " + error;
  LOGE("%s", msg.c_str());
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  ptrBaseStack->PostTask(CREATEOFFERFAILED, nullptr);
}

void CreateOfferObserver::OnSuccess() {
  LOGI("%s", __FUNCTION__);
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack || nullptr == mDesc) {
    LOGE("BaseStack is reset!");
    return;
  }
  if (nullptr == ptrBaseStack->peer_connection_) {
    LOGE("%s peerconnection is null", __func__);
    return;
  }
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  std::string sdp;
  if (vhStream->mState != VhallStreamState::Uninitialized) {
    SignalingMessageMsg msg;
    msg.mStreamId = vhStream->mStreamId;
    /* switch sdp for single/multi channel, mNumSpatialLayers = 3 at most */
    if (vhStream->mLocal && (vhStream->mNumSpatialLayers == 2 || vhStream->mNumSpatialLayers == 3)) {
      /* simulcast: create another sdp */
      mDesc->ToString(&sdp);
      webrtc::SdpParseError error;
      m_session_description.release();
      m_session_description.reset();
      m_session_description = webrtc::CreateSessionDescription(mDesc->GetType(), SdpHelpers::EnableSimulcast(sdp, vhStream->mNumSpatialLayers), &error);
      if (!m_session_description.get()) {
        LOGE("CreateSessionDescription fail");
        std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
        if (ptrBaseStack) {
          ptrBaseStack->PostTask(MsgCreateSDPFail, nullptr);
        }
        return;
      }
      /* set local sdp */
      ptrBaseStack->peer_connection_->SetLocalDescription(ptrBaseStack->mSetLocalDescriptionObserver, m_session_description.get());
      msg.mType = webrtc::SdpTypeToString(m_session_description->GetType());
      sdp = "";
      m_session_description->ToString(&sdp);
      LOGI("Offer SDP: %s", sdp.c_str());
      msg.mSdp = sdp;
    }
    else {
      /* default */
      ptrBaseStack->peer_connection_->SetLocalDescription(ptrBaseStack->mSetLocalDescriptionObserver, mDesc); /* set local sdp */
      msg.mType = webrtc::SdpTypeToString(mDesc->GetType());

      mDesc->ToString(&sdp);
      LOGI("Offer SDP: %s", sdp.c_str());
      msg.mSdp = sdp;
    }
    /* uplog */
    if (vhStream->mLocal) {
      std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
      if (reportLog) {
        reportLog->upLog(sendLocalOffer, ptrBaseStack->GetStreamID(), std::string("Offer SDP:") + sdp);
      }
    }
    else {
      std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
      if (reportLog) {
        reportLog->upLog(sendRemoteOffer, ptrBaseStack->GetStreamID(), std::string("sdp:") + sdp);
      }
    }
    std::string sendstr = msg.ToJsonStr();
    vhStream->SendData(sendstr, [&](const RespMsg &msg) -> void {
      LOGI("create offer response: %s", msg.mResult.c_str());
    });
    mDesc = nullptr;
  }
  else {
    LOGE("Stream is Uninitialized!");
    return;
  }
  LOGD("%s complete", __FUNCTION__);
}

void CreateOfferObserver::OnFailure() {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  std::string msg = "Create Offer Failure";
  vhStream->OnPeerConnectionErr(msg);
}


NS_VH_END