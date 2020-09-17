//
//  ec_room.cpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//
#include <sstream>

#include "vh_room.h"
#include "vh_stream.h"
#include "vh_signaling_channel.h"
#include "vh_uplog.h"
#include "common/message_type.h"
#include "json/json.h"
#include "common/vhall_log.h"
#include "common/vhall_utility.h"
#include "rtc_base/thread.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "tool/platformInfo.h"
// fix webrtc bug
#include "rtc_base/synchronization/rw_lock_win.h"

const char vhall::VHRoom::vhallSDKVersion[] = "2.4.6.1";
const char vhall::VHRoom::vhallSDKReleaseData[] = "20200813";

typedef enum {
  MSG_ROOM_CONNECT,
  MSG_ROOM_DISCONNECT,
  MSG_GET_OVERSEA,
  MSG_ROOM_CONNECT_RESP,
  MSG_PUBLISH_STREAM,
  MSG_UNPUBLISH_STREAM,
  MSG_SUBSCRIBE_STREAM,
  MSG_UNSUBSCRIBE_STREAM,
  MSG_SETSIMULCAST,
  MSG_RESUME_LOCALSTREAMS,
  MSG_CLEAN_REMOTESTREAMS,
  MSG_MUTE_STREAM,
  MSG_REMOVE_STREAM,
  MSG_SINGNALINGMESSAGE,
  MSG_SETMAXUSRCOUNT,
  MSG_AddStreamToMix,
  MSG_DelStreamFromMix,

  MSG_CONFIG_ROOM_BROADCAST,
  MSG_SET_MIX_LAYOUT_MODE,
  MSG_SET_LAYOUT_MAIN_SCREEN,
  MSG_START_ROOM_BROAD_CAST,
  MSG_STOP_ROOM_BROAD_CAST,

  MSG_VHALL_MESSAGE,

  MSG_NETWORK_CHANGED,
  MSG_STREAM_FAILED,
  MSG_CLEAR_ALL,
} ROOM_MSG;

namespace rtc {
  class DeviceInfo : public rtc::MessageData {
  public:
    DeviceInfo(std::shared_ptr<vhall::VHRoom> room, std::string streamId, uint32_t index) {
      mRoom = room;
      mStreamId = streamId;
      mIndex = index;
    };
    virtual ~DeviceInfo() {};
  public:
    std::shared_ptr<vhall::VHRoom> mRoom;
    std::string mStreamId;
    uint32_t mIndex;
  };

  class StreamMsgData : public rtc::MessageData {
  public:
    StreamMsgData(std::shared_ptr<vhall::VHRoom> room) {
      mRoom = room;
      mStream = nullptr;
      mCallback = [](const std::string& resp)->void {};
      mTimeOut = 0;
      mConfigStr = 0;
      mStreamID = "";
      mDual = 0;
    }
    StreamMsgData(std::shared_ptr<vhall::VHRoom> room, std::shared_ptr<vhall::VHStream> stream, vhall::VHEventCallback callback = [](const std::string& resp)->void {}, uint64_t timeOut = 0) : mConfigStr(0), mStreamID("") {
      mRoom = room;
      mStream = stream;
      mCallback = callback;
      mTimeOut = timeOut;
    }
    StreamMsgData(std::shared_ptr<vhall::VHRoom> room, std::shared_ptr<vhall::VHStream> stream, const vhall::VHEventCB &callback = [](const vhall::RespMsg resp)->void {}, uint64_t timeOut = 0) : mConfigStr(0), mStreamID("") {
      mRoom = room;
      mStream = stream;
      mCb = callback;
      mTimeOut = timeOut;
    }
    StreamMsgData(std::shared_ptr<vhall::VHRoom> room, std::shared_ptr<vhall::VHStream> stream, const vhall::VHPublishEventCB &callback = [](const vhall::RespMsg resp)->void {}, uint64_t timeOut = 0) : mConfigStr(0), mStreamID("") {
      mRoom = room;
      mStream = stream;
      mPublishCb = callback;
      mTimeOut = timeOut;
    }
    virtual ~StreamMsgData() {};
  public:
    std::shared_ptr<vhall::VHRoom>   mRoom;
    /* for stream */
    std::shared_ptr<vhall::VHStream> mStream;
    vhall::VHEventCallback           mCallback;
    vhall::VHEventCB                 mCb;
    vhall::VHPublishEventCB          mPublishCb;
    uint64_t                         mTimeOut;
    int                              mDual;
    /* BroadCast */
    vhall::RoomBroadCastOpt   mOptions;
    UINT32                    mConfigStr;
    std::string               mStreamID;
    /* network status */
    vhall::BaseEvent          mEvent;
    /* event from server */
    std::shared_ptr<vhall::SocketEvent> mSocketEvent = nullptr;

    std::string               streamId = "";
    std::string               mMessage = "";
    std::string               mReason;
    int                       mMaxUsrCount = 5;

