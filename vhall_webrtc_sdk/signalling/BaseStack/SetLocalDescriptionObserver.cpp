#include "BaseStack.h"
#include "SetLocalDescriptionObserver.h"

NS_VH_BEGIN

// 设置answer相关回调
SetLocalDescriptionObserver::SetLocalDescriptionObserver(std::shared_ptr<BaseStack> p) {
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

SetLocalDescriptionObserver::~SetLocalDescriptionObserver() {
  LOGI("~SetLocalDescriptionCallback");
}

void SetLocalDescriptionObserver::OnSuccess() {
  LOGI("Set Local SDP Success");
}

void SetLocalDescriptionObserver::OnFailure(const std::string& error) {
  mErrorMsg = error;
  LOGE("%s", error.c_str());
  std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
  if (!ptrBaseStack) {
    LOGE("BaseStack is reset!");
    return;
  }
  ptrBaseStack->PostTask(SetLocalDescriptionFailed, nullptr);
}

void SetLocalDescriptionObserver::OnFailure() {
  std::string msg = "Set Local SDP Failure: " + mErrorMsg;
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return;
  }
  vhStream->OnPeerConnectionErr(msg);
}

NS_VH_END