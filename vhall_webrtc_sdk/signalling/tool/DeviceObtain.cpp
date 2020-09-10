#pragma once
#include <windows.h>
#include <minwindef.h>
#include <tchar.h>
#include <string>
#include <list>
#include <dshow.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <corecrt_wstring.h>
#include <cassert>

#include "DeviceObtain.h"
#include <propsys.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "3rdparty/common/UtilityTool.h"
#include "3rdparty/common/interface/IDeckLinkDevice.h"
#include "../../vhall_live_core/libdshowcapture/source/dshow-base.hpp"
#include "3rdparty/common/baseClass/DebugTrace.h"

#pragma comment(lib, "strmiids.lib")



void WINAPI FreeMediaType_(AM_MEDIA_TYPE& mt) {
   if (mt.cbFormat != 0) {
      CoTaskMemFree((LPVOID)mt.pbFormat);
      mt.cbFormat = 0;
      mt.pbFormat = NULL;
   }

   SafeRelease(mt.pUnk);
}

struct MediaOutputInfo {
   VideoOutputType videoType;
   AM_MEDIA_TYPE *mediaType;
   UINT64 minFrameInterval, maxFrameInterval;
   UINT minCX, minCY;
   UINT maxCX, maxCY;
   UINT xGranularity, yGranularity;
   bool bUsingFourCC;
   inline void FreeData() {
      FreeMediaType_(*mediaType);
      CoTaskMemFree(mediaType);
   }
};


#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))

//CTSTR lpRoxioVideoCaptureGUID = TEXT("{6994AD05-93EF-11D0-A3-CC-00-A0-C9-22-31-96}");
const GUID PIN_CATEGORY_ROXIOCAPTURE = { 0x6994AD05, 0x93EF, 0x11D0, { 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96 } };

const GUID MEDIASUBTYPE_I420 = { 0x30323449, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

CDeviceObtain::CDeviceObtain() {
    CRITICAL_SECTION *pSection = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(pSection);
    m_apiMutex=pSection;
}

CDeviceObtain::~CDeviceObtain() {
   DeleteCriticalSection((CRITICAL_SECTION*)m_apiMutex);
   free(m_apiMutex);
}
void CDeviceObtain::Lock()
{
   EnterCriticalSection((CRITICAL_SECTION*)m_apiMutex);
}
void CDeviceObtain::Unlock()
{
   LeaveCriticalSection((CRITICAL_SECTION*)m_apiMutex);
}

bool CDeviceObtain::Create() {
   return true;
}

void CDeviceObtain::Destroy() {

}

void CDeviceObtain::GetDefaultDevices(DeviceList &deviceList) {

}
void CDeviceObtain::GetCoreAudioDevices(DeviceList &deviceList,EDataFlow eType)
{
   const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
   const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
   IMMDeviceEnumerator *pEnumerator = NULL;
   HRESULT err;
   HRESULT Hr = ::CoInitialize(NULL);
   // Core Audio
   err = CoCreateInstance(CLSID_MMDeviceEnumerator,NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
   if (FAILED(err) || NULL == pEnumerator) {
      assert(FALSE);
      ::CoUninitialize();
      return;
   }

   IMMDeviceCollection *collection = NULL;
   DWORD flags = DEVICE_STATE_ACTIVE;
   //flags |= DEVICE_STATE_UNPLUGGED;

   IMMDevice* pIMMDevice = NULL;
   err = pEnumerator->GetDefaultAudioEndpoint(eType, eConsole, &pIMMDevice);
   if (FAILED(err) || pIMMDevice == NULL) {
      SafeRelease(pEnumerator);
      ::CoUninitialize();
      return;
   }

   const wchar_t* wsDefaultDeviceID = NULL;
   if (NULL != pIMMDevice) {
      err = pIMMDevice->GetId((LPWSTR*)&wsDefaultDeviceID);
   }

   if (FAILED(err)) {
      assert(FALSE);
      SafeRelease(pIMMDevice);
      SafeRelease(pEnumerator);
      ::CoUninitialize();
      return;
   }

   err = pEnumerator->EnumAudioEndpoints(eType, flags, &collection);
   if (FAILED(err)) {
      assert(FALSE);
      SafeRelease(collection);
      SafeRelease(pEnumerator);
      SafeRelease(pIMMDevice);
      ::CoUninitialize();
      return;
   }

   UINT count;
   if (SUCCEEDED(collection->GetCount(&count))) {
      for (UINT i = 0; i < count; i++) {
         IMMDevice *device = NULL;
         if (SUCCEEDED(collection->Item(i, &device))) {
            const wchar_t* wstrID = NULL;
            if (SUCCEEDED(device->GetId((LPWSTR*)&wstrID))) {
               IPropertyStore *store = NULL;
               if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store))) {
                  PROPVARIANT varName;
                  PropVariantInit(&varName);

                  if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &varName))) {
                     wchar_t* wstrName = varName.pwszVal;
                     wstring wsTmpName = varName.pwszVal;
                     wchar_t * lowerDeviceName = _wcslwr((wchar_t*)wsTmpName.c_str());
                     DeviceInfo info;
                     wcscpy(info.m_sDeviceID, wstrID);
                     wcscpy(info.m_sDeviceName, wstrName);

                     wsprintf(info.m_sDeviceDisPlayName,L"%s _Core Audio", wstrName);   
                     info.m_sDeviceType=TYPE_COREAUDIO;
                     deviceList.push_back(info);
                  }
               }
               SafeRelease(store);
            } 
         }
         SafeRelease(device);
      }
   }
   //-------------------------------------------------------
   SafeRelease(collection);
   SafeRelease(pEnumerator);
   CoUninitialize();
}