    /* mute event */
    bool mMuteAudio = false;
    bool mMuteVideo = false;
    /* 订阅选项 */
    vhall::SubscribeOption* mSubOption = nullptr;
  };
  class MsgHandler : public rtc::MessageHandler {
  public:
    MsgHandler() {
      mRoom.reset();
    };
    virtual ~MsgHandler() {};
    virtual void OnMessage(rtc::Message* msg) {
      DeviceInfo* devInfo = nullptr;
      StreamMsgData* msgData = (dynamic_cast<StreamMsgData*>(msg->pdata));
      devInfo = (dynamic_cast<DeviceInfo*>(msg->pdata));
      if (!msgData && !devInfo) {
        LOGE("msg->pdata == nullptr");
        return;
      }
      if (devInfo) {
        mRoom = devInfo->mRoom;
      }
      if (msgData) {
        mRoom = msgData->mRoom;
      }
      LOGI("taskId:%d", msg->message_id);
      switch (msg->message_id) {
      case MSG_ROOM_CONNECT:
        mRoom->OnConnect();
        break;
      case MSG_ROOM_DISCONNECT:
        mRoom->OnDisconnect();
        break;
      case MSG_GET_OVERSEA:
        mRoom->OnGetOversea(msgData->mStream, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_ROOM_CONNECT_RESP:
        mRoom->OnConnectedResp(msgData->mMessage);
        break;
      case MSG_PUBLISH_STREAM:
        mRoom->OnPublish(msgData->mStream, msgData->mPublishCb, msgData->mTimeOut);
        msgData->mStream = nullptr;
        break;
      case MSG_UNPUBLISH_STREAM:
        mRoom->OnUnPublish(msgData->mStream, msgData->mCb, msgData->mTimeOut);
        msgData->mStream = nullptr;
        break;
      case MSG_SINGNALINGMESSAGE:
        mRoom->OnSendSignalingMessage(msgData->mMessage, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_SETMAXUSRCOUNT:
        mRoom->OnSetMaxUsrCount(msgData->mMaxUsrCount, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_REMOVE_STREAM:
        mRoom->RemoveStream(msgData->mStream);
        break;
      case MSG_SUBSCRIBE_STREAM:
        mRoom->OnSubscribe(msgData->mStream, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_UNSUBSCRIBE_STREAM:
        mRoom->OnUnSubscribe(msgData->mStream, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_SETSIMULCAST:
        mRoom->OnSetSimulCast(msgData->mStream, msgData->mDual, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_MUTE_STREAM:
        mRoom->OnMuteStream(msgData->mStream, msgData->mMuteAudio, msgData->mMuteVideo, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_RESUME_LOCALSTREAMS:
        mRoom->OnClearLocalStream();
        break;
      case MSG_CLEAN_REMOTESTREAMS:
        mRoom->OnClearRemoteStreams();
        break;
      case MSG_AddStreamToMix:
        mRoom->OnAddStreamToMix(msgData->streamId, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_DelStreamFromMix:
        mRoom->OnDelStreamFromMix(msgData->streamId, msgData->mCb, msgData->mTimeOut);
        break;

      case MSG_CONFIG_ROOM_BROADCAST:
        msgData->mOptions.publishUrl;
        mRoom->OnconfigRoomBroadCast(msgData->mOptions, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_SET_MIX_LAYOUT_MODE:
        mRoom->OnsetMixLayoutMode(msgData->mConfigStr, msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_SET_LAYOUT_MAIN_SCREEN:
        mRoom->OnsetMixLayoutMainScreen(msgData->mStreamID.c_str(), msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_START_ROOM_BROAD_CAST:
        mRoom->OnstartRoomBroadCast(msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_STOP_ROOM_BROAD_CAST:
        mRoom->OnstopRoomBroadCast(msgData->mCb, msgData->mTimeOut);
        break;
      case MSG_VHALL_MESSAGE:
        if (msgData->mSocketEvent) {
          mRoom->OnSocketOnVhallMessage(*(msgData->mSocketEvent));
        }
        break;
      case MSG_NETWORK_CHANGED:
        mRoom->OnSocketOnNetworkChanged(msgData->mEvent);
        break;
      case MSG_STREAM_FAILED:
        mRoom->OnStreamFailed(msgData->mStream, msgData->mMessage, msgData->mReason);
        break;
      case MSG_CLEAR_ALL:
        mRoom->OnClearAll();
        break;
      default:
        break;
      }
      if (msgData) {
        delete msgData;
        msgData = nullptr;
      }
      if (devInfo) {
        delete devInfo;
        devInfo = nullptr;
      }
      LOGI("done");
    }
  public:
    std::shared_ptr<vhall::VHRoom> mRoom;
  };

}

NS_VH_BEGIN

std::string trimBracket(const std::string msg) {
  if (msg.substr(0, 1).compare("[") != 0 || msg.substr(msg.length() - 1, msg.length()).compare("]") != 0) {
    return msg;
  }

  return msg.substr(1, msg.length() - 2);
}

VHRoom::VHRoom(struct RoomOptions &RoomOpt):
mP2p(false), mRoomId(""), mMsgHandler(nullptr)
{
   InitLog();
   LOGI("Create VHRoom: %s", RoomOpt.token.c_str());
   mRemoteStreams.clear();
   mLocalStreams.clear();
   mStreamList.clear();
   spec = RoomOpt;
   reportLog = NULL;
   mMsgHandler = nullptr;
   mWorkThread.reset();
   mStopFlag = true;
   // webrtc bug
   CreateRWLockWin();
}

VHRoom::~VHRoom() {
  LOGI("~VHRoom");
  if (mState != VhallDisconnected) {
    Disconnect();
  }
  mSocket.reset();
  if (mWorkThread.get()) {
    mWorkThread->Clear(mMsgHandler.get());
    mWorkThread->Stop();
    mWorkThread.reset();
  }
}

void VHRoom::OnConnectedResp(std::string& msg) {
  TokenRespMsg response(msg);
  LOGI("Room Connection response message %s", msg.c_str());
  if (response.mRespType == "success") {
    if (reportLog) {
      /* upLog SDK version Info */
      reportLog->upLog("182601", "0", "sdk version info");
    }
    mState = VhallConnected;
    mP2p = response.mP2p;
    mRoomId = response.mId;
    mSocket->SetClientId(response.mClientId);
    mDefaultVideoBW = response.mDefaultVideoBW;
    mMaxVideoBW = response.mMaxVideoBW;
    mIceServers = response.mIceServers;
    mUser = response.mUser;
    /* clear output stream list */
    mStreamList.clear();
    for (auto &stream : response.mStreams) {
      std::shared_ptr<VHStream> vhStream = std::make_shared<VHStream>(*stream.get());
      if (!vhStream) {
        continue;
      }
      vhStream->mLocal = false;
      vhStream->mRoom = shared_from_this();
      mStreamList.push_back(vhStream);
      std::unique_lock<std::mutex> lock(mRemoteLock);
      mRemoteStreams[stream->mId] = vhStream;
      lock.unlock();
    }
    if (reportLog) {
      /* 房间连接成功 */
      reportLog->SetClientID(response.mClientId); 
      reportLog->UpLogin(connectSignalling, "room connected successfully");
    }
    /* clear task */
    mWorkThread->Clear(mMsgHandler.get());
    RoomEvent event;
    event.mType = ROOM_CONNECTED;
    event.mStreams = mStreamList;
    this->DispatchEvent(event);
    mStreamList.clear();
    /* auto subscribe remote streams, post ROOM_CONNECTED first */
    if (spec.autoSubscribe) {
      autoSubscrbieAll();
    }
  }
  else {
    LOGE("Not Connected! Error:%s", response.mResp.c_str());
    if (reportLog) {
      reportLog->upLog(signallingConnectFailed, "0", std::string("reason: connected but join romm failed, token:") + mSocket->spec.token + ", response" + response.mRespType + ", errCode: " + Utility::ToString(response
        .mCode));
    }
    RoomEvent event;
    event.mErrorType = RoomEvent::ServerRefused;
    event.mCode = response.mCode;
    event.mType = ROOM_ERROR;
    event.mMessage = response.mResp;
    this->DispatchEvent(event);
  }
}

void VHRoom::ConnectedResp(std::string msg) {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mMessage = msg;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_ROOM_CONNECT_RESP, stmMsgData);
  }
  else {
    LOGE("MSG_ROOM_CONNECT_RESP error");
  }
}

void VHRoom::OnConnect() {
  LOGI("VHRoom Connection, version:%s", spec.version.c_str());
  if (mSocket) {
    mSocket->RemoveAllEventListener();
    mSocket->Disconnect();
    mSocket.reset();
    LOGI("VHRoom reset connect!");
  }

  // init uplog param
  std::string v = spec.version;
  TokenMsg token(spec.token, v, spec.platForm, spec.attribute, spec.cascade);
  struct upLogOptions opts;
  opts.uid = token.mUserId;
  opts.aid = token.mRoomId;
  opts.url = spec.upLogUrl;
  opts.biz_role = spec.biz_role;
  opts.biz_id = spec.biz_id != "" ? spec.biz_id : token.mRoomId;
  opts.biz_des01 = spec.biz_des01;
  opts.biz_des02 = spec.biz_des02;
  opts.bu = spec.bu;
  opts.vid = spec.vid;
  opts.vfid = spec.vfid;
  opts.app_id = spec.app_id;
  opts.sd = token.host;
  opts.version = spec.version;
  opts.release_date = spec.release_date;
  opts.device_type = "windows pc";
  DWORD majorVer = 0;
  DWORD minorVer = 0;
  DWORD buildNum = 0;
  if (PlatForm::GetNtVersionNumbers(majorVer, minorVer, buildNum)) {
    std::string version = "windows " + Utility::ToString<DWORD>(majorVer) + "." + Utility::ToString<DWORD>(minorVer) + "." + Utility::ToString<DWORD>(buildNum);
    opts.osv = version;
  }
  if (reportLog) {
    reportLog.reset();
  }
  reportLog = std::make_shared<VHLogReport>(opts);
  if (reportLog) {
    reportLog->start();
  }
  VHStream::SetReport(reportLog);

  mSocket = std::make_shared<VHSignalingChannel>(spec.enableRoomReconnect);
  if (mSocket) {
    mSocket->Init(spec.attempts);
    mSocket->AddEventListener(SIGNALING_MESSAGE_VHALL, VH_CALLBACK_1(VHRoom::SocketOnVhallMessage, this));
    mSocket->AddEventListener(ON_REMOVE_STREAM, VH_CALLBACK_1(VHRoom::SocketOnRemoveStream, this));
    mSocket->AddEventListener(ON_ADD_STREAM, VH_CALLBACK_1(VHRoom::SocketOnAddStream, this));
    mSocket->AddEventListener(DISCONNECT, VH_CALLBACK_1(VHRoom::SocketOnDisconnect, this));
    mSocket->AddEventListener(ROOM_RECONNECTED, VH_CALLBACK_1(VHRoom::SocketOnReconnected, this));
    mSocket->AddEventListener(CONNECTION_FAILED, VH_CALLBACK_1(VHRoom::SocketOnICEConnectionFailed, this));
    mSocket->AddEventListener(VHERROR, VH_CALLBACK_1(VHRoom::SocketOnError, this));
    mSocket->AddEventListener(ROOM_NETWORKCHANGED, VH_CALLBACK_1(VHRoom::SocketOnNetworkChanged, this));
    mSocket->AddEventListener(ROOM_RECONNECTING, VH_CALLBACK_1(VHRoom::SocketReconnecting, this));
    mSocket->AddEventListener(ROOM_CONNECTING, VH_CALLBACK_1(VHRoom::SocketConnecting, this));
    mSocket->AddEventListener(ROOM_RECOVER, VH_CALLBACK_1(VHRoom::SocketRecover, this));
    mSocket->AddEventListener(ON_STREAM_MIXED, VH_CALLBACK_1(VHRoom::SocketOnStreamMixed, this));
  }
  else {
    if (reportLog) {
      reportLog->upLog(signallingConnectFailed, "0", std::string("reason: InitSocketFailed"));
    }
    RoomEvent event;
    event.mErrorType = RoomEvent::InitSocketFailed;
    event.mType = ROOM_ERROR;
    this->DispatchEvent(event);
    LOGE("Create web socket failure!");
    return;
  }

  mSocket->mRoom = shared_from_this();
  mSocket->spec = spec;
  bool autoSubscribe = spec.autoSubscribe;
  bool ret = mSocket->Connect(spec.token, [&, autoSubscribe](const std::string& msg)->void {
    /* clear local streams when reconnect */
    ClearLocalStream();
    /* clear remote stream-map && stream list */
    ClearRemoteStreams();
    ConnectedResp(msg);
  }, [&](const std::string& msg)->void {
    LOGE("Not Connected! Error:%s, token: %s", msg.c_str(), spec.token.c_str());
    if (reportLog) {
      reportLog->upLog(signallingConnectFailed, "0", std::string("reason: Connect failed: token") + spec.token);
    }
    RoomEvent event;
    event.mErrorType = RoomEvent::ConnectFailed;
    event.mType = ROOM_ERROR;
    this->DispatchEvent(event);
  });

  if (!ret) {
    LOGE("Not Connected! Error: init failed");
    if (reportLog) {
      reportLog->upLog(signallingConnectFailed, "0", std::string("reason: init Connected failed, ") + "token:" + mSocket->spec.token);
    }
    RoomEvent event;
    event.mErrorType = RoomEvent::ConnectFailed;
    event.mType = ROOM_ERROR;
    this->DispatchEvent(event);
  }
}

void VHRoom::OnDisconnect() {
  LOGD("Disconnection requested");
  if (mSocket) {
    mSocket->RemoveAllEventListener();
    mSocket->Disconnect();
    mSocket.reset();
  }
  OnClearAll();
}

void VHRoom::OnGetOversea(std::shared_ptr<VHStream> &stream, const VHEventCB & callback, uint64_t timeOut) {
  OverseaResp respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket not set";
    callback(respMsg);
    LOGE("Stream Publish failure, socket is empty!");
    return;
  }
  if (!stream.get()) {
    respMsg.mResult = "failed";
    respMsg.mCode = StreamIsEmpty;
    respMsg.mMsg = "stream is empty or not initialized";
    callback(respMsg);
    return;
  }
  GetOverseaMsg msg;
  msg.mStreamId = stream->mId;
  msg.mIsSub = stream->mLocal ? false : true;
  mSocket->GetOversea(msg.ToJsonStr(), [&, stream, callback](const std::string& response)->void {
    OverseaResp respMsg;
    respMsg.ToObject(response);
    stream->mIsOversea = respMsg.mIsOversea;
    callback(respMsg);
  }, timeOut);
  LOGI("done");
}

void VHRoom::OnPublish(std::shared_ptr<VHStream> &stream, const VHPublishEventCB &callback, uint64_t timeOut) {
  LOGI("Publish Stream");

  std::stringstream stream_uplog_msg;
  stream_uplog_msg << "user:";
  stream_uplog_msg << stream->mUser.mId;
  stream_uplog_msg << ",";
  stream_uplog_msg << "option: width " << stream->videoOpt.maxWidth << ",height " << stream->videoOpt.maxHeight << ", fps " << stream->videoOpt.maxFrameRate;
  std::string str_uplog_msg = stream_uplog_msg.str();
  if (reportLog) {
    reportLog->upLog(invokePublish, stream->mStreamId, str_uplog_msg);
  }
  PublishRespMsg respMsg;
  if (!mSocket) {
    if (reportLog) {
      reportLog->upLog(invokePublishFailed, stream->mStreamId, str_uplog_msg + ", reason: mSocket is null");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket not set";
    callback(respMsg);
    LOGE("Stream Publish failure, socket is empty!");
    return;
  }
  if (!stream.get()) {
    if (reportLog) {
      reportLog->upLog(LocalStreamNotExists, "0", str_uplog_msg + "reason: stream is null");
    }
  }
  if (!stream.get() || !stream->mLocal || stream->GetStreamState() == VhallStreamState::Uninitialized) {
    respMsg.mResult = "failed";
    respMsg.mCode = StreamIsEmpty;
    respMsg.mMsg = "stream is empty or not initialized";
    callback(respMsg);
    return;
  }
  if (stream->GetStreamState() == VhallStreamState::Publish) {
    respMsg.mResult = "failed";
    respMsg.mCode = StreamPublished;
    respMsg.mMsg = "stream is empty or not initialized";
    callback(respMsg);
    return;
  }
  stream->SetStreamState(VhallStreamState::Publish);
  PublishMsg options;
  options.mState = "erizo";
  options.mData = stream->HasData();
  options.mAudio = stream->HasAudio();
  options.mVideo = stream->HasVideo();
  options.mScreen = stream->mScreen;
  options.mCreateOffer = false;
  options.mStreamAttributes = stream->mStreamAttributes;
  options.mMetaData.mType = "publisher";
  options.mMinVideoBW = stream->videoOpt.minVideoBW;
  options.mMaxVideoBW = stream->videoOpt.maxVideoBW;
  options.mSpatialLayers = stream->mNumSpatialLayers;
  options.mStreamType = stream->mStreamType;
  options.mMuteStreams.mAudio = stream->IsAudioMuted();
  options.mMuteStreams.mVideo = stream->IsVideoMuted();
  options.mVideoCodecPreference = stream->mVideoCodecPreference;
  options.mMix = stream->mMix;
  options.videoOpt = stream->videoOpt;
  stream->mUser = this->mUser;
  stream->mRoom = shared_from_this();
  LOGI("Publish options: %s", options.ToJsonStr().c_str());
  if (reportLog) {
    reportLog->upLog(sendPublishSignalling, stream->mStreamId, str_uplog_msg + ",reason:default");
  }
  mSocket->Publish(options.ToJsonStr(), [&, stream, callback, str_uplog_msg](const std::string& response)->void {
    PublishRespMsg resp;
    resp.ToObject(response);
    if (!resp.mResult.compare("success")) {
      if (reportLog) {
        reportLog->upLog(publishSuccess, stream->mStreamId, response);
      }
      stream->SetStreamState(VhallStreamState::Publish);
      stream->InitPeer();
      LOGI("Publish response %s", response.c_str());
      // 设置流id
      stream->SetID(resp.mStreamId);
      // 放入本地流列表
      std::unique_lock<std::mutex> lock(mLocalLock);
      mLocalStreams[stream->mId] = stream;
      lock.unlock();
    }
    else {
      stream->SetStreamState(VhallStreamState::UnPublish);
      if (reportLog) {
        reportLog->upLog(publishFailed, stream->mStreamId, str_uplog_msg + std::string(" response.resualt:") + resp.mResult + ",errorCode:" + Utility::ToString(resp.mCode));
      }
    }
    callback(resp);
  }, timeOut);
}

void VHRoom::OnUnPublish(std::shared_ptr<VHStream> &stream, const VHEventCB & callback, uint64_t timeOut, std::string reason) {
  LOGI("UnPublish Stream");
  if (reportLog) {
    reportLog->upLog(invokeUnpublish, stream->mStreamId, std::string("reason: ") + reason);
  }
  RespMsg respMsg;
  if (stream.get() && reportLog) {
    reportLog->stopPublishMessageLog(stream);
  }
  if (!stream.get()) {
    if (reportLog) {
      reportLog->upLog(LocalStreamNotExists, "0", "reason: LocalStreamNotExists");
    }
  }
  if (!stream.get() || !stream->mLocal) {
    if (reportLog) {
      reportLog->upLog(invokeUnpublishFailed, stream->mStreamId, "reason: stream is null or islocal");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = StreamIsEmpty;
    respMsg.mMsg = "stream is empty or not local";
    callback(respMsg);
    LOGE("Cannot unpublish, stream does not exist or is not local");
    return;
  }
  LOGI("UnPublish Stream, id: %s", stream->GetID());
  stream->SetStreamState(VhallStreamState::UnPublish); 
  if (mSocket) {
    UnpublishMsg msg(stream->mId);
    msg.mReason = reason;
    if(reportLog) {
      reportLog->upLog(sendUnpublishSignalling, stream->mStreamId, "reason: default");
    }
    std::string streamID = stream->mStreamId;
    mSocket->UnPublish(msg.ToJsonStr(), [&, streamID, callback](const std::string &response)->void {
      LOGI("unpublish response %s", response.c_str());
      RespMsg resp;
      resp.ToObject(response);
      LOGI("ToObject done");
      if (!resp.mResult.compare("success")) {
        /* success */
        LOGI("Line:%d", __LINE__);
        if (reportLog) {
          LOGI("Line:%d", __LINE__);
          reportLog->upLog(unpublishSuccess, streamID, "reason: default");
        }
      }
      else {
        if (reportLog) {
          reportLog->upLog(unpublishFailed, streamID, std::string("signalling response:") + response);
        }
      }
      LOGI("unpublish cb %s", response.c_str());
      callback(resp);
    }, timeOut);
  } else {
    if (reportLog) {
      reportLog->upLog(invokeUnpublishFailed, stream->mStreamId, "reason: mSocket is null");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("Stream UnPublish failure, socket is empty!");
  }

  stream->DeInitPeer();
  std::unique_lock<std::mutex> lock(mLocalLock);
  if (mLocalStreams[stream->mId]) {
    mLocalStreams.erase(stream->mId);
  }
  lock.unlock();
  stream->mRoom.reset();
  stream->mId = -1;
  stream->mStreamId = "";
}

void VHRoom::OnSendSignalingMessage(const std::string & msg, const VHEventCB & callback, uint64_t timeOut) {
  mSocket->SendMessage("signaling_message", msg, [callback](const std::string &msg)->void {
    if (callback) {
      RespMsg respMsg;
      respMsg.ToObject(msg);
      callback(respMsg);
    }
  }, timeOut);
}

void VHRoom::OnSetMaxUsrCount(const int & maxUsrCount, const VHEventCB & callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (!mSocket) {
    if (reportLog) {
      // TODO: uplog
    }
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("Stream Subscribe failure, socket is empty!");
    return;
  }

  SetMaxUsrCountMsg msg;
  msg.mMaxUsrCount = maxUsrCount;
  mSocket->SetMaxUserCount(msg.ToJsonStr(), [callback](const std::string &msg)->void {
    if (callback) {
      RespMsg respMsg;
      respMsg.ToObject(msg);
      callback(respMsg);
    }
  }, timeOut);
}

void VHRoom::OnSubscribe(std::shared_ptr<VHStream> &stream, const VHEventCB & callback, uint64_t timeOut) {
  LOGI("Subscribe Stream");
  if (reportLog) {
    reportLog->upLog(invokeSubscribe, stream->mStreamId, "reason: default");
  }
  RespMsg respMsg;
  if (!mSocket) {
    if (reportLog) {
      reportLog->upLog(invokeSubscribeFailed, stream->mStreamId,"reason: invokeSubscribeFailed: mSocket is null");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("Stream Subscribe failure, socket is empty!");
    return;
  }
  if (!stream.get() || stream->mLocal) {
    if (reportLog) {
      reportLog->upLog(invokeSubscribeFailed, "", "reason: stream is null or local");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = StreamIsEmpty;
    respMsg.mMsg = "stream is empty or is local";
    callback(respMsg);
    LOGE("Subscribe fail: stream is null");
    return;
  }
  if (stream->GetStreamState() == VhallStreamState::Subscribe) {
    if (reportLog) {
      reportLog->upLog(invokeSubscribeFailed, stream->mStreamId, "reason: Subscribed already");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = StreamSubscribed;
    respMsg.mMsg = "stream is Subscribed";
    callback(respMsg);
    LOGE("Subscribe fail: stream is Subscribed");
    return;
  }
  stream->SetStreamState(VhallStreamState::Subscribe);
  LOGI("Subscribe Stream, id: %s", stream->GetID());

  /* config subscribe options */
  stream->mData = true;
  SubscribeMsg msg(*stream);
  msg.mMuteStreams.mAudio = stream->IsAudioMuted();
  msg.mMuteStreams.mVideo = stream->IsVideoMuted();
  msg.mDual = stream->mDual;
  stream->mRoom = shared_from_this();
  stream->Init();
  if (reportLog) {
    reportLog->upLog(sendSubscribeSignalling, stream->mStreamId, std::string(SUBSCRIBE) + ":" + msg.ToJsonStr());
  }
  mSocket->Subscribe(msg.ToJsonStr(), [&, stream, callback](const std::string &response)->void {
    LOGI("subscribe response %s", response.c_str());
    RespMsg respMsg;
    respMsg.ToObject(response);
    if (!respMsg.mResult.compare("success")) {
      stream->SetStreamState(VhallStreamState::Subscribe);
      if (reportLog) {
        reportLog->upLog(subscribeSuccess, stream->mStreamId, std::string("response:") + response.c_str());
      }
    }
    else {
      stream->SetStreamState(VhallStreamState::UnSubscribe);
      if (reportLog) {
        reportLog->upLog(subscribeFailed, stream->mStreamId, std::string("response:") + response.c_str());
      }
      RemoveStream(stream);
    }
    callback(respMsg);
  }, timeOut);
}

void VHRoom::OnUnSubscribe(std::shared_ptr<VHStream> &stream, const VHEventCB & callback, uint64_t timeOut, std::string reason) {
  LOGI("UnSubscribe Stream");
  if (reportLog) {
    reportLog->upLog(invokeUnsubscribe, stream->mStreamId, "reason: default");
  }
  RespMsg respMsg;
  if (reportLog) {
    reportLog->stopPlayMessageLog(stream);
  }
  if (!mSocket) {
    if (reportLog) {
      reportLog->upLog(invokeUnsubscribeFailed, stream->mStreamId, "reason: socket is null");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("Stream UnSubscribe failure, socket is empty!");
    return;
  }
  if (!stream.get() || stream->mLocal) {
    if (reportLog) {
      reportLog->upLog(invokeUnsubscribeFailed, stream->mStreamId, "reason: stream is null or local");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = StreamIsEmpty;
    respMsg.mMsg = "stream is empty or is local";
    callback(respMsg);
    LOGE("Cannot UnSubscribe, stream does not exist or is local");
    return;
  }
  if (VhallStreamState::Subscribe != stream->GetStreamState()) {
    if (reportLog) {
      reportLog->upLog(invokeUnsubscribeFailed, stream->mStreamId, "reason: stream not subscribed");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = StreamNotSubscribed;
    respMsg.mMsg = "stream not subscribed";
    callback(respMsg);
    LOGE("Cannot UnSubscribe, stream not subscribedl");
    return;
  }
  LOGI("UnSubscribe Stream, id: %s", stream->GetID());
  //stream->SetStreamState(VhallStreamState::UnSubscribe);
  if (mSocket) {
    if (reportLog) {
      reportLog->upLog(sendUnsubscribeSignalling, stream->mStreamId, "reason: default");
    }
    UnSubscribeMsg unsubscribeMsg;
    unsubscribeMsg.mStreamId = stream->mId;
    unsubscribeMsg.mReason = reason;
    mSocket->UnSubscribe(unsubscribeMsg.ToJsonStr(), [&, stream, callback](const std::string &response)->void {
      LOGI("unsubscribe response %s", response.c_str());
      RespMsg respMsg;
      respMsg.ToObject(response);
      if (!respMsg.mResult.compare("success")) {
        /* success */
        if (reportLog) {
          reportLog->upLog(unsubscribeSuccess);
        }
      }
      else {
        if (reportLog) {
          reportLog->upLog(unsubscribeReturnFailed, stream->mStreamId, "signalling response:" + response);
        }
      }
      callback(respMsg);
    }, timeOut);
  }
  RemoveStream(stream);
}

void VHRoom::OnSetSimulCast(std::shared_ptr<VHStream> &stream, const int & dual, const VHEventCB & callback, uint64_t timeOut) {
  LOGI("SetSimulCast_");
  RespMsg respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("SetSimulCast_ failed, socket is empty!");
    return;
  }
  if (!stream.get() || stream->mLocal) {
    respMsg.mResult = "failed";
    respMsg.mCode = StreamIsEmpty;
    respMsg.mMsg = "stream is empty or is local";
    callback(respMsg);
    LOGE("Cannot SetSimulCast_, stream does not exist");
    return;
  }
  SimulcastMsg msg;
  msg.mType = "updatestream";
  msg.mStreamId = stream->mId;
  msg.mDual = stream->mDual = dual;
  SendSignalingMessage(msg.ToJsonStr(), [&, callback](const RespMsg &msg)->void {
    if (callback) {
      callback(msg);
    }
  }, timeOut);
}

void VHRoom::DispatchStreamSubscribed(std::shared_ptr<VHStream> stream) {
  LOGI("DispatchStreamSubscribed");
  if (reportLog) {
    reportLog->startPlayMessageLog(stream);
  }
  StreamEvent event;
  event.mType = STREAM_SUBSCRIBED;
  event.mStreams = stream;
  this->DispatchEvent(event);
  LOGI("end DispatchStreamSubscribed");
}

void VHRoom::SendSignalingMessage(const std::string &msg, const VHEventCB & callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  LOGI("SendSignalingMessage");
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this(), nullptr, callback, timeOut);
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mMessage = msg;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_SINGNALINGMESSAGE, stmMsgData);
  }
  else {
    LOGE("SendSignalingMessage error");
  }
}

void VHRoom::SetMaxUsrCount(const int maxUsrCount, const VHEventCB & callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  LOGI("SetMaxUserCount");
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this(), nullptr, callback, timeOut);
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mMaxUsrCount = maxUsrCount;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_SETMAXUSRCOUNT, stmMsgData);
  }
  else {
    LOGE("SetMaxUserCount error");
  }
}

void VHRoom::SocketOnAddStream(BaseEvent& event) {
  LOGI("SocketOnAddStream");
  SocketEvent *socketEvent =static_cast<SocketEvent *>(&event);
  LOGI("SocketOnAddStream  %s", socketEvent->mArgs.c_str());
  AddStreamRespMsg resp(socketEvent->mArgs);
  /* check local stream */
  std::unique_lock<std::mutex> Llock(mLocalLock);
  std::shared_ptr<VHStream> localStream = mLocalStreams[resp.mId];
  Llock.unlock();
  if (localStream) {
    LOGW("local stream added to server");
    return;
  }
  StreamMsg streamMsg;
  streamMsg.mAudio = resp.mAudio;
  streamMsg.mData = resp.mData;
  streamMsg.mId = resp.mId;
  streamMsg.mLocal = false;
  streamMsg.mScreen = false; // TODO
  streamMsg.mStreamType = resp.mStreamType;
  streamMsg.mUser = resp.mUser;
  streamMsg.mVideo = resp.mVideo;
  streamMsg.mNumSpatialLayers = resp.mNumSpatialLayers;
  streamMsg.mStreamAttributes = resp.mStreamAttributes;

  if (reportLog) {
    reportLog->upLog(newStreamAddedToRoom, resp.mId, std::string("msg:") + socketEvent->mArgs);
  }
  std::shared_ptr<VHStream> stream = std::make_shared<VHStream>(streamMsg);
  if (!stream) {
    LOGE("create new stream failed");
    return;
  }
  std::unique_lock<std::mutex> lock(mRemoteLock);
  mRemoteStreams[streamMsg.mId] = stream;
  lock.unlock();

  if (spec.autoSubscribe) {
    if (isLocalStream(stream->mId.c_str()) != 1) {
      autoSubscrbie(stream);
    }
    else {
      LOGD("will not subscribe local stream");
    }
  }
  else {
    AddStreamEvent Addevent;
    Addevent.mType = STREAM_ADDED;
    Addevent.mStream = stream;
    this->DispatchEvent(Addevent);
  }
}

void VHRoom::SocketOnVhallMessage(BaseEvent& event) {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  stmMsgData->mSocketEvent = std::make_shared<SocketEvent>(*static_cast<SocketEvent*>(&event));
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_VHALL_MESSAGE, stmMsgData);
  }
  else {
    LOGE("SocketOnVhallMessage error");
  }
}

 void VHRoom::OnSocketOnVhallMessage(BaseEvent& event) {
  SocketEvent *socketEvent =static_cast<SocketEvent *>(&event);
  LOGI("SocketOnVhallMessage: %s", socketEvent->mArgs.c_str());
  SignalingMessageVhallRespMsg resp(socketEvent->mArgs);
  std::shared_ptr<VHStream> stream = nullptr;
  if (resp.mPeerId != "") {
    std::unique_lock<std::mutex> lock(mRemoteLock);
    stream = mRemoteStreams[resp.mPeerId];
    lock.unlock();
  } else {
    std::unique_lock<std::mutex> lock(mLocalLock);
    stream = mLocalStreams[resp.mStreamId];
    lock.unlock();
  }
  if (stream ) {
    if (VhallStreamState::UnPublish != stream->GetStreamState() && VhallStreamState::UnSubscribe != stream->GetStreamState())
    if (resp.mMess.mType == "ready") {
      /* 获取流是否使用海外节点 */
      GetOversea(stream, [](const RespMsg & resp)->void {}, 5000);
      if (stream->mLocal && reportLog) {
        /* 推流成功，流量上报 */
        reportLog->startPublishMessageLog(stream);
      }
      StreamEvent event;
      event.mType = STREAM_READY;
      event.mStreams = stream;
      this->DispatchEvent(event);
    }
    stream->OnMessageFromErizo(resp);
  }
}

void VHRoom::SocketOnRemoveStream(BaseEvent& event) {
  SocketEvent *socketEvent =static_cast<SocketEvent *>(&event);
  RemoveStreamRespMsg resp(socketEvent->mArgs);
  LOGI("RemoveStreamRespMsg  %s", resp.mId.c_str());
  std::shared_ptr<VHStream> stream;
  if (resp.mId != "") {
    // 异常
    std::unique_lock<std::mutex> Llock(mLocalLock);
    stream = mLocalStreams[resp.mId];
    Llock.unlock();
    if (stream) {
      LOGI("mLocalStreams");
      if (reportLog) {
        reportLog->upLog(serverDeleteLocalStream, resp.mId, "receive serverDeleteLocalStream");
        reportLog->upLog(ulStreamFailed, resp.mId, "receive serverDeleteLocalStream");
      }
      std::string msg = "Stream failed after connection";
      StreamFailed(stream, msg, "onRemoveStream");
    }
    // 正常
    std::unique_lock<std::mutex> Rlock(mRemoteLock);
    stream = mRemoteStreams[resp.mId];
    Rlock.unlock();
    if (stream) {
      LOGI("mRemote Stream, stop PlayMessageLog and release");
      if (reportLog) {
        reportLog->upLog(streamDeleted, resp.mId, "reason: remote stream Deleted");
        reportLog->stopPlayMessageLog(stream);
      }
      std::unique_lock<std::mutex> lock(mRemoteLock);
      mRemoteStreams.erase(resp.mId);
      lock.unlock();
      {
        /* here must sync invoke by sio thread */
        if (reportLog) {
          reportLog->stopPlayMessageLog(stream);
        }
        RemoveStream(stream);
      }
      StreamEvent event;
      event.mType = STREAM_REMOVED;
      event.mStreams = stream;
      this->DispatchEvent(event);
    }
  }
  LOGD("end %s", __FUNCTION__);
}

void VHRoom::SocketOnDisconnect(BaseEvent& event) {
  LOGI("SocketOnDisconnect");
  if (reportLog) {
    reportLog->upLog(signallingConnectFailed, "0", std::string("reason: SocketOnDisconnect from server"));
  }

  RoomEvent RoomEvent;
  RoomEvent.mErrorType = RoomEvent::SocketDisconnected;
  RoomEvent.mType = ROOM_ERROR;
  this->DispatchEvent(RoomEvent);
}
void VHRoom::SocketOnReconnected(BaseEvent& event) {
   LOGD("SocketOnReconnected");
   RoomEvent Revent;
   Revent.mType = ROOM_RECONNECTED;
   Revent.mMessage = "room reconnected, clean streams";
   this->DispatchEvent(Revent);
}

void VHRoom::SocketOnICEConnectionFailed(BaseEvent& event) {
  LOGE("SocketOnICEConnectionFailed");
  SocketEvent *socketEvent = static_cast<SocketEvent *>(&event);
  LOGE("SocketOnICEConnectionFailed  %s", socketEvent->mArgs.c_str());
  ICEConnectFailedRespMsg resp(socketEvent->mArgs);
  std::shared_ptr<VHStream> vhStream;
  if (resp.type == "publish") {
    vhStream = mLocalStreams[resp.streamId];
  } else {
    vhStream = mRemoteStreams[resp.streamId];
  }
  if (vhStream) {
    std::string msg = "SocketOnICEConnectionFailed";
    if (reportLog) {
      reportLog->upLog(ulStreamFailed, resp.streamId, "receive SocketOnICEConnectionFailed frome server");
    }
    StreamFailed(vhStream, msg, "connection_failed");
  } else {
    LOGE("The VHStream is empty!");
  }
}

void VHRoom::SocketOnError(BaseEvent& event) {
  LOGE("SocketOnError");
}
void VHRoom::SocketOnStreamMixed(BaseEvent& event) {
  LOGI("SocketOnStreamMixed");
  SocketEvent *socketEvent = static_cast<SocketEvent *>(&event);
  LOGI("SocketOnStreamMixed  %s", socketEvent->mArgs.c_str());
  StreamMixedMsg mixMsg(socketEvent->mArgs.c_str());

  StreamMsg streamMsg;
  streamMsg.mAudio = mixMsg.mAudio;
  streamMsg.mData = mixMsg.mData;
  streamMsg.mId = mixMsg.mId;
  streamMsg.mLocal = false;
  streamMsg.mScreen = mixMsg.mScreen; // TODO
  streamMsg.mStreamType = mixMsg.mStreamType;
  streamMsg.mUser = mixMsg.mUser;
  streamMsg.mVideo = mixMsg.mVideo;
  streamMsg.mNumSpatialLayers = mixMsg.mSpatialLayers;
  streamMsg.mStreamAttributes = mixMsg.mAttributes;

  AddStreamEvent Addevent;
  Addevent.mType = ON_STREAM_MIXED;
  Addevent.mStream = std::make_shared<VHStream>(streamMsg);
  this->DispatchEvent(Addevent);
}
void VHRoom::OnSocketOnNetworkChanged(BaseEvent& event) {
   LOGW("SocketOnNetworkChanged");
   std::unique_lock<std::mutex> Llock(mLocalLock);
   std::unordered_map<std::string, std::shared_ptr<VHStream>> mTempLocalStreams(mLocalStreams);
   Llock.unlock();
   if (!mTempLocalStreams.empty()) {
      auto iter = mTempLocalStreams.begin();
      for (auto iter = mTempLocalStreams.begin(); iter != mTempLocalStreams.end(); ++iter) {
         auto stream = iter->second;
         if (stream && VhallStreamState::Publish == stream->GetStreamState()) {
           OnUnPublish(stream, [](const RespMsg& resp) -> void {
              if (resp.mResult.compare("success")) {
                LOGE("SocketOnNetworkChanged: unpublish localStream failed, error msg: %s", resp.mMsg.c_str());
              }
            }, 10000);
           OnPublish(stream, [&, stream](const RespMsg& resp) -> void {
              if (resp.mResult.compare("success")) {
                LOGE("SocketOnNetworkChanged: publish localStream failed, error msg: %s", resp.mMsg.c_str());
                std::string msg = resp.mMsg;
                StreamFailed(stream, msg, "connection_failed");
              }
              else {
                LOGD("OnSocketOnNetworkChanged(): republish success, new stream id:%s", stream->mId.c_str());
                StreamEvent event;
                event.mType = STREAM_REPUBLISHED;
                event.mStreams = stream;
                this->DispatchEvent(event);
              }
            }, 10000);
         }
      }
   }
   mTempLocalStreams.clear();

   std::unique_lock<std::mutex> Rlock(mRemoteLock);
   std::unordered_map<std::string, std::shared_ptr<VHStream>> mTempStreams;
   mTempStreams = mRemoteStreams;
   Rlock.unlock();

   if (!mTempStreams.empty()) {
      auto iter = mTempStreams.begin();
      for (; iter != mTempStreams.end(); ) {
         auto stream = iter->second;
         ++iter;
         if (stream && VhallStreamState::Subscribe == stream->GetStreamState())
         {
           OnUnSubscribe(stream, [&, stream](const RespMsg& resp) -> void {
              if (resp.mResult.compare("success")) {
                LOGE("SocketOnNetworkChanged: unsubscribe stream failed, id: %s, error msg: %s", stream->mId.c_str(), resp.mMsg.c_str());
              }
            }, 10000);
            StreamEvent event;
            event.mType = STREAM_REMOVED;
            event.mStreams = stream;
            this->DispatchEvent(event);
            OnSubscribe(stream, [&, stream](const RespMsg& resp) -> void {
              if (resp.mResult.compare("success")) {
                LOGE("SocketOnNetworkChanged: subscribe stream failed, id: %s, error msg: %s", stream->mId.c_str(), resp.mMsg.c_str());
                std::string msg = resp.mMsg;
                StreamFailed(stream, msg, "connection_failed");
              }
            }, 10000);
         }
      }
   }
   mTempStreams.clear();
   RoomEvent net_event;
   net_event.mType = ROOM_NETWORKCHANGED;
   this->DispatchEvent(net_event);
}

void VHRoom::RemoveStream(std::shared_ptr<VHStream> stream) {
  LOGI("RemoveStream");
  if (stream && stream->mBaseStack) {
    LOGI("RemoveStream: streamId: %s, userId: %s", stream->GetID(), stream->GetUserId());
    stream->stop();
    if (stream->GetStreamState() != VhallStreamState::Uninitialized) {
      stream->close();
    }
  }
  if (stream && !stream->mLocal) {
    stream->Destroy();
  }
}

void VHRoom::StreamFailed(std::shared_ptr<VHStream> stream, std::string &msg, std::string reason) {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  stmMsgData->mStream = stream;
  stmMsgData->mMessage = msg;
  stmMsgData->mReason = reason;
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_STREAM_FAILED, stmMsgData);
  }
  else {
    LOGE("SocketOnNetworkChanged error");
  }
}

void VHRoom::MuteStream(std::shared_ptr<VHStream>& stream, bool muteAudio, bool muteVideo, const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* msgData = new rtc::StreamMsgData(shared_from_this(), stream, callback, timeOut);
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    msgData->mMuteAudio = muteAudio;
    msgData->mMuteVideo = muteVideo;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_MUTE_STREAM, msgData);
  }
  else {
    LOGE("MuteStream error");
  }
}

void VHRoom::AddStreamToMix(std::string streamId, const VHEventCB & callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData*msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    msgData->streamId = streamId;
    msgData->mCb = callback;
    msgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_AddStreamToMix, msgData);
  }
  else {
    LOGE("AddStreamToMix error");
  }
}

void VHRoom::DelStreamFromMix(std::string streamId, const VHEventCB & callback) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData*msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    msgData->streamId = streamId;
    msgData->mCb = callback;
    msgData->mTimeOut = 0;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_DelStreamFromMix, msgData);
  }
  else {
    LOGE("DelStreamFromMix error");
  }
}

