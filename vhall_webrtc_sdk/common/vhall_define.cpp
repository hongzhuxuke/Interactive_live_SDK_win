//
//  vhall_define.cpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/21.
//  Copyright © 2017年 ilong. All rights reserved.
//

//#include "vhall_define.h"
//#include <chrono>
//#ifdef VHALL_WIN
//#include <atlstr.h>
//#else
//#include <sys/time.h>
//#endif // VHALL_WIN
//#include <sstream>
//
//VHALL_LogLevel vhall_log_level = VHALL_LOGALL;
//
//CString vformat(const char *fmt, ...)
//{
//  va_list ap;
//  va_start(ap, fmt);
//  int len = vsnprintf(nullptr, 0, fmt, ap);
//  va_end(ap);
//  std::string buf(len + 1, '\0');
//  va_start(ap, fmt);
//  vsnprintf(&buf[0], buf.size(), fmt, ap);
//  va_end(ap);
//  buf.pop_back();
//  CString test = buf.c_str();
//  return test.AllocSysString();
//}
//
//std::string GetFormatDate(){
//   const int buff_len = 255;
//   char tmpBuf[buff_len];
//#ifdef WIN32
//   SYSTEMTIME sys;
//   GetLocalTime(&sys);
//   snprintf(tmpBuf, buff_len, "%d-%02d-%02d %02d:%02d:%02d.%03d",
//            sys.wYear, sys.wMonth, sys.wDay,
//            sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
//#else
//   struct timeval  tv;
//   struct tm       *p;
//   gettimeofday(&tv, NULL);
//   p = localtime(&tv.tv_sec);
//   snprintf(tmpBuf, buff_len, "%d-%02d-%02d %02d:%02d:%02d.%03d",
//            1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
//            p->tm_hour, p->tm_min, p->tm_sec, (int)(tv.tv_usec/1000));
//#endif
//   return std::string(tmpBuf);
//}
//
//uint64_t GetTimestampMs() {
//   auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
//   return ms.count();
//}
