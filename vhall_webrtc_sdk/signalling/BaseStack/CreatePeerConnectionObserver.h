#ifndef VH_CREATEPEERCONNECTIONOBSERVER_H
#define VH_CREATEPEERCONNECTIONOBSERVER_H

#include "common/vhall_define.h"

#include "api/peer_connection_interface.h"

NS_VH_BEGIN

class BaseStack;
class VHStream;

class CreatePeerConnectionObserver :
  public webrtc::PeerConnectionObserver {
public:
  CreatePeerConnectionObserver(std::shared_ptr<BaseStack> p);
  ~CreatePeerConnectionObserver();
  //
  // PeerConnectionObserver implementation.
  //
  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state);
  void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream);
  void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream);
  void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel);
  void OnRenegotiationNeeded();
  void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state);
  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state);
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
  void OnIceConnectionReceivingChange(bool receiving);

  /* async task */
  void OnIceCandidateComplete();
  void OnIceConnectionFailed();

  std::weak_ptr<BaseStack> mPtrBaseStack;
  std::weak_ptr<VHStream> mVhStream;
};

NS_VH_END

#endif /* ec_stream_h */