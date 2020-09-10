//  sio_test_sample.cpp
//
//  Created by Melo Yao on 3/24/15.
//

#include "socketio/src/sio_client.h"

#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <sstream>
#include "signalling/transporter/message_wrapper.h"
/*#include "json/rapidjson.h"
#include "json/document-wrapper.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
*/
#include "common/base64.h"
#include "common/vhall_define.h"

#include "signalling/transporter/socketio_transport.h"

#ifdef WIN32
#define HIGHLIGHT(__O__) std::cout<<__O__<<std::endl
#define EM(__O__) std::cout<<__O__<<std::endl

#include <stdio.h>
#include <tchar.h>
#define MAIN_FUNC int _tmain(int argc, _TCHAR* argv[])
#else
#define HIGHLIGHT(__O__) std::cout<<"\e[1;31m"<<__O__<<"\e[0m"<<std::endl
#define EM(__O__) std::cout<<"\e[1;30;1m"<<__O__<<"\e[0m"<<std::endl

#define MAIN_FUNC int main(int argc ,const char* args[])
#endif

using namespace sio;
using namespace std;
std::mutex _lock;

std::condition_variable_any _cond;
bool connect_finish = false;

class connection_listener
{
	sio::client &handler;

public:

	connection_listener(sio::client& h) :
		handler(h)
	{
	}


	void on_connected()
	{
		_lock.lock();
		_cond.notify_all();
		connect_finish = true;
		_lock.unlock();
	}
	void on_close(client::close_reason const& reason)
	{
		std::cout << "sio closed " << std::endl;
		exit(0);
	}

	void on_fail()
	{
		std::cout << "sio failed " << std::endl;
		exit(0);
	}
};

int participants = -1;

socket::ptr current_socket;

void bind_events()
{
	current_socket->on("new message", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		string message = data->get_map()["message"]->get_string();
		EM(user << ":" << message);
		_lock.unlock();
	}));

	current_socket->on("user joined", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		participants = data->get_map()["numUsers"]->get_int();
		bool plural = participants != 1;

		//     abc "
		HIGHLIGHT(user << " joined" << "\nthere" << (plural ? " are " : "'s ") << participants << (plural ? " participants" : " participant"));
		_lock.unlock();
	}));
	current_socket->on("user left", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp)
	{
		_lock.lock();
		string user = data->get_map()["username"]->get_string();
		participants = data->get_map()["numUsers"]->get_int();
		bool plural = participants != 1;
		HIGHLIGHT(user << " left" << "\nthere" << (plural ? " are " : "'s ") << participants << (plural ? " participants" : " participant"));
		_lock.unlock();
	}));
}
int test();
MAIN_FUNC
{
	test();
	return 0;
}
//嘉宾
//#define TOKEN_ID "eyJ0b2tlbiI6eyJyb29tSWQiOiJwYWFzXzEyMzQ1Njc4OSIsInVzZXJJZCI6Ik5EVTIiLCJyb2xlIjoicHJlc2VudGVyIiwic2VydmljZSI6IjVhNDhhZjIyOTA2MTU5ZjVlYTJhZjViZiIsImNyZWF0aW9uRGF0ZSI6MTUxNTEzNzg4MTQ0Mywic2VjdXJlIjpmYWxzZSwicDJwIjpmYWxzZSwiaG9zdCI6IjE5Mi4xNjguMS4xMzU6ODA4MSJ9LCJzaWduYXR1cmUiOiJOR1V5TnpabFpEWTRPV000WWpJNFlqZzRaVEV3TjJNNFpEZzVOVEJoWXpFeVlXTmpPREppTXc9PSJ9"
//主持人
#define TOKEN_ID "eyJ0b2tlbiI6eyJyb29tSWQiOiJwYWFzXzEyMzQ1Njc4OSIsInVzZXJJZCI6Ik1USXoiLCJyb2xlIjoiYWRtaW5pc3RyYXRvciIsInNlcnZpY2UiOiI1YTQ4YWYyMjkwNjE1OWY1ZWEyYWY1YmYiLCJjcmVhdGlvbkRhdGUiOjE1MTUxMzc4NDk3OTAsInNlY3VyZSI6ZmFsc2UsInAycCI6ZmFsc2UsImhvc3QiOiIxOTIuMTY4LjEuMTM1OjgwODEifSwic2lnbmF0dXJlIjoiWldWaVltUmpaR0ZpWVRNNU9EVmxZbU5sTjJRNFpqSTJNamhqTWpVeU56SmlNRE0xWVRjM1pBPT0ifQ=="

std::string UrlFromTokenParse(const std::string& token);

sio::message::ptr create_token() {
	sio::message::ptr dstMsg = sio::object_message::create();
	sio::object_message* omsg = (sio::object_message*)dstMsg.get();

	sio::message::ptr tokenPtr = sio::object_message::create();
	sio::object_message* tokenMsg = (sio::object_message*)tokenPtr.get();

	tokenMsg->insert("roomId", "paas_123456789");
	tokenMsg->insert("userId", "MTIz");
	tokenMsg->insert("role", "administrator");
	tokenMsg->insert("service", "5a48af22906159f5ea2af5bf");
	tokenMsg->insert("creationDate", "1515137849790");
	tokenMsg->insert("secure", sio::bool_message::create(false));
	tokenMsg->insert("p2p", sio::bool_message::create(false));
	tokenMsg->insert("host", "192.168.1.135:8081");

	omsg->insert("token", tokenPtr);
	omsg->insert("signature", "ZWViYmRjZGFiYTM5ODVlYmNlN2Q4ZjI2MjhjMjUyNzJiMDM1YTc3ZA==");
	return dstMsg;
}

