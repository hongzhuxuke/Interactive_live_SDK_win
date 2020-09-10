#ifndef __SOCKETIO_TRANSPORT_INCLUDE_H__
#define __SOCKETIO_TRANSPORT_INCLUDE_H__

#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include "common/vhall_define.h"
#include "common/vhall_log.h"
#include "socketio/src/sio_client.h"


typedef std::function<void(const std::string&,const std::string&)> EventCallback;

class SocketIOTransport {
public:
   class Deleagte {
   public:
      Deleagte(){};
      virtual ~Deleagte(){};
      virtual void OnOpen(){LOGI("socketio open");};
      virtual void OnFail(){LOGI("socketio fail");};
      virtual void OnReconnecting(){LOGI("socketio reconnecting");};
      virtual void OnReconnect(unsigned, unsigned){LOGI("socketio reconnect");};
      virtual void OnClose(){LOGI("socketio close");};
      virtual void OnSocketOpen(std::string const& nsp){LOGI("socketio socket open");};
      virtual void OnSocketClose(std::string const& nsp){LOGI("socketio socket close");};
   };
public:
	SocketIOTransport();
	~SocketIOTransport();
   
   void SetDelegate(const std::weak_ptr<Deleagte> &delegate);
   
   int Connect(const std::string &uri);
   
   void Disconnect();

   void Sync_Disconnect();
   
   void On(const std::string& event_name,const EventCallback &callback_func);
   
   void Off(std::string const& event_name);
   
   void OffAll();
   
   int SendMessage(const std::string &event, const std::string & obj_msg, const EventCallback &callback_func, uint64_t timeOut = 0);
   
   int SendSdpMessage(const std::string &event, const std::string & obj_msg, const EventCallback &callback_func, uint64_t timeOut = 0);
   
   void SetReconnectAttempts(int attempts);
   
   void SetReconnectDelay(unsigned millis);
   
   void SetReconnectDelayMax(unsigned millis);
   
private:
   std::shared_ptr<sio::client> mSioClient;
   std::weak_ptr<Deleagte> mDelegate;
};


#endif // __SOCKETIO_TRANSPORT_INCLUDE_H__
