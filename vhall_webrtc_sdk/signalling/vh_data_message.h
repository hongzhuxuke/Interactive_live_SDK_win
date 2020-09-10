//
//  vh_data_model.h
//  VhallSignaling
//
//  Created by ilong on 2018/1/3.
//  Copyright © 2018年 ilong. All rights reserved.
//

#ifndef vh_data_model_h
#define vh_data_model_h

#include "common/vhall_define.h"
#include "common/vhall_log.h"
#include "common/video_profile.h"
#include "tool/vhall_dev_format.h"
#include <cstdio>
#include <unordered_map>
#include <list>
#include <vector>
#include <string>
#include <memory>

NS_VH_BEGIN

class VH_DLL DataMsg {
public:
   DataMsg();
   virtual ~DataMsg();
   virtual bool ToObject(const std::string &json_str);
   virtual std::string ToJsonStr();
};

class VH_DLL RespMsg : public DataMsg {
public:
  RespMsg() {
    mCode = 0;
    mResult = "failed";
    mMsg = "";
  };
  virtual bool ToObject(const std::string &json_str) override;
  virtual std::string ToJsonStr() override;
  int mCode;
  std::string mResult;
  std::string mMsg;
};

class VH_DLL MuteStreams {
public:
   MuteStreams();
   ~MuteStreams(){};
   bool mAudio;
   bool mVideo;
};

class VH_DLL MetaData {
public:
   std::string mActualName;
   std::string mName;
   std::string mType;
};

class VH_DLL User:public DataMsg {
public:
   class VH_DLL Permissions {
   public:
      Permissions();
      ~Permissions(){};
      bool mControlhandlers;
      bool mOwner;
      bool mPublish;
      bool mRecord;
      bool mStats;
      bool mSubscribe;
   };
   std::string mId;
   std::string mRole;
   std::string mUserAttributes;
   Permissions mPermissions;
};

class VH_DLL TokenMsg:public DataMsg {
public:
   TokenMsg(const std::string &json_str, std::string & version, std::string & pf, std::string& attrbutes, bool cascade);
   //TokenMsg(const std::string &json_str) {};
   ~TokenMsg();
   virtual bool ToObject(const std::string &json_str) override;
   std::string mToken;
   std::string mUrl;
   std::string host;
   std::string mSdkVersion;
   std::string mPlatform;
   std::string mAttributes;
   std::string mRoomId;
   std::string mUserId;
   bool        mCascade;
};

class VH_DLL videoOptions {
public:
  videoOptions() {
    mProfileIndex = VIDEO_PROFILE_UNDEFINED;
    maxWidth = 320;
    minWidth = 320;
    maxHeight = 240;
    minHeight = 240;
    maxFrameRate = 15;
    minFrameRate = 10;
    maxVideoBW = 800;
    minVideoBW = 450;
    startVideoBW = 600;
    ratioW = 4;
    ratioH = 3;
    lockRatio = false;
  }
  ~videoOptions() {}
public:
  std::string devId = "";
  VideoProfileIndex mProfileIndex;
  INT32 maxWidth;
  INT32 minWidth;
  INT32 maxHeight;
  INT32 minHeight;
  INT32 maxFrameRate;
  INT32 minFrameRate;
  INT32 maxVideoBW;
  INT32 minVideoBW;
  INT32 startVideoBW;
  std::shared_ptr<VideoFormat> DshowFormat;
  VideoFormat publishFormat;
  bool lockRatio;
  INT32 ratioW;
  INT32 ratioH;
};

class VH_DLL StreamMsg:public DataMsg {
public:
   StreamMsg();
   virtual ~StreamMsg();
   std::string mId;
   bool mLocal;
   bool mAudio;
   bool mVideo;
   bool mData;
   bool mScreen;
   User mUser;
   MuteStreams mMuteStreams;
   std::string mStreamAttributes;
   int mStreamType; // 0: pure audio, 1: only video, 2: AV, 3: screen, 4: file 5:window capture 6:region share
   int mNumSpatialLayers = 0; // Single-Channel : not set, Multi-Channel 2
   std::string mVideoCodecPreference;
   bool reconnect; // reconnect
   videoOptions videoOpt;
   bool mMix;
   /* uplog info */
   bool mIsOversea = false;
   /* degrade mode */
   int mDegradationPreference = 0;  // 0 kDegradationDisabled; 1 kMaintainResolution; 2 kMaintainFramerate; 3 kBalanced(not used)
};

