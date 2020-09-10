//
//  ec_room.hpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//

#ifndef vh_room_h
#define vh_room_h

#include <cstdio>
#include <unordered_map>
#include <list>
#include <map>
#include <mutex>
#include <atomic>
#include "vh_events.h"
#include "common/vhall_define.h"
#include "vh_data_message.h"
#include "common/video_profile.h"
#include "Event/BroadCastEvents.h"
#define DST 10 // DefaultSignallingTimeOut

namespace rtc {
  class Thread;
  class MsgHandler;
  class MessageHandler;
  class SubscribeOption;
}

NS_VH_BEGIN
typedef std::function<void(const std::string& resp)> VHEventCallback;
typedef std::function<void(const RespMsg& resp)> VHEventCB;
typedef std::function<void(const PublishRespMsg& resp)> VHPublishEventCB;

enum ErrNum {
  Reserved = 40000,
  SocketError,
  StreamIsEmpty,
  ParamsError,
  StreamPublished,
  StreamSubscribed,
  StreamNotSubscribed,
  RoomNotConnected,
  ThreadNotInitialized,
};

class VHStream;
class VHSignalingChannel;
class VHLogReport;

class VH_DLL RoomOptions {
public:
  RoomOptions();
  ~RoomOptions() {};
public:
  UINT32 maxAudioBW;
  UINT32 maxVideoBW;
  UINT32 defaultVideoBW;
  std::string token;
  std::string upLogUrl;
  int biz_role;
  std::string biz_id;
  int biz_des01;
  int biz_des02;
  std::string bu;
  std::string uid;
  std::string vid;
  std::string vfid;
  std::string app_id;
  int attempts; /* times of trying reconnect */
  bool enableRoomReconnect;
  bool autoSubscribe;
  bool cascade;

  std::string platForm;;
  std::string attribute;

  std::string version;
  std::string release_date;
};

struct PublishOptions {
  int mMinVideoBW;
  int mMaxVideoBW;
  std::string mScheme;
  std::string mState;
  int mStreamType;
  MetaData  mMetaData;
  MuteStreams mMuteStreams;
};

class RoomBroadCastOpt {
public:
  RoomBroadCastOpt() {
    mProfileIndex = BROADCAST_VIDEO_PROFILE_UNDEFINED;
    width = 1280;
    height = 720;
    framerate = 20;
    bitrate = 1000;
    publishUrl = "";
    layoutMode = 8;
    precast_pic_exist = true;
    backgroundColor = "0x333338";
    borderColor = "0x666666";
    borderWidth = 2;
    borderExist = false;
  }
public:
  BroadCastVideoProfileIndex mProfileIndex;
  UINT32 width;
  UINT32 height;
  UINT32 framerate;
  UINT32 bitrate;
  UINT32 layoutMode;
  int customizedYM = 0;
  bool precast_pic_exist;
  std::string publishUrl;
  std::string backgroundColor;
  std::string borderColor;
  int borderWidth;
  bool borderExist;
};