void VHRoom::OnStreamFailed(std::shared_ptr<VHStream> stream, std::string &msg, std::string reason) {
  LOGE("onStreamFailed, msg: %s", msg.c_str());
  if (!stream) {
    LOGE("The VHStream is empty!");
    return;
  }

  if (stream->mLocal) {
    LOGE("onStreamFailed: streamId: %s, userId: %s", stream->GetID(), stream->GetUserId());
    this->OnUnPublish(stream, [](const RespMsg & resp)->void {}, 0, reason);

    StreamEvent event;
    event.mType = STREAM_FAILED;
    event.mStreams = stream;
    event.mMessage = msg;
    this->DispatchEvent(event);
  }
  else {
    this->OnUnSubscribe(stream, [](const RespMsg & resp)->void {}, 0, reason);
    StreamEvent event;
    event.mType = STREAM_REMOVED;
    event.mStreams = stream;
    this->DispatchEvent(event);

    LOGI("Stream Reconnect");
    StreamMsg config = *stream;
    std::shared_ptr<VHStream> newStream = std::make_shared<VHStream>(config);
    if (spec.autoSubscribe) {
      autoSubscrbie(stream);
    }
    else {
      ReconnectStreamEvent ReEvent;
      ReEvent.mType = STREAM_RECONNECT;
      ReEvent.OldStream = stream;
      ReEvent.NewStream = stream;
      this->DispatchEvent(ReEvent);
    }
  }
}

