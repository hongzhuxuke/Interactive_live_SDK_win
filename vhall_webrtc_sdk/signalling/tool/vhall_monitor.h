#pragma once
#include "../vh_events.h"
#include <windows.h>
#include <cstdio>
#include <tchar.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <pdh.h>
#include <pdhmsg.h>

namespace vhall {

#define MemPercent       "PercentOfMemory"
#define TotalPhyMem      "TotalKBOfPhysicalMemory"
#define FreePhyMem       "FreeKBOfPhysicalMemory"
#define TotalPagingFile  "TotalKBOfGagingFile"
#define FreePagingFile   "FreeKBOfPagingFile"
#define TotalVirMem      "TotalKBOfVirtualMemory"
#define FreeVirMem       "FreeKBOfVirtualMemory"
#define ExtMem           "FreeKBOfExtendedMemory"
#define CpuLoadPercent   "PercentOfCpuLoad"

  class __declspec(dllexport)VhallMonitor : public EventDispatcher {
  public:
    VhallMonitor();
    virtual ~VhallMonitor();
    bool Start(long long interval);
    void Process();
    void Destroy();
    bool SetMemShreshold(int threshold);
    bool SetCpuLoadShreshold(double threshold);
    double CurrentProcessMemoryInfo(); // MB
    bool SetInterval(long long interval);
    static bool GetMemoryState(MEMORYSTATUSEX& state);
  private:
    bool InitCpuQuery();
    void DeInitCpuQuery();
    double QueryCpuLoad();
  private:
    HCOUNTER                mCounter;
    HQUERY                  mQuery;

    std::atomic<long long>  mInterval;
    std::thread             mThread;
    std::condition_variable mCv;
    std::atomic<bool>       mRunFlag;
    MEMORYSTATUSEX          mStatex;

    std::atomic<int>        mMemPerShreshold;
    std::atomic<double>     mCpuLoadPerShreshold;
  };
}