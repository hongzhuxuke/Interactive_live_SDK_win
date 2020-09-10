//
//  ec_events.cpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//

#include "vh_events.h"
#include <algorithm>

NS_VH_BEGIN

BaseEvent::~BaseEvent(){
   
}

RoomEvent::~RoomEvent(){
   
}

StreamEvent::~StreamEvent(){
   
}

ReconnectStreamEvent::~ReconnectStreamEvent() {

}

AddStreamEvent::~AddStreamEvent() {

}

PublisherEvent::~PublisherEvent(){
   
}

EventDispatcher::EventDispatcher(){
   mDispatcher.clear();
}

EventDispatcher::~EventDispatcher(){
   mDispatcher.clear();
}

int EventDispatcher::AddEventListener(const std::string &event_type, const VHEvent &listener){
   if (mDispatcher.find(event_type) == mDispatcher.end()) {
      std::vector<VHEvent> array;
      mDispatcher[event_type] = std::move(array);
   }
   mDispatcher[event_type].push_back(listener);
   return (int)mDispatcher[event_type].size()-1;
}

void EventDispatcher::RemoveEventListener(const std::string &event_type, const int index){
   if (mDispatcher.find(event_type) == mDispatcher.end()) {
      return;
   }
   auto listeners = mDispatcher[event_type];
   if (listeners.size()>index) {
      listeners.erase(listeners.begin()+index);
   }
}

void EventDispatcher::RemoveAllEventListener(){
   for (auto &item:mDispatcher) {
      item.second.clear();
   }
   mDispatcher.clear();
}

void EventDispatcher::DispatchEvent(BaseEvent&event) {
   if (event.mType.empty()) {
      LOGE("event type is empty!");
      return;
   }
   LOGI("Event Type is %s",event.mType.c_str());
   auto listeners = mDispatcher.find(event.mType) != mDispatcher.end()?mDispatcher[event.mType]:std::vector<VHEvent>();
   for (auto &l:listeners) {
      l(event);
   }
}

NS_VH_END