void VHRoom::OnClearLocalStream() {
  std::unique_lock<std::mutex> Llock(mLocalLock);
  std::unordered_map<std::string, std::shared_ptr<VHStream>> mTempLocalStreams(mLocalStreams);
  mLocalStreams.clear();
  Llock.unlock();
  if (!mTempLocalStreams.empty()) {
    //auto iter = mTempLocalStreams.begin();
    for (auto iter = mTempLocalStreams.begin(); iter != mTempLocalStreams.end(); ++iter) {
      auto stream = iter->second;
      if (stream && VhallStreamState::Publish == stream->GetStreamState()) {
        // TODO: 若unpublish失败，服务端会多一路无效流
        OnUnPublish(stream, [](const RespMsg& resp) -> void {
          if (resp.mResult.compare("success")) {
            LOGE("republish: unPublish localStream failed, error msg: %s", resp.mMsg.c_str());
          }
        }, 10000, "room reconnected after disconnected ");
      }
    }
  }
}
void VHRoom::ClearLocalStream() {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
  rtc::MessageData* msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_RESUME_LOCALSTREAMS, msgData);
  }
  else {
    LOGE("Disconnect error");
  }
}

void VHRoom::CleanStream() {
   std::unordered_map<std::string, std::shared_ptr<VHStream>> mTempStreams;
   mTempStreams.clear();

   std::unique_lock<std::mutex> Rlock(mRemoteLock);
   mTempStreams = mRemoteStreams;
   Rlock.unlock();

   if (!mTempStreams.empty()) {
      auto iter = mTempStreams.begin();
      for (; iter != mTempStreams.end(); ) {
         auto current = iter->second;
         ++iter;
         if (!current) {
           continue;
         }
         if (reportLog) {
           reportLog->stopPlayMessageLog(current);
         }
         RemoveStream(current);
         StreamEvent event;
         event.mType = STREAM_REMOVED;
         event.mStreams = current;
         this->DispatchEvent(event);
      }
      std::unique_lock<std::mutex> Rlock(mRemoteLock);
      mRemoteStreams.clear();
      Rlock.unlock();
   }
   mTempStreams.clear();

   std::unique_lock<std::mutex> Llock(mLocalLock);
   mTempStreams = mLocalStreams;
   Llock.unlock();
   if (!mTempStreams.empty()) {
      auto iter = mTempStreams.begin();
      for (; iter != mTempStreams.end(); ) {
         auto current = iter->second;
         ++iter;
         if (!current) {
           continue;
         }
         OnUnPublish(current, [](const RespMsg & resp)->void {}, 0, "clean streams");
      }
      std::unique_lock<std::mutex> Llock(mLocalLock);
      mLocalStreams.clear();
      Llock.unlock();
   }
   mTempStreams.clear();
}
void VHRoom::ClearRemoteStreams() {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
   rtc::MessageData* msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_CLEAN_REMOTESTREAMS, msgData);
  }
  else {
    LOGE("CleanRemoteStreams error");
  }
}
void VHRoom::OnClearRemoteStreams() {
  std::unique_lock<std::mutex> Rlock(mRemoteLock);
  if (!mRemoteStreams.empty()) {
    auto iter = mRemoteStreams.begin();
    for (; iter != mRemoteStreams.end(); ) {
      auto current = iter->second;
      ++iter;
      if (!current) {
        continue;
      }
      mRemoteStreams.erase(current->mId);
      if (reportLog) {
        reportLog->stopPlayMessageLog(current);
      }
      RemoveStream(current);
      StreamEvent event;
      event.mType = STREAM_REMOVED;
      event.mStreams = current;
      this->DispatchEvent(event);
    }
  }
  mRemoteStreams.clear();
  Rlock.unlock();
}

