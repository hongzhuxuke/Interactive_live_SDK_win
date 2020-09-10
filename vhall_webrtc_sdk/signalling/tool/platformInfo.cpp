#include "platformInfo.h"

#include <windows.h>
#include <cstdio>
#include <VersionHelpers.h>
namespace vhall {
  bool PlatForm::GetNtVersionNumbers(DWORD&dwMajorVer, DWORD& dwMinorVer, DWORD& dwBuildNumber)
  {
    BOOL bRet = FALSE;
    HMODULE hModNtdll = NULL;
    if (hModNtdll = ::LoadLibraryW(L"ntdll.dll"))
    {
      typedef void (WINAPI *pfRTLGETNTVERSIONNUMBERS)(DWORD*, DWORD*, DWORD*);
      pfRTLGETNTVERSIONNUMBERS pfRtlGetNtVersionNumbers;
      pfRtlGetNtVersionNumbers = (pfRTLGETNTVERSIONNUMBERS)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
      if (pfRtlGetNtVersionNumbers)
      {
        pfRtlGetNtVersionNumbers(&dwMajorVer, &dwMinorVer, &dwBuildNumber);
        dwBuildNumber &= 0x0ffff;
        bRet = TRUE;
      }
      ::FreeLibrary(hModNtdll);
      hModNtdll = NULL;
    }
    return bRet;
  }

  std::string PlatForm::GetVersion() {
    std::string version = "";
    do {
      if (IsWindowsXPOrGreater())
      {
        version = "XP";
      }
      if (IsWindowsXPSP1OrGreater())
      {
        version = "XPSP1";
      }
      if (IsWindowsXPSP2OrGreater())
      {
        version = "XPSP2";
      }
      if (IsWindowsXPSP3OrGreater())
      {
        version = "XPSP3";
      }
      if (IsWindowsVistaOrGreater())
      {
        version = "Vista";
      }
      if (IsWindowsVistaSP1OrGreater())
      {
        version = "VistaSP1";
      }
      if (IsWindowsVistaSP2OrGreater())
      {
        version = "VistaSP2";
      }
      if (IsWindows7OrGreater())
      {
        version = "Windows7";
      }
      if (IsWindows7SP1OrGreater())
      {
        version = "Windows7SP";
      }
      if (IsWindows8OrGreater())
      {
        version = "Windows8";
      }
      if (IsWindows8Point1OrGreater())
      {
        version = "Windows8Point1";
      }
      if (IsWindows10OrGreater())
      {
        version = "Windows10";
      }
    } while (0);
    if (IsWindowsServer())
    {
      version += "_Server";
    }
    else
    {
      version += "_Client";
    }

    DWORD majorVer = 0;
    DWORD minorVer = 0;
    DWORD buildNum = 0;

    bool ret = GetNtVersionNumbers(majorVer, minorVer, buildNum);
    if (ret) {
      version += (std::string("_versionNum:") + Utility::ToString<DWORD>(majorVer) + "_" + Utility::ToString<DWORD>(minorVer) + "_" + Utility::ToString<DWORD>(buildNum));
    }

    return version;
  }
}
