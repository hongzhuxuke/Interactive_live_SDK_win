#include "vhall_log.h"
#include "string.h"
#include "live_sys.h"
#include "stdlib.h"
#include "stdarg.h"
#include <strstream>
#include <shlobj.h>
#include <shlwapi.h>

#include "rtc_base/log_sinks.h"

int GenerateLogId() {
  static int i = 0;
  return i++;
}

LogWriter::LogWriter(int  level) :
  mLevel(level),
  mId(GenerateLogId())
{

}

int ConsoleLogWriter::Init(void * InitParam) {
  if (!InitParam) {
    fprintf(stderr, "InitParam error\n");
    return -1;
  }
  ConsoleInitParam * param = (ConsoleInitParam*)InitParam;
  mType = param->nType;
  if (mType == 0) {
    mfp = stdout;
  } else if (mType == 1) {
    mfp = stderr;
  } else {
    fprintf(stderr, "InitParam error nType=0 stdout, nType=1 stderr now nType=%d", mType);
    return -1;
  }
  return 0;
}

int ConsoleLogWriter::WriteLog(int level, const char *header, const char* fmt, va_list ap) {
  if (level < mLevel) {
    return 0;
  }

  if (mfp == NULL) {
    return -1;
  }
  fprintf(mfp, "%s:", header);
  vfprintf(mfp, fmt, ap);
  fprintf(mfp, "\n");
  fflush(mfp);
  return 0;
}

void ConsoleLogWriter::Destroy() {

}

ConsoleLogWriter::ConsoleLogWriter(int level)
  :LogWriter(level) {
  mType = 0;
  mfp = NULL;
}

int FileLogWriter::Init(void * InitParam) {
  FileInitParam * param = (FileInitParam*)InitParam;
  if (!param || param->pFilePathName == NULL) {
    fprintf(stderr, "InitParam error\n");
    return -1;
  }

  memset(mFilePathName, 0, sizeof(mFilePathName));
  memcpy(mFilePathName, param->pFilePathName, strlen(param->pFilePathName));

  mPartionSize = param->nPartionSize;
  mPartionTime = param->nPartionTime;
  if (mPartionTime > 0 && mPartionSize > 0) {
    fprintf(stderr, "InitParam error nPartionSize canot both > 0");
    return -1;
  }

  if (mfp) {
    Destroy();
  }

  memset(mPartionFileName, 0, sizeof(mPartionFileName));
  if (mPartionSize > 0) {	//size partion
    sprintf(mPartionFileName, "%s_%d", mFilePathName, 0);
  } else if (mPartionTime> 0) { //time partion
    sprintf(mPartionFileName, "%s_%s", mFilePathName, GetFormatDate2().c_str());
  } // no partion
  else {
    sprintf(mPartionFileName, "%s_%s", mFilePathName, GetFormatDate2().c_str());
  }
  mfp = fopen(mPartionFileName, "w+");

  if (!mfp) {
    fprintf(stderr, "could not open log file %s", mPartionFileName);
    return -1;
  }

  mCPCount = 0;
  localtime(&mCPStartTime);
  mCPPostfix = 0;

  return 0;
}
int FileLogWriter::WriteLog(int level, const char *header, const char* fmt, va_list ap) {

  if (level < mLevel) {
    return 0;
  }

  if (0 != CheckPartion()) {
    return -1;
  }

  if (mfp == NULL) {
    return -1;
  }

  fprintf(mfp, "%s:", header);
  vfprintf(mfp, fmt, ap);
  fprintf(mfp, "\n");
  fflush(mfp);
  return 0;
}
void FileLogWriter::Destroy() {
  fclose(mfp);
}
FileLogWriter::FileLogWriter(int level) :
  LogWriter(level) {

  mPartionSize = 0;
  mPartionTime = 0;//sec
  mfp = NULL;
  mCPCount = 0; //curent partion Byte count
  mCPPostfix = 0; //curent partion postfix
  mCPStartTime = 0; //curent partion start time;
}