void VHRoom::ClearAll() {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
  rtc::MessageData* msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_CLEAR_ALL, msgData);
  }
  else {
    LOGE("Connect error");
  }
}
void VHRoom::OnClearAll() {
  LOGI("ClearAll");
  CleanStream();
  if (mSocket) {
    mSocket->RemoveAllEventListener();
    mState = VhallDisconnected;
    mSocket->Disconnect();
  }
  mSocket.reset();
  //RoomEvent Revent;
  //Revent.mType = ROOM_DISCONNECTED;
  //Revent.mMessage = "expected-disconnection";
  //this->DispatchEvent(Revent);
}

// about mix broadCast
void VHRoom::OnconfigRoomBroadCast(RoomBroadCastOpt& Options, const VHEventCB & callback, uint64_t timeOut) {
  LOGI("configRoomBroadCast");
  RespMsg respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("configRoomBroadCast failure, socket is empty!");
    return;
  }
  Json::Value dictdata(Json::ValueType::objectValue);
  Json::Value config(Json::ValueType::objectValue);
  Json::Value resolution(Json::ValueType::arrayValue);
  Json::Value border(Json::ValueType::objectValue);

  if (Options.mProfileIndex != BROADCAST_VIDEO_PROFILE_UNDEFINED) {
    std::shared_ptr<BroadCastProfile> profile = mBroadCastProfileList.GetProfile(Options.mProfileIndex);
    if (profile && profile.get()) {
      Options.width = profile->mWidth;
      Options.height = profile->mHeight;
      Options.bitrate = profile->mBitRate;
      Options.framerate = profile->mFrameRate;
    }
    else {
      respMsg.mResult = "failed";
      respMsg.mCode = ParamsError;
      respMsg.mMsg = "ProfileIndex not exist";
      callback(respMsg);
      LOGE("configRoomBroadCast failure, ProfileIndex not exist");
      return;
    }
  }

  resolution.append(Options.width);
  resolution.append(Options.height);

  config["customizedYM"] = Json::Value(Options.customizedYM);
  config["resolution"] = Json::Value(resolution);
  config["framerate"] = Json::Value(Options.framerate);
  config["bitrate"] = Json::Value(Options.bitrate);
  config["publishUrl"] = Json::Value(Options.publishUrl);
  config["layoutMode"] = Json::Value(Options.layoutMode);
  config["precast_pic_exist"] = Json::Value(Options.precast_pic_exist);

  config["backgroundColor"] = Json::Value(Options.backgroundColor);
  border["color"] = Json::Value(Options.borderColor);
  border["width"] = Json::Value(Options.borderWidth);
  border["exist"] = Json::Value(Options.borderExist);
  config["border"] = border;

  Json::FastWriter root;
  dictdata["roomID"] = Json::Value(mRoomId);
  dictdata["config"] = Json::Value(root.write(config));
  
  if (reportLog) {
    reportLog->upLog(invokeConfigRoomBroadCast, "0", std::string("config: ") + root.write(dictdata));
  }
  mSocket->ConfigRoomBroadCast(root.write(dictdata), [&, callback](const std::string &data)->void {
    RespMsg respMsg;
    respMsg.ToObject(data);
    if (respMsg.mMsg == "") {
      respMsg.mMsg = broad_cast_events_.GetDetail(respMsg.mCode);
    }
    callback(respMsg);
    if (!respMsg.mResult.compare("success")) {
      if (reportLog) {
        reportLog->upLog(configRoomBroadCastSuccess, "0", "configRoomBroadCast success");
      }
    }
    else {
      if (reportLog) {
        reportLog->upLog(configRoomBroadCastFailed, "0", std::string("response:") + data);
      }
    }
  }, timeOut);
}

