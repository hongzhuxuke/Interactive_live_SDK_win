#ifndef vh_uplog_h
#define vh_uplog_h

#include <cstdio>
#include <unordered_map>
#include <list>
#include <vector>
#include <string>
#include <memory>
#include "json/json.h"
#include "vh_uplog_define.h"
#include "common/vhall_define.h"
#include "common/vhall_timer.h"
#include "rtc_base/thread.h"
#include "api/task_queue/default_task_queue_factory.h"

class HttpManagerInterface;

NS_VH_BEGIN

class VHStream;

struct upLogOptions {
  upLogOptions() {
    uid = "";
    aid = "";
    url = "";
    biz_role = 0;
    biz_id = "0";
    biz_des01 = 0;
    biz_des02 = 0;
    bu = "";
    vid = "";
    vfid = "";
    app_id = "";
    sd = "";
    version = "2.0.0.0";
    release_date = "";
    osv = "windows";
    isOversea = false;
    device_type = "windows_pc";
  }

  std::string uid;
  std::string aid;
  std::string url;
  INT32 biz_role;
  std::string biz_id;
  INT32 biz_des01;
  INT32 biz_des02;
  std::string bu;
  std::string vid;
  std::string vfid;
  std::string app_id;
  std::string sd;
  std::string version;
  std::string release_date;
  std::string osv;
  bool isOversea;
  std::string device_type;
};
class UpLogMessage : public rtc::MessageData {
public:
  UpLogMessage(std::string key, std::string streamId = "0", std::string msg = "") {
    mKey = key;
    mStreamId = streamId;
    mMsg = msg;
  }

  std::string mKey = "";
  std::string mStreamId = "0";
  std::string mMsg = "";
  std::string mDeviceMsg = "";
};

struct SendAudioSsrc;
struct SendVideoSsrc;
struct ReceiveVideoSsrc;
struct ReceiveAudioSsrc;

class VHLogReport:
  public rtc::MessageHandler {
public:
  VHLogReport(struct upLogOptions opt);
  ~VHLogReport();
  void start();
  void uplogDataFlow();
  //void connectSignallingLog();
  void disconnectSignallingLog();
  //void connectSignallingFailureLog();

  void playMessageLog();
  void playMonitorLog(Json::Value& report_array);
  void startPlayMessageLog(const std::shared_ptr<VHStream> &stream);
  void stopPlayMessageLog(const std::shared_ptr<VHStream> &stream);

  void publishMessageLog();
  void publishMonitorLog(Json::Value& report_array);
  void startPublishMessageLog(const std::shared_ptr<VHStream> &stream);
  void stopPublishMessageLog(const std::shared_ptr<VHStream> &stream);
  
  void upLogVolume();

  /* common */
  void upLoadSdkVersionInfo();// 182601	上报SDK版本信息
  void upInitStreamParam(std::string streamId, std::string message);
  void upLogVideoDevice(std::string key, std::string streamId = "0", std::string device = "", std::string msg = "");
  void upLogAudioDevice(std::string key, std::string streamId = "0", std::string device = "", std::string msg = "");
  void upLog(std::string key, std::string streamId = "0", std::string msg = "");
  void UpLogin(std::string key, std::string msg = "");
  enum {
    UP_MSG,        // 事件上报
    UP_AUDIODEVMSG,
    UP_VIDEODEVMSG,
    UP_VOLUME,
    UP_DATAFLOW,   // 计费流量上报
    UP_QUALITYSTATS, // 互动质量上报
    UP_LOGIN,      // 计费连接上报
    UP_INITPAR,

    FlushLocal,
    FlushRemote,
  };

  void httpRequestCB(const std::string& msg, int code, const std::string userData);
  void SetClientID(std::string cid);
  std::string GetCurTsInMS();
protected:
  std::string random();
private:
  void onQualityStats();

  virtual void OnMessage(rtc::Message* msg);
  void OnUpLog(std::string key, std::string streamId = "0", std::string msg = "");
  void OnUpLogDevice(int devType/*0:audio, 1:video*/, std::string key, std::string streamId = "0", std::string device = "", std::string msg = "");
  void OnUpLogin(std::string key, std::string msg = "");
  void OnUpInitStreamParam(std::string streamId, std::string message);

  void FlushLocalStreamList();
  void FlushRemoteStreamList();
  void GetToSite(std::string reqestUrl);
private:
  std::unique_ptr<rtc::Thread> mUpThread = nullptr;

  std::string sessionId;
  std::string prefixK;
  std::string url; /* 计费接口 */
  std::string monitorUrl; /* 监控接口 */
  INT32 biz_role;
  std::string biz_id;
  INT32 biz_des01;
  INT32 biz_des02;
  std::string busynessUnit;
  std::string device_type;
  std::string osv;
  bool isOversea;
  std::string pf;
  std::string ver;
  std::string uid;
  std::string aid;
  std::string cid;
  std::string vid;
  std::string vfid;
  std::string app_id;
  std::string sd;
  std::string message;
  std::string release_date;
  std::shared_ptr<HttpManagerInterface> httpManager = nullptr;

  std::unordered_map<uint64_t, std::shared_ptr<Timer>> playHeart;
  std::unordered_map<uint64_t, std::shared_ptr<Timer>> publishHeart;

  /* 计费 */
  std::unordered_map<std::string, std::weak_ptr<VHStream>> mRemoteStreams;
  std::unordered_map<std::string, std::shared_ptr<ReceiveVideoSsrc>> mRemoteVideoSsrc;
  std::unordered_map<std::string, std::shared_ptr<ReceiveAudioSsrc>> mRemoteAudioSsrc;
  std::unordered_map<std::string, std::weak_ptr<VHStream>> mLocalStreams;
  std::unordered_map<std::string, std::shared_ptr<SendVideoSsrc>> mLocalVideoSsrc;
  std::unordered_map<std::string, std::shared_ptr<SendAudioSsrc>> mLocalAudioSsrc;

  /* 互动监控系统 */
  std::unordered_map<std::string, std::shared_ptr<ReceiveVideoSsrc>> mRemoteVideoSsrcMonitor;
  std::unordered_map<std::string, std::shared_ptr<ReceiveAudioSsrc>> mRemoteAudioSsrcMonitor;
  std::unordered_map<std::string, std::shared_ptr<SendVideoSsrc>> mLocalVideoSsrcMonitor;
  std::unordered_map<std::string, std::shared_ptr<SendAudioSsrc>> mLocalAudioSsrcMonitor;

  std::unordered_map<std::string, std::weak_ptr<VHStream>> mRemoteAddStreams;
  std::unordered_map<std::string, std::weak_ptr<VHStream>> mLocalAddStreams;
  std::unordered_map<std::string, std::weak_ptr<VHStream>> mRemoteDelStreams;
  std::unordered_map<std::string, std::weak_ptr<VHStream>> mLocalDelStreams;
  std::mutex mRemoteStreamMutex;
  std::mutex mLocalStreamMutex;
};
NS_VH_END

#endif