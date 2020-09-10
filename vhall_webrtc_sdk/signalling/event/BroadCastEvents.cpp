#include "BroadCastEvents.h"

vhall::BroadCastEvents::BroadCastEvents() {
  error_code_detail_[RESP_SUCCESS] = "成功";
  error_code_detail_[RESP_AVROOMCONTROLLER_BASE_PARAMS_BADFORMAT] = "参数类型错误";
  error_code_detail_[RESP_AVROOMCONTROLLER_ROOM_NOTFOUND] = "房间未找到";
  error_code_detail_[RESP_AVROOMCONTROLLER_RTMPURL_ILLEGAL] = "rtmp推送地址不合法";
  error_code_detail_[RESP_AVROOMCONTROLLER_RTMPURL_NOTPROVIDED] = "未提供rtmp推流地址";
  error_code_detail_[RESP_AVROOMCONTROLLER_RESOLUTION_OUTOFRANGE] = "混流分辨率超出范围";
  error_code_detail_[RESP_AVROOMCONTROLLER_BITRATE_OUTOFRANGE] = "混流输出视频码率超出范围（做默认处理1000kbps）";
  error_code_detail_[RESP_AVROOMCONTROLLER_FRAMERATE_OUTOFRANGE] = "混流帧率超出范围（做默认处理20fps）";
  error_code_detail_[RESP_AVROOMCONTROLLER_LAYOUTMODE_OUTOFRANGE] = "混流模式值超出范围";
  error_code_detail_[RESP_AVROOMCONTROLLER_BGCOLOR_ILLEGAL] = "	画布背景色设置不合法";
  error_code_detail_[RESP_AVROOMCONTROLLER_BORDER_ERR] = "边框设置参数错误";
  error_code_detail_[RESP_AVROOMCONTROLLER_STREAMLIST_ILLEGAL] = "自定义布局参数不合法";
  error_code_detail_[RESP_AVROOM_RESOURCE_NOT_INIT] = "混流房间未初始化";
  error_code_detail_[RESP_AVROOM_BROADCAST_NOT_STARTED] = "混流rtmp推送未开始";
  error_code_detail_[RESP_AVROOM_BROADCAST_NOT_CONFIGED] = "混流房间还未配置";
  error_code_detail_[RESP_AVROOM_PEERID_NOTFOUND] = "混流房间找不到该成员";
  error_code_detail_[RESP_AVROOM_FORCE_STOPPED_ALREADY] = "混流已强制停止";
  error_code_detail_[RESP_AVROOM_HAS_NO_MEMBERS] = "混流房间没有要混的视频流";
  error_code_detail_[RESP_EXTERNALCHANNEL_ADDED] = "channel 成功加入到混流房间";
  error_code_detail_[RESP_EXTERNALCHANNEL_ADD_ERR] = "channel 加入到混流房间失败";
}

std::string vhall::BroadCastEvents::GetDetail(int errorCode) {
  std::string detail = error_code_detail_[errorCode];
  if ("" == detail) {
    if (errorCode == 0 || errorCode == 200) {
      detail = "成功";
    }
    else {
      detail = "unknown error";
    }
  }
  return detail;
}
