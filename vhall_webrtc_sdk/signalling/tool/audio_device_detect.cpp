#include "audio_device_detect.h"
#include "../../common/vhall_log.h"
#include <codecvt>
namespace vhall {
  AudioDeviceDetect::AudioDeviceDetect()
  : _initalize(false),
    _ptrEnumerator(NULL),
    _ptrRenderCollection(NULL),
    _ptrCaptureCollection(NULL),
    _ptrDeviceOut(NULL),
    _ptrDeviceIn(NULL) {

  }

  AudioDeviceDetect::~AudioDeviceDetect() {
    LOGI("%s destroyed", __FUNCTION__);
    Destroy();
  }

  bool AudioDeviceDetect::Init() {
    if (!_comInit) {
      _comInit.reset(new ScopedCOMInitializer(ScopedCOMInitializer::kMTA));
      if (!_comInit->succeeded()) {
        // Things will work even if an STA thread is calling this method but we
        // want to ensure that MTA is used and therefore return false here.
        return false;
      }
    }
    HRESULT ret = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&_ptrEnumerator));
    if (_ptrEnumerator) {
      _initalize = true;
    }
    DWORD errorNum = GetLastError();
    bool successed = NULL != _ptrEnumerator;
    if (!successed) {
      LOGE("AudioDeviceDetect::Init failed");
    }
    return successed;
  }

  bool AudioDeviceDetect::Destroy() {
    SAFE_RELEASE(_ptrRenderCollection);
    SAFE_RELEASE(_ptrCaptureCollection);
    SAFE_RELEASE(_ptrDeviceOut);
    SAFE_RELEASE(_ptrDeviceIn);
    SAFE_RELEASE(_ptrEnumerator);
    _comInit.reset();
    LOGI("Destroy done");
    return true;
  }

  int16_t AudioDeviceDetect::PlayoutDevices() {
    if (!_initalize) {
      return -1;
    }
    std::unique_lock<std::mutex> lock(_mtx);
    if (_RefreshDeviceList(eRender) != -1) {
      return (_DeviceListCount(eRender));
    }
    return -1;
  }

  int16_t AudioDeviceDetect::RecordingDevices() {
    if (!_initalize) {
      return -1;
    }
    std::unique_lock<std::mutex> lock(_mtx);
    if (_RefreshDeviceList(eCapture) != -1) {
      return (_DeviceListCount(eCapture));
    }
    return -1;
  }

  int32_t AudioDeviceDetect::PlayoutDeviceCap(uint16_t index, std::wstring& name, std::wstring& guid, WAVEFORMATEX& fomat) {
    if (!_initalize) {
      return -1;
    }

    int role(-1);
    const int16_t nDevices(PlayoutDevices());  // also updates the list of devices
    // Special fix for the case when the user selects '-1' as index (<=> Default
    // Communication Device)
    if (index == (uint16_t)(-1)) {
      role = eCommunications;
      index = 0;
      LOGD("Default Communication endpoint device will be used");
    }
    else if (index == (uint16_t)(-2)) {
      role = eConsole;
      index = 0;
      LOGD("Default Communication endpoint device will be used");
    }

    if ((index > (nDevices - 1))) {
      return -1;
    }
    name = L"";
    guid = L"";
    std::unique_lock<std::mutex> lock(_mtx);
    int32_t ret(-1);
    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName) / sizeof(szDeviceName)[0];
    // Get the endpoint device's friendly-name
    if (role >= 0) {
      ret = _GetDefaultDeviceName(eRender, ERole(role), szDeviceName, bufferLen);
    }
    else {
      ret = _GetListDeviceName(eRender, index, szDeviceName, bufferLen);
    }
    if (ret == 0) {
      name = szDeviceName;
    }
    // Get the endpoint ID string (uniquely identifies the device among all audio
    // endpoint devices)
    if (role >= 0) {
      ret = _GetDefaultDeviceID(eRender, ERole(role), szDeviceName, bufferLen);
    }
    else {
      ret = _GetListDeviceID(eRender, index, szDeviceName, bufferLen);
    }
    if (ret == 0) {
      guid = szDeviceName;
    }
    // Get the endpoint device's wavforamtex
    memset(&fomat, 0, sizeof(WAVEFORMATEX));
    if (role >= 0) {
      ret = _GetDefaultDeviceFormatex(eRender, ERole(role), fomat);
    }
    else {
      ret = _GetListDeviceFormatex(eRender, index, fomat);
    }
    return ret;
  }

  int32_t AudioDeviceDetect::RecordingDeviceCap(uint16_t index, std::wstring& name, std::wstring& guid, WAVEFORMATEX& fomat) {
    if (!_initalize) {
      return -1;
    }

    int role(-1);
    const int16_t nDevices(RecordingDevices());  // also updates the list of devices
    // Special fix for the case when the user selects '-1' as index (<=> Default
    // Communication Device)
    if (index == (uint16_t)(-1)) {
      role = eCommunications;
      index = 0;
      LOGD("Default Communication endpoint device will be used");
    }
    else if (index == (uint16_t)(-2)) {
      role = eConsole;
      index = 0;
      LOGD("Default endpoint device will be used");
    }
    if ((index > (nDevices - 1))) {
      return -1;
    }
    name = L"";
    guid = L"";
    std::unique_lock<std::mutex> lock(_mtx);
    int32_t ret(-1);
    WCHAR szDeviceName[MAX_PATH];
    const int bufferLen = sizeof(szDeviceName) / sizeof(szDeviceName)[0];
    // Get the endpoint device's friendly-name
    if (role >= 0) {
      ret = _GetDefaultDeviceName(eCapture, ERole(role), szDeviceName, bufferLen);
    }
    else {
      ret = _GetListDeviceName(eCapture, index, szDeviceName, bufferLen);
    }
    if (ret == 0) {
      name = szDeviceName;
    }
    // Get the endpoint ID string (uniquely identifies the device among all audio
    // endpoint devices)
    if (role >= 0) {
      ret = _GetDefaultDeviceID(eCapture, ERole(role), szDeviceName, bufferLen);
    }
    else {
      ret = _GetListDeviceID(eCapture, index, szDeviceName, bufferLen);
    }
    if (ret == 0) {
      guid = szDeviceName;
    }
    // Get the endpoint device's wavforamtex
    memset(&fomat, 0, sizeof(WAVEFORMATEX));
    if (role >= 0) {
      ret = _GetDefaultDeviceFormatex(eCapture, ERole(role), fomat);
    }
    else {
      ret = _GetListDeviceFormatex(eCapture, index, fomat);
    }
    return ret;
  }

  bool AudioDeviceDetect::PlayoutDevicesList(std::vector<std::shared_ptr<AudioDevProperty>>& playoutList) {

    std::shared_ptr<AudioDeviceDetect> audioDevDetect = std::make_shared<AudioDeviceDetect>();
    if (!audioDevDetect || !audioDevDetect.get() || !audioDevDetect->Init()) {
      return false;
    }
    int16_t numDev = audioDevDetect->PlayoutDevices();
    if (numDev > 0) {
      std::wstring name = L"";
      std::wstring guid = L"";
      WAVEFORMATEX format = { 0 };
      /* Get Default-communication dev info */
      audioDevDetect->PlayoutDeviceCap(-1, name, guid, format);
      std::wstring defaultComunicationGuid = guid;
      /* Get default dev info */
      audioDevDetect->PlayoutDeviceCap(-2, name, guid, format);
      std::wstring defaultGuid = guid;
      for (auto i = 0; i < numDev; i++) {
        name = L"";
        guid = L"";
        memset(&format, 0, sizeof(format));
        audioDevDetect->PlayoutDeviceCap(i, name, guid, format);
        WaveFormat waveFormat;
        waveFormat.wFormatTag = format.wFormatTag;
        waveFormat.nChannels = format.nChannels;
        waveFormat.nSamplesPerSec = format.nSamplesPerSec;
        waveFormat.nAvgBytesPerSec = format.nAvgBytesPerSec;
        waveFormat.nBlockAlign = format.nBlockAlign;
        waveFormat.wBitsPerSample = format.wBitsPerSample;
        waveFormat.cbSize = format.cbSize;
        std::shared_ptr<AudioDevProperty> audioDevPro = std::make_shared<AudioDevProperty>(i, name , guid, waveFormat);
        if (audioDevPro) {
          if (!defaultGuid.compare(guid)) {
            audioDevPro->isDefaultDevice = true;
          }
          if (!defaultComunicationGuid.compare(guid)) {
            audioDevPro->isDefalutCommunicationDevice = true;
          }
          playoutList.push_back(audioDevPro);
        }
      }
    }
    return true;
  }

  bool AudioDeviceDetect::RecordingDevicesList(std::vector<std::shared_ptr<AudioDevProperty>>& RecordingList) {
    std::shared_ptr<AudioDeviceDetect> audioDevDetect = std::make_shared<AudioDeviceDetect>();
    if (!audioDevDetect || !audioDevDetect.get() || !audioDevDetect->Init()) {
      return false;
    }
    int16_t numDev = audioDevDetect->RecordingDevices();
    if (numDev > 0) {
      std::wstring name = L"";
      std::wstring guid = L"";
      WAVEFORMATEX format = { 0 };
      /* Get Default-communication dev info */
      audioDevDetect->RecordingDeviceCap(-1, name, guid, format);
      std::wstring defaultComunicationGuid = guid;
      /* Get default dev info */
      name = L"";
      guid = L"";
      memset(&format, 0, sizeof(format));
      audioDevDetect->RecordingDeviceCap(-2, name, guid, format);
      std::wstring defaultGuid = guid;
      for (auto i = 0; i < numDev; i++) {
        name = L"";
        guid = L"";
        memset(&format, 0, sizeof(format));
        audioDevDetect->RecordingDeviceCap(i, name, guid, format);
        WaveFormat waveFormat;
        waveFormat.wFormatTag = format.wFormatTag;
        waveFormat.nChannels = format.nChannels;
        waveFormat.nSamplesPerSec = format.nSamplesPerSec;
        waveFormat.nAvgBytesPerSec = format.nAvgBytesPerSec;
        waveFormat.nBlockAlign = format.nBlockAlign;
        waveFormat.wBitsPerSample = format.wBitsPerSample;
        waveFormat.cbSize = format.cbSize;
        std::shared_ptr<AudioDevProperty> audioDevPro = std::make_shared<AudioDevProperty>(i, name, guid, waveFormat);
        if (audioDevPro) {
          if (!defaultGuid.compare(guid)) {
            audioDevPro->isDefaultDevice = true;
          }
          if (!defaultComunicationGuid.compare(guid)) {
            audioDevPro->isDefalutCommunicationDevice = true;
          }
          RecordingList.push_back(audioDevPro);
        }
      }
    }
    return true;
  }

  int32_t AudioDeviceDetect::_EnumerateEndpointDevicesAll(EDataFlow dataFlow) const {
    LOGI("%s", __FUNCTION__);

    assert(_ptrEnumerator != NULL);

    HRESULT hr = S_OK;
    IMMDeviceCollection* pCollection = NULL;
    IMMDevice* pEndpoint = NULL;
    IPropertyStore* pProps = NULL;
    IAudioEndpointVolume* pEndpointVolume = NULL;
    LPWSTR pwszID = NULL;
    // use the IMMDeviceCollection interface...
    UINT count = 0;
    // Generate a collection of audio endpoint devices in the system.
    // Get states for *all* endpoint devices.
    // Output: IMMDeviceCollection interface.
    hr = _ptrEnumerator->EnumAudioEndpoints(dataFlow,  // data-flow direction (input parameter)
                                            DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED | DEVICE_STATE_UNPLUGGED,
                                            &pCollection);  // release interface when done
    EXIT_ON_ERROR(hr);
    // Retrieve a count of the devices in the device collection.
    hr = pCollection->GetCount(&count);
    EXIT_ON_ERROR(hr);
    if (dataFlow == eRender) {
      LOGD("#rendering endpoint devices (counting all): %d", count);
    }
    else if (dataFlow == eCapture) {
      LOGD("#capturing endpoint devices (counting all): %d", count);
    }
    if (count == 0) {
      return 0;
    }

    // Each loop prints the name of an endpoint device.
    for (ULONG i = 0; i < count; i++) {
      DWORD dwHwSupportMask = 0;
      UINT nChannelCount(0);
      LOGD("Endpoint %d :", i);

      // Get pointer to endpoint number i.
      // Output: IMMDevice interface.
      hr = pCollection->Item(i, &pEndpoint);
      CONTINUE_ON_ERROR(hr);

      // use the IMMDevice interface of the specified endpoint device...
      // Get the endpoint ID string (uniquely identifies the device among all
      // audio endpoint devices)
      hr = pEndpoint->GetId(&pwszID);
      CONTINUE_ON_ERROR(hr);
      LOGD("ID string    : %s", pwszID);

      // Retrieve an interface to the device's property store.
      // Output: IPropertyStore interface.
      hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
      CONTINUE_ON_ERROR(hr);

      // use the IPropertyStore interface...

      PROPVARIANT varName;
      // Initialize container for property value.
      PropVariantInit(&varName);

      // Get the endpoint's friendly-name property.
      // Example: "Speakers (Realtek High Definition Audio)"
      hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
      CONTINUE_ON_ERROR(hr);
      LOGD("friendly name: \"%s\"", varName.pwszVal);

      // Get the endpoint's current device state
      DWORD dwState;
      hr = pEndpoint->GetState(&dwState);
      CONTINUE_ON_ERROR(hr);
      if (dwState & DEVICE_STATE_ACTIVE)
        LOGD("state (%x)  : *ACTIVE*", dwState);
      if (dwState & DEVICE_STATE_DISABLED)
        LOGD("state (%x)  : DISABLED", dwState);
      if (dwState & DEVICE_STATE_NOTPRESENT)
        LOGD("state (%x)  : NOTPRESENT", dwState);
      if (dwState & DEVICE_STATE_UNPLUGGED)
        LOGD("state (%x)  : UNPLUGGED", dwState);

      // Check the hardware volume capabilities.
      hr = pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
      CONTINUE_ON_ERROR(hr);
      hr = pEndpointVolume->QueryHardwareSupport(&dwHwSupportMask);
      CONTINUE_ON_ERROR(hr);
      if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_VOLUME) {
        // The audio endpoint device supports a hardware volume control
        LOGD("hwmask (%x) : HARDWARE_SUPPORT_VOLUME", dwHwSupportMask);
      }
      if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_MUTE) {
        // The audio endpoint device supports a hardware mute control
        LOGD("hwmask (%x) : HARDWARE_SUPPORT_MUTE", dwHwSupportMask);
      }
      if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_METER) {
        // The audio endpoint device supports a hardware peak meter
        LOGD("hwmask (%x) : HARDWARE_SUPPORT_METER", dwHwSupportMask);
      }
      // Check the channel count (#channels in the audio stream that enters or
      // leaves the audio endpoint device)
      hr = pEndpointVolume->GetChannelCount(&nChannelCount);
      CONTINUE_ON_ERROR(hr);
      LOGD("#channels    : %d", nChannelCount);

      if (dwHwSupportMask & ENDPOINT_HARDWARE_SUPPORT_VOLUME) {
        // Get the volume range.
        float fLevelMinDB(0.0);
        float fLevelMaxDB(0.0);
        float fVolumeIncrementDB(0.0);
        hr = pEndpointVolume->GetVolumeRange(&fLevelMinDB, &fLevelMaxDB, &fVolumeIncrementDB);
        CONTINUE_ON_ERROR(hr);
        LOGD("volume range : %f (min), %f (max), %f (inc) [dB]", fLevelMinDB, fLevelMaxDB, fVolumeIncrementDB);

        // The volume range from vmin = fLevelMinDB to vmax = fLevelMaxDB is
        // divided into n uniform intervals of size vinc = fVolumeIncrementDB,
        // where n = (vmax ?vmin) / vinc. The values vmin, vmax, and vinc are
        // measured in decibels. The client can set the volume level to one of n +
        // 1 discrete values in the range from vmin to vmax.
        int n = (int)((fLevelMaxDB - fLevelMinDB) / fVolumeIncrementDB);
        LOGD("#intervals   : %d", n);

        // Get information about the current step in the volume range.
        // This method represents the volume level of the audio stream that enters
        // or leaves the audio endpoint device as an index or "step" in a range of
        // discrete volume levels. Output value nStepCount is the number of steps
        // in the range. Output value nStep is the step index of the current
        // volume level. If the number of steps is n = nStepCount, then step index
        // nStep can assume values from 0 (minimum volume) to n ?1 (maximum
        // volume).
        UINT nStep(0);
        UINT nStepCount(0);
        hr = pEndpointVolume->GetVolumeStepInfo(&nStep, &nStepCount);
        CONTINUE_ON_ERROR(hr);
        LOGD("volume steps : %u (nStep), %u (nStepCount)", nStep, nStepCount);
      }
    Next:
      if (FAILED(hr)) {
        LOGD("Error when logging device information");
      }
      CoTaskMemFree(pwszID);
      pwszID = NULL;
      PropVariantClear(&varName);
      SAFE_RELEASE(pProps);
      SAFE_RELEASE(pEndpoint);
      SAFE_RELEASE(pEndpointVolume);
    }
    SAFE_RELEASE(pCollection);
    return 0;

  Exit:
    _TraceCOMError(hr);
    CoTaskMemFree(pwszID);
    pwszID = NULL;
    SAFE_RELEASE(pCollection);
    SAFE_RELEASE(pEndpoint);
    SAFE_RELEASE(pEndpointVolume);
    SAFE_RELEASE(pProps);
    return -1;
  }

  int32_t AudioDeviceDetect::_RefreshDeviceList(EDataFlow dir) {
    HRESULT hr = S_OK;
    IMMDeviceCollection* pCollection = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(_ptrEnumerator != NULL);

    // Create a fresh list of devices using the specified direction
    hr = _ptrEnumerator->EnumAudioEndpoints(dir, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pCollection);
      return -1;
    }

    if (dir == eRender) {
      SAFE_RELEASE(_ptrRenderCollection);
      _ptrRenderCollection = pCollection;
    }
    else {
      SAFE_RELEASE(_ptrCaptureCollection);
      _ptrCaptureCollection = pCollection;
    }
    return 0;
  }
  int16_t AudioDeviceDetect::_DeviceListCount(EDataFlow dir) {
    LOGI(" ", __FUNCTION__);

    HRESULT hr = S_OK;
    UINT count = 0;

    assert(eRender == dir || eCapture == dir);

    if (eRender == dir && NULL != _ptrRenderCollection) {
      hr = _ptrRenderCollection->GetCount(&count);
    }
    else if (NULL != _ptrCaptureCollection) {
      hr = _ptrCaptureCollection->GetCount(&count);
    }

    if (FAILED(hr)) {
      _TraceCOMError(hr);
      return -1;
    }

    return static_cast<int16_t>(count);
    return 0;
  }

  int32_t AudioDeviceDetect::_GetDefaultDeviceName(EDataFlow dir, ERole role, LPWSTR szBuffer, int bufferLen) {
    LOGI("%s", __FUNCTION__);
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(role == eConsole || role == eCommunications);
    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(dir, role, &pDevice);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pDevice);
      return -1;
    }
    int32_t res = _GetDeviceName(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
  }

  int32_t AudioDeviceDetect::_GetListDeviceName(EDataFlow dir, int index, LPWSTR szBuffer, int bufferLen) {
    LOGI("%s", __FUNCTION__);
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;
    assert(dir == eRender || dir == eCapture);
    if (eRender == dir && NULL != _ptrRenderCollection) {
      hr = _ptrRenderCollection->Item(index, &pDevice);
    }
    else if (NULL != _ptrCaptureCollection) {
      hr = _ptrCaptureCollection->Item(index, &pDevice);
    }
    IPropertyStore* pProps = NULL;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr)) {
      pProps;
      SAFE_RELEASE(pProps);
    }
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pDevice);
      return -1;
    }
    int32_t res = _GetDeviceName(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
  }

  int32_t AudioDeviceDetect::_GetDeviceName(IMMDevice* pDevice, LPWSTR pszBuffer, int bufferLen) {
    LOGI("%s", __FUNCTION__);
    static const WCHAR szDefault[] = L"<Device not available>";
    HRESULT hr = E_FAIL;
    IPropertyStore* pProps = NULL;
    PROPVARIANT varName;

    assert(pszBuffer != NULL);
    assert(bufferLen > 0);

    if (pDevice != NULL) {
      hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
      if (FAILED(hr)) {
        LOGE("IMMDevice::OpenPropertyStore failed, hr = %x", hr);
      }
    }
    // Initialize container for property value.
    PropVariantInit(&varName);
    if (SUCCEEDED(hr)) {
      // Get the endpoint device's friendly-name property.
      hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
      if (FAILED(hr)) {
        LOGE("IPropertyStore::GetValue failed, hr = %x", hr);
      }
    }
    if ((SUCCEEDED(hr)) && (VT_EMPTY == varName.vt)) {
      hr = E_FAIL;
      LOGE("IPropertyStore::GetValue returned no value, hr = %x", hr);
    }
    if ((SUCCEEDED(hr)) && (VT_LPWSTR != varName.vt)) {
      // The returned value is not a wide null terminated string.
      hr = E_UNEXPECTED;
      LOGE("IPropertyStore::GetValue returned unexpected, type, hr = %x", hr) ;
    }
    if (SUCCEEDED(hr) && (varName.pwszVal != NULL)) {
      // Copy the valid device name to the provided ouput buffer.
      wcsncpy_s(pszBuffer, bufferLen, varName.pwszVal, _TRUNCATE);
    }
    else {
      // Failed to find the device name.
      wcsncpy_s(pszBuffer, bufferLen, szDefault, _TRUNCATE);
    }
    PropVariantClear(&varName);
    SAFE_RELEASE(pProps);
    return 0;
  }

  int32_t AudioDeviceDetect::_GetListDeviceID(EDataFlow dir, int index, LPWSTR szBuffer, int bufferLen) {
    LOGI("%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;

    assert(dir == eRender || dir == eCapture);

    if (eRender == dir && NULL != _ptrRenderCollection) {
      hr = _ptrRenderCollection->Item(index, &pDevice);
    }
    else if (NULL != _ptrCaptureCollection) {
      hr = _ptrCaptureCollection->Item(index, &pDevice);
    }
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pDevice);
      return -1;
    }
    int32_t res = _GetDeviceID(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
  }

  int32_t AudioDeviceDetect::_GetDefaultDeviceID(EDataFlow dir, ERole role, LPWSTR szBuffer, int bufferLen) {
    LOGI("%s", __FUNCTION__);

    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(role == eConsole || role == eCommunications);
    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(dir, role, &pDevice);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pDevice);
      return -1;
    }
    int32_t res = _GetDeviceID(pDevice, szBuffer, bufferLen);
    SAFE_RELEASE(pDevice);
    return res;
  }

  int32_t AudioDeviceDetect::_GetDefaultDeviceIndex(EDataFlow dir, ERole role, int* index) {
    LOGI("%s", __FUNCTION__);

    HRESULT hr = S_OK;
    WCHAR szDefaultDeviceID[MAX_PATH] = { 0 };
    WCHAR szDeviceID[MAX_PATH] = { 0 };

    const size_t kDeviceIDLength = sizeof(szDeviceID) / sizeof(szDeviceID[0]);
    assert(kDeviceIDLength == sizeof(szDefaultDeviceID) / sizeof(szDefaultDeviceID[0]));
    if (_GetDefaultDeviceID(dir, role, szDefaultDeviceID, kDeviceIDLength) == -1) {
      return -1;
    }
    IMMDeviceCollection* collection = _ptrCaptureCollection;
    if (dir == eRender) {
      collection = _ptrRenderCollection;
    }
    if (!collection) {
      LOGE("Device collection not valid");
      return -1;
    }
    UINT count = 0;
    hr = collection->GetCount(&count);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      return -1;
    }
    *index = -1;
    for (UINT i = 0; i < count; i++) {
      memset(szDeviceID, 0, sizeof(szDeviceID));
      //rtc::scoped_refptr<IMMDevice> device;
      IMMDevice * device = nullptr;
      {
        IMMDevice* ptrDevice = NULL;
        hr = collection->Item(i, &ptrDevice);
        if (FAILED(hr) || ptrDevice == NULL) {
          _TraceCOMError(hr);
          return -1;
        }
        device = ptrDevice;
        SAFE_RELEASE(ptrDevice);
      }
      if (_GetDeviceID(device, szDeviceID, kDeviceIDLength) == -1) {
        return -1;
      }
      if (wcsncmp(szDefaultDeviceID, szDeviceID, kDeviceIDLength) == 0) {
        // Found a match.
        *index = i;
        break;
      }
      SAFE_RELEASE(device);
    }
    if (*index == -1) {
      LOGE("Unable to find collection index for default device");
      return -1;
    }
    return 0;
  }

  int32_t AudioDeviceDetect::_GetDeviceID(IMMDevice* pDevice, LPWSTR pszBuffer, int bufferLen) {
    LOGI("%s", __FUNCTION__);
    static const WCHAR szDefault[] = L"<Device not available>";
    HRESULT hr = E_FAIL;
    LPWSTR pwszID = NULL;

    assert(pszBuffer != NULL);
    assert(bufferLen > 0);

    if (pDevice != NULL) {
      hr = pDevice->GetId(&pwszID);
    }
    if (hr == S_OK) {
      // Found the device ID.
      wcsncpy_s(pszBuffer, bufferLen, pwszID, _TRUNCATE);
    }
    else {
      // Failed to find the device ID.
      wcsncpy_s(pszBuffer, bufferLen, szDefault, _TRUNCATE);
    }
    CoTaskMemFree(pwszID);
    return 0;
  }

  int32_t AudioDeviceDetect::_GetDefaultDeviceFormatex(EDataFlow dir, ERole role, WAVEFORMATEX& fomat) {
    LOGI("%s", __FUNCTION__);
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;

    assert(dir == eRender || dir == eCapture);
    assert(role == eConsole || role == eCommunications);
    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(dir, role, &pDevice);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pDevice);
      return -1;
    }
    int32_t res = _GetDeviceFormatex(pDevice, fomat);
    SAFE_RELEASE(pDevice);
    return res;
  }

  int32_t AudioDeviceDetect::_GetListDeviceFormatex(EDataFlow dir, int index, WAVEFORMATEX& format) {
    LOGI("%s", __FUNCTION__);
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;
    assert(dir == eRender || dir == eCapture);
    if (eRender == dir && NULL != _ptrRenderCollection) {
      hr = _ptrRenderCollection->Item(index, &pDevice);
    }
    else if (NULL != _ptrCaptureCollection) {
      hr = _ptrCaptureCollection->Item(index, &pDevice);
    }
    IPropertyStore* pProps = NULL;
    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr)) {
      pProps;
      SAFE_RELEASE(pProps);
    }
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pDevice);
      return -1;
    }
    int32_t res = _GetDeviceFormatex(pDevice, format);
    SAFE_RELEASE(pDevice);
    return res;
  }

  int32_t AudioDeviceDetect::_GetDeviceFormatex(IMMDevice* pDevice, WAVEFORMATEX& format) {
    LOGI("%s", __FUNCTION__);
    static const WCHAR szDefault[] = L"<Device not available>";
    HRESULT hr = E_FAIL;
    IPropertyStore* pProps = NULL;
    PROPVARIANT prop;

    assert(pDevice != NULL);

    if (pDevice != NULL) {
      hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
      if (FAILED(hr)) {
        LOGE("IMMDevice::OpenPropertyStore failed, hr = %x", hr);
      }
    }
    // Initialize container for property value.
    PropVariantInit(&prop);
    memset(&format, 0, sizeof(PWAVEFORMATEX));
    if (SUCCEEDED(hr)) {
      // Get the endpoint device's friendly-name property.
      hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
      if (SUCCEEDED(hr) && prop.blob.pBlobData) {
        memcpy(&format, (PWAVEFORMATEX)prop.blob.pBlobData, sizeof(WAVEFORMATEX));
        LOGE("nSamplesPerSec:%d, nChannels:%d, hr: %x", format.nSamplesPerSec, format.nChannels, hr);
      }
      else if (FAILED(hr)) {
        LOGE("IPropertyStore::GetValue failed, hr = %x", hr);
      }
    }
    if ((SUCCEEDED(hr)) && (VT_EMPTY == prop.vt)) {
      hr = E_FAIL;
      LOGE("IPropertyStore::GetValue returned no value, hr = %x", hr);
    }
    if ((SUCCEEDED(hr)) && (VT_LPWSTR != prop.vt)) {
      // The returned value is not a wide null terminated string.
      hr = E_UNEXPECTED;
      LOGE("IPropertyStore::GetValue returned unexpected, type, hr = %x", hr);
    }
    PropVariantClear(&prop);
    SAFE_RELEASE(pProps);
    return 0;
  }

  int32_t AudioDeviceDetect::_GetDefaultDevice(EDataFlow dir, ERole role, IMMDevice** ppDevice) {
    LOGI("%s", __FUNCTION__);
    HRESULT hr(S_OK);
    
    assert(_ptrEnumerator != NULL);

    hr = _ptrEnumerator->GetDefaultAudioEndpoint(dir, role, ppDevice);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      return -1;
    }
    return 0;
  }

  int32_t AudioDeviceDetect::_GetListDevice(EDataFlow dir, int index, IMMDevice** ppDevice) {
    HRESULT hr(S_OK);

    assert(_ptrEnumerator != NULL);

    IMMDeviceCollection* pCollection = NULL;
    hr = _ptrEnumerator->EnumAudioEndpoints(
      dir,
      DEVICE_STATE_ACTIVE,  // only active endpoints are OK
      &pCollection);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pCollection);
      return -1;
    }
    hr = pCollection->Item(index, ppDevice);
    if (FAILED(hr)) {
      _TraceCOMError(hr);
      SAFE_RELEASE(pCollection);
      return -1;
    }
    return 0;
  }

  void AudioDeviceDetect::_TraceCOMError(HRESULT hr) const {
    TCHAR buf[MAXERRORLENGTH];
    TCHAR errorText[MAXERRORLENGTH];

    const DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD dwLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    // Gets the system's human readable message string for this HRESULT.
    // All error message in English by default.
    DWORD messageLength = ::FormatMessageW(dwFlags, 0, hr, dwLangID, errorText, MAXERRORLENGTH, NULL);

    assert(messageLength <= MAXERRORLENGTH);

    // Trims tailing white space (FormatMessage() leaves a trailing cr-lf.).
    for (; messageLength && ::isspace(errorText[messageLength - 1]); --messageLength) {
      errorText[messageLength - 1] = '\0';
    }

    LOGE("Core Audio method failed (hr=%l)");
    StringCchPrintf(buf, MAXERRORLENGTH, TEXT("Error details: "));
    StringCchCat(buf, MAXERRORLENGTH, errorText);
    LOGE("error:%s",WideToUTF8(buf));
  }

  char* AudioDeviceDetect::WideToUTF8(const TCHAR* src) const {
#ifdef UNICODE
    const size_t kStrLen = sizeof(_str);
    memset(_str, 0, kStrLen);
    // Get required size (in bytes) to be able to complete the conversion.
    unsigned int required_size = (unsigned int)WideCharToMultiByte(CP_UTF8, 0, src, -1, _str, 0, 0, 0);
    if (required_size <= kStrLen) {
      // Process the entire input string, including the terminating null char.
      if (WideCharToMultiByte(CP_UTF8, 0, src, -1, _str, kStrLen, 0, 0) == 0)
        memset(_str, 0, kStrLen);
    }
    return _str;
#else
    return const_cast<char*>(src);
#endif
  }
}