#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include "AudioSendFrame.h"

namespace webrtc {
  class AudioDeviceModule;
  class PeerConnectionFactoryInterface;
  class AudioPlayDataReader;
  class TaskQueueFactory;
}

namespace vhall {
  class VHTools;

  class __declspec(dllexport) AudioPlayDeviceModule {
  public:
    static AudioPlayDeviceModule* GetInstance();
    static void Release();
    int SetPlay(std::string audioFile, uint16_t playOutDeviceIndex, const uint32_t samplesPerSec, const size_t nChannels);
    void SetAudioListen(AudioSendFrame* listener);
    bool Start();
    bool Stop();
  private:
    AudioPlayDeviceModule();
    ~AudioPlayDeviceModule();
    //AudioPlayDeviceModule(const AudioPlayDeviceModule&) {};
    AudioPlayDeviceModule& operator=(const AudioPlayDeviceModule&) { return *this; };
    bool CreatePeerConnectionFactory();
    friend class AudioPlayDeviceModule;
  private:
    bool initialized = false;
    std::shared_ptr<VHTools> mDeviceTool = nullptr;
    AudioSendFrame* mListener = nullptr;
    std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;
  };
}