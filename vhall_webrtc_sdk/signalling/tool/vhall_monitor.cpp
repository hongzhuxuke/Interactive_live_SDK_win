#include "vhall_monitor.h"
#include "common/vhall_log.h"
#include <Psapi.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "Psapi.lib")  

namespace vhall {
  VhallMonitor::VhallMonitor() {
    mCounter = nullptr;
    mQuery = nullptr;
    mInterval = 0;
    mRunFlag = false;
    memset(&mStatex, 0, sizeof(MEMORYSTATUSEX));
    mMemPerShreshold = 95;
    mCpuLoadPerShreshold = 95;
  }

  VhallMonitor::~VhallMonitor() {
    Destroy();
  }

  bool VhallMonitor::Start(long long interval) {
    if (mRunFlag) {
      LOGW("VhallMonitor is already running");
      return true;
    }
    if (interval <= 0) {
      LOGE("task excute interval error");
      return false;
    }
    mInterval = interval;
   
    bool initialized = InitCpuQuery();
    if (!initialized) {
      return false;
    }
    /* setup thread */
    mRunFlag = true;
    mThread = std::thread(std::bind(&VhallMonitor::Process, this));
    return true;
  }

  void VhallMonitor::Process() {
    while (mRunFlag) {
      /* obtain memory info */
      memset(&mStatex, 0, sizeof(MEMORYSTATUSEX));
      mStatex.dwLength = sizeof(MEMORYSTATUSEX);
      GlobalMemoryStatusEx(&mStatex);
      if (mStatex.dwMemoryLoad >= mMemPerShreshold) {
        MonitorEvent event;
        event.mType = MemPercent;
        event.mPercentOfMemoryInUse = mStatex.dwMemoryLoad;
        event.mTotalPhysMem = mStatex.ullTotalPhys / 1024 / 1024;
        event.mAvailPhysMem = mStatex.ullAvailPhys / 1024 / 1024;
        event.mTotalVirtualMem = mStatex.ullTotalVirtual / 1024 / 1024;
        event.mAvailVirtualMem = mStatex.ullAvailVirtual / 1024 / 1024;
        event.mCurProcessMem = CurrentProcessMemoryInfo();
        DispatchEvent(event);
      }
      double cpuLoad = QueryCpuLoad();
      if (cpuLoad >= mCpuLoadPerShreshold) {
        MonitorEvent event;
        event.mType = CpuLoadPercent;
        event.mPercentOfCpuLoad = cpuLoad;
        DispatchEvent(event);
      }

      /* wait for next loop */
      std::mutex mtx;
      std::unique_lock<std::mutex> lock(mtx);
      while (mCv.wait_for(lock, std::chrono::milliseconds(mInterval)) == std::cv_status::timeout) {
        break;
      };
    }
  }

  void VhallMonitor::Destroy() {
    mRunFlag = false;
    mInterval = 0;
    mCv.notify_all();
    if (mThread.joinable()) {
      mThread.join();
    }
    RemoveAllEventListener();
    DeInitCpuQuery();
  }

  bool VhallMonitor::SetMemShreshold(int threshold) {
    if (threshold < 0 || threshold > 100) {
      LOGE("error thread value");
      return false;
    }
    mMemPerShreshold = threshold;
    return true;
  }

  bool VhallMonitor::SetCpuLoadShreshold(double threshold) {
    if (threshold < 0 || threshold > 100) {
      LOGE("error thread value");
      return false;
    }
    mCpuLoadPerShreshold = threshold;
    return true;
  }

  bool VhallMonitor::SetInterval(long long interval)
  {
    if (interval <= 0) {
      return false;
    }
    mInterval = interval;
    return true;
  }

  /* 显示当前程序的内存使用情况 */
  double VhallMonitor::CurrentProcessMemoryInfo() {
    HANDLE handle = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS_EX pmc = { 0 };
    //int a = sizeof(pmc);
    if (!GetProcessMemoryInfo(handle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
      DWORD errCode = GetLastError();
      LOGE("GetProcessMemoryInfo fail, lastErrorCode:%d", errCode);
      return 0;
    }

    ///* 占用的物理内存峰值 */
    //LOGI("PeakWorkingSetSize:%d(KB)", pmc.PeakWorkingSetSize / 1024);
    ///* 当前占用的物理内存 */
    //LOGI("WorkingSetSize:%d(KB)", pmc.WorkingSetSize / 1024);
    ///* 占用的虚拟内存峰值（物理内存+页文件），对应任务管理器中的commit列值 */
    //LOGI("PeakPagefileUsage:%d(KB)", pmc.PeakPagefileUsage / 1024);
    ///* 当前占用的虚拟内存（物理内存+页文件），对应任务管理器中的commit列值 */
    //LOGI("PagefileUsage:%d(KB)", pmc.PagefileUsage / 1024);
    ///* 等同于当前占用的虚拟内存（物理内存+页文件），对应任务管理器中的commit列值，并不是任务管理器中的私有独占内存的意思 */
    //LOGI("PrivateUsage:%d(KB)", pmc.PrivateUsage / 1024);
    return pmc.WorkingSetSize / 1024 / 1024;
  }

  bool VhallMonitor::GetMemoryState(MEMORYSTATUSEX& state) {
    state.dwLength = sizeof(MEMORYSTATUSEX);
    return GlobalMemoryStatusEx(&state);
  }

  bool VhallMonitor::InitCpuQuery() {
    PDH_STATUS status = ERROR_SUCCESS;
    
    status = PdhOpenQuery(nullptr, 0, &mQuery);
    if (ERROR_SUCCESS != status)
    {
      LOGE("PdhOpenQuery failed with 0x%x\n", status);
      return false;
    }
    status = PdhAddCounter(mQuery, L"\\Processor(0)\\% Processor Time", 0, &mCounter);
    if (ERROR_SUCCESS != status)
    {
      LOGE("PdhAddCounter failed with 0x%x\n", status);
      return false;
    }
    // Read a performance data record.
    status = PdhCollectQueryData(mQuery);
    if (ERROR_SUCCESS != status)
    {
      LOGE("PdhCollectQueryData failed with 0x%x\n", status);
      return false;
    }
    return true;
  }

  void VhallMonitor::DeInitCpuQuery() {
    if (mQuery) {
      PdhCloseQuery(mQuery);
      mQuery = nullptr;
    }
  }

  double VhallMonitor::QueryCpuLoad() {
    PDH_FMT_COUNTERVALUE ItemBuffer;
    memset(&ItemBuffer, 0, sizeof(ItemBuffer));

    PDH_STATUS status = PdhCollectQueryData(mQuery);
    if (ERROR_SUCCESS == status)
    {
      // Format the performance data record.
      status = PdhGetFormattedCounterValue(mCounter, PDH_FMT_DOUBLE, (LPDWORD)NULL, &ItemBuffer);
      if (ERROR_SUCCESS != status)
      {
        LOGE("PdhGetFormattedCounterValue failed with 0x%x.\n", status);
      }
      //LOGI("Formatted counter value = %.20g\n", ItemBuffer.doubleValue);
    }
    else
    {
      if (PDH_NO_MORE_DATA != status)
      {
        LOGE("PdhCollectQueryData failed with 0x%x\n", status);
      }
    }
    return ItemBuffer.doubleValue;
  }


}