void VHRoom::OnsetMixLayoutMode(UINT32 configStr, const VHEventCB & callback, uint64_t timeOut) {
  LOGI("setMixLayoutMode");
  RespMsg respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("setMixLayoutMode failure, socket is empty!");
    return;
  }

  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["roomID"] = Json::Value(mRoomId);
  dictdata["config"] = Json::Value(configStr);

  Json::FastWriter root;
  if (reportLog) {
    reportLog->upLog(invokeSetMixLayoutMode, "0", "config: " + Utility::ToString(configStr));
  }
  mSocket->SetMixLayoutMode(root.write(dictdata), [&, callback, configStr](const std::string &data)->void {
    RespMsg respMsg;
    respMsg.ToObject(data);
    if (respMsg.mMsg == "") {
      respMsg.mMsg = broad_cast_events_.GetDetail(respMsg.mCode);
    }
    callback(respMsg);
    if (!respMsg.mResult.compare("success")) {
      if (reportLog) {
        reportLog->upLog(setMixLayoutSuccess, "0", "setMixLayout Success, config: " + Utility::ToString(configStr));
      }
    }
    else {
      if (reportLog) {
        reportLog->upLog(setMixLayoutFailed, "0", "setMixLayout Failed, response:" + data);
      }
    }
  }, timeOut);
}

