#pragma once
#include <wmcodecdsp.h>      // CLSID_CWMAudioAEC
// (must be before audioclient.h)
#include <Audioclient.h>     // WASAPI
#include <Audiopolicy.h>
#include <Mmdeviceapi.h>     // MMDevice
#include <avrt.h>            // Avrt
#include <endpointvolume.h>
#include <mediaobj.h>        // IMediaObject
#include <list>
#include <Functiondiscoverykeys_devpkey.h>
#include "signalling/BaseStack/AdmConfig.h"
#include "rtc_base/thread.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/video_capture/I_DshowDeviceEvent.h"

namespace vhall {
  class VHTools;

  class AudioNotifyMessage : public rtc::MessageData {
  public:
    AudioNotifyMessage() {
    }

    EDataFlow    flow = (EDataFlow)0;
    ERole        role = (ERole)0;
    std::wstring pwstrDeviceId = L"";
    DWORD        dwNewState = 0;
    PROPERTYKEY  key;

    long         eventCode = 0;
    long         param1 = 0;
    long         param2 = 0;

    // core audio operate event
    std::string errorType = "";
    std::string detail = "";
    long hr = 0;
  };

  class AudioDeviceNotify : public IMMNotificationClient, public rtc::MessageHandler, public I_DShowDevieEvent {
    LONG _cRef;
    IMMDeviceEnumerator *_pEnumerator;

    // Private function to print device-friendly name
    HRESULT _PrintDeviceName(LPCWSTR  pwstrId);

  public:
    AudioDeviceNotify(IMMDeviceEnumerator *pEnumerator);
    ~AudioDeviceNotify();
    bool Init();
    void Destroy();
    enum NotifyEvent {
      E_DefaultDeviceChanged,
      E_DeviceAdded,
      E_DeviceRemoved,
      E_DeviceStateChanged,
      E_PropertyValueChanged,
      E_DshowNotify,
      E_CoreAudioEvent, // core audio operation
    };
    // IUnknown methods -- AddRef, Release, and QueryInterface
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);
    /* CoreAudio Callback methods for device-event notifications. */
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
    /* DShow event */
    virtual void OnAudioNotify(long eventCode, long param1, long param2);
    virtual void OnVideoNotify(long eventCode, long param1, long param2) {};

    // core audio operate event
    void CoreAudioError(std::string errorType, long hr, std::string detail);

    virtual void OnMessage(rtc::Message* msg);
  private:
    void DefaultDeviceChanged(EDataFlow flow, ERole role, std::wstring pwstrDeviceId);
    void DeviceAdded(std::wstring pwstrDeviceId);
    void DeviceRemoved(std::wstring pwstrDeviceId);
    void DeviceStateChanged(std::wstring pwstrDeviceId, DWORD dwNewState);
    void PropertyValueChanged(std::wstring pwstrDeviceId, const PROPERTYKEY key);
    void AudioNotify(long eventCode, long param1, long param2);
    void OnCoreAudioError(std::string errorType, long hr, std::string detail);
  private:
    std::unique_ptr<rtc::Thread>           mThread = nullptr;
    std::shared_ptr<VHTools>               mDevTool = nullptr;
  };
}