#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <windows.h>
#include <ShlObj.h>

#include "json/json.h"
#include "common/base64.h"
//#include "common/vhall_httpclient.h" 
#include "signalling/BaseStack/BaseStack.h"
#include "signalling/vh_stream.h"
#include "vh_uplog.h"
#include "common/urlParse.h"
#include "3rdparty/VhallNetWork/VhallNetWork/VhallNetWork/VhallNetWorkInterface.h"

NS_VH_BEGIN

VHLogReport::VHLogReport(struct upLogOptions opt) {
  sessionId = random();
  prefixK = "";
  url = opt.url != "" ? opt.url : "https://t-dc.e.vhall.com/login";
  Url oriUrl(url.c_str());
  std::string path = oriUrl.GetPath();
  monitorUrl = url.substr(0, url.find(path)) + "/monitor";
  biz_role = opt.biz_role;
  biz_id = opt.biz_id;
  biz_des01 = opt.biz_des01;
  biz_des02 = 2;
  busynessUnit = opt.bu;
  device_type = opt.device_type;
  osv = opt.osv;
  pf = "8";
  ver = opt.version;
  uid = opt.uid;
  aid = opt.aid;
  vid = opt.vid;
  vfid = opt.vfid;
  app_id = opt.app_id;
  sd = opt.sd;
  message = "-";
  release_date = opt.release_date;
  //ZeroMemory(&streamDetail, sizeof(streamDetail));
  mUpThread = rtc::Thread::Create();
  if (mUpThread) {
    mUpThread->Start();
  }
  else {
    LOGE("init uplog thread failed");
  }
  
  /* http manager */
  TCHAR fullPath[MAX_PATH] = { 0 };
  SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullPath);
  httpManager = GetHttpManagerInstance("vh_rtc_sdk");
  httpManager->Init(fullPath);
}

VHLogReport::~VHLogReport() {
  LOGI("~VHLogReport");
  if (mUpThread) {
    LOGI("stop task Thread");
    mUpThread->Clear(this);
    mUpThread->Stop();
    mUpThread.reset();
  }
  LOGI("~VHLogReport end");
}

void VHLogReport::uplogDataFlow() {
  /* up play/push data flow */
  playMessageLog();
  publishMessageLog();
  if (mUpThread) {
    mUpThread->PostDelayed(RTC_FROM_HERE, 30000, this, UP_DATAFLOW, nullptr);
  }
  else {
    LOGE("upLog failed, could not find UpThread");
  }
}

void VHLogReport::start() {
  /* start up data flow */
  if (mUpThread) {
    mUpThread->PostDelayed(RTC_FROM_HERE, 30000, this, UP_DATAFLOW, nullptr);
    mUpThread->PostDelayed(RTC_FROM_HERE, 15000, this, UP_VOLUME, nullptr);
    mUpThread->PostDelayed(RTC_FROM_HERE, 3000, this, UP_QUALITYSTATS, nullptr);
  }
  else {
    LOGE("upLog failed, could not find UpThread");
  }
}

void VHLogReport::disconnectSignallingLog() {
  std::string k = "182002";
  std::string id = random();
  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["uid"] = Json::Value(uid);
  dictdata["aid"] = Json::Value(aid);
  dictdata["cid"] = Json::Value(cid);
  dictdata["bu"] = Json::Value(busynessUnit);
  dictdata["ld"] = Json::Value(GetCurTsInMS());
  dictdata["s"] = Json::Value(sessionId);
  dictdata["pf"] = Json::Value(pf);
  dictdata["ver"] = Json::Value(ver);
  dictdata["dt"] = Json::Value("windows");
  dictdata["_m"] = Json::Value(message);
  dictdata["osv"] = Json::Value(osv);
  Json::FastWriter root;
  std::string token = talk_base::Base64::Encode(root.write(dictdata));
  /* 计费系统上报 */
  std::string upUrl = url + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
  /* 互动监控系统上报 */
  upUrl = monitorUrl + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
}

