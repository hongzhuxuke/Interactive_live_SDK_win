#ifndef VH_SETLOCALDESCRIPTIONOBSERVER_H
#define VH_SETLOCALDESCRIPTIONOBSERVER_H

#include "common/vhall_define.h"

#include "api/peer_connection_interface.h"

NS_VH_BEGIN

class BaseStack;
class VHStream;

class SetLocalDescriptionObserver :
  public webrtc::SetSessionDescriptionObserver {
public:
  SetLocalDescriptionObserver(std::shared_ptr<BaseStack> p);
  ~SetLocalDescriptionObserver();
  //
  // SetSessionDescriptionObserver implementation.
  //
  void OnSuccess();
  void OnFailure(const std::string& error);

  // for sync
  void OnFailure();
  std::string mErrorMsg = "";
  std::weak_ptr<BaseStack> mPtrBaseStack;
  std::weak_ptr<VHStream> mVhStream;
};

NS_VH_END

#endif /* ec_stream_h */