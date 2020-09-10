#include "AdmConfig.h"
#include "BaseAudioDeviceModule.h"

std::mutex admMutx;
static std::shared_ptr<vhall::BaseAudioDeviceModule> gBaseAdm = nullptr;
static std::shared_ptr<vhall::EventDispatcher> gAudioDispatcher = nullptr;
namespace vhall {
  bool AdmConfig::InitializeAdm() {
    std::unique_lock<std::mutex> lock(admMutx);
    if (gBaseAdm) {
      return true;
    }
    if (!gAudioDispatcher) {
      gAudioDispatcher.reset(new EventDispatcher);
    }
    if (!gAudioDispatcher) {
      return false;
    }
    gBaseAdm = std::make_shared<BaseAudioDeviceModule>();
    if (gBaseAdm) {
      gBaseAdm->mDispatcher = gAudioDispatcher;
      gBaseAdm->Init();
    }
    else {
      return false;
    }
    return true;
  }
  bool AdmConfig::DeInitAdm() {
    std::unique_lock<std::mutex> lock(admMutx);
    if (gBaseAdm) {
      gBaseAdm.reset();
    }
    return true;
  }
  std::shared_ptr<EventDispatcher> AdmConfig::GetAudioDeviceEventDispatcher() {
    std::unique_lock<std::mutex> lock(admMutx);
    if (!gAudioDispatcher) {
      gAudioDispatcher.reset(new EventDispatcher);
    }
    return gAudioDispatcher;
  }
  std::shared_ptr<BaseAudioDeviceModule> AdmConfig::GetAdmInstance() {
    if (AdmConfig::InitializeAdm()) {
      std::unique_lock<std::mutex> lock(admMutx);
      return gBaseAdm;
    }
    else {
      return nullptr;
    }
  }
}