void CDeviceObtain::GetDecklinkInputDevices(DeviceList &deviceList)
{

   unsigned int iDeckLinkDeviceCnt = GetDeckLinkDeviceNum();
   for (unsigned int i = 0; i < iDeckLinkDeviceCnt; i++) {
      const wchar_t* deviceName = GetDeckLinkDeviceName(i);
      if (deviceName != NULL) {
         DeviceInfo info;
         wcscpy(info.m_sDeviceName, deviceName);
         wsprintf(info.m_sDeviceDisPlayName,L"%s capture card API", deviceName);   
         info.m_sDeviceType=TYPE_DECKLINK;
         deviceList.push_back(info);
      }
   }
}

void CDeviceObtain::GetDShowInputAudioDevicesByAudioFilter(DeviceList &deviceList,int iDeckLinkDeviceCnt)
{
   ICreateDevEnum *deviceEnum = NULL;
   IEnumMoniker *videoDeviceEnum = NULL;

   HRESULT err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
   if (FAILED(err) || deviceEnum == NULL) {
      return ;
   }

   err = deviceEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &videoDeviceEnum, 0);
   if (FAILED(err) || videoDeviceEnum == NULL) {
      SafeRelease(deviceEnum);
      return ;
   }

   IMoniker *deviceMoniker = NULL;
   DWORD deviceCount = 0;
   int i = 0;
   while (videoDeviceEnum->Next(1, &deviceMoniker, &deviceCount) == S_OK) {
      IPropertyBag *propertyData = NULL;
      err = deviceMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
      if (SUCCEEDED(err)) {
         VARIANT friendlyNameValue, devicePathValue;
         friendlyNameValue.vt = VT_BSTR;
         friendlyNameValue.bstrVal = NULL;
         devicePathValue.vt = VT_BSTR;
         devicePathValue.bstrVal = NULL;

         err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
         propertyData->Read(L"DevicePath", &devicePathValue, NULL);
         if (SUCCEEDED(err)) {
            std::wstring strDeviceName = (const wchar_t *)friendlyNameValue.bstrVal;
            std::wstring tmpDeviceName = strDeviceName;
            wchar_t * lowerDeviceName = _wcslwr((wchar_t *)tmpDeviceName.c_str());
            if (wcsstr(lowerDeviceName, L"blackmagic ") != NULL || wcsstr(lowerDeviceName, L"decklink ") != NULL) {
               if (iDeckLinkDeviceCnt == 0 && wcsstr(lowerDeviceName, L"blackmagic web presenter") == NULL){
                  continue;
               }
            }
            
            IBaseFilter *filter = NULL;
            IPropertyBag *pBag = 0;
            err = deviceMoniker->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
            if (SUCCEEDED(err)) {
               DeviceInfo info;
               if(devicePathValue.bstrVal==NULL)
               {
                  memset(info.m_sDeviceID,0,sizeof(info.m_sDeviceID));
               }
               else
               {
                  wcscpy(info.m_sDeviceID, devicePathValue.bstrVal);
               }
               wcscpy(info.m_sDeviceName, friendlyNameValue.bstrVal);
               wsprintf(info.m_sDeviceDisPlayName,L"%s _DShow", info.m_sDeviceName);   
               info.m_sDeviceType=TYPE_DSHOW_AUDIO;
               deviceList.push_back(info);
            }
            SafeRelease(filter);
         }
      }
      SafeRelease(propertyData);
      SafeRelease(deviceMoniker);
   }
   SafeRelease(videoDeviceEnum);
   SafeRelease(deviceEnum);
}
void CDeviceObtain::GetDShowInputAudioDevicesByVideoFilter(DeviceList &deviceList,int iDeckLinkDeviceCnt)
{
   ICreateDevEnum *deviceEnum = NULL;
   IEnumMoniker *videoDeviceEnum = NULL;

   HRESULT err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
   if (FAILED(err) || deviceEnum == NULL) {
      return ;
   }

   err = deviceEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &videoDeviceEnum, 0);
   if (FAILED(err) || videoDeviceEnum == NULL) {
      deviceEnum->Release();
      return ;
   }

   if (err == S_FALSE) //no devices
      return ;

   IMoniker *deviceMoniker = NULL;
   DWORD deviceCount = 0;
   int i = 0;
   while (videoDeviceEnum->Next(1, &deviceMoniker, &deviceCount) == S_OK) {
      IPropertyBag *propertyData = NULL;
      err = deviceMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
      if (SUCCEEDED(err)) {
         VARIANT friendlyNameValue, devicePathValue;
         friendlyNameValue.vt = VT_BSTR;
         friendlyNameValue.bstrVal = NULL;
         devicePathValue.vt = VT_BSTR;
         devicePathValue.bstrVal = NULL;

         err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
         if(FAILED(err))
         {
            continue;
         }
         
         propertyData->Read(L"DevicePath", &devicePathValue, NULL);
         if (SUCCEEDED(err)) {
            std::wstring strDeviceName = (const wchar_t *)friendlyNameValue.bstrVal;
            std::wstring tmpDeviceName = strDeviceName;
            wchar_t * lowerDeviceName = _wcslwr((wchar_t *)tmpDeviceName.c_str());
            if (wcsstr(lowerDeviceName, L"blackmagic ") != NULL || wcsstr(lowerDeviceName, L"decklink ") != NULL) 
            {
               if(iDeckLinkDeviceCnt == 0 && wcsstr(lowerDeviceName, L"blackmagic web presenter") == NULL)
               {
                  continue;
               }
            }
            
            IBaseFilter *filter = NULL;
            IPropertyBag *pBag = 0;
            err = deviceMoniker->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
            if (SUCCEEDED(err)) 
            {
               DeviceInfo info;
               if(devicePathValue.bstrVal==NULL)
               {
                  memset(info.m_sDeviceID,0,sizeof(info.m_sDeviceID));
               }
               else
               {
                  wcscpy(info.m_sDeviceID, devicePathValue.bstrVal);
               }
               wcscpy(info.m_sDeviceName, friendlyNameValue.bstrVal);
               
               wsprintf(info.m_sDeviceDisPlayName,L"%s _DShow Video", info.m_sDeviceName);   
               info.m_sDeviceType=TYPE_DSHOW_VIDEO;

               IPin *pin = NULL;
               if(DShow::GetFilterPin(filter,MEDIATYPE_Audio,PIN_CATEGORY_CAPTURE,PINDIR_OUTPUT,&pin)){         
                  deviceList.push_back(info);
               }
               SafeRelease(pin);
            } 
            SafeRelease(filter);  
         }
      }
      SafeRelease(propertyData);
      SafeRelease(deviceMoniker);
   }
   SafeRelease(videoDeviceEnum);
   SafeRelease(deviceEnum);
}