int FileLogWriter::CheckPartion() {
  if (mPartionSize > 0) {
    if (mCPCount > mPartionSize) { //create new file
      fclose(mfp);
      memset(mPartionFileName, 0, sizeof(mPartionFileName));
      sprintf(mPartionFileName, "%s_%d", mFilePathName, ++mCPPostfix);

      mfp = fopen(mPartionFileName, "w+");
      if (!mfp) {
        fprintf(stderr, "could not open log file %s", mPartionFileName);
        return -1;
      }
    }
  } else if (mPartionTime > 0) {
    time_t curent_time = 0;
    localtime(&curent_time);
    if (curent_time - mCPStartTime > mPartionTime) {
      fclose(mfp);
      memset(mPartionFileName, 0, sizeof(mPartionFileName));
      sprintf(mPartionFileName, "%s_%s", mFilePathName, GetFormatDate2().c_str());

      mfp = fopen(mPartionFileName, "w+");
      if (!mfp) {
        fprintf(stderr, "could not open log file %s", mPartionFileName);
        return -1;
      }
    }
  }
  return 0;
}

VhallLog::VhallLog() {
  mWriterList.clear();
  vhall_lock_init(&mLock);
}

VhallLog:: ~VhallLog() {
  RmoveAllLog();
  vhall_lock_destroy(&mLock);
}

void VhallLog::Info(std::string header, const char* fmt, ...) {

  vhall_lock(&mLock);

  va_list ap;
  va_start(ap, fmt);
  for (int i = 0; i < mWriterList.size(); i++) {
    mWriterList[i]->WriteLog(VHALL_LOG_LEVEL_INFO, header.c_str(), fmt, ap);
  }
  va_end(ap);

  vhall_unlock(&mLock);

}

void VhallLog::Debug(std::string header, const char* fmt, ...) {

  vhall_lock(&mLock);

  va_list ap;
  va_start(ap, fmt);
  for (int i = 0; i < mWriterList.size(); i++) {
    mWriterList[i]->WriteLog(VHALL_LOG_LEVEL_DEBUG, header.c_str(), fmt, ap);
  }
  va_end(ap);

  vhall_unlock(&mLock);

}

void VhallLog::Warn(std::string header, const char* fmt, ...) {

  vhall_lock(&mLock);

  va_list ap;
  va_start(ap, fmt);
  for (int i = 0; i < mWriterList.size(); i++) {
    mWriterList[i]->WriteLog(VHALL_LOG_LEVEL_WARN, header.c_str(), fmt, ap);
  }
  va_end(ap);

  vhall_unlock(&mLock);

}

void VhallLog::Error(std::string header, const char* fmt, ...) {

  vhall_lock(&mLock);

  va_list ap;
  va_start(ap, fmt);
  for (int i = 0; i < mWriterList.size(); i++) {
    mWriterList[i]->WriteLog(VHALL_LOG_LEVEL_ERROR, header.c_str(), fmt, ap);
  }
  va_end(ap);

  vhall_unlock(&mLock);
}

int  VhallLog::AddLog(VhallLogType type, void *InitParam, int level) {
  int ret = -1;

  vhall_lock(&mLock);

  if (type == VHALL_LOG_TYPE_CONSOLE) {
    ConsoleInitParam* param = (ConsoleInitParam*)InitParam;
    LogWriter * clog = new ConsoleLogWriter(level);
    clog->Init(param);
    mWriterList.push_back(clog);
    ret = clog->GetId();
  } else if (type == VHALL_LOG_TYPE_FILE) {
    FileInitParam* param = (FileInitParam*)InitParam;
    LogWriter * flog = new FileLogWriter(level);
    flog->Init(param);
    mWriterList.push_back(flog);
    ret = flog->GetId();
  } else if (type == VHALL_LOG_TYPE_SOCKET) {
    //TODO add sock log
    ret = -1;
  } else {
    fprintf(stderr, "Log Type unrecgnized LogType=%d", type);
    ret = -1;
  }

  vhall_unlock(&mLock);

  return ret;
}

int  VhallLog::RemoveLog(int LogId) {
  int ret = -1;
  vhall_lock(&mLock);
  std::vector<LogWriter*>::iterator it;
  for (it = mWriterList.begin(); it != mWriterList.end(); it++) {
    if ((*it)->GetId() == LogId) {
      (*it)->Destroy();
      delete (*it);
      mWriterList.erase(it);
      ret = 0;
      break;
    }
  }
  vhall_unlock(&mLock);
  return ret;
}

