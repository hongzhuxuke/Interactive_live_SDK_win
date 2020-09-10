#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma  comment(lib,"ws2_32.lib")

class GetLocalIp
{
public:
   GetLocalIp();
   GetLocalIp(std::string serverIp, std::string port);
   virtual ~GetLocalIp();
   std::string GetLocalAddr();
private:
   int Socket_setup();
   int Socket_cleanup();
   struct addrinfo* Dns_resolve(std::string host, std::string port);
   std::string Get_addr_ip(const struct addrinfo * addr);
   void Addrinfo_free(addrinfo * info);
private:
   std::string mServerIp = "8.8.8.8"; // google dns server, actually not used, any address is ok
   std::string mPort = "80";
   SOCKET mSocket = INVALID_SOCKET;
};

namespace vhall {
  class Utility {
  public:
    static uint64_t GetTimestampMs();
    template<typename T>
    static std::string ToString(T arg);
  };

  template<typename T>
  inline std::string Utility::ToString(T arg) {
    std::stringstream ss;
    ss << arg;
    return ss.str();
  }
}