class VH_DLL VHRoom
  : public EventDispatcher, 
    public std::enable_shared_from_this<VHRoom> {
public:
   VHRoom(struct RoomOptions &RoomOpt);
   ~VHRoom();
   bool Connect();
   bool Disconnect();
   bool GetOversea(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   // TODO Add PublishOptions for publish
   // void Publish(std::shared_ptr<VHStream> stream, PublishOptions &publish_opt);
   void Publish(std::shared_ptr<VHStream> &stream,const VHPublishEventCB &callback = [](const PublishRespMsg & resp)->void {}, uint64_t timeOut = DST, int delay = 0);
   void UnPublish(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void SendSignalingMessage(const std::string &msg, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void SetMaxUsrCount(const int maxUsrCount, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void Subscribe(std::shared_ptr<VHStream> &stream, const SubscribeOption* option = nullptr, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST, int delay = 0);
   void Subscribe(std::string stream_id, const SubscribeOption* option = nullptr, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST, int delay = 0);
   void UnSubscribe(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void UnSubscribe(std::string &stream_id, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void SetSimulCast(std::shared_ptr<VHStream> &stream, const int & dual, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void DispatchStreamSubscribed(std::shared_ptr<VHStream> stream);
   void StreamFailed(std::shared_ptr<VHStream> stream, std::string &msg, std::string reason);
   void MuteStream(std::shared_ptr<VHStream> &stream, bool muteAudio, bool muteVideo, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void AddStreamToMix(std::string streamId, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void DelStreamFromMix(std::string streamId, const VHEventCB &callback = [](const RespMsg & resp)->void {});

   // about BroadCast
   void configRoomBroadCast(RoomBroadCastOpt& Options, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void setMixLayoutMode(UINT32 configStr, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void setMixLayoutMainScreen(const char *streamID, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void startRoomBroadCast(const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void stopRoomBroadCast(const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = DST);
   void ClearRemoteStreams();
   void ClearLocalStream();
   std::string GetIceServers();
   int isLocalStream(const char* id);

   std::unordered_map<std::string, std::shared_ptr<VHStream>> GetRemoteStreams();
private:
   /* work method */
   void ConnectedResp(std::string msg);
   void OnConnectedResp(std::string & msg);
   void OnConnect();
   void OnDisconnect();
   void OnGetOversea(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnPublish(std::shared_ptr<VHStream> &stream, const VHPublishEventCB &callback = [](const PublishRespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnUnPublish(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0, std::string reason = "default");
   void OnSendSignalingMessage(const std::string &msg, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnSetMaxUsrCount(const int &maxUsrCount, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnSubscribe(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnUnSubscribe(std::shared_ptr<VHStream> &stream, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0, std::string reason = "default");
   void OnSetSimulCast(std::shared_ptr<VHStream> &stream, const int & dual, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnconfigRoomBroadCast(RoomBroadCastOpt& Options, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnsetMixLayoutMode(UINT32 configStr, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnsetMixLayoutMainScreen(const char *streamID, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnstartRoomBroadCast(const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnstopRoomBroadCast(const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnSocketOnNetworkChanged(BaseEvent& event);
   void OnStreamFailed(std::shared_ptr<VHStream> stream, std::string &msg, std::string reason);
   void OnMuteStream(std::shared_ptr<VHStream> &stream, bool muteAudio, bool muteVideo, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnAddStreamToMix(std::string streamId, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
   void OnDelStreamFromMix(std::string streamId, const VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);

   void RemoveStream(std::shared_ptr<VHStream> stream);
   // Function that handles server events
   void SocketOnAddStream(BaseEvent& event);
   void SocketOnVhallMessage(BaseEvent& event);
   void SocketOnRemoveStream(BaseEvent& event);
   void SocketOnDisconnect(BaseEvent& event);
   void SocketOnReconnected(BaseEvent& event);
   void SocketOnICEConnectionFailed(BaseEvent& event);
   void SocketOnError(BaseEvent& event);
   void SocketOnStreamMixed(BaseEvent& event);
   /* sync server events */
   void OnSocketOnVhallMessage(BaseEvent& event);

   /* network */
   void SocketOnNetworkChanged(BaseEvent& event);
   void SocketReconnecting(BaseEvent& event);
   void SocketConnecting(BaseEvent& event);
   void SocketRecover(BaseEvent& event);
   void OnClearLocalStream();
   void CleanStream();
   void OnClearRemoteStreams();
   void ClearAll();
   void OnClearAll();
   // fix webrtc bug
   void CreateRWLockWin();

   void autoSubscrbie(std::shared_ptr<VHStream> stream, int delay = 0);
   void autoSubscrbieAll();
public:
  std::string mRoomId;
  static const char vhallSDKVersion[];
  static const char vhallSDKReleaseData[];
private:
   int         mMaxVideoBW;
   int         mDefaultVideoBW;
   bool        mP2p;
   VhallConnectState mState;
   BroadCastProfileList mBroadCastProfileList;
   struct RoomOptions spec;
   std::shared_ptr<VHSignalingChannel> mSocket;
   std::shared_ptr<rtc::MessageHandler> mMsgHandler;
   std::unique_ptr<rtc::Thread> mWorkThread;
   std::mutex                   mThreadMtx;
   std::atomic<bool>            mStopFlag;
   std::unordered_map<std::string, std::shared_ptr<VHStream>> mRemoteStreams;
   std::unordered_map<std::string, std::shared_ptr<VHStream>> mLocalStreams;
   std::list<std::shared_ptr<VHStream>> mStreamList;
   TokenRespMsg::IceServers mIceServers;
   User     mUser;
   std::mutex mRemoteLock;
   std::mutex mLocalLock;
   /* uplog */
   std::shared_ptr<VHLogReport> reportLog;
   BroadCastEvents broad_cast_events_;
   friend class rtc::MsgHandler;
};

NS_VH_END
#endif /* vh_room_h */
