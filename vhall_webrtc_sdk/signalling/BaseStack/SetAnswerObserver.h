#ifndef VH_SETANSWEROBSERVER_H
#define VH_SETANSWEROBSERVER_H

#include "common/vhall_define.h"

#include "api/peer_connection_interface.h"

NS_VH_BEGIN

class BaseStack;
class VHStream;

class SetAnswerObserver :
  public webrtc::SetSessionDescriptionObserver {
public:
  SetAnswerObserver(std::shared_ptr<BaseStack> p);
  ~SetAnswerObserver();
  //
  // SetSessionDescriptionObserver implementation.
  //
  void OnSuccess();
  void OnFailure(const std::string& error);

  /* thread sync */
  void OnSuccess_();
  void OnFailure();

  std::weak_ptr<BaseStack> mPtrBaseStack;
  std::weak_ptr<VHStream> mVhStream;

private:
  std::string mErrorMsg = "";
};

NS_VH_END

#endif /* ec_stream_h */