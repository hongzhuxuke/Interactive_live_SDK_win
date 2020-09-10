#ifndef VH_CREATEOFFEROBSERVER_H
#define VH_CREATEOFFEROBSERVER_H

#include "common/vhall_define.h"

#include "api/peer_connection_interface.h"

NS_VH_BEGIN

class BaseStack;
class VHStream;

class CreateOfferObserver :
  public webrtc::CreateSessionDescriptionObserver {
public:
  CreateOfferObserver(std::shared_ptr<BaseStack> p);
  ~CreateOfferObserver();
  //
  // CreateSessionDescriptionObserver implementation.
  //
  void OnSuccess(webrtc::SessionDescriptionInterface* desc);
  void OnFailure(const std::string& error);

  /* for thread sync */
  void OnSuccess();
  void OnFailure();

  std::weak_ptr<BaseStack> mPtrBaseStack;
  std::weak_ptr<VHStream> mVhStream;
  std::unique_ptr<webrtc::SessionDescriptionInterface> m_session_description = nullptr;
private:
  webrtc::SessionDescriptionInterface* mDesc = nullptr;
};

NS_VH_END

#endif /* ec_stream_h */