void CDeviceObtain::GetAudioDevices(DeviceList &deviceList, EDataFlow eType) {
   Lock();   
   unsigned int iDeckLinkDeviceCnt = GetDeckLinkDeviceNum();
   if(eType==1)
   {
      GetCoreAudioDevices(deviceList,eType);
      GetDecklinkInputDevices(deviceList);
      GetDShowInputAudioDevicesByAudioFilter(deviceList,iDeckLinkDeviceCnt);
      GetDShowInputAudioDevicesByVideoFilter(deviceList,iDeckLinkDeviceCnt);
   }
   else
   {
      GetCoreAudioDevices(deviceList,eType);
   }
   Unlock();
}

void CDeviceObtain::GetVedioDevices(DeviceList &deviceList) {
   ICreateDevEnum *deviceEnum;
   IEnumMoniker *videoDeviceEnum;

   HRESULT err;
   err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
   if (FAILED(err) || deviceEnum == NULL) {
      return;
   }

   err = deviceEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &videoDeviceEnum, 0);
   if (FAILED(err) || videoDeviceEnum == NULL) {
      deviceEnum->Release();
      return;
   }

   if (err == S_FALSE) //no devices
      return;

   //------------------------------------------

   IMoniker *deviceInfoMoniker = NULL;
   DWORD count = 0;
   while (videoDeviceEnum->Next(1, &deviceInfoMoniker, &count) == S_OK) {
      IPropertyBag *propertyData = NULL;
      err = deviceInfoMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
      if (SUCCEEDED(err)) {
         VARIANT friendlyNameValue, devicePathValue;
         friendlyNameValue.vt = VT_BSTR;
         friendlyNameValue.bstrVal = NULL;
         devicePathValue.vt = VT_BSTR;
         devicePathValue.bstrVal = NULL;

         err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
         propertyData->Read(L"DevicePath", &devicePathValue, NULL);

         IBaseFilter* filter = NULL;
         if (SUCCEEDED(err)) {
            wchar_t* strDeviceName = (wchar_t*)friendlyNameValue.bstrVal;
            wstring tmpDeviceName = strDeviceName;
            wchar_t * lowerDeviceName = _wcslwr((wchar_t*)tmpDeviceName.c_str());
            IPropertyBag *pBag = 0;
            
            err = deviceInfoMoniker->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
            if (SUCCEEDED(err)) {
               DeviceInfo info;
               //Luminositi ,this virtual camera will crash when AddFilter by Start Up by web
               if(wcsstr(strDeviceName,L"Luminositi")==NULL&&
                  wcsstr(strDeviceName,L"91KBOX")==NULL)
               {
                  wcscpy(info.m_sDeviceName, strDeviceName);
                  wsprintf(info.m_sDeviceDisPlayName,L"%s _DShow",strDeviceName);   
                  if (NULL != devicePathValue.bstrVal) {
                     wcscpy_s(info.m_sDeviceID, MAX_DEVICE_ID_LEN ,devicePathValue.bstrVal);
                  } 
                  
                  info.m_sDeviceType=TYPE_DSHOW_VIDEO; 
                  deviceList.push_back(info);
               }
            }
            SafeRelease(filter);
         }
         SafeRelease(propertyData);
      }
      SafeRelease(deviceInfoMoniker);
   }
   SafeRelease(videoDeviceEnum);
   SafeRelease(deviceEnum);

   //获取DeckLinkDevice
   unsigned int iDeviceCnt = GetDeckLinkDeviceNum();
   for (unsigned int i = 0; i < iDeviceCnt; i++) {
      const wchar_t* deviceName = GetDeckLinkDeviceName(i);
      if (deviceName != NULL) {
         DeviceInfo info;
         wcscpy(info.m_sDeviceName, deviceName);
         wsprintf(info.m_sDeviceDisPlayName,L"%s _Capture Card API",deviceName);   
         info.m_sDeviceType=TYPE_DECKLINK;
         deviceList.push_back(info);
      }
   }
}

