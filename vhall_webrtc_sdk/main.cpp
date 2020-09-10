
#include "signalling/vh_stream.h"
#include "signalling/vh_room.h"
#include "common/vhall_define.h"
#include "common/message_type.h"
#include "signalling/vh_events.h"
#include "signalling/vh_tools.h"

#include <Windows.h>

#include <string>
#include <ostream>

#include "main_wnd.h"

 int main() {

   // 测试用窗口
   MainWnd w;
   w.Create();

   // 工具类
   std::shared_ptr<vhall::VHTools> mTools;
   mTools = std::make_shared<vhall::VHTools>();

   // 创建本地流参数
  vhall::StreamMsg streamConfig;
  streamConfig.mAudio = true;
  streamConfig.mVideo = true;
  streamConfig.mData = true;
  streamConfig.mStreamType = 2;
  // streamConfig.mStreamType = 3; // desktop
  // streamConfig.mStreamType = 4; // file
  streamConfig.videoOpt.maxWidth = 960;
  streamConfig.videoOpt.minWidth = 960;
  streamConfig.videoOpt.maxHeight = 544;
  streamConfig.videoOpt.minHeight = 544;
  streamConfig.videoOpt.maxVideoBW = 500;
  streamConfig.videoOpt.maxFrameRate = 25;
  streamConfig.videoOpt.minFrameRate = 25;
  streamConfig.videoOpt.maxVideoBW = 1500;
  streamConfig.videoOpt.minVideoBW = 1400;

  // 创建本地流对象
  std::shared_ptr<vhall::VHStream> LocalStream;
  LocalStream = std::make_shared<vhall::VHStream>(streamConfig);

  // 远端流指针
  //std::shared_ptr<vhall::VHStream> RemoteStream;

  //// 获取麦克风设备列表
  //for (int index = 0; index < mTools->GetAudioInDevices(); ++index) {
  //  const uint32_t kSize = 256;
  //  char name[kSize] = { 0 };
  //  char id[kSize] = { 0 };
  //  // 获取麦克风设备名
  //  mTools->GetAudioInDevName(index, name, kSize, id, kSize);
  //  RTC_LOG(INFO) << "Audio In: " << "name: " << name << " id: "<< id;
  //}
  //// 获取扬声器设备列表
  //for (int index = 0; index < mTools->GetAudioOutDevices(); ++index) {
  //  const uint32_t kSize = 256;
  //  char name[kSize] = { 0 };
  //  char id[kSize] = { 0 };
  //  // 获取扬声器设备名
  //  mTools->GetAudioOutDevName(index, name, kSize, id, kSize);
  //  RTC_LOG(INFO) << "Audio Out: " << "name: " << name << " id: " << id;
  //}
  //// 获取摄像头设备列表
  //for (int index = 0; index < mTools->GetVideoDevices(); ++index) {
  //  const uint32_t kSize = 256;
  //  char name[kSize] = { 0 };
  //  char id[kSize] = { 0 };
  //  // 获取摄像头设备名
  //  mTools->GetVideoDevName(index, name, kSize, id, kSize);
  //  
  //  RTC_LOG(INFO) << "Video: " << "name: " << name << " id: " << id;
  //}
  //// 获取桌面列表
  //for (int index = 0; index < mTools->GetDesktops(); ++index) {
  //  // 设置桌面
  //  LocalStream->SetDesktop(index);
  //  break;
  //}

  // 选择设备，传入设备序号(index)
  //LocalStream->SetAudioInDevice(2);
  //LocalStream->SetVideoDevice(0);
  
  //
  //LocalStream->SetAudioInVolume(50);
  //LocalStream->SetAudioOutVolume(50);
  ////
  //LocalStream->MuteVideo(true);
  //LocalStream->MuteAudio(true);

  // 设置完设备参数后Init
  LocalStream->Init();

  // 获取本地摄像头成功监听回调
  LocalStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
    // 播放本地视频流
    LocalStream->play(w.wnd_);
  });

  // 获取本地摄像头失败监听回调
  LocalStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
  });

  // 获取本地流
  LocalStream->Getlocalstream();


  //// 测试用链接token
  //std::string token = "eyJ0b2tlbiI6IHsiaG9zdCI6ICIxOTIuMTY4LjEuMTM1OjgwODEiLCAicm9sZSI6ICJhZG1pbmlzdHJhdG9yIiwgInNlY3VyZSI6IGZhbHNlLCAic2VydmljZSI6ICI1YTQ4YWYyMjkwNjE1OWY1ZWEyYWY1YmYiLCAidXNlcklkIjogImNYZGwiLCAiY3JlYXRpb25EYXRlIjogMTUyMjA1MDgwOTY1NywgInJvb21JZCI6ICJwYWFzXzEzNDY3OTg1MiIsICJwMnAiOiBmYWxzZX0sICJzaWduYXR1cmUiOiAiWVRGak5tTXdNVE5tWXprd01XUTRNMkkwWVRWbE1XRTBNVE16Tnprek9UTmxOREZsTkdRNU5nPT0ifQ==";
  //
  //// 创建房间（客户端）对象
  //// http://wiki.vhall.com/index.php?id=rd:smsa3.0:design
  //struct vhall::RoomOptions specInput;
  //specInput.upLogUrl = "https://t-dc.e.vhall.com/login";
  //specInput.biz_role = 1;
  //specInput.biz_id = "";
  //specInput.biz_des01 = 2;
  //specInput.biz_des02 = 1;
  //specInput.bu = "0";
  //specInput.vid = "";
  //specInput.vfid = "";
  //specInput.app_id = "0";
  //specInput.token = token.c_str();
  //std::shared_ptr<vhall::VHRoom> vRoom;
  //vRoom = std::make_shared<vhall::VHRoom>(specInput);

  //// 连接房间失败监听回调
  //vRoom->AddEventListener(ROOM_ERROR, [&](vhall::BaseEvent &Event)->void {
  //  RTC_LOG(INFO) << "!!!!!!!!!!!!!!!!!!!!GetCurrentThreadId: " << GetCurrentThreadId();
  //});

  //// 推拉流失败监听回调
  //vRoom->AddEventListener(STREAM_FAILED, [&](vhall::BaseEvent &Event)->void {
  //  vhall::StreamEvent *FailedStream = static_cast<vhall::StreamEvent *>(&Event);
  //});

  //// 远端流退出监听回调
  //vRoom->AddEventListener(STREAM_REMOVED, [&](vhall::BaseEvent &Event)->void {
  //  vhall::StreamEvent *RemoteStream = static_cast<vhall::StreamEvent *>(&Event);
  //});

  //// 房间（退出）断开监听回调
  //vRoom->AddEventListener(ROOM_DISCONNECTED, [&](vhall::BaseEvent &Event)->void {
  //  vhall::RoomEvent *rEvent = static_cast<vhall::RoomEvent *>(&Event);
  //  rEvent->mMessage;
  //});

  //// 连接（进入）房间成功监听回调
  //vRoom->AddEventListener(ROOM_CONNECTED, [&](vhall::BaseEvent &Event)->void {
  //  vhall::RoomEvent *rEvent = static_cast<vhall::RoomEvent *>(&Event);
  //  // 列出房间内已存在的流
  //  for (auto &stream : rEvent->mStreams) {
  //    // 订阅（拉取）房间内已存在的流
  //    //RemoteStream = stream;
  //    //RemoteStream->SetAudioOutDevice(0);
  //    //vRoom->Subscribe(stream);
  //    //std::string id = stream->GetUserId();
  //    //RTC_LOG(INFO) << id;
  //  }
  //  // 推送本地流
  //  vRoom->Publish(LocalStream, [&](const std::string &msg) -> void {
  //    RTC_LOG(INFO) << "!!!!!!!!!!!!!!!!!!!! " << msg;
  //    LocalStream->FilePlay((char *)"C:\\Users\\zhouchao\\Desktop\\1.mp4");
  //  });
  //  LOGI("Event %s", Event.mType.c_str());
  //});

  //// 房间添加新远端或本地流监听回调
  //vRoom->AddEventListener(ROOM_ONADDSTEAM, [&](vhall::BaseEvent &Event)->void {
  //  RTC_LOG(INFO) << "!!!!!!!!!!!!!!!!!!!!GetCurrentThreadId: " << GetCurrentThreadId();
  //  vhall::AddStreamEvent *AddStream = static_cast<vhall::AddStreamEvent *>(&Event);
  //  // 判断非本地流
  //  RTC_LOG(INFO) << AddStream->mStream->GetID();
  //  RTC_LOG(INFO) << LocalStream->GetID();
  //  // if (strcmp(AddStream->mStream->GetID() ,LocalStream->GetID()) != 0) {
  //    // 设置远端流播放设备
  //    //RemoteStream = AddStream->mStream;
  //    //RemoteStream->SetAudioOutDevice(1);
  //    // 订阅（拉取）远端流
  //    vRoom->Subscribe(AddStream->mStream);
  //  // }
  //  LOGI("Event %s", Event.mType.c_str());
  //});

  //// 订阅（拉取）远端流成功监听回调
  //vRoom->AddEventListener(STREAM_SUBSCRIBED, [&](vhall::BaseEvent &Event)->void {
  //  RTC_LOG(INFO) << "!!!!!!!!!!!!!!!!!!!!GetCurrentThreadId: " << GetCurrentThreadId();
  //  vhall::StreamEvent *AddStream = static_cast<vhall::StreamEvent *>(&Event);
  //  // 播放远端流
  //  AddStream->mStreams->play(w.wnd_);
  //  LOGI("Event %s", Event.mType.c_str());
  //});

  //// 连接房间
  //vRoom->Connect();

  // Sleep(1000*25);

  // LocalStream->stop();
  // LocalStream->close();
  //// 断开房间链接，注意如果已调用close关闭本地流，请在确认STREAM_REMOVED事件后调用此接口。
  // vRoom->Disconnect();
   

  //Sleep(1000 * 15);
  // // 静音
  // LocalStream->MuteAudio(false);
  // // 关闭摄像头
  // LocalStream->MuteVideo(false);
  // // 设置麦克风音量 0 到 100
  // LocalStream->SetAudioInVolume(100);
  // // 设置扬声器音量 0 到o 100
  // LocalStream->SetAudioOutVolume(100);

  // Main loop.
int i = 0;
  MSG msg;
  BOOL gm;
  while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);

    // RTC_LOG(INFO) << msg.message;
  }

  LOGI("return");
//
//std::string strUrl = "http://192.168.1.127/api/edf?ab=1&be=2";
//std::wstring wstrUrl(strUrl.begin(), strUrl.end());
//
//  GetToSite(wstrUrl.c_str());
  return 0;
}
