#ifndef __MESSAGE_WRAPPER_INCLUDE_H__
#define __MESSAGE_WRAPPER_INCLUDE_H__
#include <string>
#include "socketio/src/sio_message.h"

class MessageWrapper {
public:
	static sio::message::ptr ToMessage(std::string json);
	static std::string FromMessage(sio::message::ptr);
};

#endif // __MESSAGE_WRAPPER_INCLUDE_H__
