//
//  ec_signaling_channel.cpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright Â© 2017å¹´ ilong. All rights reserved.
//

#include "vh_room.h"
#include "vh_signaling_channel.h"
#include "common/vhall_utility.h"
#include "socketio/lib/rapidjson/include/rapidjson/rapidjson.h"
#include "socketio/lib/rapidjson/include/rapidjson/document.h"

NS_VH_BEGIN

#define RECONNECT_DELAT_TIME_MS 3000
#define RECONNECT_DELAT_MAX_TIME_MS 3000

VHSignalingChannel::VHSignalingChannel(bool enableReconnect) {
   mSIOClient = std::make_shared<SocketIOTransport>();
   if (mSIOClient==nullptr) {
      LOGE("mSIOClient is null");
   }
   mState = VhallDisconnected;
   mRoom = nullptr;
   mEnableReconnect = enableReconnect;
}

VHSignalingChannel::~VHSignalingChannel() {
  if (mSIOClient) {
    mSIOClient->OffAll();
    mSIOClient.reset();
  }
  LOGI("~VHSignalingChannel");
}

void VHSignalingChannel::Init(int attempts) {
   if (mSIOClient) {
      std::weak_ptr<SocketIOTransport::Deleagte> delegate(shared_from_this());
      mSIOClient->SetDelegate(delegate);
      mSIOClient->SetReconnectAttempts(attempts);
      mSIOClient->SetReconnectDelay(RECONNECT_DELAT_TIME_MS);
      mSIOClient->SetReconnectDelayMax(RECONNECT_DELAT_MAX_TIME_MS);
      mSIOClient->OffAll();
   }
}

