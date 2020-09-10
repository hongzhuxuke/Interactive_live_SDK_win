#pragma once
#include<iostream>
#include<memory>

namespace vhall {
  class BaseAudioDeviceModule;
  class EventDispatcher;
  class __declspec(dllexport)AdmConfig {
  public:
    static bool InitializeAdm();
    static bool DeInitAdm();
    static std::shared_ptr<EventDispatcher> GetAudioDeviceEventDispatcher();
    static std::shared_ptr<BaseAudioDeviceModule> GetAdmInstance();
  };
}

