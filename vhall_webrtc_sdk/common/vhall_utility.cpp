#include "vhall_utility.h"
#include "vhall_log.h"

#pragma comment (lib, "Ws2_32.lib")
GetLocalIp::GetLocalIp() {
   mServerIp = "8.8.8.8";
   mPort = "80";
   mSocket = INVALID_SOCKET;
   Socket_setup();
}
GetLocalIp::GetLocalIp(std::string serverIp, std::string port)
{
   mServerIp = serverIp;
   mPort = port;
   mSocket = INVALID_SOCKET;
   Socket_setup();
}
GetLocalIp::~GetLocalIp() {
   if (mSocket != INVALID_SOCKET) {
      ::closesocket(mSocket);
   }
   Socket_cleanup();
}
std::string GetLocalIp::GetLocalAddr() {
   struct addrinfo * addrinfo = Dns_resolve(mServerIp, mPort);
   if (nullptr == addrinfo) {
      return "";
   }
   std::string server_ip = Get_addr_ip(addrinfo);
   if (!server_ip.compare("")) {
      LOGE("server_ip == null.");
      return "";
   }
   LOGD("server_ip: %s", server_ip.c_str());

   if (mSocket != INVALID_SOCKET) {
      ::closesocket(mSocket);
   }
   SOCKET mScket = INVALID_SOCKET;
   while (addrinfo != NULL) {
      mScket = ::socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
      if (mScket != INVALID_SOCKET) {
         LOGD("create socket success.");
         break;
      }
      addrinfo = addrinfo->ai_next;
   }
   if (mScket == INVALID_SOCKET) {
      LOGE("create socket fail.");
      return "";
   }
   int retval = ::connect(mScket, addrinfo->ai_addr, addrinfo->ai_addrlen);
   if (retval < 0) {
      LOGE("connect fail, Error number: %d, Error message: %s", errno, strerror(errno));
      return "";
   }

   char ip_[128] = { 0 };
   const char* p = nullptr;
   sockaddr_storage addr_storage = { 0 };
   socklen_t addrlen = sizeof(addr_storage);
   sockaddr* addr_ = reinterpret_cast<sockaddr*>(&addr_storage);
   int result = ::getsockname(mScket, addr_, &addrlen);
   if (result >= 0) {
     
      if (addr_storage.ss_family == AF_INET)
      {
         const sockaddr_in* saddr = reinterpret_cast<const sockaddr_in*>(&addr_storage);
         p = inet_ntop(addrinfo->ai_family, &saddr->sin_addr, ip_, sizeof(ip_));
      }
      else if (addr_storage.ss_family == AF_INET6) {
         const sockaddr_in6* saddr = reinterpret_cast<const sockaddr_in6*>(&addr_storage);
         p = inet_ntop(addrinfo->ai_family, &saddr->sin6_addr, ip_, sizeof(ip_));
      }
   }
   if (p != nullptr) {
      LOGD("Local IP address is: %s", ip_);
   }
   else {
      LOGE("unable to get local addr, Error number: %d,  Error message: %s", errno, strerror(errno));
      return "";
   }
   ::closesocket(mScket);
   mScket = INVALID_SOCKET;
   return std::string(p);
}
int GetLocalIp::Socket_setup() {
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;

   /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
   wVersionRequested = MAKEWORD(2, 2);

   err = WSAStartup(wVersionRequested, &wsaData);
   if (err != 0) {
      LOGE("WSAStartup failed with error: %d, could not find a usable Winsock DLL\n", err);
      return -1;
   }
   return 0;
}
int GetLocalIp::Socket_cleanup() {
   WSACleanup();
   return 0;
}
struct addrinfo *  GetLocalIp::Dns_resolve(std::string host, std::string port) {
   struct addrinfo *answer = nullptr, hint;
   memset(&hint, 0, sizeof(hint));
   hint.ai_family = AF_UNSPEC;
   hint.ai_socktype = SOCK_DGRAM;//SOCK_STREAM;
   int ret = getaddrinfo(host.c_str(), port.c_str(), &hint, &answer);
   if (ret != 0) {
      LOGE("getaddrinfo fail.");
      return nullptr;
   }
   return answer;
}
std::string GetLocalIp::Get_addr_ip(const struct addrinfo * addr) {
   const struct addrinfo * curr = nullptr;
   char ip[128];
   for (curr = addr; curr != nullptr; curr = curr->ai_next) {
      switch (curr->ai_family) {
      case AF_UNSPEC:
         //do something here
         break;
      case AF_INET:
         inet_ntop(AF_INET, &((struct sockaddr_in *)curr->ai_addr)->sin_addr, ip, sizeof(ip));
         return std::string(ip);
      case AF_INET6:
         inet_ntop(AF_INET6, &((struct sockaddr_in6 *)curr->ai_addr)->sin6_addr, ip, sizeof(ip));
         return std::string(ip);
      }
   }
   return "";
}
void  GetLocalIp::Addrinfo_free(addrinfo * info) {
   if (info) {
      freeaddrinfo(info);
   }
}
namespace vhall {
  static bool have_clockfreq = false;
  static LARGE_INTEGER clock_freq;
  static uint64_t beginTime = 0;

  static inline uint64_t get_clockfreq(void)
  {
    if (!have_clockfreq)
      QueryPerformanceFrequency(&clock_freq);
    return clock_freq.QuadPart;
  }

  static uint64_t os_gettime_ns(void)
  {
    LARGE_INTEGER current_time;
    double time_val;

    QueryPerformanceCounter(&current_time);
    time_val = (double)current_time.QuadPart;
    time_val *= 1000000000.0;
    time_val /= (double)get_clockfreq();

    return (uint64_t)time_val;
  }

  uint64_t Utility::GetTimestampMs() {
    return os_gettime_ns() / 1000000;
  }
}