#pragma once
#include <iostream>
#include <unordered_map>
#include <atomic>
#include <list>
#include <vector>
#include <string>
#include <string>
#include <memory>
#include <mutex>
//#include "talk/base/messagehandler.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/thread.h"

//#include "rtc_base/thread.h"
//#include "rtc_base/messagequeue.h"

class HttpManagerInterface;

namespace talk_base {
  class Thread;
}

namespace vhall {
  class UpStats :public rtc::MessageHandler {
  public:
    UpStats();
    virtual ~UpStats();
    bool Init();
    void SetUrl(std::string url);
    void Destroy();
    void DoUpStats(std::string message);
    enum UpStatsEvent {
      UpMsg,
    };
  private:
    void OnDoUpStats(std::string message);
    virtual void OnMessage(rtc::Message* msg);
    void httpRequestCB(const std::string& msg, int code, const std::string userData);
    void GetToSite(std::string reqestUrl);
  private:
    std::unique_ptr<rtc::Thread> mThread;
    std::atomic<bool>            mInitialized;
    std::string                  mUrl;
    std::mutex                   mUrlMtx;
    std::shared_ptr<HttpManagerInterface> httpManager = nullptr;
  };

}