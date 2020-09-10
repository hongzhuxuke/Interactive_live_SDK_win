//
//  ec_data_model.cpp
//  VhallSignaling
//
//  Created by ilong on 2018/1/3.
//  Copyright © 2018年 ilong. All rights reserved.
//

#include "vh_data_message.h"
#include "common/vhall_define.h"
#include "common/base64.h"
#include "socketio/lib/rapidjson/include/rapidjson/rapidjson.h"
#include "socketio/lib/rapidjson/include/rapidjson/document.h"
#include "socketio/lib/rapidjson/include/rapidjson/stringbuffer.h"
#include "socketio/lib/rapidjson/include/rapidjson/prettywriter.h"
#include <sstream>

NS_VH_BEGIN

DataMsg::DataMsg(){
   
}

DataMsg::~DataMsg(){
   
}

bool RespMsg::ToObject(const std::string &json_str) {
  LOGD("json_str:%s", json_str.c_str());
  rapidjson::Document Doc;
  Doc.Parse<0>(json_str.c_str());
  if (Doc.HasParseError()) {
    LOGE("GetParseError%d\n", Doc.GetParseError());
    return false;
  }
  if (!Doc.IsArray() || Doc.Size() <= 0) {
    LOGE("this is not Array type.");
    return false;
  }

  rapidjson::Value& resp = Doc[0];
  if (!resp.IsObject()) {
    LOGE("RespMsg str not a object");
    return false;
  }
  if (resp.HasMember("result") && resp["result"].IsString()) {
    mResult = resp["result"].GetString();
  }
  if (resp.HasMember("code") && resp["code"].IsInt()) {
    mCode = resp["code"].GetInt();
  }
  if (resp.HasMember("msg") && resp["msg"].IsObject()) {
    rapidjson::Value & msg = resp["msg"];
    if (200 == mCode) {
      /*  success info */
    }
    else {
      if (msg.HasMember("message") && msg["message"].IsString()) {
        mMsg = msg["message"].GetString();
      }
    }
  }
  return true;
}

