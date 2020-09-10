//
//  ec_events.hpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//

#ifndef vh_events_h
#define vh_events_h

#include <string>
#include <list>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <sstream>
#include "common/vhall_define.h"
#include "common/vhall_log.h"

NS_VH_BEGIN
class VHStream;

class VH_DLL BaseEvent {
   
public:
   virtual ~BaseEvent();
   std::string mType = "";
   template<typename T>
   static std::string ToString(T arg){
      std::stringstream ss;
      ss << arg;
      return ss.str();
   }
};

class VH_DLL MonitorEvent :public BaseEvent {
public:
  MonitorEvent() {};
  virtual ~MonitorEvent() {};
  DWORD mPercentOfMemoryInUse = 0;
  DWORD mTotalPhysMem = 0; // MB
  DWORD mAvailPhysMem = 0;
  DWORD mTotalVirtualMem = 0;
  DWORD mAvailVirtualMem = 0;
  float mCurProcessMem = 0;
  double mPercentOfCpuLoad = 0;
};

class AudioDeviceEvent :public BaseEvent {
public:
  enum DEVICE_STATE {
    ACTIVE,
    DISABLED,
    NOTPRESENT,
    UNPLUGGED
  };
  std::wstring deviceId = L"";
  DEVICE_STATE state = ACTIVE;
  
  // core audio info
  long hr = 0;
  std::string detail = "";

};

class VH_DLL RoomEvent:public BaseEvent {
   
public:
   ~RoomEvent();
   enum ErrorType {
     Undefined = -1,
     InitSocketFailed,
     ConnectFailed,
     ServerRefused,
     SocketDisconnected,
   };
   int mCode = 0; // server response
   ErrorType mErrorType = Undefined;
   std::string mMessage = "";
   std::list<std::shared_ptr<VHStream>> mStreams;
};

class VH_DLL StreamEvent:public BaseEvent {
   
public:
   ~StreamEvent();
   std::string mMessage;
   std::shared_ptr<VHStream> mStreams;
};

class VH_DLL ReconnectStreamEvent :public BaseEvent {

public:
  ~ReconnectStreamEvent();
  std::string mMessage;
  std::shared_ptr<VHStream> OldStream;
  std::shared_ptr<VHStream> NewStream;
};

class VH_DLL AddStreamEvent :public BaseEvent {

public:
  ~AddStreamEvent();
  std::shared_ptr<VHStream> mStream;
};

class VH_DLL PublisherEvent:public BaseEvent {
   
public:
   ~PublisherEvent();
};

//c++11 style callbacks entities will be created using (which uses std::bind)
typedef std::function<void(BaseEvent&)> VHEvent;
//c++11 map to callbacks
typedef std::unordered_map<std::string, std::vector<VHEvent>> VHEventRegistry;

class VH_DLL EventDispatcher {
   
public:
   EventDispatcher();
   virtual ~EventDispatcher();
   virtual int AddEventListener(const std::string &event_type, const VHEvent &listener);
   virtual void RemoveEventListener(const std::string &event_type, const int index);
   virtual void RemoveAllEventListener();
   virtual void DispatchEvent(BaseEvent &event);
private:
   VHEventRegistry mDispatcher;
};


NS_VH_END

#endif
