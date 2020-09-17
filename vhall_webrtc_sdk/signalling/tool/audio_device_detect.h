#pragma once
#include "vhall_dev_format.h"
#include <stdint.h>
#include <cassert>
#include <cstring>
#include <comdef.h>
#include <dmo.h>
#include <mmsystem.h>
#include <strsafe.h>
#include <uuids.h>
#include <windows.h>
#include <mutex>
#include <vector>

#include <wmcodecdsp.h>      // CLSID_CWMAudioAEC
// (must be before audioclient.h)
#include <Audioclient.h>     // WASAPI
#include <Audiopolicy.h>

// order is important
#include <initguid.h>
#include <Mmdeviceapi.h>     // MMDevice
#include <functiondiscoverykeys_devpkey.h>

#include <endpointvolume.h>
#include <mediaobj.h>        // IMediaObject
#include <iomanip>

// Macro that calls a COM method returning HRESULT value.
#define EXIT_ON_ERROR(hres) \
  do {                      \
    if (FAILED(hres))       \
      goto Exit;            \
  } while (0)

// Macro that continues to a COM error.
#define CONTINUE_ON_ERROR(hres) \
  do {                          \
    if (FAILED(hres))           \
      goto Next;                \
  } while (0)

// Macro that releases a COM object if not NULL.
#define SAFE_RELEASE(p) \
  do {                  \
    if ((p)) {          \
      (p)->Release();   \
      (p) = NULL;       \
    }                   \
  } while (0)

namespace vhall {
  const int kAdmMaxDeviceNameSize = 128;
  const int kAdmMaxFileNameSize = 512;
  const int kAdmMaxGuidSize = 128;

  class ScopedCOMInitializer {
  public:
    // Enum value provided to initialize the thread as an MTA instead of STA.
    enum SelectMTA { kMTA };

    // Constructor for STA initialization.
    ScopedCOMInitializer() { Initialize(COINIT_APARTMENTTHREADED); }

    // Constructor for MTA initialization.
    explicit ScopedCOMInitializer(SelectMTA mta) {
      Initialize(COINIT_MULTITHREADED);
    }

    ~ScopedCOMInitializer() {
      if (SUCCEEDED(hr_))
        CoUninitialize();
    }

    bool succeeded() const { return SUCCEEDED(hr_); }
  private:
    void Initialize(COINIT init) { hr_ = CoInitializeEx(NULL, init); }
    HRESULT hr_;
    ScopedCOMInitializer(const ScopedCOMInitializer&);
    void operator=(const ScopedCOMInitializer&);
  };
}

namespace vhall {
  class AudioDeviceDetect {
  public:
    AudioDeviceDetect();
    virtual ~AudioDeviceDetect();
    bool Init();
    bool Destroy();
    // Device enumeration
    int16_t PlayoutDevices();
    int16_t RecordingDevices();
    int32_t PlayoutDeviceCap(uint16_t index, std::wstring& name, std::wstring& guid, WAVEFORMATEX& fomat);
    int32_t RecordingDeviceCap(uint16_t index, std::wstring& name, std::wstring& guid, WAVEFORMATEX& fomat);
    static bool PlayoutDevicesList(std::vector<std::shared_ptr<AudioDevProperty>>& playoutList);
    static bool RecordingDevicesList(std::vector<std::shared_ptr<AudioDevProperty>>& RecordingList);
  private:
    int32_t _EnumerateEndpointDevicesAll(EDataFlow dataFlow) const;
    int32_t _RefreshDeviceList(EDataFlow dir);
    int16_t _DeviceListCount(EDataFlow dir);
    /* get device name */
    int32_t _GetDefaultDeviceName(EDataFlow dir, ERole role, LPWSTR szBuffer, int bufferLen);
    int32_t _GetListDeviceName(EDataFlow dir, int index, LPWSTR szBuffer, int bufferLen);
    int32_t _GetDeviceName(IMMDevice* pDevice, LPWSTR pszBuffer, int bufferLen);
    /* get device id */
    int32_t _GetListDeviceID(EDataFlow dir, int index, LPWSTR szBuffer, int bufferLen);
    int32_t _GetDefaultDeviceID(EDataFlow dir, ERole role, LPWSTR szBuffer, int bufferLen);
    int32_t _GetDeviceID(IMMDevice* pDevice, LPWSTR pszBuffer, int bufferLen);
    /* get device formatex */
    int32_t _GetDefaultDeviceFormatex(EDataFlow dir, ERole role, WAVEFORMATEX& fomat);
    int32_t _GetListDeviceFormatex(EDataFlow dir, int index, WAVEFORMATEX& format);
    int32_t _GetDeviceFormatex(IMMDevice* pDevice, WAVEFORMATEX& format);
    /* common */
    int32_t _GetDefaultDeviceIndex(EDataFlow dir, ERole role, int* index);
    int32_t _GetDefaultDevice(EDataFlow dir, ERole role, IMMDevice** ppDevice);
    int32_t _GetListDevice(EDataFlow dir, int index, IMMDevice** ppDevice);

    void _TraceCOMError(HRESULT hr) const;
    char* WideToUTF8(const TCHAR* src) const;
  private:
    bool                                _initalize;
    std::mutex                          _mtx;
    mutable char                        _str[512];

    IMMDeviceEnumerator *               _ptrEnumerator;
    IMMDeviceCollection*                _ptrRenderCollection;
    IMMDeviceCollection*                _ptrCaptureCollection;
    IMMDevice*                          _ptrDeviceOut;
    IMMDevice*                          _ptrDeviceIn;
    std::shared_ptr<ScopedCOMInitializer> _comInit;
  };
}