std::string RespMsg::ToJsonStr() {
  rapidjson::Document jsonDoc;
  rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator();

  rapidjson::Value resp(rapidjson::kArrayType);
  rapidjson::Value root(rapidjson::kObjectType);
  // rapidjson::Value result("failed", allocator);
  root.AddMember("result", rapidjson::StringRef(mResult.c_str()), allocator);
  root.AddMember("code", mCode, allocator);

  rapidjson::Value msg(rapidjson::kObjectType);
  // rapidjson::Value message("send time-out", allocator);
  msg.AddMember("message", rapidjson::StringRef(mMsg.c_str()), allocator);
  root.AddMember("msg", msg, allocator);
  resp.PushBack(root, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  resp.Accept(writer);
  return buffer.GetString();
}

bool DataMsg::ToObject(const std::string &json_str){
  return true;
}

std::string DataMsg::ToJsonStr(){
   return "";
}

MuteStreams::MuteStreams(){
   mAudio = false;
   mVideo = false;
}

User::Permissions::Permissions(){
   mControlhandlers = false;
   mOwner = false;
   mPublish = false;
   mRecord = false;
   mStats = false;
   mSubscribe = false;
}

TokenMsg::TokenMsg(const std::string &json_str, std::string & version, std::string & pf, std::string& attrbutes, bool cascade):
mToken(""),
mUrl(""),
mSdkVersion(version),
mPlatform(pf),
mRoomId(""),
mAttributes(attrbutes),
mCascade(cascade) {
  ToObject(json_str);
}

TokenMsg::~TokenMsg(){
   
}

bool TokenMsg::ToObject(const std::string &json_str){
   LOGD("json_str:%s", json_str.c_str());
   if (talk_base::Base64::IsBase64Encoded(json_str)) {
      mToken = talk_base::Base64::Decode(json_str, talk_base::Base64::DO_PARSE_ANY);
   }else{
      mToken = json_str;
   }
   LOGI("token json:%s",mToken.c_str());
   rapidjson::Document doc;                    /*  创建一个Document对象 rapidJson的相关操作都在Document类中 */
   doc.Parse<0>(mToken.c_str());               /*  通过Parse方法将Json数据解析出来 */
   if (doc.HasParseError()){
      LOGE("VHTokenMsg ParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
     LOGE("this is not object type.");
     return false;
   }
   
   std::stringstream pre;
   pre.str("");
   rapidjson::Value& valObject = doc["token"];
   if (valObject.IsObject()) {
     rapidjson::Value& valRoomId = valObject["roomId"];
     if (valRoomId.IsString()) {
       mRoomId = valRoomId.GetString();
     }
     rapidjson::Value& valUserId = valObject["userId"];
     if (valUserId.IsString()) {
       mUserId = valUserId.GetString();
     }
      rapidjson::Value& valString = valObject["host"];
      if (valString.IsString()) {
         host = valString.GetString();
      }
      bool secure = false;
      rapidjson::Value& valBool = valObject["secure"];
      if (valBool.IsBool()) {
         secure = valBool.GetBool();
      }
      if (secure) {
         pre << "wss://";
      }else{
         pre << "ws://";
      }
      pre << std::move(host);
      mUrl = pre.str();
   }
   /* update token msg */
   rapidjson::Document jsonDoc;                    /*  创建一个Document对象 rapidJson的相关操作都在Document类中 */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */
   rapidjson::Value root(rapidjson::kObjectType);
   rapidjson::Value version(mSdkVersion.c_str(), allocator);
   root.AddMember("version", version, allocator);
   rapidjson::Value pf(mPlatform.c_str(), allocator);
   root.AddMember("pf", pf, allocator);
   rapidjson::Value attributes(mAttributes.c_str(), allocator);
   root.AddMember("attributes", attributes, allocator);
   root.AddMember("cascade", mCascade, allocator);
   root.AddMember("token", doc, allocator);
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   root.Accept(writer);
   mToken = buffer.GetString();
   LOGI("mToken:\n%s\n", mToken.c_str());

   return true;
}

StreamMsg::StreamMsg() {
  mId = "";
  reconnect = true;
  mLocal = true;
  mAudio = true;
  mVideo = true;
  mData = true;
  mStreamAttributes.clear();
  mNumSpatialLayers = 0;
  mVideoCodecPreference = "VP8";
  mMix = true;
}

StreamMsg::~StreamMsg(){
   
}

TokenRespMsg::TokenRespMsg(const std::string &json_str):
mDefaultVideoBW(0),
mMaxVideoBW(0),
mP2p(false){
   mStreams.clear();
   ToObject(json_str);
   mCode = 0;
}

TokenRespMsg::~TokenRespMsg(){
   
}

std::string TokenRespMsg::IceServers::ToJsonStr(){
   /*  生成Json串 */
   rapidjson::Document jsonDoc;    /*  生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */
   rapidjson::Value root(rapidjson::kArrayType);
   if (mIceServers.size()<=0) {
      return "";
   }
   for (auto url:mIceServers) {
      rapidjson::Value urlStr(url.c_str(),allocator);
      root.PushBack(urlStr, allocator);
   }
   rapidjson::Value obj(rapidjson::kObjectType);
   obj.AddMember("urls", root, allocator);
   rapidjson::Value username(mUsername.c_str(),allocator);
   obj.AddMember("username",username, allocator);
   rapidjson::Value credential(mCredential.c_str(),allocator);
   obj.AddMember("credential",credential, allocator);
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   obj.Accept(writer);
   return buffer.GetString();
}

bool TokenRespMsg::ToObject(const std::string &json_str){
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;
   doc.Parse<0>(json_str.c_str());
   if (doc.HasParseError()){
      LOGE("VHTokenRespMsg ParseError%d\n",doc.GetParseError());
      return false;
   }
   if (doc.IsArray() && doc.Size() > 0) {
      rapidjson::Value& response = doc[0];
      if (!response.IsObject()) {
        LOGE("not a object");
        return false;
      }
      if (response.HasMember("result")&& response["result"].IsString()) {
         mRespType = response["result"].GetString();
      }
      if (response.HasMember("code") && response["code"].IsInt()) {
         mCode = response["code"].GetInt();
         if (200 != mCode) {
           if (response.HasMember("msg") && response["msg"].IsString()) {
             mResp = response["msg"].GetString();
           }
         }
      }
      if(response.IsObject()) {
        if (response.HasMember("msg") && response["msg"].IsObject()) {
          rapidjson::Value& resp = response["msg"];

          if (resp.HasMember("clientId") && resp["clientId"].IsString()) {
            mClientId = resp["clientId"].GetString();
          }
          if (resp.HasMember("id") && resp["id"].IsString()) {
            mId = resp["id"].GetString();
          }
          if (resp.HasMember("userIp") && resp["userIp"].IsString()) {
            mUserIp = resp["userIp"].GetString();
          }
          if (resp.HasMember("defaultVideoBW") && resp["defaultVideoBW"].IsInt()) {
            mDefaultVideoBW = resp["defaultVideoBW"].GetInt();
          }
          if (resp.HasMember("maxVideoBW") && resp["maxVideoBW"].IsInt()) {
            mMaxVideoBW = resp["maxVideoBW"].GetInt();
          }
          if (resp.HasMember("p2p") && resp["p2p"].IsBool()) {
            mMaxVideoBW = resp["p2p"].GetBool();
          }
          if (resp.HasMember("iceServers") && resp["iceServers"].IsArray()) {
            rapidjson::Value& iceServers = resp["iceServers"];
            for (int i = 0; i < iceServers.Size(); i++) {
              rapidjson::Value& iceServer = iceServers[i];
              if (iceServer.IsObject()) {
                if (iceServer.HasMember("url") && iceServer["url"].IsString()) {
                  mIceServers.mIceServers.push_back(iceServer["url"].GetString());
                }
                if (iceServer.HasMember("credential") && iceServer["credential"].IsString()) {
                  mIceServers.mCredential = iceServer["credential"].GetString();
                }
                if (iceServer.HasMember("username") && iceServer["username"].IsString()) {
                  mIceServers.mUsername = iceServer["username"].GetString();
                }
              }
            }
          }
          if (resp.HasMember("streams") && resp["streams"].IsArray()) {
            rapidjson::Value& streams = resp["streams"];
            for (int i = 0; i < streams.Size(); i++) {
              rapidjson::Value& stream = streams[i];
              if (stream.IsObject()) {
                auto vhStream = std::make_shared<StreamMsg>();
                if (stream.HasMember("id") && stream["id"].IsString()) {
                  vhStream->mId = stream["id"].GetString();
                }
                if (stream.HasMember("data") && stream["data"].IsBool()) {
                  vhStream->mData = stream["data"].GetBool();
                }
                if (stream.HasMember("audio") && stream["audio"].IsBool()) {
                  vhStream->mAudio = stream["audio"].GetBool();
                }
                if (stream.HasMember("video") && stream["video"].IsBool()) {
                  vhStream->mVideo = stream["video"].GetBool();
                }
                if (stream.HasMember("mix") && stream["mix"].IsBool()) {
                  vhStream->mMix = stream["video"].GetBool();
                }
                if (stream.HasMember("streamType") && stream["streamType"].IsInt()) {
                  vhStream->mStreamType = stream["streamType"].GetInt();
                }
                if (stream.HasMember("attributes") && stream["attributes"].IsString()) {
                  vhStream->mStreamAttributes = stream["attributes"].GetString();
                }
                if (stream.HasMember("muteStream") && stream["muteStream"].IsObject()) {
                  rapidjson::Value& muteStream = stream["muteStream"];
                  if (muteStream.HasMember("audio") && muteStream["audio"].IsBool()) {
                    vhStream->mMuteStreams.mAudio = muteStream["audio"].GetBool();
                  }
                  if (muteStream.HasMember("video") && muteStream["video"].IsBool()) {
                    vhStream->mMuteStreams.mVideo = muteStream["video"].GetBool();
                  }
                }
                if (stream.HasMember("simulcast") && stream["simulcast"].IsObject()) {
                  rapidjson::Value& simulcast = stream["simulcast"];
                  if (simulcast.HasMember("numSpatialLayers") && simulcast["numSpatialLayers"].IsInt()) {
                    vhStream->mNumSpatialLayers = simulcast["numSpatialLayers"].GetInt();
                  }
                }
                if (stream.HasMember("user") && stream["user"].IsObject()) {
                  rapidjson::Value& user = stream["user"];
                  if (user.HasMember("id") && user["id"].IsString()) {
                    vhStream->mUser.mId = user["id"].GetString();
                  }
                  if (user.HasMember("role") && user["role"].IsString()) {
                    vhStream->mUser.mRole = user["role"].GetString();
                  }
                  if (user.HasMember("attributes") && user["attributes"].IsString()) {
                    vhStream->mUser.mUserAttributes = user["attributes"].GetString();
                  }
                  if (user.HasMember("permissions") && user["permissions"].IsObject()) {
                    rapidjson::Value& permissions = user["permissions"];
                    if (permissions.HasMember("controlhandlers") && permissions["controlhandlers"].IsBool()) {
                      vhStream->mUser.mPermissions.mControlhandlers = permissions["controlhandlers"].GetBool();
                    }
                    if (permissions.HasMember("owner") && permissions["owner"].IsBool()) {
                      vhStream->mUser.mPermissions.mOwner = permissions["owner"].GetBool();
                    }
                    if (permissions.HasMember("publish") && permissions["publish"].IsBool()) {
                      vhStream->mUser.mPermissions.mPublish = permissions["publish"].GetBool();
                    }
                    if (permissions.HasMember("record") && permissions["record"].IsBool()) {
                      vhStream->mUser.mPermissions.mRecord = permissions["record"].GetBool();
                    }
                    if (permissions.HasMember("stats") && permissions["stats"].IsBool()) {
                      vhStream->mUser.mPermissions.mStats = permissions["stats"].GetBool();
                    }
                    if (permissions.HasMember("subscribe") && permissions["subscribe"].IsBool()) {
                      vhStream->mUser.mPermissions.mSubscribe = permissions["subscribe"].GetBool();
                    }
                  }
                }
                mStreams.push_back(vhStream);
              }
            }
          }
          if (resp.HasMember("user") && resp["user"].IsObject()) {
            rapidjson::Value& user = resp["user"];
            if (user.HasMember("id") && user["id"].IsString()) {
              mUser.mId = user["id"].GetString();
            }
            if (user.HasMember("role") && user["role"].IsString()) {
              mUser.mRole = user["role"].GetString();
            }
            if (user.HasMember("attributes") && user["attributes"].IsString()) {
              mUser.mUserAttributes = user["attributes"].GetString();
            }
            if (user.HasMember("permissions") && user["permissions"].IsObject()) {
              rapidjson::Value& permissions = user["permissions"];
              if (permissions.HasMember("controlhandlers") && permissions["controlhandlers"].IsBool()) {
                mUser.mPermissions.mControlhandlers = permissions["controlhandlers"].GetBool();
              }
              if (permissions.HasMember("owner") && permissions["owner"].IsBool()) {
                mUser.mPermissions.mOwner = permissions["owner"].GetBool();
              }
              if (permissions.HasMember("publish") && permissions["publish"].IsBool()) {
                mUser.mPermissions.mPublish = permissions["publish"].GetBool();
              }
              if (permissions.HasMember("record") && permissions["record"].IsBool()) {
                mUser.mPermissions.mRecord = permissions["record"].GetBool();
              }
              if (permissions.HasMember("stats") && permissions["stats"].IsBool()) {
                mUser.mPermissions.mStats = permissions["stats"].GetBool();
              }
              if (permissions.HasMember("subscribe") && permissions["subscribe"].IsBool()) {
                mUser.mPermissions.mSubscribe = permissions["subscribe"].GetBool();
              }
            }
          }
        }
      }
   }
   return true;
}

PublishMsg::PublishMsg() :
  mAudio(false),
  mVideo(false),
  mMinVideoBW(0),
  mMaxVideoBW(0),
  mData(false),
  mCreateOffer(false),
  mScreen(false),
  mSpatialLayers(0),
  mStreamAttributes(""),
  mScheme(""),
  mStreamType(2),
  mState("mcu"),
  mVideoCodecPreference("VP8"),
  mMix(true) {
   
}

PublishMsg::~PublishMsg(){
   
}

bool PublishMsg::ToObject(const std::string &json_str) {
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;                      /*  创建一个Document对象 rapidJson的相关操作都在Document类中 */
   doc.Parse<0>(json_str.c_str());               /*  通过Parse方法将Json数据解析出来 */
   if (doc.HasParseError()){
      LOGE("GetParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
      LOGE("this is not object type.");
      return false;
   }
   if (doc.HasMember("audio")&&doc["audio"].IsBool()) {
      mAudio = doc["audio"].GetBool();
   }
   if (doc.HasMember("video")&&doc["video"].IsBool()) {
      mVideo = doc["video"].GetBool();
   }
   if (doc.HasMember("data")&&doc["data"].IsBool()) {
      mData = doc["data"].GetBool();
   }
   if (doc.HasMember("screen") && doc["screen"].IsBool()) {
      mScreen = doc["screen"].GetBool();
   }
   if (doc.HasMember("createOffer") && doc["createOffer"].IsBool()) {
      mCreateOffer = doc["createOffer"].GetBool();
   }
   if (doc.HasMember("attributes") && doc["attributes"].IsString()) {
      mStreamAttributes = doc["attributes"].GetString();
   }
   if (doc.HasMember("scheme") && doc["scheme"].IsString()) {
      mScheme = doc["scheme"].GetString();
   }
   if (doc.HasMember("minVideoBW")&&doc["minVideoBW"].IsInt()) {
      mMinVideoBW = doc["minVideoBW"].GetInt();
   }
   if (doc.HasMember("state")&&doc["state"].IsString()) {
      mState = doc["state"].GetString();
   }
   if (doc.HasMember("streamType")&&doc["streamType"].IsInt()) {
      mStreamType = doc["streamType"].GetInt();
   }
   if (doc.HasMember("simulcast") && doc["simulcast"].IsObject()) {
      rapidjson::Value& simulcast = doc["simulcast"];
      if (simulcast.HasMember("numSpatialLayers") && simulcast["numSpatialLayers"].IsInt()) {
         mSpatialLayers = simulcast["numSpatialLayers"].GetInt();
      }
   }
   if (doc.HasMember("videoCodecPreference") && doc["videoCodecPreference"].IsString()) {
     mVideoCodecPreference = doc["videoCodecPreference"].GetString();
   }
   if (doc.HasMember("metadata")&&doc["metadata"].IsObject()) {
      rapidjson::Value& metadata = doc["metadata"];
      if (metadata.HasMember("actualName")&&metadata["actualName"].IsString()) {
         mMetaData.mActualName = metadata["actualName"].GetString();
      }
      if (metadata.HasMember("name")&&metadata["name"].IsString()) {
         mMetaData.mName = metadata["name"].GetString();
      }
      if (metadata.HasMember("type")&&metadata["type"].IsString()) {
         mMetaData.mType = metadata["type"].GetString();
      }
   }
   if (doc.HasMember("muteStream")&&doc["muteStream"].IsObject()) {
      rapidjson::Value& muteStream = doc["muteStream"];
      if (muteStream.HasMember("audio")&&muteStream["audio"].IsBool()) {
         mMuteStreams.mAudio = muteStream["audio"].GetBool();
      }
      if (muteStream.HasMember("video")&&muteStream["video"].IsBool()) {
         mMuteStreams.mVideo = muteStream["video"].GetBool();
      }
   }
   return true;
}

bool PublishRespMsg::ToObject(const std::string &json_str) {
  LOGD("json_str:%s", json_str.c_str());
  rapidjson::Document Doc;
  Doc.Parse<0>(json_str.c_str());
  if (Doc.HasParseError()) {
    LOGE("GetParseError%d\n", Doc.GetParseError());
    return false;
  }
  if (!Doc.IsArray() || Doc.Size() <= 0) {
    LOGE("this is not Array type.");
    return false;
  }
  rapidjson::Value& resp = Doc[0];
  if (!resp.IsObject()) {
    LOGE("Not a object");
    return false;
  }
  if (resp.HasMember("result") && resp["result"].IsString()) {
    mResult = resp["result"].GetString();
  }
  if (resp.HasMember("code") && resp["code"].IsInt()) {
    mCode = resp["code"].GetInt();
  }
  if (resp.HasMember("msg") && resp["msg"].IsObject()) {
    rapidjson::Value & msg = resp["msg"];
    if (200 == mCode) {
      if (msg.HasMember("streamId") && msg["streamId"].IsString()) {
        mStreamId = msg["streamId"].GetString();
      }
    }
    else {
      if (msg.HasMember("message") && msg["message"].IsString()) {
        mMsg = msg["message"].GetString();
      }
    }
  }
  return true;
}

std::string UnpublishMsg::ToJsonStr() {
  /*  生成Json串 */
  rapidjson::Document jsonDoc;    /*  生成一个dom元素Document */
  rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */

  rapidjson::Value root(rapidjson::kObjectType);
  rapidjson::Value streamId(mStreamId.c_str(), allocator);
  root.AddMember("streamId", streamId, allocator);
  rapidjson::Value reason(mReason.c_str(), allocator);
  root.AddMember("reason", reason, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  root.Accept(writer);
  return buffer.GetString();
}

std::string PublishMsg::ToJsonStr(){
   /*  生成Json串 */
   rapidjson::Document jsonDoc;    /*  生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */
   
   rapidjson::Value root(rapidjson::kObjectType);
   rapidjson::Value state(mState.c_str(),allocator);
   root.AddMember("state", state, allocator);
   root.AddMember("audio",mAudio,allocator);
   root.AddMember("video",mVideo,allocator);
   root.AddMember("data",mData,allocator);
   root.AddMember("streamType", mStreamType, allocator);
   root.AddMember("screen", mScreen, allocator);
   root.AddMember("createOffer", mCreateOffer, allocator);
   root.AddMember("minVideoBW", mMinVideoBW, allocator);
   root.AddMember("mix", mMix, allocator);

   rapidjson::Value attributes(mStreamAttributes.c_str(), allocator);
   root.AddMember("attributes", attributes, allocator);

   /*  simulcast */
   if (2 == mSpatialLayers || 3 == mSpatialLayers) {
     rapidjson::Value simulCast(rapidjson::kObjectType);
     simulCast.AddMember("numSpatialLayers", mSpatialLayers, allocator);
     root.AddMember("simulcast", simulCast, allocator);
   }
   /*  videoCodecPreference */
   root.AddMember("videoCodecPreference", rapidjson::StringRef(mVideoCodecPreference.c_str()), allocator);

   /*  metadata */
   rapidjson::Value metadata(rapidjson::kObjectType);/*  创建一个Object类型的元素 */
   rapidjson::Value type(mMetaData.mType.c_str(),allocator);
   metadata.AddMember("type", type, allocator);
   rapidjson::Value name(mMetaData.mName.c_str(),allocator);
   metadata.AddMember("name", name, allocator);
   rapidjson::Value actualName(mMetaData.mActualName.c_str(),allocator);
   metadata.AddMember("actualName", actualName, allocator);
   root.AddMember("metadata", metadata, allocator);
   
   /*  muteStream */
   rapidjson::Value muteStream(rapidjson::kObjectType); /* 创建一个Object类型的元素 */
   muteStream.AddMember("audio", mMuteStreams.mAudio, allocator);
   muteStream.AddMember("video", mMuteStreams.mVideo, allocator);
   root.AddMember("muteStream", muteStream, allocator);
   rapidjson::Value scheme(mScheme.c_str(),allocator);
   
   /* profile */
   rapidjson::Value stream_profile(rapidjson::kObjectType);
   rapidjson::Value video_size(rapidjson::kArrayType);//video size
   rapidjson::Value minWidth(rapidjson::kNumberType);
   minWidth.SetInt(videoOpt.minWidth);
   video_size.PushBack(minWidth, allocator);

   rapidjson::Value minHeight(rapidjson::kNumberType);
   minHeight.SetInt(videoOpt.minHeight);
   video_size.PushBack(minHeight, allocator);

   rapidjson::Value maxWidth(rapidjson::kNumberType);
   maxWidth.SetInt(videoOpt.maxWidth);
   video_size.PushBack(maxWidth, allocator);

   rapidjson::Value maxHeight(rapidjson::kNumberType);
   maxHeight.SetInt(videoOpt.maxHeight);
   video_size.PushBack(maxHeight, allocator);

   stream_profile.AddMember("videoSize", video_size, allocator);// add video size

   rapidjson::Value video_frameRate(rapidjson::kArrayType);//video framerate
   rapidjson::Value minFrameRate(rapidjson::kNumberType);
   minFrameRate.SetInt(videoOpt.minFrameRate);
   video_frameRate.PushBack(minFrameRate, allocator);

   rapidjson::Value maxFrameRate(rapidjson::kNumberType);
   maxFrameRate.SetInt(videoOpt.maxFrameRate);
   video_frameRate.PushBack(maxFrameRate, allocator);

   stream_profile.AddMember("videoFrameRate", video_frameRate, allocator); //add video framerate

   rapidjson::Value bit_rate(rapidjson::kArrayType);// bit rate
   rapidjson::Value minVideoBW(rapidjson::kNumberType);
   minVideoBW.SetInt(videoOpt.minVideoBW);
   bit_rate.PushBack(minVideoBW, allocator);

   rapidjson::Value maxVideoBW(rapidjson::kNumberType);
   maxVideoBW.SetInt(videoOpt.maxVideoBW);
   bit_rate.PushBack(maxVideoBW, allocator);

   stream_profile.AddMember("bitRate", bit_rate, allocator);//add bit rate

   root.AddMember("profile", stream_profile, allocator); // add profile

   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   root.Accept(writer);
   return buffer.GetString();
}

SignalingMessageVhallRespMsg::SignalingMessageVhallRespMsg(const std::string &json_str){
   mStreamId = "";
   mPeerId = "";
   mJson_str = json_str;
   ToObject(json_str);
}

SignalingMessageVhallRespMsg::~SignalingMessageVhallRespMsg(){
   
}

bool SignalingMessageVhallRespMsg::ToObject(const std::string &json_str){
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;                      /*  创建一个Document对象 rapidJson的相关操作都在Document类中 */
   doc.Parse<0>(json_str.c_str());               /*  通过Parse方法将Json数据解析出来 */
   if (doc.HasParseError()){
      LOGE("GetParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
      LOGE("this is not object type.");
      return false;
   }
   if (doc.HasMember("peerId")&&doc["peerId"].IsString()) {
      mPeerId = doc["peerId"].GetString();
   }
   if (doc.HasMember("streamId")&&doc["streamId"].IsString()) {
      mStreamId = doc["streamId"].GetString();
   }else{
      mStreamId = mPeerId;
   }
   if (doc.HasMember("mess")&&doc["mess"].IsObject()) {
      rapidjson::Value& mess = doc["mess"];
      if (mess.HasMember("type")&&mess["type"].IsString()) {
         mMess.mType = mess["type"].GetString();
      }
      if (mess.HasMember("agentId")&&mess["agentId"].IsString()) {
         mMess.mAgentId = mess["agentId"].GetString();
      }
      if (mess.HasMember("sdp")&&mess["sdp"].IsString()) {
         mMess.mSdp = mess["sdp"].GetString();
      }
   }
   return true;
}

SignalingMessageMsg::SignalingMessageMsg(){
   
}

SignalingMessageMsg::~SignalingMessageMsg(){
   
}

bool SignalingMessageMsg::ToObject(const std::string &json_str) {
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;
   doc.Parse<0>(json_str.c_str());
   if (doc.HasParseError()){
      LOGE("GetParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
      LOGE("this is not object type.");
      return false;
   }
   if (doc.HasMember("streamId")&&doc["streamId"].IsString()) {
      mStreamId = doc["streamId"].GetString();
   }
   if (doc.HasMember("msg")&&doc["msg"].IsObject()) {
      rapidjson::Value& msg = doc["msg"];
      if (msg.HasMember("type")&&msg["type"].IsString()) {
         mType = msg["type"].GetString();
      }
      if (mType=="offer") {
         if (msg.HasMember("sdp")&&msg["sdp"].IsString()) {
            mSdp = msg["sdp"].GetString();
         }
      }else{
         if (msg.HasMember("candidate")&&msg["candidate"].IsObject()) {
            rapidjson::Value& candidate = msg["candidate"];
            if (candidate.HasMember("sdpMLineIndex")&&candidate["sdpMLineIndex"].IsInt()) {
               mCandidate.mSdpMLineIndex = candidate["sdpMLineIndex"].GetInt();
            }
            if (candidate.HasMember("sdpMid")&&candidate["sdpMid"].IsString()) {
               mCandidate.mSdpMid = candidate["sdpMid"].GetString();
            }
            if (candidate.HasMember("candidate")&&candidate["candidate"].IsString()) {
               mCandidate.mCandidate = candidate["candidate"].GetString();
            }
         }
      }
   }
   return true;
}

std::string SignalingMessageMsg::ToJsonStr(){
   /*  生成Json串 */
   rapidjson::Document jsonDoc;    /*  生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */
   rapidjson::Value root(rapidjson::kObjectType);
   if (mType=="offer") {
      rapidjson::Value msg(rapidjson::kObjectType);/*  创建一个Object类型的元素 */
      rapidjson::Value type(mType.c_str(),allocator);
      msg.AddMember("type",type, allocator);
      rapidjson::Value sdp(mSdp.c_str(),allocator);
      msg.AddMember("sdp",sdp, allocator);
      root.AddMember("msg", msg, allocator);
   }else if(mType=="updatestream"){
      rapidjson::Value msg(rapidjson::kObjectType);
      rapidjson::Value type(mType.c_str(),allocator);
      msg.AddMember("type",type, allocator);
      rapidjson::Value config(rapidjson::kObjectType);
      rapidjson::Value muteStream(rapidjson::kObjectType);
      muteStream.AddMember("video", mMuteStreams.mVideo, allocator);
      muteStream.AddMember("audio", mMuteStreams.mAudio, allocator);
      config.AddMember("muteStream", muteStream, allocator);
      msg.AddMember("config", config, allocator);
      root.AddMember("msg", msg, allocator);
   }else{
      rapidjson::Value msg(rapidjson::kObjectType); /* 创建一个Object类型的元素 */
      rapidjson::Value candidate(rapidjson::kObjectType);/*  创建一个Object类型的元素 */
      candidate.AddMember("sdpMLineIndex",mCandidate.mSdpMLineIndex, allocator);
      rapidjson::Value sdpMid(mCandidate.mSdpMid.c_str(),allocator);
      candidate.AddMember("sdpMid",sdpMid, allocator);
      rapidjson::Value candidate1(mCandidate.mCandidate.c_str(),allocator);
      candidate.AddMember("candidate",candidate1, allocator);
      msg.AddMember("candidate",candidate, allocator);
      rapidjson::Value type(mType.c_str(),allocator);
      msg.AddMember("type", type, allocator);
      root.AddMember("msg", msg, allocator);
   }
   rapidjson::Value streamId(mStreamId.c_str(), allocator);
   root.AddMember("streamId", streamId, allocator);
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   root.Accept(writer);
   return buffer.GetString();
}

std::string SimulcastMsg::ToJsonStr() {
  /*  生成Json串 */
  rapidjson::Document jsonDoc;    /*  生成一个dom元素Document */
  rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */
  rapidjson::Value root(rapidjson::kObjectType);
  rapidjson::Value msg(rapidjson::kObjectType);
  rapidjson::Value type(mType.c_str(), allocator);
  msg.AddMember("type", type, allocator);
  rapidjson::Value config(rapidjson::kObjectType);
  config.AddMember("dual", mDual, allocator);
  msg.AddMember("config", config, allocator);
  root.AddMember("msg", msg, allocator);
  rapidjson::Value streamId(mStreamId.c_str(), allocator);
  root.AddMember("streamId", streamId, allocator);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  root.Accept(writer);
  return buffer.GetString();
}

SubscribeMsg::SubscribeMsg(const StreamMsg &stream):
mAudio(false),
mVideo(false),
mData(false),
mSlideShowMode(false) {
   mAudio = stream.mAudio;
   mVideo = stream.mVideo;
   mData = stream.mData;
   mStreamId = stream.mId;
   mSlideShowMode = false;
   mCreateOffer = false;
   mMetaData.mType = "subscriber";
   mMuteStreams.mAudio = false;
   mMuteStreams.mVideo = false;
   mDual = 1;
}

SubscribeMsg::~SubscribeMsg(){
   
}

bool SubscribeMsg::ToObject(const std::string &json_str){
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;
   doc.Parse<0>(json_str.c_str());
   if (doc.HasParseError()){
      LOGE("GetParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
      LOGE("this is not object type.");
      return false;
   }
   if (doc.HasMember("audio")&&doc["audio"].IsBool()) {
      mAudio = doc["audio"].GetBool();
   }
   if (doc.HasMember("video")&&doc["video"].IsBool()) {
      mVideo = doc["video"].GetBool();
   }
   if (doc.HasMember("data")&&doc["data"].IsBool()) {
      mData = doc["data"].GetBool();
   }
   if (doc.HasMember("streamId")&&doc["streamId"].IsString()) {
      mStreamId = doc["streamId"].GetString();
   }
   if (doc.HasMember("slideShowMode")&&doc["slideShowMode"].IsBool()) {
      mSlideShowMode = doc["slideShowMode"].GetBool();
   }
   if (doc.HasMember("createOffer") && doc["createOffer"].IsBool()) {
     mCreateOffer = doc["createOffer"].GetBool();
   }
   if (doc.HasMember("metadata")&&doc["metadata"].IsObject()) {
      rapidjson::Value& metadata = doc["metadata"];
      if (metadata.HasMember("actualName")&&metadata["actualName"].IsString()) {
         mMetaData.mActualName = metadata["actualName"].GetString();
      }
      if (metadata.HasMember("name")&&metadata["name"].IsString()) {
         mMetaData.mName = metadata["name"].GetString();
      }
      if (metadata.HasMember("type")&&metadata["type"].IsString()) {
         mMetaData.mType = metadata["type"].GetString();
      }
   }
   if (doc.HasMember("muteStream")&&doc["muteStream"].IsObject()) {
      rapidjson::Value& muteStream = doc["muteStream"];
      if (muteStream.HasMember("audio")&&muteStream["audio"].IsBool()) {
         mMuteStreams.mAudio = muteStream["audio"].GetBool();
      }
      if (muteStream.HasMember("video")&&muteStream["video"].IsBool()) {
         mMuteStreams.mVideo = muteStream["video"].GetBool();
      }
   }
   return true;
}

std::string SubscribeMsg::ToJsonStr(){
   /*  生成Json串 */
   rapidjson::Document jsonDoc;    /* 生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /* 获取分配器 */
   
   rapidjson::Value root(rapidjson::kObjectType);
   root.AddMember("audio",mAudio,allocator);
   root.AddMember("video",mVideo,allocator);
   root.AddMember("data",mData,allocator);
   //root.AddMember("createOffer", mCreateOffer, allocator);
   rapidjson::Value streamId(mStreamId.c_str(), allocator);
   root.AddMember("streamId", streamId, allocator);
   rapidjson::Value metadata(rapidjson::kObjectType); /* 创建一个Object类型的元素 */
   rapidjson::Value type(mMetaData.mType.c_str(),allocator);
   metadata.AddMember("type", type, allocator);
   root.AddMember("metadata", metadata, allocator);
   root.AddMember("slideShowMode", mSlideShowMode, allocator);
   rapidjson::Value muteStream(rapidjson::kObjectType); /* 创建一个Object类型的元素 */
   muteStream.AddMember("audio", mMuteStreams.mAudio, allocator);
   muteStream.AddMember("video", mMuteStreams.mVideo, allocator);
   root.AddMember("muteStream", muteStream, allocator);
   root.AddMember("dual", mDual, allocator);
   
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   root.Accept(writer);
   return buffer.GetString();
}

std::string UnSubscribeMsg::ToJsonStr() {
  rapidjson::Document jsonDoc;
  rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator();

  rapidjson::Value root(rapidjson::kObjectType);
  rapidjson::Value streamId(mStreamId.c_str(), allocator);
  root.AddMember("streamId", streamId, allocator);
  rapidjson::Value reason(mReason.c_str(), allocator);
  root.AddMember("reason", reason, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  root.Accept(writer);
  return buffer.GetString();
}

AddStreamRespMsg::AddStreamRespMsg(const std::string &json_str){
  mId = "";
  mLocal = false;
  mAudio = false;
  mData = false;
  mScreen = false;
  mUser;
  mMuteStreams.mAudio = false;
  mMuteStreams.mVideo = false;
  mStreamAttributes = "";
  mStreamType = 2; // 0: pure audio, 1: only video, 2: AV, 3: screen, 4: file
  mNumSpatialLayers = 0; // Single-Channel : not set, Multi-Channel 2
  mMix = true;
  ToObject(json_str);
}

AddStreamRespMsg::~AddStreamRespMsg(){
   
}

bool AddStreamRespMsg::ToObject(const std::string &json_str) {
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;
   doc.Parse<0>(json_str.c_str());
   if (doc.HasParseError()){
      LOGE("GetParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
      LOGE("this is not object type.");
      return false;
   }
   if (doc.HasMember("id")&&doc["id"].IsString()) {
      mId = doc["id"].GetString();
   }
   if (doc.HasMember("audio")&&doc["audio"].IsBool()) {
      mAudio = doc["audio"].GetBool();
   }
   if (doc.HasMember("video")&&doc["video"].IsBool()) {
      mVideo = doc["video"].GetBool();
   }
   if (doc.HasMember("mix") && doc["mix"].IsBool()) {
     mMix = doc["mix"].GetBool();
   }
   if (doc.HasMember("data")&&doc["data"].IsBool()) {
      mData = doc["data"].GetBool();
   }
   if (doc.HasMember("screen") && doc["screen"].IsBool()) {
      mScreen = doc["screen"].GetBool();
   }
   if (doc.HasMember("streamType")&&doc["streamType"].IsInt()) {
      mStreamType = doc["streamType"].GetInt();
   }
   if (doc.HasMember("attributes") && doc["attributes"].IsString()) {
      mStreamAttributes = doc["attributes"].GetString();
   }
   if (doc.HasMember("simulcast") && doc["simulcast"].IsObject()) {
     rapidjson::Value& simulcast = doc["simulcast"];
     if (simulcast.HasMember("numSpatialLayers") && simulcast["numSpatialLayers"].IsInt()) {
       mNumSpatialLayers = simulcast["numSpatialLayers"].GetInt();
     }
   }
   if (doc.HasMember("muteStream") && doc["muteStream"].IsObject()) {
     rapidjson::Value& muteStream = doc["muteStream"];
     if (muteStream.HasMember("audio") && muteStream["audio"].IsBool()) {
       mMuteStreams.mAudio = muteStream["audio"].GetBool();
     }
     if (muteStream.HasMember("video") && muteStream["video"].IsBool()) {
       mMuteStreams.mVideo = muteStream["video"].GetBool();
     }
   }
   if (doc.HasMember("user")&&doc["user"].IsObject()) {
      rapidjson::Value& user = doc["user"];
      if (user.HasMember("id")&&user["id"].IsString()) {
         mUser.mId = user["id"].GetString();
      }
      if (user.HasMember("role")&&user["role"].IsString()) {
         mUser.mRole = user["role"].GetString();
      }
      if (user.HasMember("attributes") && user["attributes"].IsString()) {
        mUser.mUserAttributes = user["attributes"].GetString();
      }
      if (user.HasMember("permissions")&&user["permissions"].IsObject()) {
         rapidjson::Value& permissions = user["permissions"];
         if (permissions.HasMember("publish")&&permissions["publish"].IsBool()) {
            mUser.mPermissions.mPublish = permissions["publish"].GetBool();
         }
         if (permissions.HasMember("subscribe")&&permissions["subscribe"].IsBool()) {
            mUser.mPermissions.mSubscribe = permissions["subscribe"].GetBool();
         }
         if (permissions.HasMember("record")&&permissions["record"].IsBool()) {
            mUser.mPermissions.mRecord = permissions["record"].GetBool();
         }
         if (permissions.HasMember("stats")&&permissions["stats"].IsBool()) {
            mUser.mPermissions.mStats = permissions["stats"].GetBool();
         }
         if (permissions.HasMember("controlhandlers")&&permissions["controlhandlers"].IsBool()) {
            mUser.mPermissions.mControlhandlers = permissions["controlhandlers"].GetBool();
         }
         if (permissions.HasMember("owner")&&permissions["owner"].IsBool()) {
            mUser.mPermissions.mOwner = permissions["owner"].GetBool();
         }
      }
   }
   return true;
}

RemoveStreamRespMsg::RemoveStreamRespMsg(const std::string &json_str):mId("")
{
  ToObject(json_str);
}

RemoveStreamRespMsg::~RemoveStreamRespMsg(){
   
}

bool RemoveStreamRespMsg::ToObject(const std::string &json_str){
   LOGD("json_str:%s", json_str.c_str());
   rapidjson::Document doc;
   doc.Parse<0>(json_str.c_str());
   if (doc.HasParseError()){
      LOGE("GetParseError%d\n",doc.GetParseError());
      return false;
   }
   if (!doc.IsObject()) {
      LOGE("this is not object type.");
      return false;
   }
   if (doc.HasMember("id")&&doc["id"].IsString()) {
      mId = doc["id"].GetString();
   }
   return true;
}

StreamMixedMsg::StreamMixedMsg(const std::string &json_str) {
     mId = "";
     mAudio = false;
     mVideo = false;
     mData = false;
     mScreen = false;
     mAttributes = "";
     mStreamType = -1; /*  0: pure audio, 1: only video, 2: AV, 3: screen, 4: file */
     mSpatialLayers = 0;; /*  Single-Channel : not set, Multi-Channel 2 */
     ToObject(json_str);
}
bool StreamMixedMsg::ToObject(const std::string &json_str) {
  LOGD("json_str:%s", json_str.c_str());
  rapidjson::Document doc;
  doc.Parse<0>(json_str.c_str());
  if (doc.HasParseError()) {
    LOGE("GetParseError%d\n", doc.GetParseError());
    return false;
  }
  if (!doc.IsObject()) {
    LOGE("this is not object type.");
    return false;
  }
  if (doc.HasMember("id") && doc["id"].IsString()) {
    mId = doc["id"].GetString();
  }
  if (doc.HasMember("audio") && doc["audio"].IsBool()) {
    mAudio = doc["audio"].GetBool();
  }
  if (doc.HasMember("video") && doc["video"].IsBool()) {
    mVideo = doc["video"].GetBool();
  }
  if (doc.HasMember("screen") && doc["screen"].IsBool()) {
    mScreen = doc["screen"].GetBool();
  }
  if (doc.HasMember("data") && doc["data"].IsBool()) {
    mData = doc["data"].GetBool();
  }
  if (doc.HasMember("attributes") && doc["attributes"].IsString()) {
    mAttributes = doc["attributes"].GetString();
  }
  if (doc.HasMember("user") && doc["user"].IsObject()) {
    rapidjson::Value& user = doc["user"];
    if (user.HasMember("id") && user["id"].IsString()) {
      mUser.mId = user["id"].GetString();
    }
    if (user.HasMember("role") && user["role"].IsString()) {
      mUser.mRole = user["role"].GetString();
    }
    if (user.HasMember("attributes") && user["attributes"].IsString()) {
      mUser.mUserAttributes = user["attributes"].GetString();
    }
    if (user.HasMember("permissions") && user["permissions"].IsObject()) {
      rapidjson::Value& permissions = user["permissions"];
      if (permissions.HasMember("publish") && permissions["publish"].IsBool()) {
        mUser.mPermissions.mPublish = permissions["publish"].GetBool();
      }
      if (permissions.HasMember("subscribe") && permissions["subscribe"].IsBool()) {
        mUser.mPermissions.mSubscribe = permissions["subscribe"].GetBool();
      }
      if (permissions.HasMember("record") && permissions["record"].IsBool()) {
        mUser.mPermissions.mRecord = permissions["record"].GetBool();
      }
      if (permissions.HasMember("stats") && permissions["stats"].IsBool()) {
        mUser.mPermissions.mStats = permissions["stats"].GetBool();
      }
      if (permissions.HasMember("controlhandlers") && permissions["controlhandlers"].IsBool()) {
        mUser.mPermissions.mControlhandlers = permissions["controlhandlers"].GetBool();
      }
      if (permissions.HasMember("owner") && permissions["owner"].IsBool()) {
        mUser.mPermissions.mOwner = permissions["owner"].GetBool();
      }
    }
  }
  if (doc.HasMember("simulcast") && doc["simulcast"].IsObject()) {
    rapidjson::Value& simulcast = doc["simulcast"];
    if (simulcast.HasMember("numSpatialLayers") && simulcast["numSpatialLayers"].IsInt()) {
      mSpatialLayers = simulcast["numSpatialLayers"].GetInt();
    }
  }
  if (doc.HasMember("streamType") && doc["streamType"].IsInt()) {
    mStreamType = doc["streamType"].GetInt();
  }
  return true;
}

ConfigRoomMsg::ConfigRoomMsg(){
   
}

ConfigRoomMsg::~ConfigRoomMsg(){
   
}

ConfigRoomMsg::Config::Config(){
   mBitrate = 0;
   mFramerate = 0;
   mResolution.clear();
}

std::string ConfigRoomMsg::ToJsonStr(){
   /* 生成Json串 */
   rapidjson::Document jsonDoc;    /* 生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /* 获取分配器 */
   std::string configStr;
   {
      rapidjson::Value config(rapidjson::kObjectType);
      config.AddMember("bitrate",mConfig.mBitrate,allocator);
      config.AddMember("framerate",mConfig.mFramerate,allocator);
      rapidjson::Value layoutMode(mConfig.mLayoutMode.c_str(),allocator);
      config.AddMember("layoutMode",layoutMode,allocator);
      rapidjson::Value publishUrl(mConfig.mPublishUrl.c_str(),allocator);
      config.AddMember("publishUrl",publishUrl,allocator);
      rapidjson::Value resolution(rapidjson::kArrayType);
      for (auto item:mConfig.mResolution) {
         rapidjson::Value value(item);
         resolution.PushBack(value,allocator);
      }
      config.AddMember("resolution",resolution,allocator);
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      config.Accept(writer);
      configStr = buffer.GetString();
   }
   rapidjson::Value roomConfig(rapidjson::kObjectType);
   rapidjson::Value roomId(mRoomId.c_str(),allocator);
   roomConfig.AddMember("roomID",roomId,allocator);
   rapidjson::Value config(configStr.c_str(),allocator);
   roomConfig.AddMember("config",config,allocator);
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   roomConfig.Accept(writer);
   return buffer.GetString();
}

MixConfigMsg::MixConfigMsg(){
   mConfigs.clear();
}

MixConfigMsg::~MixConfigMsg(){
   mConfigs.clear();
}

MixConfigMsg::Config::Config(){
   mVideo = false;
   mAuido = false;
}

std::string MixConfigMsg::ToJsonStr(){
   /* 生成Json串 */
   rapidjson::Document jsonDoc;    /* 生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /* 获取分配器 */
   std::string configStr;
   {
      rapidjson::Value configs(rapidjson::kArrayType);
      for (auto &item:mConfigs) {
         rapidjson::Value value(rapidjson::kObjectType);
         rapidjson::Value streamId(item->mStreamId.c_str(),allocator);
         value.AddMember("streamID",streamId,allocator);
         rapidjson::Value config(rapidjson::kObjectType);
         config.AddMember("audio",item->mAuido,allocator);
         config.AddMember("video",item->mVideo,allocator);
         value.AddMember("config", config, allocator);
         configs.PushBack(value,allocator);
      }
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      configs.Accept(writer);
      configStr = buffer.GetString();
   }
   rapidjson::Value mixConfig(rapidjson::kObjectType);
   rapidjson::Value roomId(mRoomId.c_str(),allocator);
   mixConfig.AddMember("roomID",roomId,allocator);
   rapidjson::Value config(configStr.c_str(),allocator);
   mixConfig.AddMember("config",config,allocator);
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   mixConfig.Accept(writer);
   return buffer.GetString();
}

MuteStreamMsg::MuteStreamMsg(){
  mStreamId ="0";
  mMuteStreams.mAudio = false;
  mMuteStreams.mVideo = false;
  mIsLocal = false;
}

MuteStreamMsg::~MuteStreamMsg(){
   
}

std::string MuteStreamMsg::ToJsonStr(){
   /*  生成Json串 */
   rapidjson::Document jsonDoc;    /*  生成一个dom元素Document */
   rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /*  获取分配器 */
   rapidjson::Value root(rapidjson::kObjectType);
   rapidjson::Value msg(rapidjson::kObjectType);
   rapidjson::Value config(rapidjson::kObjectType);
   rapidjson::Value muteStream(rapidjson::kObjectType);
   muteStream.AddMember("audio", mMuteStreams.mAudio, allocator);
   muteStream.AddMember("video", mMuteStreams.mVideo, allocator);

   config.AddMember("muteStream", muteStream, allocator);
   config.AddMember("localStream", mIsLocal, allocator);

   msg.AddMember("config", config, allocator);
   rapidjson::Value type("updatestream", allocator);
   msg.AddMember("type", type, allocator);

   root.AddMember("msg", msg, allocator);
   rapidjson::Value streamId(mStreamId.c_str(), allocator);
   root.AddMember("streamId", streamId, allocator);

   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   root.Accept(writer);
   return buffer.GetString();
}

ICEConnectFailedRespMsg::ICEConnectFailedRespMsg(const std::string &json_str) {
  ToObject(json_str);
}

ICEConnectFailedRespMsg::~ICEConnectFailedRespMsg() {

}

bool ICEConnectFailedRespMsg::ToObject(const std::string &json_str) {
  LOGD("json_str:%s", json_str.c_str());
  rapidjson::Document doc;
  doc.Parse<0>(json_str.c_str());
  if (doc.HasParseError()) {
    LOGE("GetParseError%d\n", doc.GetParseError());
    return false;
  }
  if (!doc.IsObject()) {
    LOGE("this is not object type.");
    return false;
  }
  if (doc.HasMember("streamId") && doc["streamId"].IsString()) {
    streamId = doc["streamId"].GetString();
  }
  if (doc.HasMember("type") && doc["type"].IsString()) {
    type = doc["type"].GetString();
  }
  return true;
}

GetOverseaMsg::GetOverseaMsg()
{
  mStreamId = "";
  mIsSub = "";
}

bool GetOverseaMsg::ToObject(const std::string & json_str) {

  return false;
}

std::string GetOverseaMsg::ToJsonStr() {
  /*  生成Json串 */
  rapidjson::Document jsonDoc;    /* 生成一个dom元素Document */
  rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /* 获取分配器 */

  rapidjson::Value root(rapidjson::kObjectType);
  root.AddMember("isSub", mIsSub, allocator);
  rapidjson::Value streamId(mStreamId.c_str(), allocator);
  root.AddMember("streamId", streamId, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  root.Accept(writer);
  return buffer.GetString();
}

bool OverseaResp::ToObject(const std::string &json_str) {
  LOGD("json_str:%s", json_str.c_str());
  rapidjson::Document Doc;
  Doc.Parse<0>(json_str.c_str());
  if (Doc.HasParseError()) {
    LOGE("GetParseError%d\n", Doc.GetParseError());
    return false;
  }
  if (!Doc.IsArray() || Doc.Size() <= 0) {
    LOGE("this is not Array type.");
    return false;
  }

  rapidjson::Value& resp = Doc[0];
  if (!resp.IsObject()) {
    LOGE("RespMsg str not a object");
    return false;
  }
  if (resp.HasMember("result") && resp["result"].IsString()) {
    mResult = resp["result"].GetString();
  }
  if (resp.HasMember("code") && resp["code"].IsInt()) {
    mCode = resp["code"].GetInt();
  }
  if (resp.HasMember("msg") && resp["msg"].IsObject()) {
    rapidjson::Value & msg = resp["msg"];
    if (200 == mCode) {
      /*  success info */
      if (msg.HasMember("result") && msg["result"].IsBool()) {
        mIsOversea = msg["result"].GetBool();
      }
    }
    else {
      if (msg.HasMember("message") && msg["message"].IsString()) {
        mMsg = msg["message"].GetString();
      }
    }
  }
  return true;
}

bool SetMaxUsrCountMsg::ToObject(const std::string & json_str) {
  return false;
}

std::string SetMaxUsrCountMsg::ToJsonStr() {
  /*  生成Json串 */
  rapidjson::Document jsonDoc;    /* 生成一个dom元素Document */
  rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator(); /* 获取分配器 */

  rapidjson::Value root(rapidjson::kObjectType);
  root.AddMember("maxNum", mMaxUsrCount, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  root.Accept(writer);
  return buffer.GetString();
}

NS_VH_END