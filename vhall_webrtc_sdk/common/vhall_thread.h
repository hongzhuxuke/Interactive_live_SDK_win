#ifndef VHALLTHREAD_H
#define VHALLTHREAD_H

#include <iostream>
#include <thread>
#include <condition_variable>
#include <memory>

class VThread {
public:
  VThread() :
    m_finished(false), m_isStart(false), m_isPrepared(false) {

  }

  void start() {
    m_thd = std::make_unique<std::thread>(&VThread::entry, this);
    m_tid = m_thd->get_id();
    m_hwnd = m_thd->native_handle();

    m_thd->detach(); 
    m_isStart = true;
    wait_prep();
  }

  bool wait(std::chrono::milliseconds waitTime) {
    bool ret = true;

    if (!m_isStart)
      return ret;

    std::unique_lock<std::mutex> lock(m_cvM);
    while (!m_finished) {
      if (m_cv.wait_for(lock, waitTime) == std::cv_status::timeout) {
        ret = false;
        break;
      }
    }

    return ret;
  }

  void wait() {
    if (!m_isStart)
      return;

    std::unique_lock<std::mutex> lock(m_cvM);
    while (!m_finished)
      m_cv.wait(lock);
  }

  void wait_prep() {
    if (m_isPrepared)
      return;

    std::unique_lock<std::mutex> prep_lock(m_prepM);
    while (!m_isPrepared)
      m_prepcv.wait(prep_lock);
  }

  std::thread::id getTid() { return m_tid; }
  std::thread::native_handle_type getHwnd() { return m_hwnd; }
  
  virtual void run() {}

  virtual void prepare() {}

protected:
  VThread(const VThread&) = delete;
  VThread& operator =(const VThread&) = delete;
private:
  void entry() {

    this->prepare();

    m_isPrepared = true;

    std::unique_lock<std::mutex> prep_lock(m_prepM);
    prep_lock.unlock();
    m_prepcv.notify_all();
    
    this->run();
    
    std::unique_lock<std::mutex> lock(m_cvM);
    m_finished = true;
    lock.unlock();
    
    m_cv.notify_all();

    m_isStart = false;
  }
private:
  std::mutex m_cvM;
  std::condition_variable m_cv;
  bool m_finished;

  std::mutex m_prepM;
  std::condition_variable m_prepcv;

  bool m_isStart;
  bool m_isPrepared;
  std::thread::id m_tid;
  std::thread::native_handle_type m_hwnd;

  std::unique_ptr<std::thread> m_thd;
};

#endif  // !VHALLTHREAD_H