void test_callback(const std::string& name, message::ptr const& message, bool need_ack, message::list& ack_message) {
	HIGHLIGHT(name << "::" << message);
}
 #include "common/arithmetic.h"

int test() {
	std::string token;
	if (talk_base::Base64::IsBase64Encoded(TOKEN_ID)) {
		token = talk_base::Base64::Decode(TOKEN_ID, talk_base::Base64::DO_PARSE_ANY);

	}
		/* char decodebase64[256] = "";
	 int len = 0;
	 decode_base64(TOKEN_ID, strlen(TOKEN_ID), decodebase64, &len);
	 token = decodebase64;*/
	 /*std::string uri = UrlFromTokenParse(token);
	if (uri.empty()) {
		LOGE("uri is empty!");
		return 0;
	}*/

	sio::message::ptr tokenMsg = MessageWrapper::ToMessage(token);	
	std::string payload = MessageWrapper::FromMessage(tokenMsg);

	std::string uri = "ws://192.168.1.135:8081";
	SocketIOTransport socketioTransport;
	socketioTransport.Init(uri);
	string nickname;
	
	// First message with the token
	LOGI("First message with the token.");	
	socketioTransport.RegisterEvent("token", test_callback);
	socketioTransport.RegisterEvent("connected", test_callback);
	socketioTransport.RegisterEvent("error", test_callback);
	
	socketioTransport.Start();

	//socketioTransport.SendMessage("token", create_token());
	socketioTransport.SendMessage("token", tokenMsg);
	nickname = "";
	while (nickname.length() == 0) {
		HIGHLIGHT("Type your nickname:");

		getline(cin, nickname);
	}
	return 0;
}
/*
std::string UrlFromTokenParse(const std::string& token) {
	LOGI("token json:%s", token.c_str());
	rapidjson::Document doc;                   ///< 创建一个Document对象 rapidJson的相关操作都在Document类中
	doc.Parse<0>(token.c_str());               ///< 通过Parse方法将Json数据解析出来
	if (doc.HasParseError()) {
		LOGE("GetParseError%d\n", doc.GetParseError());
		return "";
	}
	std::stringstream pre;
	pre.str("");
	rapidjson::Value& valObject = doc["token"];

	if (valObject.IsObject()) {

		//      rapidjson::StringBuffer buffer;
		//      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		//      valObject.Accept(writer);
		//      mToken = buffer.GetString();
		//      LOGI("token:%s",mToken.c_str());

		rapidjson::Value& valString = valObject["host"];
		std::string host;
		if (valString.IsString()) {
			host = valString.GetString();
		}
		bool secure = false;
		rapidjson::Value& valBool = valObject["secure"];
		if (valBool.IsBool()) {
			secure = valBool.GetBool();
		}
		if (secure) {
			pre << "wss://";
		}
		else {
			pre << "ws://";
		}
		pre << host;
	}
	return pre.str();
}*/

int test1() {

	sio::client h;
	connection_listener l(h);

	h.set_open_listener(std::bind(&connection_listener::on_connected, &l));
	h.set_close_listener(std::bind(&connection_listener::on_close, &l, std::placeholders::_1));
	h.set_fail_listener(std::bind(&connection_listener::on_fail, &l));
	//h.connect("http://127.0.0.1:3000");
	h.connect("ws://192.168.1.135:8081");
	_lock.lock();
	if (!connect_finish)
	{
		_cond.wait(_lock);
	}
	_lock.unlock();
	current_socket = h.socket();
Login:
	string nickname;
	while (nickname.length() == 0) {
		HIGHLIGHT("Type your nickname:");

		getline(cin, nickname);
	}
	current_socket->on("login", sio::socket::event_listener_aux([&](string const& name, message::ptr const& data, bool isAck, message::list &ack_resp) {
		_lock.lock();
		participants = data->get_map()["numUsers"]->get_int();
		bool plural = participants != 1;
		HIGHLIGHT("Welcome to Socket.IO Chat-\nthere" << (plural ? " are " : "'s ") << participants << (plural ? " participants" : " participant"));
		_cond.notify_all();
		_lock.unlock();
		current_socket->off("login");
	}));
	current_socket->emit("add user", nickname);
	_lock.lock();
	if (participants<0) {
		_cond.wait(_lock);
	}
	_lock.unlock();
	bind_events();

	HIGHLIGHT("Start to chat,commands:\n'$exit' : exit chat\n'$nsp <namespace>' : change namespace");
	for (std::string line; std::getline(std::cin, line);) {
		if (line.length()>0)
		{
			if (line == "$exit")
			{
				break;
			}
			else if (line.length() > 5 && line.substr(0, 5) == "$nsp ")
			{
				string new_nsp = line.substr(5);
				if (new_nsp == current_socket->get_namespace())
				{
					continue;
				}
				current_socket->off_all();
				current_socket->off_error();
				//per socket.io, default nsp should never been closed.
				if (current_socket->get_namespace() != "/")
				{
					current_socket->close();
				}
				current_socket = h.socket(new_nsp);
				bind_events();
				//if change to default nsp, we do not need to login again (since it is not closed).
				if (current_socket->get_namespace() == "/")
				{
					continue;
				}
				goto Login;
			}
			current_socket->emit("new message", line);
			_lock.lock();
			EM("\t\t\t" << line << ":" << "You");
			_lock.unlock();
		}
	}
	HIGHLIGHT("Closing...");
	h.sync_close();
	h.clear_con_listeners();
	return 0;
}