void VHRoom::OnsetMixLayoutMainScreen(const char *streamID, const VHEventCB & callback, uint64_t timeOut) {
  std::string streamId = streamID;
  LOGI("setMixLayoutMainScreen, streamID:%s", streamID);
  RespMsg respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("setMixLayoutMainScreen failure, socket is empty!");
    return;
  }

  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["roomID"] = Json::Value(mRoomId);
  dictdata["config"] = Json::Value(streamID);

  Json::FastWriter root;
  if (reportLog) {
    reportLog->upLog(invodeSetMixLayoutMainScreen, streamId, "invodeSetMixLayoutMainScreen, config: " + root.write(dictdata));
  }
  mSocket->SetMixLayoutMainScreen(root.write(dictdata), [&, streamId, callback](const std::string &data)->void {
    RespMsg respMsg;
    respMsg.ToObject(data);
    if (respMsg.mMsg == "") {
      respMsg.mMsg = broad_cast_events_.GetDetail(respMsg.mCode);
    }
    callback(respMsg);
    if (!respMsg.mResult.compare("success")) {
      if (reportLog) {
        reportLog->upLog(setMixLayoutMainScreenSuccess, streamId, "setMixLayoutMainScreen Success, id: " + streamId);
      }
    }
    else {
      if (reportLog) {
        reportLog->upLog(setMixLayoutMainScreenFailed, streamId, "setMixLayoutMainScreen Failed, response: " + data);
      }
    }
  }, timeOut);
}

void VHRoom::OnstartRoomBroadCast(const VHEventCB & callback, uint64_t timeOut) {
  LOGI("startRoomBroadCast");
  RespMsg respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    return;
  }

  if (reportLog) {
    reportLog->upLog(invokeStartRoomBroadCast, "0", "call invokeStartRoomBroadCast api");
  }
  mSocket->StartRoomBroadCast(mRoomId, [&, callback](const std::string &data)->void {
    RespMsg respMsg;
    respMsg.ToObject(data);
    if (respMsg.mMsg == "") {
      respMsg.mMsg = broad_cast_events_.GetDetail(respMsg.mCode);
    }
    callback(respMsg);
    if (!respMsg.mResult.compare("success")) {
      if (reportLog) {
        reportLog->upLog(startRoomBroadCastSuccess, "0", "startRoomBroadCast Success");
      }
    }
    else {
      if (reportLog) {
        reportLog->upLog(startRoomBroadCastFailed, "0", "startRoomBroadCast Failed, response :" + data);
      }
    }
  }, timeOut);
}

void VHRoom::OnstopRoomBroadCast(const VHEventCB & callback, uint64_t timeOut) {
  LOGI("stopRoomBroadCast");
  RespMsg respMsg;
  if (!mSocket) {
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    return;
  }
  if (reportLog) {
    reportLog->upLog(invokeStopRoomBroadCast, "0", "call StopRoomBroadCast api");
  }
  mSocket->StopRoomBroadCast(mRoomId, [&, callback](const std::string &data)->void {
    RespMsg respMsg;
    respMsg.ToObject(data);
    if (respMsg.mMsg == "") {
      respMsg.mMsg = broad_cast_events_.GetDetail(respMsg.mCode);
    }
    callback(respMsg);
    if (!respMsg.mResult.compare("success")) {
      if (reportLog) {
        reportLog->upLog(stopRoomBroadCastSuccess, "0", "stopRoomBroadCast Success");
      }
    }
    else {
      if (reportLog) {
        reportLog->upLog(stopRoomBroadCastFailed, "0", "stopRoomBroadCast Failed, response: " + data);
      }
    }
  }, timeOut);
}
// end about mix broadCast

void VHRoom::SocketOnNetworkChanged(BaseEvent& event) {
  if (mStopFlag) {
    LOGE("room not connected");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  stmMsgData->mEvent = event;
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_NETWORK_CHANGED, stmMsgData);
  }
  else {
    LOGE("SocketOnNetworkChanged error");
  }
}
void VHRoom::SocketReconnecting(BaseEvent& event) {
  RoomEvent roomEvent;
  roomEvent.mType = ROOM_RECONNECTING;
  this->DispatchEvent(roomEvent);
}
void VHRoom::SocketConnecting(BaseEvent& event) {
  RoomEvent roomEvent;
  roomEvent.mType = ROOM_CONNECTING;
  this->DispatchEvent(roomEvent);
}
void VHRoom::SocketRecover(BaseEvent& event) {
  RoomEvent roomEvent;
  roomEvent.mType = ROOM_RECOVER;
  this->DispatchEvent(roomEvent);
}

/* work method */
bool VHRoom::Connect() {
  if (!mStopFlag) {
    LOGE("room connected");
    return false;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (mWorkThread) {
    LOGW("connected already");
    return false;
  }
  mMsgHandler.reset(new rtc::MsgHandler());
  if (nullptr == mMsgHandler) {
    LOGE("new MsgHandler fail");
    return false;
  }
  mWorkThread = rtc::Thread::Create();
  if (nullptr == mWorkThread) {
    LOGE("mWorkThread init fail.");
    return false;
  }
  mWorkThread->Start();
  mStopFlag = false;
  rtc::MessageData* msgData  = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_ROOM_CONNECT, msgData);
  }
  else {
    LOGE("Connect error");
    return false;
  }
  return true;
}

bool VHRoom::Disconnect() {
  LOGD("");
  if (mStopFlag) {
    LOGE("room not connected");
    return false;
  }
  mStopFlag = true;
  if (reportLog) {
    /* 异步处理，网络卡顿则不上报断开消息，网络通常则上报，上报不影响room断开时效 */
    /*
    *  uplog模块绑定用户信息/room，避免创建多个room/room重连后用户信息变化 故不使用全局uplog方式
    */
    reportLog->UpLogin(disconnectSignalling, "request disconnect room");
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    return false;
  }
  /*rtc::MessageData* msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Stop();
    mWorkThread->Clear(mMsgHandler.get());
    mWorkThread->Start();
    mWorkThread->Send(RTC_FROM_HERE, mMsgHandler.get(), MSG_ROOM_DISCONNECT, msgData);
  }
  else {
    LOGE("Disconnect error");
  }*/
  LOGD("destruct work thread");
  if (mWorkThread) {
    mWorkThread->Stop();
    mWorkThread->Clear(mMsgHandler.get());
    mWorkThread.reset();
  }
  if (mMsgHandler) {
    mMsgHandler.reset();
  }
  OnDisconnect();

  if (reportLog) {
    reportLog->upLog(invokeDisconnectSignalling, "0", "reason: default");
    reportLog->upLog(signallingDisconnected, "0", "reason: default");
    reportLog.reset();
    VHStream::SetReport(nullptr);
  }
  return true;
}

bool VHRoom::GetOversea(std::shared_ptr<VHStream> &stream, const VHEventCB & callback, uint64_t timeOut) {
  LOGI("");
  if (mStopFlag) {
    LOGE("not connected");
    return false;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("work thread not initialized");
    return false;
  }
  rtc::StreamMsgData*msgData = new rtc::StreamMsgData(shared_from_this());
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    msgData->mCb = callback;
    msgData->mTimeOut = timeOut;
    msgData->mStream = stream;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_GET_OVERSEA, msgData);
  }
  else {
    LOGE("create MessageData error");
  }
  return false;
}

void VHRoom::Publish(std::shared_ptr<VHStream> &stream, const VHPublishEventCB &callback, uint64_t timeOut, int delay) {
  PublishRespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    if (reportLog) {
      std::stringstream msg;
      msg << "user:";
      msg << stream->mUser.mId;
      msg << ",";
      msg << "option: width " << stream->videoOpt.maxWidth << ",height " << stream->videoOpt.maxHeight << ", fps " << stream->videoOpt.maxFrameRate;
      reportLog->upLog(invokePublishFailed, stream->mStreamId, msg.str() + "reason: sinalling not connected");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "socket not set";
    callback(respMsg);
    LOGE("Stream Publish failure, room not connected!");
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    if (reportLog) {
      reportLog->upLog(invokePublishFailed, stream->mStreamId, std::string("invokePublishFailed:") + "WorkThread not exist");
    }
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "socket not set";
    callback(respMsg);
    LOGE("WorkThread not exist");
    return;
  }
  rtc::MessageData* msgData = new rtc::StreamMsgData(shared_from_this(), stream, callback, timeOut);
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    if (delay > 0) {
      mWorkThread->PostDelayed(RTC_FROM_HERE, delay, mMsgHandler.get(), MSG_PUBLISH_STREAM, msgData);
    }
    else {
      mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_PUBLISH_STREAM, msgData);
    }
  }
  else {
    LOGE("Publish error");
  }
}

void VHRoom::UnPublish(std::shared_ptr<VHStream>& stream, const VHEventCB & callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::MessageData*msgData = new rtc::StreamMsgData(shared_from_this(), stream, callback, timeOut);
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_UNPUBLISH_STREAM, msgData);
  }
  else {
    LOGE("UnPublish error");
  }
}

void VHRoom::Subscribe(std::shared_ptr<VHStream> &stream, const SubscribeOption* option, const VHEventCB &callback, uint64_t timeOut, int delay) {
  LOGE("Subscribe");
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  if (option) {
    stream->MuteAudio(option->mMuteAudio);
    stream->MuteVideo(option->mMuteVideo);
    stream->mDual = option->mDual;
  }
  rtc::MessageData*msgData = new rtc::StreamMsgData(shared_from_this(), stream, callback, timeOut);
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    if (delay > 0) {
      mWorkThread->PostDelayed(RTC_FROM_HERE, delay, mMsgHandler.get(), MSG_SUBSCRIBE_STREAM, msgData);
    }
    else {
      mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_SUBSCRIBE_STREAM, msgData);
    }
  }
  else {
    LOGE("Subscribe error");
  }
}