bool ResolutionListHasValue(const vector<SIZE> &resolutions, SIZE &size) {
   bool bHasResolution = false;

   for (UINT i = 0; i < resolutions.size(); i++) {
      const SIZE &testSize = resolutions[i];
      if (size.cx == testSize.cx && size.cy == testSize.cy) {
         bHasResolution = true;
         break;
      }
   }
   return bHasResolution;
}

void GetResolutions(vector<MediaOutputInfo>& mediaOutputList, vector<SIZE> &resolutions) {
   resolutions.clear();

   for (UINT i = 0; i < mediaOutputList.size(); i++) {
      MediaOutputInfo &outputInfo = mediaOutputList[i];
      SIZE size;
      size.cx = outputInfo.minCX;
      size.cy = outputInfo.minCY;
      if (!ResolutionListHasValue(resolutions, size))
         resolutions.push_back(size);

      size.cx = outputInfo.maxCX;
      size.cy = outputInfo.maxCY;
      if (!ResolutionListHasValue(resolutions, size))
         resolutions.push_back(size);
   }

   //sort
   for (UINT i = 0; i < resolutions.size(); i++) {
      SIZE &rez = resolutions[i];

      for (UINT j = i + 1; j < resolutions.size(); j++) {
         SIZE &testRez = resolutions[j];

         if (testRez.cy < rez.cy) {
            swap(resolutions[i], resolutions[j]);
            j = i;
         }
      }
   }
}

inline void DeleteMediaType(AM_MEDIA_TYPE *pmt) {
   if (pmt != NULL) {
      //FreeMediaType_(*pmt);
      CoTaskMemFree(pmt);
   }
}

VideoOutputType GetVideoOutputType(const AM_MEDIA_TYPE &media_type) {
   VideoOutputType type = VideoOutputType_None;

   if (media_type.majortype == MEDIATYPE_Video) {
      // Packed RGB formats
      if (media_type.subtype == MEDIASUBTYPE_RGB24)
         type = VideoOutputType_RGB24;
      else if (media_type.subtype == MEDIASUBTYPE_RGB32)
         type = VideoOutputType_RGB32;
      else if (media_type.subtype == MEDIASUBTYPE_ARGB32)
         type = VideoOutputType_ARGB32;

      // Planar YUV formats
      else if (media_type.subtype == MEDIASUBTYPE_I420)
         type = VideoOutputType_I420;
      else if (media_type.subtype == MEDIASUBTYPE_IYUV)
         type = VideoOutputType_I420;
      else if (media_type.subtype == MEDIASUBTYPE_YV12)
         type = VideoOutputType_YV12;

      else if (media_type.subtype == MEDIASUBTYPE_Y41P)
         type = VideoOutputType_Y41P;
      else if (media_type.subtype == MEDIASUBTYPE_YVU9)
         type = VideoOutputType_YVU9;

      // Packed YUV formats
      else if (media_type.subtype == MEDIASUBTYPE_YVYU)
         type = VideoOutputType_YVYU;
      else if (media_type.subtype == MEDIASUBTYPE_YUY2)
         type = VideoOutputType_YUY2;
      else if (media_type.subtype == MEDIASUBTYPE_UYVY)
         type = VideoOutputType_UYVY;

      else if (media_type.subtype == MEDIASUBTYPE_MPEG2_VIDEO)
         type = VideoOutputType_MPEG2_VIDEO;

      else if (media_type.subtype == MEDIASUBTYPE_H264)
         type = VideoOutputType_H264;

      else if (media_type.subtype == MEDIASUBTYPE_dvsl)
         type = VideoOutputType_dvsl;
      else if (media_type.subtype == MEDIASUBTYPE_dvsd)
         type = VideoOutputType_dvsd;
      else if (media_type.subtype == MEDIASUBTYPE_dvhd)
         type = VideoOutputType_dvhd;

      else if (media_type.subtype == MEDIASUBTYPE_MJPG)
         type = VideoOutputType_MJPG;

      else
         //nop();
         ;
   }

   return type;
}

BITMAPINFOHEADER* GetVideoBMIHeader(const AM_MEDIA_TYPE *pMT) {
   return (pMT->formattype == FORMAT_VideoInfo) ?
      &reinterpret_cast<VIDEOINFOHEADER*>(pMT->pbFormat)->bmiHeader :
      &reinterpret_cast<VIDEOINFOHEADER2*>(pMT->pbFormat)->bmiHeader;
}

