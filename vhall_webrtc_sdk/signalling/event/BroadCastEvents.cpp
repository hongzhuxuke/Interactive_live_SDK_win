#include "BroadCastEvents.h"

vhall::BroadCastEvents::BroadCastEvents() {
  error_code_detail_[RESP_SUCCESS] = "�ɹ�";
  error_code_detail_[RESP_AVROOMCONTROLLER_BASE_PARAMS_BADFORMAT] = "�������ʹ���";
  error_code_detail_[RESP_AVROOMCONTROLLER_ROOM_NOTFOUND] = "����δ�ҵ�";
  error_code_detail_[RESP_AVROOMCONTROLLER_RTMPURL_ILLEGAL] = "rtmp���͵�ַ���Ϸ�";
  error_code_detail_[RESP_AVROOMCONTROLLER_RTMPURL_NOTPROVIDED] = "δ�ṩrtmp������ַ";
  error_code_detail_[RESP_AVROOMCONTROLLER_RESOLUTION_OUTOFRANGE] = "�����ֱ��ʳ�����Χ";
  error_code_detail_[RESP_AVROOMCONTROLLER_BITRATE_OUTOFRANGE] = "���������Ƶ���ʳ�����Χ����Ĭ�ϴ���1000kbps��";
  error_code_detail_[RESP_AVROOMCONTROLLER_FRAMERATE_OUTOFRANGE] = "����֡�ʳ�����Χ����Ĭ�ϴ���20fps��";
  error_code_detail_[RESP_AVROOMCONTROLLER_LAYOUTMODE_OUTOFRANGE] = "����ģʽֵ������Χ";
  error_code_detail_[RESP_AVROOMCONTROLLER_BGCOLOR_ILLEGAL] = "	��������ɫ���ò��Ϸ�";
  error_code_detail_[RESP_AVROOMCONTROLLER_BORDER_ERR] = "�߿����ò�������";
  error_code_detail_[RESP_AVROOMCONTROLLER_STREAMLIST_ILLEGAL] = "�Զ��岼�ֲ������Ϸ�";
  error_code_detail_[RESP_AVROOM_RESOURCE_NOT_INIT] = "��������δ��ʼ��";
  error_code_detail_[RESP_AVROOM_BROADCAST_NOT_STARTED] = "����rtmp����δ��ʼ";
  error_code_detail_[RESP_AVROOM_BROADCAST_NOT_CONFIGED] = "�������仹δ����";
  error_code_detail_[RESP_AVROOM_PEERID_NOTFOUND] = "���������Ҳ����ó�Ա";
  error_code_detail_[RESP_AVROOM_FORCE_STOPPED_ALREADY] = "������ǿ��ֹͣ";
  error_code_detail_[RESP_AVROOM_HAS_NO_MEMBERS] = "��������û��Ҫ�����Ƶ��";
  error_code_detail_[RESP_EXTERNALCHANNEL_ADDED] = "channel �ɹ����뵽��������";
  error_code_detail_[RESP_EXTERNALCHANNEL_ADD_ERR] = "channel ���뵽��������ʧ��";
}

std::string vhall::BroadCastEvents::GetDetail(int errorCode) {
  std::string detail = error_code_detail_[errorCode];
  if ("" == detail) {
    if (errorCode == 0 || errorCode == 200) {
      detail = "�ɹ�";
    }
    else {
      detail = "unknown error";
    }
  }
  return detail;
}
