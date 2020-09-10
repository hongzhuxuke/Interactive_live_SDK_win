#pragma once
#include <iostream>
#include <string>
#include "common/vhall_utility.h"
namespace vhall {
  class PlatForm {
  public:
    //-------------------------------------------------------------------------
    // 函数    : GetNtVersionNumbers
    // 功能    : 调用RtlGetNtVersionNumbers获取系统版本信息
    // 返回值  : BOOL
    // 参数    : DWORD& dwMajorVer 主版本
    // 参数    : DWORD& dwMinorVer 次版本
    // 参数    : DWORD& dwBuildNumber build号
    // 附注    :
    //-------------------------------------------------------------------------
    static bool GetNtVersionNumbers(DWORD&dwMajorVer, DWORD& dwMinorVer, DWORD& dwBuildNumber);
    static std::string GetVersion();
  };
}