VideoOutputType GetVideoOutputTypeFromFourCC(DWORD fourCC) {
   VideoOutputType type = VideoOutputType_None;

   // Packed RGB formats
   if (fourCC == '2BGR')
      type = VideoOutputType_RGB32;
   else if (fourCC == '4BGR')
      type = VideoOutputType_RGB24;
   else if (fourCC == 'ABGR')
      type = VideoOutputType_ARGB32;

   // Planar YUV formats
   else if (fourCC == '024I' || fourCC == 'VUYI')
      type = VideoOutputType_I420;
   else if (fourCC == '21VY')
      type = VideoOutputType_YV12;

   // Packed YUV formats
   else if (fourCC == 'UYVY')
      type = VideoOutputType_YVYU;
   else if (fourCC == '2YUY')
      type = VideoOutputType_YUY2;
   else if (fourCC == 'YVYU')
      type = VideoOutputType_UYVY;
   else if (fourCC == 'CYDH')
      type = VideoOutputType_HDYC;

   else if (fourCC == 'V4PM' || fourCC == '2S4M')
      type = VideoOutputType_MPEG2_VIDEO;

   else if (fourCC == '462H')
      type = VideoOutputType_H264;

   else if (fourCC == 'GPJM')
      type = VideoOutputType_MJPG;

   return type;
}

void AddOutput(AM_MEDIA_TYPE *pMT, BYTE *capsData, bool bAllowV2, vector<MediaOutputInfo> &outputInfoList) {
   VideoOutputType type = GetVideoOutputType(*pMT);

   if (pMT->formattype == FORMAT_VideoInfo || (bAllowV2 && pMT->formattype == FORMAT_VideoInfo2)) {
      VIDEO_STREAM_CONFIG_CAPS *pVSCC = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(capsData);
      VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pMT->pbFormat);
      BITMAPINFOHEADER *bmiHeader = GetVideoBMIHeader(pMT);

      bool bUsingFourCC = false;
      if (type == VideoOutputType_None) {
         type = GetVideoOutputTypeFromFourCC(bmiHeader->biCompression);
         bUsingFourCC = true;
      }

      if (type != VideoOutputType_None) {
#if ELGATO_WORKAROUND // FMB NOTE 18-Feb-14: Not necessary anymore since Elgato Game Capture v.2.20 which implements IAMStreamConfig
         if (!pVSCC && bAllowV2) {
            AddElgatoRes(pMT, 480, 360, type, outputInfoList);
            AddElgatoRes(pMT, 640, 480, type, outputInfoList);
            AddElgatoRes(pMT, 1280, 720, type, outputInfoList);
            AddElgatoRes(pMT, 1920, 1080, type, outputInfoList);
            DeleteMediaType(pMT);
            return;
         }
#endif

         MediaOutputInfo outputInfo;

         if (pVSCC) {
            outputInfo.minFrameInterval = pVSCC->MinFrameInterval;
            outputInfo.maxFrameInterval = pVSCC->MaxFrameInterval;

            outputInfo.minCX = pVSCC->InputSize.cx;
            outputInfo.maxCX = pVSCC->InputSize.cx;
            outputInfo.minCY = pVSCC->InputSize.cy;
            outputInfo.maxCY = pVSCC->InputSize.cy;

            if (!outputInfo.minCX || !outputInfo.minCY || !outputInfo.maxCX || !outputInfo.maxCY) {
               outputInfo.minCX = outputInfo.maxCX = bmiHeader->biWidth;
               outputInfo.minCY = outputInfo.maxCY = bmiHeader->biHeight;
            }

            //actually due to the other code in GetResolutionFPSInfo, we can have this granularity
            // back to the way it was.  now, even if it's corrupted, it will always work
            outputInfo.xGranularity = max(pVSCC->OutputGranularityX, 1);
            outputInfo.yGranularity = max(pVSCC->OutputGranularityY, 1);
         } else {
            outputInfo.minCX = outputInfo.maxCX = bmiHeader->biWidth;
            outputInfo.minCY = outputInfo.maxCY = bmiHeader->biHeight;
            if (pVih->AvgTimePerFrame != 0)
               outputInfo.minFrameInterval = outputInfo.maxFrameInterval = pVih->AvgTimePerFrame;
            else
               outputInfo.minFrameInterval = outputInfo.maxFrameInterval = 10000000 / 30; //elgato hack // FMB NOTE 18-Feb-14: Not necessary anymore since Elgato Game Capture v.2.20 which implements IAMStreamConfig

            outputInfo.xGranularity = outputInfo.yGranularity = 1;
         }

         outputInfo.mediaType = pMT;
         outputInfo.videoType = type;
         outputInfo.bUsingFourCC = bUsingFourCC;

         outputInfoList.push_back(outputInfo);

         return;
      }
   }

   DeleteMediaType(pMT);
}

