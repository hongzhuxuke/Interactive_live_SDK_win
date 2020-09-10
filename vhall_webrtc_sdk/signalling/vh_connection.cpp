//
//  ec_connection.cpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/29.
//  Copyright © 2017年 ilong. All rights reserved.
//

#include "vh_connection.h"

NS_VH_BEGIN

ConnectionEvent::ConnectionEvent(){
   
}

ConnectionEvent::ConnectionEvent(const std::string &type,const std::string &msg){
   mType = type;
   mMsg = msg;
}

ConnectionEvent::~ConnectionEvent(){
   
}

VHConnection::VHConnection(){
   
}

VHConnection::~VHConnection(){
   
}

void VHConnection::SetDelegate(const std::weak_ptr<Delegate> &delegate){
   mDelegate = delegate;
}

void VHConnection::DispatchEvent(BaseEvent &event){
   EventDispatcher::DispatchEvent(event);
   std::shared_ptr<Delegate> delegate(mDelegate.lock());
   if (delegate) {
      delegate->OnEvent(event);
   }
}

void VHConnection::DispatchEvent1(BaseEvent &event){
   EventDispatcher::DispatchEvent(event);
}

NS_VH_END
