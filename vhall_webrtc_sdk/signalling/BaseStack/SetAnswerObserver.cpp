#include "BaseStack.h"
#include "SetAnswerObserver.h"
#include "../vh_uplog.h"
NS_VH_BEGIN

// 设置answer相关回调
SetAnswerObserver::SetAnswerObserver(std::shared_ptr<BaseStack> p) {
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

SetAnswerObserver::~SetAnswerObserver() {
  LOGI("~SetAnswerObserver");
}
void SetAnswerObserver::OnSuccess() {
  LOGI("Create Answer SDP");
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack || !ptrBaseStack->peer_connection_) {
    LOGE("BaseStack is reset!");
    return;
  }
  if (ptrBaseStack) {
    ptrBaseStack->PostTask(SETANSWERSUCCESS, nullptr);
  }
}
void SetAnswerObserver::OnSuccess_() {
  LOGI("%s", __FUNCTION__);
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack || !ptrBaseStack->peer_connection_) {
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
  msg.mCandidate.mSdpMid = "end";
  msg.mCandidate.mSdpMLineIndex = -1;
  msg.mStreamId = vhStream->mStreamId;
  msg.mCandidate.mCandidate = "end";
  /* cache candidate */
  ptrBaseStack->listIceCandidate.push_back(msg);
  /* uplog */
  if (vhStream->mLocal) {
    std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
    if (reportLog) {
      reportLog->upLog(sendLocalCandidate, ptrBaseStack->GetStreamID(), "candidate:" + msg.ToJsonStr());
    }
  }
  else {
    std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
    if (reportLog) {
      reportLog->upLog(sendRemoteCandidate, ptrBaseStack->GetStreamID(), "sendRemoteCandidate");
    }
  }
}
void SetAnswerObserver::OnFailure() {
  std::string msg = "Create Answer SDP Failure: " + mErrorMsg;
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  vhStream->OnPeerConnectionErr(msg);
}
void SetAnswerObserver::OnFailure(const std::string& error) {
  mErrorMsg = error;
  LOGE("%s", error.c_str());
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  ptrBaseStack->PostTask(SetAnswerFailed, nullptr);
}

NS_VH_END