void VHRoom::Subscribe(std::string stream_id, const SubscribeOption* option, const VHEventCB &callback, uint64_t timeOut, int delay) {
  std::unique_lock<std::mutex> Rlock(mRemoteLock);
  std::unordered_map<std::string, std::shared_ptr<VHStream>>::iterator iter = mRemoteStreams.find(stream_id);
  if (iter != mRemoteStreams.end()) {
    std::shared_ptr<VHStream> stream = iter->second;
    if (stream) {
      if (option) {
        stream->MuteAudio(option->mMuteAudio);
        stream->MuteVideo(option->mMuteVideo);
        stream->mDual = option->mDual;
      }
      Subscribe(stream, option, callback, timeOut, delay);
      return;
    }
  }
  RespMsg respMsg;
  respMsg.mResult = "failed";
  respMsg.mCode = StreamIsEmpty;
  respMsg.mMsg = "stream is empty";
  callback(respMsg);
  LOGE("Stream Subscribe failure, stream is empty!");
  return;
}

void VHRoom::autoSubscrbieAll() {
  std::unique_lock<std::mutex> lock(mRemoteLock);
  std::unordered_map<std::string, std::shared_ptr<VHStream>> mTempStreams = mRemoteStreams;;
  lock.unlock();
  auto iter = mTempStreams.begin();
  for (; iter != mTempStreams.end(); ) {
    auto stream = iter->second;
    ++iter;
    if (stream)
    {
      autoSubscrbie(stream);
    }
  }
}

void VHRoom::autoSubscrbie(std::shared_ptr<VHStream> stream, int delay) {
  if (!stream) {
    LOGE("stream is null");
    return;
  }
  std::unique_lock<std::mutex> lock(mRemoteLock);
  std::shared_ptr<VHStream> subStream = mRemoteStreams[stream->mId];
  lock.unlock();
  if (!subStream) {
    LOGW("autoSubscrbie failed, stream is null");
    return;
  }
  Subscribe(subStream, nullptr, [&, subStream](const vhall::RespMsg& resp) -> void {
    if (!resp.mResult.compare("failed")) {
      LOGE("autoSubscrbie failed");
      if (resp.mCode != StreamSubscribed) {
        LOGE("autoSubscrbie failed, subscribe after 3sec");
        std::shared_ptr<VHStream> stream_ = subStream;
        UnSubscribe(stream_, [](const RespMsg& resp) -> void {});
        autoSubscrbie(subStream, 3000);
      }
    }
  }, 10000, delay);
}

void VHRoom::UnSubscribe(std::shared_ptr<VHStream> &stream, const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::MessageData*msgData = new rtc::StreamMsgData(shared_from_this(), stream, callback, timeOut);
  if (msgData && mMsgHandler.get() && mWorkThread.get()) {
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_UNSUBSCRIBE_STREAM, msgData);
  }
  else {
    LOGE("UnSubscribe error");
  }
}

void  VHRoom::UnSubscribe(std::string &stream_id, const VHEventCB &callback, uint64_t timeOut) {
  std::unique_lock<std::mutex> Rlock(mRemoteLock);
  std::unordered_map<std::string, std::shared_ptr<VHStream>>::iterator iter = mRemoteStreams.find(stream_id);
  if (iter != mRemoteStreams.end()) {
    std::shared_ptr<VHStream> stream = iter->second;
    if (stream) {
      UnSubscribe(stream, callback, timeOut);
      return;
    }
  }
  RespMsg respMsg;
  respMsg.mResult = "failed";
  respMsg.mCode = StreamIsEmpty;
  respMsg.mMsg = "stream is empty";
  callback(respMsg);
  LOGE("Stream UnSubscribe failure, stream is empty!");
  return;
}

void VHRoom::SetSimulCast(std::shared_ptr<VHStream> &stream, const int & dual, const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mStream = stream;
    stmMsgData->mDual = dual;
    stmMsgData->mCb = callback;
    stmMsgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_SETSIMULCAST, stmMsgData);
  }
  else {
    LOGE("SetSimulCast error");
  }
}
void VHRoom::configRoomBroadCast(RoomBroadCastOpt& Options, const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mOptions = Options;
    stmMsgData->mCb = callback;
    stmMsgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_CONFIG_ROOM_BROADCAST, stmMsgData);
  }
  else {
    LOGE("configRoomBroadCast error");
  }
}

void VHRoom::setMixLayoutMode(UINT32 configStr, const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mConfigStr = configStr;
    stmMsgData->mCb = callback;
    stmMsgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_SET_MIX_LAYOUT_MODE, stmMsgData);
  }
  else {
    LOGE("setMixLayoutMode error");
  }
}

void VHRoom::setMixLayoutMainScreen(const char *streamID, const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mStreamID = streamID ? streamID : "";
    stmMsgData->mCb = callback;
    stmMsgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_SET_LAYOUT_MAIN_SCREEN, stmMsgData);
  }
  else {
    LOGE("setMixLayoutMainScreen error");
  }
}

void VHRoom::startRoomBroadCast(const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mCb = callback;
    stmMsgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_START_ROOM_BROAD_CAST, stmMsgData);
  }
  else {
    LOGE("startRoomBroadCast error");
  }
}

void VHRoom::stopRoomBroadCast(const VHEventCB &callback, uint64_t timeOut) {
  RespMsg respMsg;
  if (mStopFlag) {
    LOGE("room not connected");
    respMsg.mResult = "failed";
    respMsg.mCode = RoomNotConnected;
    respMsg.mMsg = "room not connected";
    callback(respMsg);
    return;
  }
  std::unique_lock<std::mutex> lock(mThreadMtx);
  if (!mWorkThread) {
    LOGE("WorkThread not exist");
    respMsg.mResult = "failed";
    respMsg.mCode = ThreadNotInitialized;
    respMsg.mMsg = "WorkThread not exist";
    callback(respMsg);
    return;
  }
  rtc::StreamMsgData* stmMsgData = new rtc::StreamMsgData(shared_from_this());
  if (stmMsgData && mMsgHandler.get() && mWorkThread.get()) {
    stmMsgData->mCb = callback;
    stmMsgData->mTimeOut = timeOut;
    mWorkThread->Post(RTC_FROM_HERE, mMsgHandler.get(), MSG_STOP_ROOM_BROAD_CAST, stmMsgData);
  }
  else {
    LOGE("stopRoomBroadCast error");
  }
}

int VHRoom::isLocalStream(const char *id) {
  LOGI("isLocalStream");
  std::string uid = id;
  std::unique_lock<std::mutex> Llock(mLocalLock);
  if (mLocalStreams[uid]) {
    Llock.unlock();
    return 1;
  }
  Llock.unlock();

  std::unique_lock<std::mutex> Rlock(mRemoteLock);
  if (mRemoteStreams[uid]) {
    Rlock.unlock();
    return 0;
  }
  Rlock.unlock();
  return -1;
}

void VHRoom::OnMuteStream(std::shared_ptr<VHStream>& stream, bool muteAudio, bool muteVideo, const VHEventCB & callback, uint64_t timeOut) {
  MuteStreamMsg muteMsg;
  muteMsg.mIsLocal = stream->mLocal;
  muteMsg.mStreamId = stream->mStreamId;
  muteMsg.mMuteStreams.mAudio = muteAudio;
  muteMsg.mMuteStreams.mVideo = muteVideo;
  SendSignalingMessage(muteMsg.ToJsonStr(), [&, callback](const RespMsg &msg)->void {
    if (callback) {
      callback(msg);
    }
  }, timeOut);
}

void VHRoom::OnAddStreamToMix(std::string streamId, const VHEventCB & callback, uint64_t timeOut) {
  if (mSocket) {
    UnpublishMsg msg(streamId); /* same message struct with unpublish */
    mSocket->AddStreamToMix(msg.ToJsonStr(), [&, callback](const std::string &response)->void {
      LOGI("AddStreamToMix response %s", response.c_str());
      RespMsg resp;
      resp.ToObject(response);
      callback(resp);
    }, timeOut);
  }
  else {
    RespMsg respMsg;
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("AddStreamToMix failure, socket is empty!");
  }
}

void VHRoom::OnDelStreamFromMix(std::string streamId, const VHEventCB & callback, uint64_t timeOut) {
  if (mSocket) {
    UnpublishMsg msg(streamId); /* same message struct with unpublish */
    mSocket->DelStreamFromMix(msg.ToJsonStr(), [&, callback](const std::string &response)->void {
      LOGI("AddStreamToMix response %s", response.c_str());
      RespMsg resp;
      resp.ToObject(response);
      callback(resp);
    }, timeOut);
  }
  else {
    RespMsg respMsg;
    respMsg.mResult = "failed";
    respMsg.mCode = SocketError;
    respMsg.mMsg = "socket is empty";
    callback(respMsg);
    LOGE("DelStreamFromMix failure, socket is empty!");
  }
}

std::unordered_map<std::string, std::shared_ptr<VHStream>> VHRoom::GetRemoteStreams() {
  std::unordered_map<std::string, std::shared_ptr<VHStream>> streams;
  streams.clear();
  std::unique_lock<std::mutex> Rlock(mRemoteLock);
  streams = mRemoteStreams;
  Rlock.unlock();
  return streams;
}

void VHRoom::CreateRWLockWin() {
  LOGI("CreateRWLockWin");
  webrtc::RWLockWin::Create();
}

RoomOptions::RoomOptions() {
  maxAudioBW = 300;
  maxVideoBW = 300;
  defaultVideoBW = 300;
  token = "";
  upLogUrl = "";
  biz_role = 0;
  biz_id = "0";
  biz_des01 = 0;
  biz_des02 = 0;
  bu = "0";
  vid = "";
  uid = "";
  vfid = "";
  app_id = "0";
  attempts = 10;
  enableRoomReconnect = 0;
  autoSubscribe = false;
  version = vhall::VHRoom::vhallSDKVersion;
  release_date = vhall::VHRoom::vhallSDKReleaseData;
  platForm = "8";
  attribute = "";
  cascade = false;
}
NS_VH_END