class VH_DLL TokenRespMsg:public DataMsg {
public:
   class VH_DLL IceServers:public DataMsg {
   public:
      virtual std::string ToJsonStr() override;
      std::string mCredential;
      std::string mUsername;
      std::list<std::string> mIceServers;
   };
   TokenRespMsg(const std::string &json_str);
   ~TokenRespMsg();
   virtual bool ToObject(const std::string &json_str) override;
   std::string mRespType; // result
   int mCode;
   std::string mResp;

   std::string mClientId;
   std::string mId;
   std::string mUserIp;
   int         mMaxVideoBW;
   int         mDefaultVideoBW;
   bool        mP2p;
   User        mUser;
   std::list<std::shared_ptr<StreamMsg>> mStreams;
   IceServers  mIceServers;
};

class VH_DLL PublishMsg:public DataMsg {
public:
   PublishMsg();
   ~PublishMsg();
   virtual bool ToObject(const std::string &json_str) override;
   virtual std::string ToJsonStr() override;
   std::string mState;
   bool mData;
   bool mAudio;
   bool mVideo;
   bool mScreen;
   std::string mStreamAttributes;
   MetaData  mMetaData;
   bool mCreateOffer;
   MuteStreams mMuteStreams;
   int mMinVideoBW;
   int mMaxVideoBW;
   std::string mScheme;
   int mStreamType;
   int mSpatialLayers;
   std::string mVideoCodecPreference;
   bool mMix;

   videoOptions videoOpt;
};

class VH_DLL PublishRespMsg :public RespMsg {
public:
  PublishRespMsg() {};
  virtual ~PublishRespMsg() {};
  virtual bool ToObject(const std::string &json_str) override;
  std::string mStreamId = "";
};

class VH_DLL UnpublishMsg :public DataMsg {
public:
  UnpublishMsg(std::string streamId) :mStreamId(streamId) {};
  virtual std::string ToJsonStr() override;
  std::string mStreamId = "";
  std::string mReason = "default";
};

class VH_DLL SignalingMessageVhallRespMsg:public DataMsg {
public:
   class VH_DLL Mess {
   public:
      std::string mType;
      std::string mAgentId;
      std::string mSdp;
   };
   SignalingMessageVhallRespMsg(const std::string &json_str);
   ~SignalingMessageVhallRespMsg();
   virtual bool ToObject(const std::string &json_str) override;
   Mess        mMess;
   std::string mStreamId;
   std::string mPeerId;
   std::string mJson_str;
};

class VH_DLL SignalingMessageMsg:public DataMsg {
public:
   class VH_DLL Candidate{
   public:
      int mSdpMLineIndex;
      std::string mSdpMid;
      std::string mCandidate;
   };
   SignalingMessageMsg();
   ~SignalingMessageMsg();
   virtual bool ToObject(const std::string &json_str) override;
   virtual std::string ToJsonStr() override;
   int mDual = 0;
   std::string mStreamId;
   std::string mType;
   Candidate mCandidate;
   MuteStreams mMuteStreams;
   std::string mSdp;
};

class VH_DLL SimulcastMsg :public DataMsg {
public:
  virtual std::string ToJsonStr() override;
  int mDual = 0;
  std::string mStreamId;
  std::string mType;
};

class VH_DLL SubscribeOption {
public:
  bool mMuteAudio = false;
  bool mMuteVideo = false;
  int  mDual = 1;
};

class VH_DLL GetOverseaMsg :public DataMsg {
public:
  GetOverseaMsg();
  ~GetOverseaMsg() {};
  virtual bool ToObject(const std::string &json_str) override;
  virtual std::string ToJsonStr() override;
  std::string mStreamId;
  bool        mIsSub;
};

class VH_DLL SetMaxUsrCountMsg:public DataMsg {
public:
  SetMaxUsrCountMsg() {};
  ~SetMaxUsrCountMsg() {};
  virtual bool ToObject(const std::string &json_str) override;
  virtual std::string ToJsonStr() override;
  int mMaxUsrCount = 5;
};

