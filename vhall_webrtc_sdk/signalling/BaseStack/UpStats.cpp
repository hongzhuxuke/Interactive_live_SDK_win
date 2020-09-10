#include "UpStats.h"
#include "json/json.h"
#include "common/vhall_log.h"
#include "common/vhall_define.h"
#include "3rdparty/VhallNetWork/VhallNetWork/VhallNetWork/VhallNetWorkInterface.h"
#include <ShlObj.h>

namespace vhall {
  class UpStatsMsg : public rtc::MessageData {
  public:
    UpStatsMsg() {};
    virtual ~UpStatsMsg() {};
    std::string mMessage = "";
  };
}

namespace vhall {

  UpStats::UpStats() {
    mInitialized = false;
    mUrl = "";
    TCHAR fullPath[MAX_PATH] = { 0 };
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullPath);
    httpManager = GetHttpManagerInstance("vh_rtc_sdk");
    httpManager->Init(fullPath);
  }
  UpStats::~UpStats() {
    Destroy();
  }

  bool UpStats::Init() {
    mThread = rtc::Thread::Create();
    if (mThread) {
      mThread->Start();
    }
    else {
      LOGE("Init thread failed");
      return false;
    }
    mInitialized = true;
    return true;
  }

  void UpStats::SetUrl(std::string url) {
    std::unique_lock<std::mutex> lock(mUrlMtx);
    mUrl = url;
    lock.unlock();
  }

  void UpStats::Destroy() {
    mInitialized = false;
    if (mThread) {
      mThread->Stop();
      mThread->Clear(this);
      mThread.reset();
    }
  }

  void UpStats::DoUpStats(std::string message) {
    if (!mThread) {
      LOGE("UpStats not initialized");
      return;
    }
    UpStatsMsg* msg = new(std::nothrow) UpStatsMsg();
    if (nullptr == msg) {
      LOGE("Create UpStatsMsg failed");
      return;
    }
    msg->mMessage = message;
    if (mThread) {
      mThread->Post(RTC_FROM_HERE, this, UpMsg, msg);
    }
  }

  void UpStats::OnDoUpStats(std::string message) {
    std::unique_lock<std::mutex> lock(mUrlMtx);
    std::string url = mUrl;
    lock.unlock();
    url += std::string("?info=");
    url += message;
    GetToSite(url);
  }

  void UpStats::OnMessage(rtc::Message * message) {
    UpStatsMsg* msg = dynamic_cast<UpStatsMsg*>(message->pdata);
    if (nullptr == msg) {
      LOGE("null message");
      return;
    }
    switch (message->message_id) {
    case UpMsg:
      OnDoUpStats(msg->mMessage);
      break;
    default:
      break;
    }
    delete msg;
  }

  void UpStats::httpRequestCB(const std::string & msg, int code, const std::string userData) {
    LOGD("%s code:%d msg:%s\n", __FUNCTION__, code, msg.c_str());
  }

  void UpStats::GetToSite(std::string reqestUrl) {
    HTTP_GET_REQUEST httpRequest(reqestUrl);
    httpRequest.SetHttpPost(true);
    httpManager->HttpGetRequest(httpRequest, VH_CALLBACK_3(UpStats::httpRequestCB, this));
  }

}