void GetOutputList(IPin *curPin, vector<MediaOutputInfo> &outputInfoList) {
   HRESULT hRes;

   IAMStreamConfig *config;
   if (SUCCEEDED(curPin->QueryInterface(IID_IAMStreamConfig, (void**)&config))) {
      int count, size;
      if (SUCCEEDED(hRes = config->GetNumberOfCapabilities(&count, &size))) {
         BYTE *capsData = (BYTE*)malloc(size);

         int priority = -1;
         for (int i = 0; i < count; i++) {
            AM_MEDIA_TYPE *pMT = nullptr;
            if (SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)) && pMT != NULL)
               AddOutput(pMT, capsData, false, outputInfoList);
         }

         free(capsData);
      } else if (hRes == E_NOTIMPL) //...usually elgato.
      {
         IEnumMediaTypes *mediaTypes;
         if (SUCCEEDED(curPin->EnumMediaTypes(&mediaTypes))) {
            ULONG i;

            AM_MEDIA_TYPE *pMT;
            if (mediaTypes->Next(1, &pMT, &i) == S_OK)
               AddOutput(pMT, NULL, true, outputInfoList);

            mediaTypes->Release();
         }
      }
      SafeRelease(config);
   }
}

bool PinHasMajorType(IPin *pin, const GUID *majorType) {
   HRESULT hRes;

   IAMStreamConfig *config;
   if (SUCCEEDED(pin->QueryInterface(IID_IAMStreamConfig, (void**)&config))) {
      int count, size;
      if (SUCCEEDED(config->GetNumberOfCapabilities(&count, &size))) {
         BYTE *capsData = (BYTE*)malloc(size);

         int priority = -1;
         for (int i = 0; i < count; i++) {
            AM_MEDIA_TYPE *pMT = nullptr;
            if (SUCCEEDED(config->GetStreamCaps(i, &pMT, capsData)) && pMT != NULL) {
               BOOL bDesiredMediaType = (pMT->majortype == *majorType);

               //FreeMediaType_(*pMT);
               CoTaskMemFree(pMT);

               if (bDesiredMediaType) {
                  free(capsData);
                  SafeRelease(config);

                  return true;
               }
            }
         }

         free(capsData);
      }

      SafeRelease(config);
   }

   AM_MEDIA_TYPE *pinMediaType;

   IEnumMediaTypes *mediaTypesEnum;
   if (FAILED(pin->EnumMediaTypes(&mediaTypesEnum)))
      return false;

   ULONG curVal = 0;
   hRes = mediaTypesEnum->Next(1, &pinMediaType, &curVal);

   mediaTypesEnum->Release();

   if (hRes != S_OK)
      return false;

   BOOL bDesiredMediaType = (pinMediaType->majortype == *majorType);
   DeleteMediaType(pinMediaType);

   if (!bDesiredMediaType)
      return false;

   return true;
}

IPin* GetOutputPin(IBaseFilter *filter, const GUID *majorType) {
   IPin *foundPin = NULL;
   IEnumPins *pins;

   if (!filter) return NULL;
   if (FAILED(filter->EnumPins(&pins))) return NULL;

   IPin *curPin;
   ULONG num;
   while (pins->Next(1, &curPin, &num) == S_OK) {
      if (majorType) {
         if (!PinHasMajorType(curPin, majorType)) {
            SafeRelease(curPin);
            continue;
         }
      }
      //------------------------------

      PIN_DIRECTION pinDir;
      if (SUCCEEDED(curPin->QueryDirection(&pinDir))) {
         if (pinDir == PINDIR_OUTPUT) {
            IKsPropertySet *propertySet;
            if (SUCCEEDED(curPin->QueryInterface(IID_IKsPropertySet, (void**)&propertySet))) {
               GUID pinCategory;
               DWORD retSize;

               PIN_INFO chi;
               curPin->QueryPinInfo(&chi);

               if (chi.pFilter)
                  chi.pFilter->Release();

               if (SUCCEEDED(propertySet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, &pinCategory, sizeof(GUID), &retSize))) {
                  if (pinCategory == PIN_CATEGORY_CAPTURE || pinCategory == PIN_CATEGORY_ROXIOCAPTURE) {
                     SafeRelease(propertySet);
                     SafeRelease(pins);

                     return curPin;
                  }
               }
               SafeRelease(propertySet);
            }
         }
      }

      SafeRelease(curPin);
   }

   SafeRelease(pins);

   return foundPin;
}

static inline UINT64 GetFrameIntervalDist(UINT64 minInterval, UINT64 maxInterval, UINT64 desiredInterval) {
   INT64 minDist = INT64(minInterval) - INT64(desiredInterval);
   INT64 maxDist = INT64(desiredInterval) - INT64(maxInterval);

   if (minDist < 0) minDist = 0;
   if (maxDist < 0) maxDist = 0;

   return UINT64(MAX(minDist, maxDist));
}