class VH_DLL OverseaResp: public RespMsg {
public:
  OverseaResp() {
    mCode = 0;
    mResult = "failed";
    mMsg = "not defined";
    mIsOversea = false;
  };
  ~OverseaResp() {};
  virtual bool ToObject(const std::string &json_str) override;
  bool mIsOversea;
};

class VH_DLL SubscribeMsg:public DataMsg {
public:
   //SubscribeMsg(){};
   SubscribeMsg(const StreamMsg &stream);
   ~SubscribeMsg();
   virtual bool ToObject(const std::string &json_str) override;
   virtual std::string ToJsonStr() override;
   std::string mStreamId;
   bool mAudio;
   bool mVideo;
   bool mData;
   std::string browser = "chrome-stable"; /* 浏览器类型（非web端可不填），字符串类型 */
   bool mCreateOffer = false;
   MetaData  mMetaData;
   MuteStreams mMuteStreams;
   bool mSlideShowMode = false;
   int  mDual; /* 订阅双流选项，0 为订阅小流；1 为订阅大流(默认值) */
};

class VH_DLL UnSubscribeMsg: public DataMsg {
public:
  UnSubscribeMsg() {};
  std::string ToJsonStr();

  std::string mStreamId;
  std::string mReason;
};

class VH_DLL AddStreamRespMsg:public StreamMsg {
public:
   AddStreamRespMsg(const std::string &json_str);
   ~AddStreamRespMsg();
   virtual bool ToObject(const std::string &json_str) override;
   std::string mId;
   bool mLocal = false;
   bool mAudio;
   bool mVideo;
   bool mData;
   bool mScreen;
   User mUser;
   MuteStreams mMuteStreams;
   std::string mStreamAttributes;
   int mStreamType; // 0: pure audio, 1: only video, 2: AV, 3: screen, 4: file
   int mNumSpatialLayers = 0; // Single-Channel : not set, Multi-Channel 2
   bool mMix;
};

class VH_DLL RemoveStreamRespMsg:public DataMsg {
public:
   RemoveStreamRespMsg(const std::string &json_str);
   ~RemoveStreamRespMsg();
   virtual bool ToObject(const std::string &json_str) override;
   std::string mId;
};

class VH_DLL StreamMixedMsg :public DataMsg {
public:
  StreamMixedMsg(const std::string &json_str);
  virtual ~StreamMixedMsg() {};
  virtual bool ToObject(const std::string &json_str);
public:
  std::string mId;
  bool mAudio;
  bool mVideo;
  bool mData;
  bool mScreen;
  std::string mAttributes;
  User mUser;
  int mStreamType; // 0: pure audio, 1: only video, 2: AV, 3: screen, 4: file
  int mSpatialLayers; // Single-Channel : not set, Multi-Channel 2
};

class VH_DLL ConfigRoomMsg:public DataMsg {

public:
   class VH_DLL Config {
   public:
      Config();
      int mBitrate;
      int mFramerate;
      std::string mLayoutMode;
      std::string mPublishUrl;
      std::vector<int> mResolution;
   };
   ConfigRoomMsg();
   ~ConfigRoomMsg();
   virtual std::string ToJsonStr() override;
   std::string mRoomId;
   Config      mConfig;
};

class VH_DLL MixConfigMsg:public DataMsg {
   
public:
   class VH_DLL Config {
   public:
      Config();
      bool mAuido;
      bool mVideo;
      std::string mStreamId;
   };
   MixConfigMsg();
   ~MixConfigMsg();
   virtual std::string ToJsonStr() override;
   std::list<std::shared_ptr<Config>> mConfigs;
   std::string mRoomId;
};

class VH_DLL MuteStreamMsg:public DataMsg {

public:
   MuteStreamMsg();
   ~MuteStreamMsg();
   virtual std::string ToJsonStr() override;
   std::string mStreamId;
   MuteStreams mMuteStreams;
   bool        mIsLocal;
};

class VH_DLL ICEConnectFailedRespMsg :public DataMsg {
public:
  ICEConnectFailedRespMsg(const std::string &json_str);
  ~ICEConnectFailedRespMsg();
  virtual bool ToObject(const std::string &json_str) override;
  std::string streamId;
  std::string type;
};

NS_VH_END

#endif /* ec_data_model_h */
