#pragma once
#include <iostream>
#include <string>
#include <unordered_map>

/* ����������Ϣ�ӿڶ��������, reference: http://wiki.vhallops.com/pages/viewpage.action?pageId=15368196 */
namespace vhall {
  __declspec(dllexport) class  BroadCastEvents final {
  public:
    BroadCastEvents();
    enum error_def {
      RESP_SUCCESS = 200, // 0 or 200
      RESP_AVROOMCONTROLLER_BASE_PARAMS_BADFORMAT = 10000,
      RESP_AVROOMCONTROLLER_ROOM_NOTFOUND = 20000,
      RESP_AVROOMCONTROLLER_RTMPURL_ILLEGAL,
      RESP_AVROOMCONTROLLER_RTMPURL_NOTPROVIDED,
      RESP_AVROOMCONTROLLER_RESOLUTION_OUTOFRANGE,
      RESP_AVROOMCONTROLLER_BITRATE_OUTOFRANGE,
      RESP_AVROOMCONTROLLER_FRAMERATE_OUTOFRANGE,
      RESP_AVROOMCONTROLLER_LAYOUTMODE_OUTOFRANGE,
      RESP_AVROOMCONTROLLER_BGCOLOR_ILLEGAL,
      RESP_AVROOMCONTROLLER_BORDER_ERR,
      RESP_AVROOMCONTROLLER_STREAMLIST_ILLEGAL,
      RESP_AVROOM_RESOURCE_NOT_INIT,
      RESP_AVROOM_BROADCAST_NOT_STARTED,
      RESP_AVROOM_BROADCAST_NOT_CONFIGED,
      RESP_AVROOM_PEERID_NOTFOUND,
      RESP_AVROOM_FORCE_STOPPED_ALREADY,
      RESP_AVROOM_HAS_NO_MEMBERS,
      RESP_EXTERNALCHANNEL_ADDED = 20020,
      RESP_EXTERNALCHANNEL_ADD_ERR,
    };
    std::string GetDetail(int errorCode);
  private:
    std::unordered_map<int, std::string> error_code_detail_;

  };
}