int VhallLog::RmoveAllLog() {
  vhall_lock(&mLock);
  for (int i = 0; i < mWriterList.size(); i++) {
    mWriterList[i]->Destroy();
    delete mWriterList[i];
  }
  mWriterList.clear();

  vhall_unlock(&mLock);
  return 0;
}

int VhallLog::SetLogLevel(int LogId, int level) {
  int ret = -1;
  vhall_lock(&mLock);
  std::vector<LogWriter*>::iterator it;
  for (it = mWriterList.begin(); it != mWriterList.end(); it++) {
    if ((*it)->GetId() == LogId) {
      ret = (*it)->SetLevel(level);
      break;
    }
  }
  vhall_unlock(&mLock);
  return ret;
}

std::string GetFormatDateTime() {
  const int buff_len = 255;
  char tmpBuf[buff_len];
#ifdef WIN32
  SYSTEMTIME sys;
  GetLocalTime(&sys);
  snprintf(tmpBuf, buff_len, "%d-%02d-%02d %02d:%02d:%02d.%03d",
    sys.wYear, sys.wMonth, sys.wDay,
    sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
#else
  struct timeval  tv;
  struct tm       *p;
  gettimeofday(&tv, NULL);
  p = localtime(&tv.tv_sec);
  snprintf(tmpBuf, buff_len, "%d-%02d-%02d %02d:%02d:%02d.%03d",
    1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec, (int)(tv.tv_usec / 1000));
#endif
  return std::string(tmpBuf);
}

std::string GetFormatDate2() {
  const int buff_len = 255;
  char tmpBuf[buff_len];
#ifdef WIN32
  SYSTEMTIME sys;
  GetLocalTime(&sys);
  snprintf(tmpBuf, buff_len, "%d_%02d_%02d_%02d_%02d_%02d_%03d",
    sys.wYear, sys.wMonth, sys.wDay,
    sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
#else
  struct timeval  tv;
  struct tm       *p;
  gettimeofday(&tv, NULL);
  p = localtime(&tv.tv_sec);
  snprintf(tmpBuf, buff_len, "%d_%02d_%02d_%02d_%02d_%02d_%03d",
    1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
    p->tm_hour, p->tm_min, p->tm_sec, (int)(tv.tv_usec / 1000));
#endif
  return std::string(tmpBuf);
}

std::string int2str(int num)
{
  std::strstream stream;
  std::string s;
  stream << num;
  stream >> s;
  return s;
}

extern VhallLog * vhall_log = new VhallLog();
bool vhall_log_enalbe = false;

void TcharToChar(const TCHAR * tchar, char * _char)
{
  int iLength;
  iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);

  WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
}

std::unique_ptr<rtc::FileRotatingLogSink> _logSink;

void InitLog() {
  if (vhall_log_enalbe) {
    return;
  }
  vhall_log_enalbe = true;

  const wchar_t *szPath = L"VhallRTC\\Info";
  TCHAR fullPath[MAX_PATH] = {0};
  char FilePath[MAX_PATH] = {0};
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullPath))) {
    // TcharToChar(szPath, fullPath);
    PathAppend(fullPath, szPath);
    TcharToChar(fullPath, FilePath);
  }

  if (!strlen(FilePath)) {
    return;
  }

  PathRemoveFileSpec(fullPath);

  if (!PathFileExists(fullPath)) {
    CreateDirectory(fullPath, NULL);
  }

  FileInitParam filepara;
  filepara.pFilePathName = FilePath;
  filepara.nPartionSize = 0;
  filepara.nPartionTime = 0;
  ADD_NEW_LOG(VhallLogType::VHALL_LOG_TYPE_FILE, &filepara, VhallLogLevel::VHALL_LOG_LEVEL_DEBUG);

  char RTCPath[MAX_PATH] = { 0 };
  char RTCName[MAX_PATH] = { 0 };
  TcharToChar(fullPath, RTCPath);
  sprintf(RTCName, "RTC_%s", GetFormatDate2().c_str());
  _logSink.reset(new rtc::FileRotatingLogSink(RTCPath, RTCName,
      1024 * 1024 * 50, 50));
  _logSink->Init();
  _logSink->DisableBuffering();
  rtc::LogMessage::LogThreads(true);
  rtc::LogMessage::LogTimestamps(true);
  rtc::LogMessage::AddLogToStream(_logSink.get(), rtc::LoggingSeverity::LS_VERBOSE);
  return;
}
