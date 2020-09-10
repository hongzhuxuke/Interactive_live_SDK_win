#include "BaseStack.h"
#include "CreatePeerConnectionObserver.h"
#include "../vh_uplog.h"
NS_VH_BEGIN

// 创建pc相关回调
CreatePeerConnectionObserver::CreatePeerConnectionObserver(std::shared_ptr<BaseStack> p) {
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

CreatePeerConnectionObserver::~CreatePeerConnectionObserver() {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  LOGI("~CreatePeerConnectionCallback, id: %s", ptrBaseStack->GetStreamID());
}

void CreatePeerConnectionObserver::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  LOGI("PC OnAdd Stream, id: %s", ptrBaseStack->GetStreamID());
  if (ptrBaseStack) {
    ptrBaseStack->PostTask(RECEIVESTREAM, nullptr);
  }
}

void CreatePeerConnectionObserver::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  LOGI("PC OnRemove Stream, id: %s", ptrBaseStack->GetStreamID());
}

void CreatePeerConnectionObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  LOGI("%s", __FUNCTION__);
  LOGI("PC OnIceCandidate Stream: %d", candidate->sdp_mline_index());
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }

  SignalingMessageMsg msg;
  msg.mType = "candidate";
  msg.mCandidate.mSdpMid = candidate->sdp_mid();
  msg.mCandidate.mSdpMLineIndex = candidate->sdp_mline_index();
  msg.mStreamId = vhStream->mStreamId;

  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    LOGE("Failed to serialize candidate, id: %s", ptrBaseStack->GetStreamID());
    return;
  }
  msg.mCandidate.mCandidate = "a=" + sdp;
  /* cache candidate */
  ptrBaseStack->listIceCandidate.push_back(msg);
  /* uplog */
  if (vhStream->mLocal) {
    std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
    if (reportLog) {
      reportLog->upLog(sendLocalCandidate, ptrBaseStack->GetStreamID(), std::string("candidate:") + msg.ToJsonStr());
    }
  }
  else {
    std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
    if (reportLog) {
      reportLog->upLog(sendRemoteCandidate, ptrBaseStack->GetStreamID(), std::string("candidate") + msg.ToJsonStr());
    }
  }
}

void CreatePeerConnectionObserver::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  LOGI("PC OnSignalingChange: %d, id: %s", new_state, ptrBaseStack->GetStreamID());
}

void CreatePeerConnectionObserver::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  LOGI("PC OnDataChannel");
}

void CreatePeerConnectionObserver::OnRenegotiationNeeded() {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  LOGI("PC OnRenegotiationNeeded, id: %s", ptrBaseStack->GetStreamID());
}

void CreatePeerConnectionObserver::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionNew", ptrBaseStack->GetStreamID());
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionChecking) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionChecking", ptrBaseStack->GetStreamID());
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionConnected", ptrBaseStack->GetStreamID());
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionCompleted", ptrBaseStack->GetStreamID());
    std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
    if (!ptrBaseStack) {
      LOGE("BaseStack is reset!");
      return;
    }
    ptrBaseStack->PostTask(IceConnectionCompleted, nullptr);
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionFailed) {
    LOGE("PC OnIceConnectionChange, id: %s IceConnectionFailed", ptrBaseStack->GetStreamID());
    std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
    if (!ptrBaseStack) {
      LOGE("BaseStack is reset!");
      return;
    }
    ptrBaseStack->PostTask(IceConnectionFailed, nullptr);
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionDisconnected) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionDisconnected", ptrBaseStack->GetStreamID());
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionClosed", ptrBaseStack->GetStreamID());
  } else if (new_state == webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionMax) {
    LOGI("PC OnIceConnectionChange, id: %s IceConnectionMax", ptrBaseStack->GetStreamID());
  } else {
    LOGE("PC OnIceConnectionChange, id: %s unknown", ptrBaseStack->GetStreamID());
  }
}

void CreatePeerConnectionObserver::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  if (webrtc::PeerConnectionInterface::kIceGatheringComplete == new_state) {
    ptrBaseStack->PostTask(IceGatheringComplete, nullptr);
  }
  LOGI("PC OnIceGatheringChange: %d, id: %s", new_state, ptrBaseStack->GetStreamID());

}

void CreatePeerConnectionObserver::OnIceConnectionReceivingChange(bool receiving) {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  LOGI("PC OnIceConnectionReceivingChange: bool-%d, id: %s", receiving, ptrBaseStack->GetStreamID());
}

void CreatePeerConnectionObserver::OnIceCandidateComplete() {
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  LOGI("Send Candidates");
  for (SignalingMessageMsg Candidate : ptrBaseStack->listIceCandidate) {
    vhStream->SendData(Candidate.ToJsonStr(), [&](const vhall::RespMsg &msg) -> void {
      LOGI("Candidate response: %s", msg.mResult.c_str());
    });
  }
  LOGI("Send Candidates complete");
}

void CreatePeerConnectionObserver::OnIceConnectionFailed() {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (vhStream) {
    if (vhStream->GetStreamState() == VhallStreamState::Publish || vhStream->GetStreamState() == VhallStreamState::Subscribe) {
      std::string msg("IceConnectionFailed");
      vhStream->OnPeerConnectionErr(msg);
    }
  }
}

NS_VH_END