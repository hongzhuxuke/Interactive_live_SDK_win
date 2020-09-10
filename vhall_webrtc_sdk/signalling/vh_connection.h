//
//  ec_connection.h
//  VhallSignaling
//
//  Created by ilong on 2017/12/29.
//  Copyright © 2017年 ilong. All rights reserved.
//

#ifndef vh_connection_h
#define vh_connection_h

#include <cstdio>
#include "vh_events.h"
#include "common/vhall_define.h"

NS_VH_BEGIN

class VH_DLL ConnectionEvent:public BaseEvent {
public:
   ConnectionEvent();
   ConnectionEvent(const std::string &type,const std::string &msg = "");
   ~ConnectionEvent();
   std::string mMsg;
   uint64_t    mStreamId;
   uint64_t    mPeerId;
};

class VH_DLL VHConnection:public EventDispatcher {
public:
   class Delegate {
   public:
      Delegate(){};
      virtual ~Delegate(){};
      virtual void OnEvent(BaseEvent &event) = 0;
   };
   VHConnection();
   ~VHConnection();
   void SetDelegate(const std::weak_ptr<Delegate> &delegate);
   virtual void DispatchEvent(BaseEvent &event);
   void DispatchEvent1(BaseEvent &event);
private:
   std::weak_ptr<Delegate> mDelegate;
};

NS_VH_END

#endif /* ec_connection_h */
