#include "AudioDeviceNotify.h"
#include "common/vhall_log.h"
#include "signalling/BaseStack/BaseAudioDeviceModule.h"
#include "signalling/BaseStack/BaseStack.h"
#include "signalling/vh_stream.h"
#include "signalling/vh_tools.h"
#include "signalling/tool/vhall_dev_format.h"
#include "signalling/vh_uplog.h"

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

namespace vhall {
  AudioDeviceNotify::AudioDeviceNotify(IMMDeviceEnumerator *pEnumerator) :
    _cRef(1),
    _pEnumerator(pEnumerator) {
  }
  AudioDeviceNotify::~AudioDeviceNotify() {
    Destroy();
  }
  bool AudioDeviceNotify::Init()
  {
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      LOGE("GetAdmInstance failed");
      return false;
    }
    mDevTool = std::make_shared<VHTools>();
    if (!mDevTool) {
      LOGE("create VHTools failed");
      return false;
    }
    mThread = rtc::Thread::Create();
    if (mThread) {
      mThread->Start();
    }
    else {
      LOGE("create thread failed");
      return false;
    }
    return true;
  }

  void AudioDeviceNotify::Destroy() {
    _pEnumerator = nullptr;
    //mAdm.reset();
    mDevTool.reset();
    if (mThread) {
      mThread->Stop();
      mThread.reset();
    }
  }

  void AudioDeviceNotify::CoreAudioError(std::string errorType, long hr, std::string detail) {
    if (!mThread) {
      return;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->errorType = errorType;
      notifyMsg->hr = hr;
      notifyMsg->detail = detail;
      mThread->Post(RTC_FROM_HERE, this, E_CoreAudioEvent, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return;
    }
  }

  void AudioDeviceNotify::OnMessage(rtc::Message * msg) {
    AudioNotifyMessage* msgData = (static_cast<AudioNotifyMessage*>(msg->pdata));
    if (!msgData) {
      return;
    }
    switch (msg->message_id) {
    case E_DefaultDeviceChanged:
      DefaultDeviceChanged(msgData->flow, msgData->role, msgData->pwstrDeviceId);
      break;
    case E_DeviceAdded:
      DeviceAdded(msgData->pwstrDeviceId);
      break;
    case E_DeviceRemoved:
      DeviceRemoved(msgData->pwstrDeviceId);
      break;
    case E_DeviceStateChanged:
      DeviceStateChanged(msgData->pwstrDeviceId, msgData->dwNewState);
      break;
    case E_PropertyValueChanged:
      PropertyValueChanged(msgData->pwstrDeviceId, msgData->key);
      break;
    case E_DshowNotify:
      AudioNotify(msgData->eventCode, msgData->param1, msgData->param2);
      break;
    case E_CoreAudioEvent:
        OnCoreAudioError(msgData->errorType, msgData->hr, msgData->detail);
        break;
    default:
      LOGE("undefine message");
      break;
    }
    /* release msgData */
    if (msgData) {
      delete msgData;
      msgData = nullptr;
    }
  }

  void AudioDeviceNotify::DefaultDeviceChanged(EDataFlow flow, ERole role, std::wstring pwstrDeviceId) {
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    AudioDeviceEvent event;
    event.mType = AUDIO_DEFAULT_DEVICE_CHANGED;
    event.deviceId = pwstrDeviceId;
    mAdm->DispatchEvent(event);
    return;
  }

  void AudioDeviceNotify::DeviceAdded(std::wstring pwstrDeviceId) {
    /* 发现新设备 */
    // todo 上报
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    AudioDeviceEvent event;
    event.mType = AUDIO_DEVICE_ADDED;
    event.deviceId = pwstrDeviceId;
    mAdm->DispatchEvent(event);
  }

  void AudioDeviceNotify::DeviceRemoved(std::wstring pwstrDeviceId) {
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
    if (reportLog) {
      reportLog->upLog(ulAudioDeviceLost, "", AUDIO_DEVICE_REMOVED);
    }
    AudioDeviceEvent event;
    event.mType = AUDIO_DEVICE_REMOVED;
    event.deviceId = pwstrDeviceId;
    mAdm->DispatchEvent(event);
    return;
  }

  void AudioDeviceNotify::DeviceStateChanged(std::wstring pwstrDeviceId, DWORD dwNewState) {
    std::string pszState = "?????";
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    AudioDeviceEvent::DEVICE_STATE state;
    switch (dwNewState)
    {
    case DEVICE_STATE_ACTIVE:
      state = AudioDeviceEvent::ACTIVE;
      break;
    case DEVICE_STATE_DISABLED:
      state = AudioDeviceEvent::DISABLED;
      break;
    case DEVICE_STATE_NOTPRESENT:
      state = AudioDeviceEvent::NOTPRESENT;
      { 
        std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
        if (reportLog) {
          reportLog->upLog(ulAudioDeviceLost, "", AUDIO_DEVICE_REMOVED);
        }
      }
      break;
    case DEVICE_STATE_UNPLUGGED:
      state = AudioDeviceEvent::UNPLUGGED;
      break;
    }
    /* 上报设备ID 事件类型 */
    AudioDeviceEvent event;
    event.mType = AUDIO_DEVICE_STATE;
    if (AudioDeviceEvent::NOTPRESENT == dwNewState) {
      event.mType = AUDIO_CAPTURE_ERROR;
    }
    event.deviceId = pwstrDeviceId;
    event.state = state;
    mAdm->DispatchEvent(event);
    LOGD("New device state is DEVICE_STATE_%s (0x%8.8x)\n", pszState.c_str(), dwNewState);
  }

  void AudioDeviceNotify::PropertyValueChanged(std::wstring pwstrDeviceId, const PROPERTYKEY key) {
    LOGD("Changed device property " "{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
      key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
      key.fmtid.Data4[0], key.fmtid.Data4[1],
      key.fmtid.Data4[2], key.fmtid.Data4[3],
      key.fmtid.Data4[4], key.fmtid.Data4[5],
      key.fmtid.Data4[6], key.fmtid.Data4[7],
      key.pid);
    /* 上报设备ID 事件类型 */
    // 一次设备事件可能上报多次
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    AudioDeviceEvent event;
    event.mType = AUDIO_DEVICE_PROPERTY;
    event.deviceId = pwstrDeviceId;
    mAdm->DispatchEvent(event);
  }

  void AudioDeviceNotify::AudioNotify(long eventCode, long param1, long param2) {
    /* 上报采集错误 */
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
    if (reportLog) {
      reportLog->upLog(ulAudioDeviceLost, "", AUDIO_CAPTURE_ERROR);
    }
    AudioDeviceEvent event;
    event.mType = AUDIO_CAPTURE_ERROR;
    mAdm->DispatchEvent(event);
  }

  void AudioDeviceNotify::OnCoreAudioError(std::string errorType, long hr, std::string detail) {
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return;
    }
    // todo 
    //std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
    //if (reportLog) {
    //  reportLog->upLog(ulAudioDeviceLost, "", AUDIO_CAPTURE_ERROR);
    //}
    AudioDeviceEvent event;
    event.mType = errorType;
    event.hr = hr;
    event.detail = detail;
    mAdm->DispatchEvent(event);
  }

  void AudioDeviceNotify::OnAudioNotify(long eventCode, long param1, long param2) {
    if (!mThread) {
      return;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->eventCode = eventCode;
      notifyMsg->param1 = param1;
      notifyMsg->param2 = param2;
      mThread->Post(RTC_FROM_HERE, this, E_DshowNotify, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return;
    }
  }


  ULONG STDMETHODCALLTYPE AudioDeviceNotify::AddRef() {
    return InterlockedIncrement(&_cRef);
  }

  ULONG STDMETHODCALLTYPE AudioDeviceNotify::Release() {
    ULONG ulRef = InterlockedDecrement(&_cRef);
    if (0 == ulRef)
    {
      delete this;
    }
    return ulRef;
  }

  HRESULT STDMETHODCALLTYPE AudioDeviceNotify::QueryInterface(REFIID riid, VOID **ppvInterface) {
    if (IID_IUnknown == riid)
    {
      AddRef();
      *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
      AddRef();
      *ppvInterface = (IMMNotificationClient*)this;
    }
    else
    {
      *ppvInterface = NULL;
      return E_NOINTERFACE;
    }
    return S_OK;
  }

  // Callback methods for device-event notifications.
  HRESULT STDMETHODCALLTYPE AudioDeviceNotify::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
    if (!mThread) {
      return S_FALSE;
    }
    if (nullptr == pwstrDeviceId) {
      LOGE("error device ID");
      return false;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->flow = flow;
      notifyMsg->role = role;
      notifyMsg->pwstrDeviceId = pwstrDeviceId;
      mThread->Post(RTC_FROM_HERE, this, E_DefaultDeviceChanged, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return S_FALSE;
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE AudioDeviceNotify::OnDeviceAdded(LPCWSTR pwstrDeviceId) {
    if (nullptr == pwstrDeviceId) {
      LOGE("error device ID");
      return false;
    }
    if (!mThread) {
      return S_FALSE;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->pwstrDeviceId = pwstrDeviceId;
      mThread->Post(RTC_FROM_HERE, this, E_DeviceAdded, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return S_FALSE;
    }

    LOGD("Added device\n");
    return S_OK;
  };

  HRESULT STDMETHODCALLTYPE AudioDeviceNotify::OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
    if (nullptr == pwstrDeviceId) {
      LOGE("error device ID");
      return false;
    }
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return S_FALSE;
    }
    if (!mThread) {
      return S_FALSE;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->pwstrDeviceId = pwstrDeviceId;
      mThread->Post(RTC_FROM_HERE, this, E_DeviceRemoved, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return S_FALSE;
    }
    LOGD("Removed device\n");
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE AudioDeviceNotify::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
    if (nullptr == pwstrDeviceId) {
      LOGE("error device ID");
      return false;
    }
    if (!mThread) {
      return S_FALSE;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->pwstrDeviceId = pwstrDeviceId;
      notifyMsg->dwNewState = dwNewState;
      mThread->Post(RTC_FROM_HERE, this, E_DeviceStateChanged, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return S_FALSE;
    }

    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE AudioDeviceNotify::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
  {
    if (nullptr == pwstrDeviceId) {
      LOGE("error device ID");
      return false;
    }
    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return S_FALSE;
    }
    if (!mThread) {
      return S_FALSE;
    }
    AudioNotifyMessage* notifyMsg = new AudioNotifyMessage();
    if (notifyMsg) {
      notifyMsg->pwstrDeviceId = pwstrDeviceId;
      notifyMsg->key = key;
      mThread->Post(RTC_FROM_HERE, this, E_PropertyValueChanged, notifyMsg);
    }
    else {
      LOGE("create AudioNotifyMessage failed");
      return S_FALSE;
    }

    return S_OK;
  }

  HRESULT AudioDeviceNotify::_PrintDeviceName(LPCWSTR pwstrId) {
    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProps = NULL;
    PROPVARIANT varString;

    std::shared_ptr<BaseAudioDeviceModule> mAdm = vhall::AdmConfig::GetAdmInstance();
    if (!mAdm) {
      return S_FALSE;
    }

    CoInitialize(NULL);
    PropVariantInit(&varString);

    if (_pEnumerator == NULL) {
      // Get enumerator for audio endpoint devices.
      hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
        NULL, CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        (void**)&_pEnumerator);
    }
    if (hr == S_OK) {
      hr = _pEnumerator->GetDevice(pwstrId, &pDevice);
    }
    if (hr == S_OK) {
      hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    }
    if (hr == S_OK) {
      // Get the endpoint device's friendly-name property.
      hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
    }
    LOGD("----------------------\nDevice name: \"%S\"\n"
      "  Endpoint ID string: \"%S\"\n", (hr == S_OK) ? varString.pwszVal : L"null device", (pwstrId != NULL) ? pwstrId : L"null ID");

    PropVariantClear(&varString);

    SAFE_RELEASE(pProps);
    SAFE_RELEASE(pDevice);
    CoUninitialize();
    return hr;
  }
}