bool CDeviceObtain::GetClosestResolutionFPS(wstring& deviceName, wstring& deviceID, SIZE &resolution, UINT64 &frameInterval, bool bPrioritizeAspect, bool bPrioritizeFPS) {
   LONG width, height;
   UINT64 internalFrameInterval = 10000000 / 25;
   //API->GetBaseSize((UINT&)width, (UINT&)height);
   // 暂时使用
   width = 1920;
   height = 1080;

   LONG bestDistance = 0x7FFFFFFF;
   SIZE bestSize;
   UINT64 maxFrameInterval = 0;
   UINT64 minFrameInterval = 0;
   UINT64 bestFrameIntervalDist = 0xFFFFFFFFFFFFFFFFLL;
   float aspectBaseSize = (float)width / height;
   float  bestAspectDistance = 10000.0f;
   vector<MediaOutputInfo> mediaOutputList;
   IBaseFilter *filter = GetDeviceByValue(CLSID_VideoInputDeviceCategory,
                                          L"FriendlyName", deviceName.c_str(),
                                          L"DevicePath", deviceID.c_str());

   if (filter) {
      //--------------------------------
      // get video info
      frameInterval = 400000;

      IPin *outputPin = GetOutputPin(filter, &MEDIATYPE_Video);
      if (outputPin) {
         for (UINT i = 0; i < mediaOutputList.size(); i++)
            mediaOutputList[i].FreeData();
         mediaOutputList.clear();
         GetOutputList(outputPin, mediaOutputList);
      }
   } else {
      return false;
   }
   for (UINT i = 0; i < mediaOutputList.size(); i++) {
      MediaOutputInfo &outputInfo = mediaOutputList[i];

      LONG outputWidth = outputInfo.minCX;
      do {
         LONG distWidth = width - outputWidth;
         if (distWidth < 0)
            break;

         if (distWidth > bestDistance) {
            outputWidth += outputInfo.xGranularity;
            continue;
         }

         LONG outputHeight = outputInfo.minCY;
         do {
            LONG distHeight = height - outputHeight;
            if (distHeight < 0)
               break;

            LONG totalDist = distHeight + distWidth;

            UINT64 frameIntervalDist = GetFrameIntervalDist(outputInfo.minFrameInterval,
                                                            outputInfo.maxFrameInterval, internalFrameInterval);
            float aspectSize = (float)outputWidth / outputHeight;
            float aspecDistance = aspectBaseSize - aspectSize;
            if (aspecDistance < 0)
               aspecDistance = 0 - aspecDistance;
            bool bBetter;
            bool isAspectSupport = true;
            if (bPrioritizeAspect) {
               isAspectSupport = aspecDistance <= bestAspectDistance;
            }
            if (bPrioritizeFPS)
               bBetter = (frameIntervalDist != bestFrameIntervalDist) ?
               (frameIntervalDist < bestFrameIntervalDist) :
               (totalDist < bestDistance);
            else
               bBetter = (totalDist != bestDistance) ?
               (totalDist < bestDistance) :
               (frameIntervalDist < bestFrameIntervalDist);

            if (bBetter && isAspectSupport) {
               bestAspectDistance = aspecDistance;
               bestDistance = totalDist;
               bestSize.cx = outputWidth;
               bestSize.cy = outputHeight;
               maxFrameInterval = outputInfo.maxFrameInterval;
               minFrameInterval = outputInfo.minFrameInterval;
               bestFrameIntervalDist = frameIntervalDist;
            }

            outputHeight += outputInfo.yGranularity;
         } while ((UINT)outputHeight <= outputInfo.maxCY);

         outputWidth += outputInfo.xGranularity;
      } while ((UINT)outputWidth <= outputInfo.maxCX);
   }

   if (bestDistance != 0x7FFFFFFF) {
      resolution.cx = bestSize.cx;
      resolution.cy = bestSize.cy;

      frameInterval = minFrameInterval;
      if (frameInterval > 1000000) {
         frameInterval = 1000000;
      } else if (frameInterval < 100000) {
         frameInterval = 100000;
      }
      return true;
   }

   return false;
}

bool CDeviceObtain::GetResolution(vector<FrameInfo>& mediaList, vector<SIZE>& resolutions, UINT64& minFrameInterval, UINT64& maxFrameInterval, const wstring& deviceName, const wstring& deviceID) {
   IBaseFilter *filter = GetDeviceByValue(CLSID_VideoInputDeviceCategory,
                                          L"FriendlyName", deviceName.c_str(),
                                          L"DevicePath", deviceID.c_str());

   vector<MediaOutputInfo> mediaOutputList;
   if (filter) {
      //--------------------------------
      // get video info 400000
      maxFrameInterval = 2000000;
      minFrameInterval = 200000;
      IPin *outputPin = GetOutputPin(filter, &MEDIATYPE_Video);
      if (outputPin) {
         for (UINT i = 0; i < mediaOutputList.size(); i++)
            mediaOutputList[i].FreeData();
         mediaOutputList.clear();
         GetOutputList(outputPin, mediaOutputList);
         GetResolutions(mediaOutputList, resolutions);
         for (UINT i = 0; i < mediaOutputList.size(); i++) {
            MediaOutputInfo &outputInfo = mediaOutputList[i];
            if (minFrameInterval< outputInfo.minFrameInterval) {
               minFrameInterval = outputInfo.minFrameInterval;
            }
            if (maxFrameInterval> outputInfo.maxFrameInterval)
               maxFrameInterval = outputInfo.maxFrameInterval;

            if (minFrameInterval> maxFrameInterval)
               maxFrameInterval = minFrameInterval;

            FrameInfo frameInfo;
            frameInfo.minFrameInterval = mediaOutputList[i].minFrameInterval;
            frameInfo.maxFrameInterval = mediaOutputList[i].maxFrameInterval;
            frameInfo.minCX = mediaOutputList[i].minCX;
            frameInfo.minCY = mediaOutputList[i].minCY;
            frameInfo.maxCX = mediaOutputList[i].maxCX;
            frameInfo.maxCY = mediaOutputList[i].maxCY;
            frameInfo.xGranularity = mediaOutputList[i].xGranularity;
            frameInfo.yGranularity = mediaOutputList[i].yGranularity;
            frameInfo.bUsingFourCC = mediaOutputList[i].bUsingFourCC;
            mediaList.push_back(frameInfo);
         }
         outputPin->Release();
         outputPin = NULL;
      }
      filter->Release();
      return true;
   }
   return false;
}

