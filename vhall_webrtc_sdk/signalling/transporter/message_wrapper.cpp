#include "common/vhall_define.h"
#include "common/vhall_log.h"
#include "message_wrapper.h"
#include "socketio/lib/rapidjson/include/rapidjson/rapidjson.h"
#include "socketio/lib/rapidjson/include/rapidjson/stringbuffer.h"
#include "socketio/lib/rapidjson/include/rapidjson/writer.h"
#include "socketio/lib/rapidjson/include/rapidjson/reader.h"
#include "socketio/lib/rapidjson/include/rapidjson/document.h"
#include <string>
#include <sstream>
#include <iostream> 

#include "socketio/src/internal/sio_client_impl.h"

using namespace rapidjson;

namespace sio
{
	sio::message::ptr from_json(Value const& value, std::vector<std::shared_ptr<const std::string> > const& buffers);
	void accept_message(message const& msg, Value& val, Document& doc, vector<shared_ptr<const string> >& buffers);
}

sio::message::ptr MessageWrapper::ToMessage(std::string payload) {
	sio::message::ptr msg = nullptr;
	rapidjson::Document doc;
	doc.Parse<0>(payload.c_str());
	if (doc.HasParseError()) {
		LOGE("GetParseError%d\n", doc.GetParseError());
		return nullptr;
	}	
	msg = sio::from_json(doc, std::vector<std::shared_ptr<const std::string>>());
	return msg;	
}
   
std::string MessageWrapper::FromMessage(sio::message::ptr _message) {
	std::string payload_ptr;
	if (_message) {
		Document doc;
      auto buffers = std::vector<std::shared_ptr<const std::string>>();
		sio::accept_message(*_message, doc, doc, buffers);
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		doc.Accept(writer);
		payload_ptr.append(buffer.GetString(), buffer.GetSize());
	}	
	return payload_ptr;
}
