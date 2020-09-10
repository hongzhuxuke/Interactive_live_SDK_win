#ifndef TIMER_H_
#define TIMER_H_

#include<functional>
#include<chrono>
#include<thread>
#include<atomic>
#include<memory>
#include<mutex>
#include<condition_variable>

class Timer {
public:
  Timer() :expired_(true), try_to_expire_(false) {
  }

  Timer(const Timer& t) {
    expired_ = t.expired_.load();
    try_to_expire_ = t.try_to_expire_.load();
  }
  ~Timer() {
    Expire();
    LOGE("timer destructed!");
  }

  void StartTimer(int interval, std::function<void()> task) {
    if (expired_ == false) {
      LOGE("timer is currently running, please expire it first...");
      return;
    }
    expired_ = false;
    std::thread([this, interval, task]() {
      while (!try_to_expire_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        task();
      }
      LOGD("stop task...");
      {
        std::lock_guard<std::mutex> locker(mutex_);
        expired_ = true;
        expired_cond_.notify_one();
      }
    }).detach();
  }

  void Expire() {
    LOGD("quit timer");
    if (expired_) {
      LOGW("time had Expired");
      return;
    }

    if (try_to_expire_) {
      LOGD("timer is trying to expire, please wait...");
      return;
    }
    try_to_expire_ = true;
    {
      LOGD("expired_cond_ wait");
      std::unique_lock<std::mutex> locker(mutex_);
      expired_cond_.wait(locker, [this] {return expired_ == true; });
      if (expired_ == true) {
        LOGD("timer expired!");
        try_to_expire_ = false;
      }
    }
    LOGD("timer quit done");
  }

  template<typename callable, class... arguments>
  void SyncWait(int after, callable&& f, arguments&&... args) {

    std::function<typename std::result_of<callable(arguments...)>::type()> task
    (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));
    std::this_thread::sleep_for(std::chrono::milliseconds(after));
    task();
  }
  template<typename callable, class... arguments>
  void AsyncWait(int after, callable&& f, arguments&&... args) {
    std::function<typename std::result_of<callable(arguments...)>::type()> task
    (std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

    std::thread([after, task]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(after));
      task();
    }).detach();
  }

private:
  std::atomic<bool> expired_;
  std::atomic<bool> try_to_expire_;
  std::mutex mutex_;
  std::condition_variable expired_cond_;
};

#endif