void CDeviceObtain::OpenPropertyPagesByName(HWND hwnd, DeviceInfo deviceInfo) {
   GUID matchGUID;
   if(deviceInfo.m_sDeviceType==TYPE_DSHOW_AUDIO)
   {
      matchGUID = CLSID_AudioInputDeviceCategory;
   }
   else if(deviceInfo.m_sDeviceType==TYPE_DSHOW_VIDEO)
   {
      matchGUID = CLSID_VideoInputDeviceCategory;
   }
   else 
   {
      return ;
   }
  
   IBaseFilter *filter = GetDeviceByValue(matchGUID,
                                          L"FriendlyName", deviceInfo.m_sDeviceName,
                                          L"DevicePath", deviceInfo.m_sDeviceID);

   if (!filter) {
      //ASSERT(FALSE);
      return;
   }

   ISpecifyPropertyPages *propPages = NULL;
   CAUUID cauuid;

   if (SUCCEEDED(filter->QueryInterface(IID_ISpecifyPropertyPages, (void**)&propPages))) {
      if (SUCCEEDED(propPages->GetPages(&cauuid))) {
         if (cauuid.cElems) {
            OleCreatePropertyFrame(hwnd, 0, 0, NULL, 1, (LPUNKNOWN*)&filter, cauuid.cElems, cauuid.pElems, 0, 0, NULL);
            CoTaskMemFree(cauuid.pElems);
         }
      }
      propPages->Release();
   }
}

IBaseFilter* CDeviceObtain::GetDeviceByValue(const IID &enumType, const WCHAR* lpType, const WCHAR* lpName, const WCHAR* lpType2, const WCHAR* lpName2) {
   //---------------------------------
   // exception devices
   //    if (scmpi(lpType2, L"DevicePath") == 0 && lpName2 && *lpName2 == L'{')
   //       return GetExceptionDevice(lpName2);

   //---------------------------------

   ICreateDevEnum *deviceEnum;
   IEnumMoniker *videoDeviceEnum;

   HRESULT err;
   err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
   if (FAILED(err) || deviceEnum == NULL) {
      return NULL;
   }

   err = deviceEnum->CreateClassEnumerator(enumType, &videoDeviceEnum, 0);
   if (FAILED(err) || videoDeviceEnum == NULL) {
      deviceEnum->Release();
      return NULL;
   }

   SafeRelease(deviceEnum);

   if (err == S_FALSE) //no devices, so NO ENUM FO U
      return NULL;

   //---------------------------------

   IBaseFilter *bestFilter = NULL;

   IMoniker *deviceInfo;
   DWORD count;
   while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK) {
      IPropertyBag *propertyData;
      err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
      if (SUCCEEDED(err)) {
         VARIANT valueThingy;
         VARIANT valueThingy2;
         VariantInit(&valueThingy);
         VariantInit(&valueThingy2);
         /*valueThingy.vt  = VT_BSTR;
         valueThingy.pbstrVal = NULL;

         valueThingy2.vt = VT_BSTR;
         valueThingy2.bstrVal = NULL;*/

         if (SUCCEEDED(propertyData->Read(lpType, &valueThingy, NULL))) {
            if (lpType2 && lpName2) {
               if (FAILED(propertyData->Read(lpType2, &valueThingy2, NULL))) {
                  // nop();
               }
            }

            SafeRelease(propertyData);

            wstring strVal1 = (const WCHAR*)valueThingy.bstrVal;

            if (0 == wcscmp(strVal1.c_str(), lpName) || 0 == wcscmp(lpName, L"Default")) {
               IBaseFilter *filter=NULL;
               err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
               if (FAILED(err)) {
                  assert(FALSE);
                  continue;
               }

               if (0 == wcscmp(lpName, L"Default")) {
                  if (!bestFilter) {
                     bestFilter = filter;
                     SafeRelease(deviceInfo);
                     SafeRelease(videoDeviceEnum);
                     return bestFilter;
                  }
               }

               if (!bestFilter) {
                  bestFilter = filter;

                  if (!lpType2 || !lpName2) {
                     SafeRelease(deviceInfo);
                     SafeRelease(videoDeviceEnum);

                     return bestFilter;
                  }
               } else if (lpType2 && lpName2) {
                  wstring strVal2 = (const WCHAR*)valueThingy2.bstrVal;
                  if (0 == wcscmp(strVal2.c_str(), lpName2)) {
                     bestFilter->Release();

                     bestFilter = filter;

                     SafeRelease(deviceInfo);
                     SafeRelease(videoDeviceEnum);

                     return bestFilter;
                  }
               } else
                  filter->Release();
            }
         }
      }

      SafeRelease(deviceInfo);
   }

   SafeRelease(videoDeviceEnum);

   return bestFilter;
}

