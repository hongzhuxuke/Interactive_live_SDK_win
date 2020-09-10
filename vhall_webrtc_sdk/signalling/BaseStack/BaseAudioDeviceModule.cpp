#include "BaseAudioDeviceModule.h"
#include "../tool/AudioDataFromCapture.h"

namespace vhall {
  BaseAudioDeviceModule::~BaseAudioDeviceModule() {
    Destroy();
  }
  void BaseAudioDeviceModule::DispatchEvent(BaseEvent & event) {
    if (mDispatcher) {
      mDispatcher->DispatchEvent(event);
    }
  }

  BaseAudioDeviceModule::BaseAudioDeviceModule() {
    mLoopBackVolume = 100;
    mMicroPhoneVolume = 100;
    mSpeakerVolume = 100;
  }

  void BaseAudioDeviceModule::Init() {
    LOGD("%s", __FUNCTION__);
    m_network_thread = rtc::Thread::CreateWithSocketServer();
    if (m_network_thread) {
      m_network_thread->Start();
    }
    m_worker_thread = rtc::Thread::Create();
    if (m_worker_thread) {
      m_worker_thread->Start();
    }
    m_signaling_thread = rtc::Thread::Create();
    if (m_signaling_thread) {
      m_signaling_thread->Start();
    }

    mThread = rtc::Thread::Create();
    if (mThread) {
      mThread->Start();
    }
  };
  void BaseAudioDeviceModule::Destroy() {
    LOGD("%s", __FUNCTION__);
    RemoveAllEventListener();
    std::unique_lock<std::mutex> lock(mMtx);
    if (mThread) {
      mThread->Stop();
      mThread.reset();
    }
    if (peer_connection_factory_) {
      peer_connection_factory_->Release();
      peer_connection_factory_.release();
      peer_connection_factory_ = nullptr;
    }
    if (mAudioDevice) {
      mAudioDevice->StopPlayout();
      mAudioDevice->StopRecording();
      mAudioDevice->Release();
      mAudioDevice.release();
    }
    if (m_network_thread) {
      m_network_thread->Stop();
      m_network_thread.reset();
    }
    if (m_worker_thread) {
      m_worker_thread->Stop();
      m_worker_thread.reset();
    }
    if (m_signaling_thread) {
      m_signaling_thread->Stop();
      m_signaling_thread.reset();
    }
    
    mAudioCaptureObserver.reset();

  }
}