bool VHSignalingChannel::Connect(const std::string& tokenStr, const VHEventCallback &callback, const VHEventCallback &error) {
  mConnectCallback = callback;
  mConnectError = error;
  std::string v = spec.version;
  mTokenMsg = std::make_shared<TokenMsg>(tokenStr, v, spec.platForm, spec.attribute, spec.cascade);
  if (!mTokenMsg || mTokenMsg->mUrl.empty()) {
    LOGE("uri is empty!");
    return false;
  }
  mToken = mTokenMsg->mToken;
  if (mSIOClient) {
    mSIOClient->On(SIGNALING_MESSAGE_VHALL, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(SIGNALING_MESSAGE, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(PUBLISH_ME, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(UNPUBLISH_ME, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(ON_BANDWIDTH_ALERT, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(ON_UPDATE_ATTRIBUTESTREAM, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(ON_REMOVE_STREAM, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(ON_ADD_STREAM, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(DISCONNECT, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(CONNECTION_FAILED, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(VHERROR, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(BROADCAST_MUTE_STREAM, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
    mSIOClient->On(ON_STREAM_MIXED, VH_CALLBACK_2(VHSignalingChannel::OnEvent, this));
  }
  int ret = -1;
  if (mSIOClient) {
    mState = VhallConnecting;
    ret = mSIOClient->Connect(mTokenMsg->mUrl);
    LOGD("VHSignalingChannel::Connect %s", mTokenMsg->mUrl.c_str());
  }
  return ret==0?true:false;
}

void VHSignalingChannel::Disconnect() {
  LOGD("VHSignalingChannel::Disconnect");
  if (mSIOClient && mState != VhallDisconnected) {
    mState = VhallDisconnected;
    mSIOClient->Sync_Disconnect();
  }
}

void VHSignalingChannel::Publish(const std::string &options, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(PUBLISH, options, callback, timeOut);
}

void VHSignalingChannel::UnPublish(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(UNPUBLISH, BaseEvent::ToString(stream_id), callback, timeOut);
}

void VHSignalingChannel::Subscribe(const std::string &options, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(SUBSCRIBE, options, callback, timeOut);
}

void VHSignalingChannel::UnSubscribe(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(UNSUBSCRIBE, BaseEvent::ToString(stream_id), callback, timeOut);
}

void VHSignalingChannel::GetOversea(const std::string & stream_id, const VHEventCallback & callback, uint64_t timeOut) {
  SendMessage(GETOVERSEA, BaseEvent::ToString(stream_id), callback, timeOut);
}

void VHSignalingChannel::StartRecording(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut) {
   char options[32] = {0};
   sprintf(options,"{\"to\":%s}",BaseEvent::ToString(stream_id).c_str());
   SendMessage(START_RECORDER, options, callback, timeOut);
}

void VHSignalingChannel::StopRecording(const std::string& stream_id, const VHEventCallback &callback, uint64_t timeOut) {
   char options[32] = {0};
   sprintf(options,"{\"id\":%s}",BaseEvent::ToString(stream_id).c_str());
   SendMessage(STOP_RECORDER, options, callback, timeOut);
}

void VHSignalingChannel::SendDataStream(const std::string &options) {
  SendMessage(SEND_DATA_STREAM, options, nullptr);
}

void VHSignalingChannel::UpdateStreamAttributes(const std::string &options) {
  SendMessage(UPDATE_STREAM_ATTRIBUTES, options, nullptr);
}

void VHSignalingChannel::SetMaxUserCount(const int count, const VHEventCallback &callback, uint64_t timeOut) {
  SendMessage(SET_MIX_LAYOUT_MAIN_SCREEN, BaseEvent::ToString(count), callback, timeOut);
}

void VHSignalingChannel::ConfigRoomBroadCast(const std::string &options, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(CONFIG_ROOM_BROAD_CAST, options, callback, timeOut);
}

void VHSignalingChannel::SetMixLayoutMainScreen(const std::string &options, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(SET_MIX_LAYOUT_MAIN_SCREEN, options, callback, timeOut);
}

void VHSignalingChannel::SetMixLayoutMode(const std::string &mode, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(SET_MIX_LAYOUT_MODE, mode, callback, timeOut);
}

// option json string
void VHSignalingChannel::SetRoomMixConfig(const std::string &room_id, const std::string &options, const VHEventCallback &callback, uint64_t timeOut) {
  char config[1024] = { 0 };
  sprintf(config, "{\"roomID\":\"%s\",\"config\":\"%s\"}", room_id.c_str(), options.c_str());
   SendMessage(SET_ROOM_MIX_CONFIG, config, callback, timeOut);
}

void VHSignalingChannel::StartRoomBroadCast(const std::string &room_id, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(START_ROOM_BROAD_CAST, room_id, callback, timeOut);
}

void VHSignalingChannel::StopRoomBroadCast(const std::string &room_id, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(STOP_ROOM_BROAD_CAST, room_id, callback, timeOut);
}

void VHSignalingChannel::SendMuteStream(const std::string &options, const VHEventCallback &callback, uint64_t timeOut) {
   SendMessage(SEND_MUTE_STREAM, options, callback, timeOut);
}

void VHSignalingChannel::SetMaxUserCount(const std::string & maxUserCount, const VHEventCallback & callback, uint64_t timeOut) {
  SendMessage(SET_MAX_USER_COUNT, maxUserCount, callback, timeOut);
}

void VHSignalingChannel::AddStreamToMix(const std::string & stream_id, const VHEventCallback & callback, uint64_t timeOut) {
  SendMessage(ADD_STREAM_TO_MIX, BaseEvent::ToString(stream_id), callback, timeOut);
}

void VHSignalingChannel::DelStreamFromMix(const std::string & stream_id, const VHEventCallback & callback, uint64_t timeOut) {
  SendMessage(DEL_STREAM_FROM_MIX, BaseEvent::ToString(stream_id), callback, timeOut);
}

void VHSignalingChannel::SendMessage(const std::string &event, const std::string &msg,const VHEventCallback &callback, uint64_t timeOut) {
  LOGD("msg:%s", msg.c_str());
  if (mState == VhallDisconnected && event != "token") {
    if (callback) {
      RespMsg respMsg;
      respMsg.mResult = "failed";
      respMsg.mCode = SocketError;
      respMsg.mMsg = "network not connect";
      callback(respMsg.ToJsonStr());
    }
    LOGE("Trying to send a message over a disconnected Socket");
    return;
  }
  if (mState == VhallConnecting) {
    if (callback) {
      RespMsg respMsg;
      respMsg.mResult = "failed";
      respMsg.mCode = SocketError;
      respMsg.mMsg = "socket Not connected";
      callback(respMsg.ToJsonStr());
    }
    LOGW("socket.io connecting...");
    return;
  }
  else if (mState == VhallReconnecting) {
    /* reconnect not available */
    RespMsg respMsg;
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "network disconnected";
    callback(respMsg.ToJsonStr());
    LOGE("Trying to send a message over a disconnected Socket");
    return;
  }
  if (mSIOClient) {
    LOGI("SendMessage: %s, net State: %d", msg.c_str(), int(mState));
    mSIOClient->SendMessage(event, msg, [callback](const std::string &event, const std::string &msg)->void {
      if (callback) {
        callback(msg);
      }
    }, timeOut);
  }
}

void VHSignalingChannel::OnOpen() {
  if (VhallReconnecting == mState || VhallConnecting == mState || !mEnableReconnect) {
    LOGD("VHSignalingChannel::OnOpen");
    mState = VhallConnected;
    SendMessage(TOKEN, mToken, [&](const std::string&msg) {
      LOGD("token resp msg:%s", msg.c_str());
      if (mConnectCallback) {
        mConnectCallback(msg);
      }
    });
    std::shared_ptr<GetLocalIp> local_ip = std::make_shared<GetLocalIp>();
    mLocalIp = local_ip->GetLocalAddr();
    local_ip.reset();
  }
};

int VHSignalingChannel::GetConnectState() {

   return mState;
}

void VHSignalingChannel::SetConnectState(VhallConnectState state) {
   mState = state;
}
void VHSignalingChannel::SetClientId(std::string clientId) {
   mClientId = clientId;
   LOGE("set mClientId: %s", mClientId.c_str());
};
void VHSignalingChannel::OnFail() {
  LOGD("socket.io fail");
  std::string msg = "Connect failed";
  mConnectError(msg);
};

void VHSignalingChannel::OnReconnecting() {
  OnEvent(ROOM_CONNECTING, "room-connecting");
  if (!mEnableReconnect && mRoom && mRoom.get()) {
    mRoom->ClearRemoteStreams();
    mRoom->ClearLocalStream();
  }
  LOGD("socket.io reconnecting");
};

void VHSignalingChannel::OnReconnect(unsigned, unsigned) {
  if (VhallConnecting == mState) {
    OnEvent(ROOM_CONNECTING, "room-connecting");
    LOGD("socket.io connecting");
  }
  else if (VhallReconnecting != mState) {
    mState = VhallReconnecting;
    OnEvent(ROOM_RECONNECTING, "room-reconnecting");
  }
  LOGD("socket.io reconnecting");
};

void VHSignalingChannel::OnClose() {
  if (mState != VhallDisconnected) {
    mState = VhallDisconnected;
    OnEvent(DISCONNECT, "OnClose");
  }
  LOGD("socket.io close");
};

void VHSignalingChannel::OnSocketOpen(std::string const& nsp) {
   LOGD("socket.io socket open");
};

void VHSignalingChannel::OnSocketClose(std::string const& nsp) {
   LOGD("socket.io socket close");
   mState = VhallDisconnected;
   OnEvent(DISCONNECT, "OnClose");
};

void VHSignalingChannel::OnEvent(const std::string& event_name, const std::string& msg) {
   LOGD("event:%s msg:%s",event_name.c_str(),msg.c_str());
   Emit(event_name, msg);
}

void VHSignalingChannel::Emit(const std::string& type, const std::string &args) {
   SocketEvent event(type,args);
   DispatchEvent(event);
}

NS_VH_END
