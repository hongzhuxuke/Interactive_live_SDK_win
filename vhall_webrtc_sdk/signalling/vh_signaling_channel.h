//
//  ec_signaling_channel.hpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//

#ifndef vh_signaling_channel_h
#define vh_signaling_channel_h

#include <cstdio>
#include <memory>
#include "common/vhall_log.h"
#include "common/vhall_define.h"
#include "vh_events.h"
#include "signalling/transporter/socketio_transport.h"
#include "vh_data_message.h"
#include <atomic>

NS_VH_BEGIN

class VHRoom;
typedef std::function<void(const std::string& resp)> VHEventCallback;

class SocketEvent:public BaseEvent{
public:
   SocketEvent(const std::string& type, const std::string &args):mArgs(args){
      mType = type;
   };
   std::string mArgs;
};

class SignalEvent: public BaseEvent
{
public:
   SignalEvent()
   {
      mType.clear();
   }
   SignalEvent(std::string type, const std::string& event, const std::string& msg, const VHEventCallback & cb, uint64_t timeOut)
   {
      mType  = type;
      mEvent = event;
      mMsg   = msg;
      mCallback = cb;
      mTimeOut = timeOut;
   }
   std::string     mEvent = "";
   std::string     mMsg   = "";
   VHEventCallback mCallback = nullptr;
   uint64_t        mTimeOut = 0;
};

class VHSignalingChannel:public EventDispatcher,public SocketIOTransport::Deleagte,public std::enable_shared_from_this<VHSignalingChannel> {
   
public:
   VHSignalingChannel(bool enableReconnect);
   ~VHSignalingChannel();
   void Init(int attempts = 0);
   bool Connect(const std::string& token, const VHEventCallback &callback, const VHEventCallback &error = [](const std::string& resp)->void {});
   void Disconnect();
   
   // option json string PublishMsg->ToJsonStr()
   void Publish(const std::string &options, const VHEventCallback &callback, uint64_t timeOut = 0);
   void UnPublish(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   // option json string SubscribeMsg->ToJsonStr()
   void Subscribe(const std::string &options, const VHEventCallback &callback, uint64_t timeOut = 0);
   void UnSubscribe(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   void GetOversea(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   void StartRecording(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   void StopRecording(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   // option json string
   void SendDataStream(const std::string &options);
   // option json string
   void UpdateStreamAttributes(const std::string &options);
   void SetMaxUserCount(const int count, const VHEventCallback &callback, uint64_t timeOut = 0);
   // option json string ConfigRoomMsg->ToJsonStr()
   void ConfigRoomBroadCast(const std::string &options, const VHEventCallback &callback, uint64_t timeOut = 0);
   void SetMixLayoutMainScreen(const std::string &options, const VHEventCallback &callback, uint64_t timeOut = 0);
   void SetMixLayoutMode(const std::string &mode, const VHEventCallback &callback, uint64_t timeOut = 0);
   // option json string MixConfigMsg->ToJsonStr()
   void SetRoomMixConfig(const std::string &room_id, const std::string &options, const VHEventCallback &callback, uint64_t timeOut = 0);
   void StartRoomBroadCast(const std::string &room_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   void StopRoomBroadCast(const std::string &room_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   // option json string MuteStreamMsg->ToJsonStr()
   void SendMuteStream(const std::string &options, const VHEventCallback &callback, uint64_t timeOut = 0);
   void SetMaxUserCount(const std::string &maxUserCount, const VHEventCallback &callback, uint64_t timeOut = 0);
   
   void AddStreamToMix(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);
   void DelStreamFromMix(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut = 0);

   void SendMessage(const std::string &event, const std::string &msg,const VHEventCallback &callback, uint64_t timeOut = 0);

   int GetConnectState(); // VhallConnectState
   void SetConnectState(VhallConnectState state);
   void SetClientId(std::string clientId);
private:
   virtual void OnOpen()override;
   virtual void OnFail()override;
   virtual void OnReconnecting()override;
   virtual void OnReconnect(unsigned, unsigned)override;
   virtual void OnClose()override;
   virtual void OnSocketOpen(std::string const& nsp)override;
   virtual void OnSocketClose(std::string const& nsp)override;
   void OnEvent(const std::string& event_name, const std::string& msg);
   void Emit(const std::string& type, const std::string &args);
private:
   std::atomic_int mState;
   std::string mToken;
   std::shared_ptr<TokenMsg> mTokenMsg = nullptr;
   std::string mClientId;
   std::string mLocalIp = "";
   VHEventCallback mConnectCallback;
   VHEventCallback mConnectError;
   std::shared_ptr<SocketIOTransport> mSIOClient;
   bool mEnableReconnect = false;
public:
  std::shared_ptr<VHRoom> mRoom;
  struct RoomOptions spec;
};

NS_VH_END

#endif /* ec_signaling_channel_h */
