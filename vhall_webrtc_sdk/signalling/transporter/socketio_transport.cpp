#include "common/vhall_define.h"
#include "signalling/transporter/socketio_transport.h"
#include "signalling/transporter/message_wrapper.h"

SocketIOTransport::SocketIOTransport() {
      mSioClient = std::make_shared<sio::client>();
}

SocketIOTransport::~SocketIOTransport() {
  if (mSioClient) {
    mSioClient->clear_con_listeners();
    mSioClient->clear_socket_listeners();
    mSioClient->sync_close();
  }
}

void SocketIOTransport::SetDelegate(const std::weak_ptr<Deleagte> &delegate){
   mDelegate = delegate;
}

int SocketIOTransport::Connect(const std::string &uri){
   if (mSioClient) {
      mSioClient->close();
      mSioClient->clear_con_listeners();
      mSioClient->clear_socket_listeners();
      mSioClient->set_open_listener([&]()->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnOpen();
         }
      });
      mSioClient->set_close_listener([&](sio::client::close_reason const& reason)->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnClose();
         }
      });
      mSioClient->set_fail_listener([&]()->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnFail();
         }
      });
      mSioClient->set_socket_open_listener([&](const std::string& nsp)->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnSocketOpen(nsp);
         }
      });
      mSioClient->set_socket_close_listener([&](const std::string& nsp)->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnSocketClose(nsp);
         }
      });
      mSioClient->set_reconnect_listener([&](unsigned a, unsigned b)->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnReconnect(a, b);
         }
      });
      mSioClient->set_reconnecting_listener([&]()->void{
         std::shared_ptr<Deleagte> obj(mDelegate.lock());
         if (obj) {
            obj->OnReconnecting();
         }
      });
      mSioClient->connect(uri);
      return 0;
   }
   return -1;
}

void SocketIOTransport::Disconnect(){
   if (mSioClient) {
      mSioClient->clear_con_listeners();
      mSioClient->clear_socket_listeners();
      mSioClient->close();
   }
}

void SocketIOTransport::Sync_Disconnect() {
  if (mSioClient) {
    mSioClient->clear_con_listeners();
    mSioClient->clear_socket_listeners();
    mSioClient->sync_close();
  }
}

void SocketIOTransport::On(const std::string& event_name,const EventCallback &callback_func){
   if (mSioClient) {
      mSioClient->socket()->on(event_name, [callback_func](sio::event& event)->void{
         callback_func(event.get_name(),MessageWrapper::FromMessage(event.get_message()));
      });
   }
}

void SocketIOTransport::Off(std::string const& event_name){
   if (mSioClient) {
      mSioClient->socket()->off(event_name);
   }
}

void SocketIOTransport::OffAll(){
   if (mSioClient) {
      mSioClient->socket()->off_all();
   }
}

int SocketIOTransport::SendMessage(const std::string &event, const std::string &obj_msg, const EventCallback& callback_func, uint64_t timeOut){
   if (mSioClient) {
      mSioClient->socket()->emit(event, MessageWrapper::ToMessage(obj_msg),[event,callback_func](sio::message::list const& msg){
         callback_func(event,MessageWrapper::FromMessage(msg.to_array_message()));
      }, timeOut);
      return 0;
   }
   return -1;
}

int SocketIOTransport::SendSdpMessage(const std::string &event, const std::string & obj_msg, const EventCallback &callback_func, uint64_t timeOut){
   if (mSioClient) {
      mSioClient->socket()->emit(event, MessageWrapper::ToMessage(obj_msg),sio::null_message::create(),[event,callback_func](sio::message::list const& msg){
         callback_func(event,MessageWrapper::FromMessage(msg.to_array_message()));
      }, timeOut);
      return 0;
   }
   return -1;
}

void SocketIOTransport::SetReconnectAttempts(int attempts){
   if (mSioClient) {
      mSioClient->set_reconnect_attempts(attempts);
   }
}

void SocketIOTransport::SetReconnectDelay(unsigned millis){
   if (mSioClient) {
      mSioClient->set_reconnect_delay(millis);
   }
}

void SocketIOTransport::SetReconnectDelayMax(unsigned millis){
   if (mSioClient) {
      mSioClient->set_reconnect_delay_max(millis);
   }
}