void VHLogReport::playMessageLog() {
  if (!mRemoteStreams.empty()) {
    for (auto iter = mRemoteStreams.begin(); iter != mRemoteStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      auto VideoSsrcIt = mRemoteVideoSsrc.find(vhStream->GetID());
      if (VideoSsrcIt == mRemoteVideoSsrc.end()) {
        LOGE("VideoSsrcIt == mRemoteVideoSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<ReceiveVideoSsrc> videoSsrc = VideoSsrcIt->second;
      if (!videoSsrc) {
        LOGE("null videoSsrc, id :%s", vhStream->GetID());
        continue;
      }
      auto AudioSsrcIt = mRemoteAudioSsrc.find(vhStream->GetID());
      if (AudioSsrcIt == mRemoteAudioSsrc.end()) {
        LOGE("VideoSsrcIt == mRemoteAudioSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<ReceiveAudioSsrc> audioSsrc = AudioSsrcIt->second;
      if (!videoSsrc) {
        LOGE("null audioSsrc, id :%s", vhStream->GetID());
        continue;
      }
      vhStream->GetVideoSubscribeStates(videoSsrc.get());
      vhStream->GetAudioSubscribeStates(audioSsrc.get());
      videoSsrc->calc();
      audioSsrc->calc();

      if (videoSsrc->CalcData.duration > 20000 && videoSsrc->CalcData.LastReceivedBytesSeg <= 0) {
        /* 20s未能接收到视频数据 */
        if (!vhStream->mLocal && vhStream->mStreamType != 0 && !vhStream->mMuteStreams.mVideo) {
          LOGE("videoReceiveLost, id: %s, LastReceivedBytesSeg:%ld", vhStream->GetID(), videoSsrc->CalcData.LastReceivedBytesSeg);
          BaseEvent event;
          event.mType = VIDEO_RECEIVE_LOST;
          vhStream->DispatchEvent(event);
        }
      }
      /*struct Bwe OverallBwe;
      vhStream->GetOverallBwe(&OverallBwe);*/

      std::string k = "182003";
      std::string id = random();
      Json::Value dictdata(Json::ValueType::objectValue);
      dictdata["s"] = Json::Value(sessionId);
      uint64 totalTime = vhStream->GetSteadyTimeStamp() - vhStream->upLogTime;
      dictdata["tt"] = Json::Value(totalTime);
      dictdata["uid"] = Json::Value(uid);
      dictdata["aid"] = Json::Value(aid);
      dictdata["cid"] = Json::Value(cid);
      dictdata["_m"] = Json::Value(message);
      dictdata["ld"] = Json::Value(GetCurTsInMS());
      dictdata["p"] = Json::Value(vhStream->GetID());
      dictdata["pf"] = Json::Value(pf);
      dictdata["ver"] = Json::Value(ver);
      dictdata["dt"] = Json::Value("windows");
      dictdata["osv"] = Json::Value(osv);
      dictdata["os"] = Json::Value(vhStream->mIsOversea);
      dictdata["errorcode"] = Json::Value(2002);
      dictdata["vtype"] = Json::Value(vhStream->mStreamType);
      dictdata["videoWidth"] = Json::Value(videoSsrc->FrameWidthReceived);
      dictdata["videoHeight"] = Json::Value(videoSsrc->FrameHeightReceived);
      int64_t bitrate = (videoSsrc->CalcData.Bitrate + audioSsrc->CalcData.Bitrate) / 1000;
      dictdata["bitrate"] = Json::Value(bitrate);
      int64_t tf = (videoSsrc->CalcData.LastReceivedBytesSeg + audioSsrc->CalcData.LastReceivedBytesSeg) * 8 / 1000;
      dictdata["tf"] = Json::Value(tf);
      int64_t uf = (videoSsrc->CalcData.LastSentBytesSeg + audioSsrc->CalcData.LastSentBytesSeg) * 8 / 1000;
      dictdata["uf"] = Json::Value(uf);
      dictdata["biz_role"] = Json::Value(biz_role);
      dictdata["biz_id"] = Json::Value(biz_id);
      dictdata["biz_des01"] = Json::Value(biz_des01);
      dictdata["biz_des02"] = Json::Value(biz_des02);
      dictdata["bu"] = Json::Value(busynessUnit);
      dictdata["vid"] = Json::Value(vid);
      dictdata["vfid"] = Json::Value(vfid);
      dictdata["app_id"] = Json::Value(app_id);
      dictdata["sd"] = Json::Value(sd);
      vhStream->upLogTime = vhStream->GetSteadyTimeStamp();
      Json::FastWriter root;
      std::string token = talk_base::Base64::Encode(root.write(dictdata));
      std::string upUrl = url + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId +"&bu=" + busynessUnit + "&token=" + token;
      GetToSite(upUrl);
    }
  }

  FlushRemoteStreamList();
}

void VHLogReport::playMonitorLog(Json::Value& report_array) {
  if (!mRemoteStreams.empty()) {
    for (auto iter = mRemoteStreams.begin(); iter != mRemoteStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      auto VideoSsrcIt = mRemoteVideoSsrcMonitor.find(vhStream->GetID());
      if (VideoSsrcIt == mRemoteVideoSsrcMonitor.end()) {
        LOGE("VideoSsrcIt == mRemoteVideoSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<ReceiveVideoSsrc> videoSsrc = VideoSsrcIt->second;
      if (!videoSsrc) {
        LOGE("null videoSsrc, id :%s", vhStream->GetID());
        continue;
      }
      auto AudioSsrcIt = mRemoteAudioSsrcMonitor.find(vhStream->GetID());
      if (AudioSsrcIt == mRemoteAudioSsrcMonitor.end()) {
        LOGE("VideoSsrcIt == mRemoteAudioSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<ReceiveAudioSsrc> audioSsrc = AudioSsrcIt->second;
      if (!videoSsrc) {
        LOGE("null audioSsrc, id :%s", vhStream->GetID());
        continue;
      }
      vhStream->GetVideoSubscribeStates(videoSsrc.get());
      vhStream->GetAudioSubscribeStates(audioSsrc.get());
      videoSsrc->calc();
      audioSsrc->calc();

      /* 带宽预测信息 */
      Bwe bwe;
      vhStream->GetOverallBwe(&bwe);

      /* 监控质量统计 */ //  dictdata[""] = Json::Value();
      std::string k = ulQualityStats;
      Json::Value dictdata(Json::ValueType::objectValue);
      dictdata["s"] = Json::Value(sessionId);
      dictdata["ld"] = Json::Value(GetCurTsInMS());
      dictdata["uid"] = Json::Value(uid);
      dictdata["aid"] = Json::Value(aid);
      dictdata["cid"] = Json::Value(cid);
      dictdata["p"] = Json::Value(vhStream->GetID());
      dictdata["vtype"] = Json::Value(vhStream->mStreamType);
      dictdata["type"] = Json::Value("recv");

      dictdata["avbw"] = Json::Value(bwe.AvailableReceiveBandwidth / 1000);
      dictdata["rtt"] = Json::Value(videoSsrc->Rtt);
      dictdata["t_kbytes"] = Json::Value((videoSsrc->BytesReceived + audioSsrc->BytesReceived) / 1000);
      dictdata["t_kbitrate"] = Json::Value((videoSsrc->CalcData.Bitrate + audioSsrc->CalcData.Bitrate) / 1000);
      dictdata["v_exist"] = Json::Value(int(vhStream->mVideo));
      dictdata["v_ssrc"] = Json::Value(videoSsrc->Ssrc);
      dictdata["v_kbytes"] = Json::Value(videoSsrc->BytesReceived / 1000);
      dictdata["v_kbitrate"] = Json::Value(videoSsrc->CalcData.Bitrate / 1000);
      dictdata["v_width"] = Json::Value(videoSsrc->FrameWidthReceived);
      dictdata["v_height"] = Json::Value(videoSsrc->FrameHeightReceived);
      dictdata["v_fmcount"] = Json::Value(0);
      dictdata["v_fmrate"] = Json::Value(videoSsrc->FrameRateDecoded);
      dictdata["v_pkt"] = Json::Value(videoSsrc->PacketsReceived);
      dictdata["v_pkt_lost"] = Json::Value(videoSsrc->PacketsLost);
      dictdata["v_nack"] = Json::Value(videoSsrc->NacksSent);
      dictdata["v_pli"] = Json::Value(videoSsrc->PlisSent);
      dictdata["v_fir"] = Json::Value(videoSsrc->FirsSent);
      dictdata["v_fmenc"] = Json::Value(0);
      dictdata["v_tet"] = Json::Value(0);
      dictdata["v_avget"] = Json::Value(0);
      dictdata["v_qpsum"] = Json::Value(0);
      dictdata["v_tpsd"] = Json::Value(0);
      dictdata["v_srcw"] = Json::Value(0);
      dictdata["v_srch"] = Json::Value(0);
      dictdata["v_srcfmt"] = Json::Value(0);
      dictdata["v_jit"] = Json::Value(0);
      dictdata["v_rtt"] = Json::Value(0);
      dictdata["v_drop"] = Json::Value(0);
      dictdata["v_freeze"] = Json::Value(0);
      dictdata["a_exist"] = Json::Value(int(vhStream->mAudio)); // todo
      dictdata["a_ssrc"] = Json::Value(audioSsrc->Ssrc);
      dictdata["a_level"] = Json::Value(audioSsrc->AudioOutputLevel);
      dictdata["a_kbytes"] = Json::Value(audioSsrc->BytesReceived / 1000);
      dictdata["a_kbitrate"] = Json::Value(audioSsrc->CalcData.Bitrate / 1000);
      dictdata["a_pkt"] = Json::Value(audioSsrc->PacketsReceived);
      dictdata["a_pkt_lost"] = Json::Value(audioSsrc->PacketsLost);
      dictdata["a_jit"] = Json::Value(audioSsrc->JitterBufferMs);
      dictdata["a_rtt"] = Json::Value(int(audioSsrc->Rtt));

      report_array.append(dictdata); // append to array
    }
  }

  FlushRemoteStreamList();

}

void VHLogReport::startPlayMessageLog(const std::shared_ptr<VHStream> &stream) {
  // 添加至列表
  mRemoteStreamMutex.lock();
  mRemoteAddStreams[stream->mStreamId] = stream;
  mRemoteDelStreams.erase(stream->mStreamId);
  mRemoteStreamMutex.unlock();

  if (mUpThread) {
    mUpThread->Post(RTC_FROM_HERE, this, FlushRemote, nullptr);
  }
}

void VHLogReport::stopPlayMessageLog(const std::shared_ptr<VHStream> &stream) {
  // 放入删除列表
  mRemoteStreamMutex.lock();
  mRemoteDelStreams[stream->mStreamId] = stream;
  mRemoteAddStreams.erase(stream->mStreamId);
  mRemoteStreamMutex.unlock();

  if (mUpThread) {
    mUpThread->Post(RTC_FROM_HERE, this, FlushRemote, nullptr);
  }
}

void VHLogReport::publishMessageLog() {
  if (!mLocalStreams.empty()) {
    for (auto iter = mLocalStreams.begin(); iter != mLocalStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      auto VideoSsrcIt = mLocalVideoSsrc.find(vhStream->GetID());
      if (VideoSsrcIt == mLocalVideoSsrc.end()) {
        LOGE("VideoSsrcIt == mLocalVideoSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<SendVideoSsrc> videoSsrc = VideoSsrcIt->second;
      if (!videoSsrc) {
        LOGE("null videoSsrc, id :%s", vhStream->GetID());
        continue;
      }
      auto AudioSsrcIt = mLocalAudioSsrc.find(vhStream->GetID());
      if (AudioSsrcIt == mLocalAudioSsrc.end()) {
        LOGE("VideoSsrcIt == mLocalAudioSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<SendAudioSsrc> audioSsrc = AudioSsrcIt->second;
      if (!audioSsrc) {
        LOGE("null audioSsrc, id :%s", vhStream->GetID());
        continue;
      }

      vhStream->GetVideoPublishStates(videoSsrc.get());
      vhStream->GetAudioPublishStates(audioSsrc.get());
      videoSsrc->calc();
      audioSsrc->calc();
      //struct Bwe OverallBwe;
      //vhStream->GetOverallBwe(&OverallBwe);

      std::string k = "182004";
      std::string id = random();
      Json::Value dictdata(Json::ValueType::objectValue);
      dictdata["s"] = Json::Value(sessionId);
      uint64_t totalTime = vhStream->GetSteadyTimeStamp() - vhStream->upLogTime;
      dictdata["tt"] = Json::Value(totalTime);
      dictdata["uid"] = Json::Value(uid);
      dictdata["aid"] = Json::Value(aid);
      dictdata["cid"] = Json::Value(cid);
      dictdata["_m"] = Json::Value(message);
      dictdata["ld"] = Json::Value(GetCurTsInMS());
      dictdata["p"] = Json::Value(vhStream->GetID());
      dictdata["pf"] = Json::Value(pf);
      dictdata["ver"] = Json::Value(ver);
      dictdata["dt"] = Json::Value("windows");
      dictdata["osv"] = Json::Value(osv);
      dictdata["os"] = Json::Value(vhStream->mIsOversea);
      dictdata["errorcode"] = Json::Value(2002);
      dictdata["vtype"] = Json::Value(vhStream->mStreamType);
      dictdata["videoWidth"] = Json::Value(videoSsrc->FrameWidthSent);
      dictdata["videoHeight"] = Json::Value(videoSsrc->FrameHeightSent);
      dictdata["frameRate"] = Json::Value(videoSsrc->FrameRateSent);
      int64_t bitrate = (audioSsrc->CalcData.Bitrate + videoSsrc->CalcData.Bitrate) / 1000;
      dictdata["bitrate"] = Json::Value(bitrate);
      int64_t tf = (videoSsrc->CalcData.LastReceivedBytesSeg + audioSsrc->CalcData.LastReceivedBytesSeg) * 8 / 1000;
      dictdata["tf"] = Json::Value(tf);
      int64_t uf = (videoSsrc->CalcData.LastSentBytesSeg + audioSsrc->CalcData.LastSentBytesSeg) * 8 / 1000;
      dictdata["uf"] = Json::Value(uf);
      dictdata["biz_role"] = Json::Value(biz_role);
      dictdata["biz_id"] = Json::Value(biz_id);
      dictdata["biz_des01"] = Json::Value(biz_des01);
      dictdata["biz_des02"] = Json::Value(biz_des02);
      dictdata["bu"] = Json::Value(busynessUnit);
      dictdata["vid"] = Json::Value(vid);
      dictdata["vfid"] = Json::Value(vfid);
      dictdata["app_id"] = Json::Value(app_id);
      dictdata["sd"] = Json::Value(sd);
      vhStream->upLogTime = vhStream->GetSteadyTimeStamp();
      Json::FastWriter root;
      std::string token = talk_base::Base64::Encode(root.write(dictdata));
      std::string upUrl = url + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
      GetToSite(upUrl);
    }
  }

  FlushLocalStreamList();
}

void VHLogReport::publishMonitorLog(Json::Value& report_array) {
  if (!mLocalStreams.empty()) {
    for (auto iter = mLocalStreams.begin(); iter != mLocalStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      auto VideoSsrcIt = mLocalVideoSsrcMonitor.find(vhStream->GetID());
      if (VideoSsrcIt == mLocalVideoSsrcMonitor.end()) {
        LOGE("VideoSsrcIt == mLocalVideoSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<SendVideoSsrc> videoSsrc = VideoSsrcIt->second;
      if (!videoSsrc) {
        LOGE("null videoSsrc, id :%s", vhStream->GetID());
        continue;
      }
      auto AudioSsrcIt = mLocalAudioSsrcMonitor.find(vhStream->GetID());
      if (AudioSsrcIt == mLocalAudioSsrcMonitor.end()) {
        LOGE("VideoSsrcIt == mLocalAudioSsrc.end(), id :%s", vhStream->GetID());
        continue;
      }
      std::shared_ptr<SendAudioSsrc> audioSsrc = AudioSsrcIt->second;
      if (!audioSsrc) {
        LOGE("null audioSsrc, id :%s", vhStream->GetID());
        continue;
      }

      vhStream->GetVideoPublishStates(videoSsrc.get());
      vhStream->GetAudioPublishStates(audioSsrc.get());
      videoSsrc->calc();
      audioSsrc->calc();
      struct Bwe bwe;
      vhStream->GetOverallBwe(&bwe);

      std::string k = ulQualityStats;
      std::string id = random();
      Json::Value dictdata(Json::ValueType::objectValue);
      
      dictdata["s"] = Json::Value(sessionId);
      dictdata["ld"] = Json::Value(GetCurTsInMS());
      dictdata["uid"] = Json::Value(uid);
      dictdata["aid"] = Json::Value(aid);
      dictdata["cid"] = Json::Value(cid);
      dictdata["p"] = Json::Value(vhStream->GetID());
      dictdata["vtype"] = Json::Value(vhStream->mStreamType);
      dictdata["type"] = Json::Value("sent");

      dictdata["avbw"] = Json::Value(bwe.AvailableSendBandwidth / 1000);
      dictdata["rtt"] = Json::Value(videoSsrc->Rtt);
      dictdata["t_kbytes"] = Json::Value((videoSsrc->BytesSent + audioSsrc->BytesSent) / 1000);
      dictdata["t_kbitrate"] = Json::Value((videoSsrc->CalcData.Bitrate + audioSsrc->CalcData.Bitrate) / 1000);
      dictdata["v_exist"] = Json::Value(int(vhStream->mVideo));
      dictdata["v_ssrc"] = Json::Value(videoSsrc->Ssrc);
      dictdata["v_kbytes"] = Json::Value(videoSsrc->BytesSent / 1000);
      dictdata["v_kbitrate"] = Json::Value(videoSsrc->CalcData.Bitrate / 1000);
      dictdata["v_width"] = Json::Value(videoSsrc->FrameWidthSent);
      dictdata["v_height"] = Json::Value(videoSsrc->FrameHeightSent);
      dictdata["v_fmcount"] = Json::Value(0);
      dictdata["v_fmrate"] = Json::Value(videoSsrc->FrameRateSent);
      dictdata["v_pkt"] = Json::Value(videoSsrc->PacketsSent);
      dictdata["v_pkt_lost"] = Json::Value(videoSsrc->PacketsLost);
      dictdata["v_nack"] = Json::Value(videoSsrc->NacksReceived);
      dictdata["v_pli"] = Json::Value(videoSsrc->PliSReceived);
      dictdata["v_fir"] = Json::Value(videoSsrc->FirsReceived);
      dictdata["v_fmenc"] = Json::Value(videoSsrc->FramesEncoded);
      dictdata["v_tet"] = Json::Value(0);
      dictdata["v_avget"] = Json::Value(videoSsrc->AvgEncodeMs);
      dictdata["v_qpsum"] = Json::Value(videoSsrc->QpSum);
      dictdata["v_tpsd"] = Json::Value(0);
      dictdata["v_srcw"] = Json::Value(vhStream->videoOpt.maxWidth/*videoSsrc->FrameWidthInput*/);
      dictdata["v_srch"] = Json::Value(vhStream->videoOpt.maxHeight/*videoSsrc->FrameHeightInput*/);
      dictdata["v_srcfmt"] = Json::Value(videoSsrc->FrameRateInput);
      dictdata["v_jit"] = Json::Value(0);
      dictdata["v_rtt"] = Json::Value(videoSsrc->Rtt);
      dictdata["v_drop"] = Json::Value(0);
      dictdata["v_freeze"] = Json::Value(0);
      dictdata["a_exist"] = Json::Value(int(vhStream->mAudio));
      dictdata["a_ssrc"] = Json::Value(audioSsrc->Ssrc);
      dictdata["a_level"] = Json::Value(audioSsrc->AudioInputLevel);
      dictdata["a_kbytes"] = Json::Value(audioSsrc->BytesSent/1000);
      dictdata["a_kbitrate"] = Json::Value(audioSsrc->CalcData.Bitrate / 1000);
      dictdata["a_pkt"] = Json::Value(audioSsrc->PacketsSent);
      dictdata["a_pkt_lost"] = Json::Value(audioSsrc->PacketsLost);
      dictdata["a_jit"] = Json::Value(audioSsrc->JitterBufferMs);
      dictdata["a_rtt"] = Json::Value(int(audioSsrc->Rtt));

      report_array.append(dictdata);
    }
  }

  FlushLocalStreamList();
}

void VHLogReport::startPublishMessageLog(const std::shared_ptr<VHStream> &stream) {
  // 添加至列表
  biz_des02 = 1;
  mLocalStreamMutex.lock();
  mLocalAddStreams[stream->mStreamId] = stream;
  mLocalDelStreams.erase(stream->mStreamId);
  mLocalStreamMutex.unlock();

  if (mUpThread) {
    mUpThread->Post(RTC_FROM_HERE, this, FlushLocal, nullptr);
  }
}

void VHLogReport::stopPublishMessageLog(const std::shared_ptr<VHStream> &stream) {
  // 放入删除列表
  mLocalStreamMutex.lock();
  mLocalDelStreams[stream->mStreamId] = stream;
  mLocalAddStreams.erase(stream->mStreamId);
  mLocalStreamMutex.unlock();
  biz_des02 = 2;

  if (mUpThread) {
    mUpThread->Post(RTC_FROM_HERE, this, FlushLocal, nullptr);
  }
}

void VHLogReport::upLogVolume() {
  for (auto iter = mRemoteStreams.begin(); iter != mRemoteStreams.end(); ++iter) {
    std::shared_ptr<VHStream> vhStream = iter->second.lock();
    if (!vhStream) {
      continue;
    }
    std::stringstream ss;
    /* uplog play volume */
    ss << vhStream->GetAudioOutVolume() / 100 * 1.0f;
    upLog(uploadAudioVolume, vhStream->GetID(), ss.str());
  }
  if (!mLocalStreams.empty()) {
    for (auto iter = mLocalStreams.begin(); iter != mLocalStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      std::stringstream ss;
      ss << ((vhStream->mStreamType == 4) ? vhStream->GetAudioInVolume() : vhStream->GetFileVolume()) / 100 * 1.0f;
      upLog(uploadAudioVolume, vhStream->GetID(), ss.str());
    }
  }

  /* set up volume task */
  if (mUpThread) {
    mUpThread->PostDelayed(RTC_FROM_HERE, 15000, this, UP_VOLUME, nullptr);
  }
}

void VHLogReport::httpRequestCB(const std::string & msg, int code, const std::string userData) {
  LOGD("%s code:%d msg:%s\n", __FUNCTION__, code, msg.c_str());
}

void VHLogReport::SetClientID(std::string cid) {
  this->cid = cid;
}

std::string VHLogReport::GetCurTsInMS() {
  std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
  int64_t cur_ts = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;
  std::stringstream ss;
  ss << cur_ts;
  return ss.str();
}

std::string VHLogReport::random() {
  std::string rd = "";
  std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
  std::chrono::duration<double> time_span = t1.time_since_epoch();
  rd = std::to_string(uint64_t(time_span.count() * 1000000000));
  return rd;
}

void VHLogReport::onQualityStats() {
  Json::Value report(Json::ValueType::arrayValue);
  playMonitorLog(report);
  publishMonitorLog(report);

  Json::FastWriter root;
  std::string report_msg = root.write(report);
  std::string token = talk_base::Base64::Encode(report_msg);
  std::string id = random(); // logID
  std::string k = ulQualityStats;
  std::string upQualityInfo = monitorUrl + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upQualityInfo);

  if (mUpThread) {
    mUpThread->PostDelayed(RTC_FROM_HERE, 3000, this, UP_QUALITYSTATS, nullptr);
  }
  else {
    LOGE("upLog failed, could not find UpThread");
  }
}

/* common */
void VHLogReport::upLoadSdkVersionInfo() {
  std::string k = "182601";
  std::string id = random();
  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["s"] = Json::Value(sessionId);
  dictdata["uid"] = Json::Value(uid);
  dictdata["aid"] = Json::Value(aid);
  dictdata["cid"] = Json::Value(cid);
  dictdata["pf"] = Json::Value(pf);
  dictdata["ver"] = Json::Value(ver);
  dictdata["dt"] = Json::Value("windows");
  dictdata["release_date"] = Json::Value(release_date);
  dictdata["bu"] = Json::Value(busynessUnit);
  dictdata["app_id"] = Json::Value(app_id);
  dictdata["dt"] = Json::Value(device_type);
  dictdata["os"] = Json::Value(osv);
  dictdata["ua"] = Json::Value(osv);
  //dictdata["cpu"] = Json::Value("none");
  //dictdata["memory"] = Json::Value("none"); // todo
  dictdata["_m"] = Json::Value("sdk version info");
  dictdata["ld"] = Json::Value(GetCurTsInMS());
  Json::FastWriter root;
  std::string token = talk_base::Base64::Encode(root.write(dictdata));
  std::string upUrl = monitorUrl + "?k=" + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
}

void VHLogReport::upInitStreamParam(std::string streamId, std::string message) {
  if (mUpThread) {
    UpLogMessage* msgData = new UpLogMessage(ulInitStreamParam, streamId, message);
    if (msgData) {
      mUpThread->Post(RTC_FROM_HERE, this, UP_INITPAR, msgData);
    }
  }
  else {
    LOGE("upLogVideoDevice failed, could not find UpThread");
  }
}

void VHLogReport::upLogVideoDevice(std::string key, std::string streamId, std::string device, std::string msg) {
  if (mUpThread) {
    UpLogMessage* msgData = new UpLogMessage(key, streamId, msg);
    if (msgData) {
      msgData->mDeviceMsg = device;
      mUpThread->Post(RTC_FROM_HERE, this, UP_VIDEODEVMSG, msgData);
    }
  }
  else {
    LOGE("upLogVideoDevice failed, could not find UpThread");
  }
}

void VHLogReport::upLogAudioDevice(std::string key, std::string streamId, std::string device, std::string msg) {
  if (mUpThread) {
    UpLogMessage* msgData = new UpLogMessage(key, streamId, msg);
    if (msgData) {
      msgData->mDeviceMsg = device;
      mUpThread->Post(RTC_FROM_HERE, this, UP_AUDIODEVMSG, msgData);
    }
  }
  else {
    LOGE("upLogDevice failed, could not find UpThread");
  }
}

void VHLogReport::upLog(std::string key, std::string streamId, std::string msg) {
  if (mUpThread) {
    UpLogMessage* msgData = new UpLogMessage(key, streamId, msg);
    if (msgData) {
      mUpThread->Post(RTC_FROM_HERE, this, UP_MSG, msgData);
    }
  }
  else {
    LOGE("upLog failed, could not find UpThread");
  }
}

void VHLogReport::UpLogin(std::string key, std::string msg) {
  if (mUpThread) {
    UpLogMessage* msgData = new UpLogMessage(key, "", msg);
    if (msgData) {
      mUpThread->Post(RTC_FROM_HERE, this, UP_LOGIN, msgData);
    }
  }
  else {
    LOGE("UP_LOGIN failed, could not find UpThread");
  }
}

void VHLogReport::OnUpLog(std::string key, std::string streamId, std::string msg) {
  std::string k = key;
  std::string id = random();
  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["s"] = Json::Value(sessionId);
  dictdata["uid"] = Json::Value(uid);
  dictdata["aid"] = Json::Value(aid);
  dictdata["cid"] = Json::Value(cid);
  dictdata["p"] = Json::Value(streamId);
  dictdata["pf"] = Json::Value(pf);
  dictdata["ver"] = Json::Value(ver);
  dictdata["dt"] = Json::Value("windows");
  dictdata["bu"] = Json::Value(busynessUnit);
  dictdata["dt"] = Json::Value(device_type);
  dictdata["osv"] = Json::Value(osv);
  dictdata["_m"] = Json::Value(msg);
  dictdata["ld"] = Json::Value(GetCurTsInMS());
  Json::FastWriter root;
  std::string token = talk_base::Base64::Encode(root.write(dictdata));
  std::string upUrl = monitorUrl + "?k=" + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
}

void VHLogReport::OnUpLogDevice(int devType, std::string key, std::string streamId, std::string device, std::string msg) {
  std::string k = key;
  std::string id = random();
  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["s"] = Json::Value(sessionId);
  dictdata["uid"] = Json::Value(uid);
  dictdata["aid"] = Json::Value(aid);
  dictdata["cid"] = Json::Value(cid);
  dictdata["pf"] = Json::Value(pf);
  dictdata["ver"] = Json::Value(ver);
  dictdata["dt"] = Json::Value("windows");
  dictdata["_m"] = Json::Value(msg);
  dictdata["ld"] = Json::Value(GetCurTsInMS());
  dictdata["osv"] = Json::Value(osv);
  if (0 == devType) { // audio
    dictdata["audioDev"] = Json::Value(device);
  }
  else if (1 == devType) {
    dictdata["VideoDev"] = Json::Value(device);
  }
  Json::FastWriter root;
  std::string token = talk_base::Base64::Encode(root.write(dictdata));
  std::string upUrl = monitorUrl + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
  LOGE("device token:%s", root.write(dictdata).c_str());
}

void VHLogReport::OnUpLogin(std::string key, std::string msg) {
  /* 连接房间 */
  std::string k = key;
  std::string id = random();
  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["s"] = Json::Value(sessionId);
  dictdata["uid"] = Json::Value(uid);
  dictdata["aid"] = Json::Value(aid);
  dictdata["cid"] = Json::Value(cid);
  dictdata["bu"] = Json::Value(busynessUnit);
  dictdata["pf"] = Json::Value(pf);
  dictdata["ver"] = Json::Value(ver);
  dictdata["dt"] = Json::Value("windows");
  dictdata["_m"] = Json::Value(msg);
  dictdata["ld"] = Json::Value(GetCurTsInMS());
  dictdata["s"] = Json::Value(sessionId);
  dictdata["osv"] = Json::Value(osv);
  Json::FastWriter root;
  std::string token = talk_base::Base64::Encode(root.write(dictdata));
  /* 计费系统 */
  std::string upUrl = url + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
  /* 互动监控 */
  upUrl = monitorUrl + "?k=" + prefixK + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
  GetToSite(upUrl);
}

void VHLogReport::OnUpInitStreamParam(std::string streamId, std::string message) {
  FlushLocalStreamList();
  if (!mLocalStreams.empty()) {
    for (auto iter = mLocalStreams.begin(); iter != mLocalStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      if (0 == streamId.compare(vhStream->GetID())) {
        std::string k = ulInitStreamParam;
        std::string id = random();
        Json::Value dictdata(Json::ValueType::objectValue);
        dictdata["s"] = Json::Value(sessionId);
        dictdata["uid"] = Json::Value(uid);
        dictdata["aid"] = Json::Value(aid);
        dictdata["cid"] = Json::Value(cid);
        dictdata["p"] = Json::Value(streamId);
        dictdata["pf"] = Json::Value(pf);
        dictdata["ver"] = Json::Value(ver);
        dictdata["dt"] = Json::Value("windows");
        dictdata["bu"] = Json::Value(busynessUnit);
        dictdata["dt"] = Json::Value(device_type);
        dictdata["osv"] = Json::Value(osv);
        dictdata["_m"] = Json::Value(message);
        dictdata["ld"] = Json::Value(GetCurTsInMS());

        dictdata["currentBitrateKbps"] = Json::Value();
        dictdata["maxBitrateKbps"] = Json::Value(vhStream->videoOpt.maxVideoBW);
        dictdata["minBitrateKbps"] = Json::Value(vhStream->videoOpt.minVideoBW);
        Json::Value mute(Json::ValueType::objectValue);
        mute["audio"] = Json::Value(vhStream->mMuteStreams.mAudio);
        mute["video"] = Json::Value(vhStream->mMuteStreams.mVideo);
        dictdata["muteStream"] = mute; // obj
        dictdata["numSpatialLayers"] = Json::Value(vhStream->mNumSpatialLayers == 2 ? 2 : 1);
        dictdata["streamType"] = Json::Value(vhStream->mStreamType);
        dictdata["videoCodecPreference"] = Json::Value(vhStream->mVideoCodecPreference);
        dictdata["videoFps"] = Json::Value(vhStream->videoOpt.maxFrameRate);
        dictdata["videoHeight"] = Json::Value(vhStream->videoOpt.maxHeight);
        dictdata["videoWidth"] = Json::Value(vhStream->videoOpt.maxWidth);

        Json::FastWriter root;
        std::string token = talk_base::Base64::Encode(root.write(dictdata));
        std::string upUrl = monitorUrl + "?k=" + k + "&id=" + id + "&s=" + sessionId + "&bu=" + busynessUnit + "&token=" + token;
        GetToSite(upUrl);
        break;
      }
    }
  }
}

void VHLogReport::FlushLocalStreamList() {
  mLocalStreamMutex.lock();
  // 添加新流
  if (!mLocalAddStreams.empty()) {
    for (auto iter = mLocalAddStreams.begin(); iter != mLocalAddStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      mLocalStreams[iter->first] = iter->second;
      /* 计费上报，初始化SSRC */
      {
        std::shared_ptr<SendVideoSsrc> videoSsrc = std::make_shared<SendVideoSsrc>();
        std::shared_ptr<SendAudioSsrc> audioSsrc = std::make_shared<SendAudioSsrc>();
        if (!audioSsrc || !videoSsrc) {
          LOGE("create publish ssrc");
          continue;
        }
        mLocalVideoSsrc[iter->first] = videoSsrc;
        mLocalAudioSsrc[iter->first] = audioSsrc;
        vhStream->GetVideoPublishStates(videoSsrc.get());
        vhStream->GetAudioPublishStates(audioSsrc.get());
        videoSsrc->calc();
        audioSsrc->calc(); 
      }

      /* 互动监控系统上报 */
      {
        std::shared_ptr<SendVideoSsrc> videoSsrc = std::make_shared<SendVideoSsrc>();
        std::shared_ptr<SendAudioSsrc> audioSsrc = std::make_shared<SendAudioSsrc>();
        if (!audioSsrc || !videoSsrc) {
          LOGE("create publish ssrc");
          continue;
        }
        mLocalVideoSsrcMonitor[iter->first] = videoSsrc;
        mLocalAudioSsrcMonitor[iter->first] = audioSsrc;
        vhStream->GetVideoPublishStates(videoSsrc.get());
        vhStream->GetAudioPublishStates(audioSsrc.get());
        videoSsrc->calc();
        audioSsrc->calc();
      }

      vhStream->upLogTime = vhStream->GetSteadyTimeStamp(); /* 开始推流时间戳 */
    }
    mLocalAddStreams.clear();
  }

  // 删除旧流
  if (!mLocalDelStreams.empty()) {
    for (auto iter = mLocalDelStreams.begin(); iter != mLocalDelStreams.end(); ++iter) {
      mLocalStreams.erase(iter->first);
      /* 计费上报 */
      mLocalVideoSsrc.erase(iter->first);
      mLocalAudioSsrc.erase(iter->first);
      
      /* 互动监控系统上报 */
      mLocalVideoSsrcMonitor.erase(iter->first);
      mLocalAudioSsrcMonitor.erase(iter->first);
    }
    mLocalDelStreams.clear();
  }

  mLocalStreamMutex.unlock();
}

void VHLogReport::FlushRemoteStreamList() {
  mRemoteStreamMutex.lock();
  // 添加新流
  if (!mRemoteAddStreams.empty()) {
    for (auto iter = mRemoteAddStreams.begin(); iter != mRemoteAddStreams.end(); ++iter) {
      std::shared_ptr<VHStream> vhStream = iter->second.lock();
      if (!vhStream) {
        continue;
      }
      mRemoteStreams[iter->first] = iter->second;

      /* 計費上報，初始化ssrc */
      {
        std::shared_ptr<ReceiveVideoSsrc> videoSsrc = std::make_shared<ReceiveVideoSsrc>();
        std::shared_ptr<ReceiveAudioSsrc> audioSsrc = std::make_shared<ReceiveAudioSsrc>();
        if (!videoSsrc || !audioSsrc) {
          LOGE("create ssrc failed");
          continue;
        }
        mRemoteVideoSsrc[iter->first] = videoSsrc;
        mRemoteAudioSsrc[iter->first] = audioSsrc;
        vhStream->GetVideoSubscribeStates(videoSsrc.get());
        vhStream->GetAudioSubscribeStates(audioSsrc.get());
        videoSsrc->calc();
        audioSsrc->calc();
      }

      /* 互动监控上报 */
      {
        std::shared_ptr<ReceiveVideoSsrc> videoSsrc = std::make_shared<ReceiveVideoSsrc>();
        std::shared_ptr<ReceiveAudioSsrc> audioSsrc = std::make_shared<ReceiveAudioSsrc>();
        if (!videoSsrc || !audioSsrc) {
          LOGE("create ssrc failed");
          continue;
        }
        mRemoteVideoSsrcMonitor[iter->first] = videoSsrc;
        mRemoteAudioSsrcMonitor[iter->first] = audioSsrc;
        vhStream->GetVideoSubscribeStates(videoSsrc.get());
        vhStream->GetAudioSubscribeStates(audioSsrc.get());
        videoSsrc->calc();
        audioSsrc->calc();
      }

      vhStream->upLogTime = vhStream->GetSteadyTimeStamp(); /* 开始拉流时间戳 */
    }
    mRemoteAddStreams.clear();
  }

  // 删除旧流
  if (!mRemoteDelStreams.empty()) {
    for (auto iter = mRemoteDelStreams.begin(); iter != mRemoteDelStreams.end(); ++iter) {
      mRemoteStreams.erase(iter->first);
      /* 计费上报 */
      mRemoteVideoSsrc.erase(iter->first);
      mRemoteAudioSsrc.erase(iter->first);
      /* 互动监控上報 */
      mRemoteVideoSsrcMonitor.erase(iter->first);
      mRemoteAudioSsrcMonitor.erase(iter->first);
    }
    mRemoteDelStreams.clear();
  }

  mRemoteStreamMutex.unlock();
}

void VHLogReport::GetToSite(std::string reqestUrl) {
  HTTP_GET_REQUEST httpRequest(reqestUrl);
  httpRequest.SetHttpPost(false);
  httpManager->HttpGetRequest(httpRequest, VH_CALLBACK_3(VHLogReport::httpRequestCB, this));
}

void VHLogReport::OnMessage(rtc::Message* msg) {
  UpLogMessage* msgData = (static_cast<UpLogMessage*>(msg->pdata));
  switch (msg->message_id) {
  case UP_AUDIODEVMSG:
    if (msgData) {
      OnUpLogDevice(0, msgData->mKey, msgData->mStreamId, msgData->mDeviceMsg, msgData->mMsg);
    }
    break;
  case UP_VIDEODEVMSG:
    if (msgData) {
      OnUpLogDevice(1, msgData->mKey, msgData->mStreamId, msgData->mDeviceMsg, msgData->mMsg);
    }
    break;
  case UP_MSG:
    if (msgData) {
      if (!msgData->mKey.compare("182601")) {
        upLoadSdkVersionInfo();
      }
      else {
        OnUpLog(msgData->mKey, msgData->mStreamId, msgData->mMsg);
      }
    }
    break;
  case UP_VOLUME:
    upLogVolume();
    break;
  case UP_DATAFLOW:
    uplogDataFlow();
    break;
  case UP_QUALITYSTATS:
    onQualityStats();
    break;
  case UP_LOGIN:
    if (msgData) {
      OnUpLogin(msgData->mKey, msgData->mMsg);
    }
    break;
  case UP_INITPAR:
    if (msgData) {
      OnUpInitStreamParam(msgData->mStreamId, msgData->mMsg);
    }
    break;
  case FlushLocal:
    FlushLocalStreamList();
    break;
  case FlushRemote:
    FlushRemoteStreamList();
    break;
  default:
    LOGE("undefine message");
    break;
  }

  /* release msgData */
  if (msgData) {
    delete msgData;
  }
};

NS_VH_END