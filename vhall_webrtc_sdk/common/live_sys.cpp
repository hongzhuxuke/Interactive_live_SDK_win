#include "live_sys.h"
#include <WinSock2.h>
#include <ws2tcpip.h>

int vhall_lock_init(vhall_lock_t*lock) {
  int ret = 0;
#if defined _WIN32 || defined WIN32
  InitializeCriticalSection(lock);
#else
  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE_NP);
  ret = pthread_mutex_init(lock, &mattr);
  pthread_mutexattr_destroy(&mattr);
#endif
  return ret;
}

int vhall_lock_destroy(vhall_lock_t*lock) {
  int ret = 0;
#if defined _WIN32 || defined WIN32
  DeleteCriticalSection(lock);
#else
  ret = pthread_mutex_destroy(lock);
#endif
  return ret;
}

int vhall_lock(vhall_lock_t*lock) {
  int ret = 0;
#if defined _WIN32 || defined WIN32
  EnterCriticalSection(lock);
#else
  ret = pthread_mutex_lock(lock);
#endif
  return ret;
}

int vhall_unlock(vhall_lock_t*lock) {
  int ret = 0;
#if defined _WIN32 || defined WIN32
  LeaveCriticalSection(lock);
#else
  ret = pthread_mutex_unlock(lock);
#endif
  return ret;
}

int vhall_cond_init(vhall_cond_t*cond)
{
  int ret = 0;
#if defined _WIN32 || defined WIN32
  //*cond = CreateEvent(NULL, FALSE, FALSE, NULL);
  InitializeConditionVariable(cond);
#else
  ret = pthread_cond_init(cond, NULL);
#endif
  return ret;
}

int vhall_cond_destroy(vhall_cond_t*cond)
{
  int ret = 0;
#if defined _WIN32 || defined WIN32
  //CloseHandle(*cond);
  //*cond = NULL; 
  //now ConditionVariable do not need to destroy
#else
  ret = pthread_cond_destroy(cond);
#endif
  return ret;
}

int vhall_cond_wait(vhall_cond_t*cond, vhall_lock_t*lock) {
  int ret = 0;
#if defined _WIN32 || defined WIN32
  /*if (lock) {
  LeaveCriticalSection(lock);
  }

  WaitForSingleObject(*cond, INFINITE);

  if (lock) {
  EnterCriticalSection(lock);
  }*/
  ret = SleepConditionVariableCS(cond, lock, INFINITE);
#else
  ret = pthread_cond_wait(cond, lock);
#endif
  return ret;
}

int vhall_cond_wait_time(vhall_cond_t*cond, vhall_lock_t*lock, int time_ms)
{
  int ret = 0;
#if defined _WIN32 || defined WIN32
  //if (lock) {
  //	LeaveCriticalSection(lock);
  //}
  //if (time_ms < 0){
  //	ret = WaitForSingleObject(*cond, INFINITE);
  //}
  //else{
  //	ret = WaitForSingleObject(*cond, time_ms);
  //}

  //if (lock) {
  //	EnterCriticalSection(lock);
  //}
  time_ms = time_ms < 0 ? INFINITE : time_ms;
  ret = SleepConditionVariableCS(cond, lock, time_ms);
#else
  if (time_ms < 0) {
    ret = pthread_cond_wait(cond, lock);
  } else {
    struct timespec ts;
#if defined(OSX)||defined(IOS) // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
    ts.tv_nsec = mts.tv_nsec;
#else
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
      return -1;
    }
#endif
    ts.tv_sec += time_ms / 1000;
    ts.tv_nsec += (time_ms % 1000) * 1000 * 1000;
    ret = pthread_cond_timedwait(cond, lock, &ts);
  }
#endif

  return ret;
}

int vhall_cond_signal(vhall_cond_t*cond)
{
  int ret = 0;
#if defined _WIN32 || defined WIN32
  //PulseEvent(*cond);
  WakeConditionVariable(cond);
#else
  ret = pthread_cond_signal(cond);
#endif
  return ret;
}