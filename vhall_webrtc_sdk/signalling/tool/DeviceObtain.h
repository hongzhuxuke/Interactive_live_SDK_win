#ifndef __DEVICE_OBTAIN_INCLUDE_H_
#define __DEVICE_OBTAIN_INCLUDE_H_

#include "3rdparty/common/def/VH_ConstDeff.h"
#include <map>
#include <vector>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

using namespace std;

struct MediaOutputInfo;
class IBaseFilter;
class CDeviceObtain
{
public:
   CDeviceObtain();
   ~CDeviceObtain();

public:
   bool Create();
   void Destroy();

   void GetDefaultDevices(DeviceList &deviceList);

   void GetAudioDevices(DeviceList &deviceList, EDataFlow eType);

   void GetVedioDevices(DeviceList &deviceList);

   bool GetResolution(vector<FrameInfo>& mediaOutputList, vector<SIZE>& resolutions, UINT64& minFrameInterval, UINT64& maxFrameInterval, const wstring& deviceName, const wstring& deviceID);

   bool GetClosestResolutionFPS(wstring& deviceName, wstring& deviceID, SIZE &resolution, UINT64 &frameInterval, bool bPrioritizeAspect, bool bPrioritizeFPS);

   void OpenPropertyPagesByName(HWND hwnd, DeviceInfo );

   IBaseFilter* GetDeviceByValue(const IID &enumType, const WCHAR* lpType, const WCHAR* lpName, const WCHAR* lpType2, const WCHAR* lpName2);
private:
   void GetCoreAudioDevices(DeviceList &deviceList,EDataFlow eType);
   void GetDecklinkInputDevices(DeviceList &deviceList);
   void GetDShowInputAudioDevicesByAudioFilter(DeviceList &deviceList,int iDeckLinkDeviceCnt);
   void GetDShowInputAudioDevicesByVideoFilter(DeviceList &deviceList,int iDeckLinkDeviceCnt);
private:
   void Lock();
   void Unlock();
private:
    HANDLE m_apiMutex;
};

#endif //__DEVICE_OBTAIN_INCLUDE_H_
