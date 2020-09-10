//
//  ec_stream.hpp
//  VhallSignaling
//
//  Created by ilong on 2017/12/26.
//  Copyright © 2017年 ilong. All rights reserved.
//

#ifndef vh_stream_h
#define vh_stream_h

#include <cstdio>
#include <string>
#include <atomic>
#include "vh_data_message.h"
#include "vh_events.h"
#include "common/vhall_define.h"
#include "common/vhall_timer.h"
#include "tool/AudioSendFrame.h"
#include "signalling/BaseStack/SnapShotCallBack.h"

NS_VH_BEGIN
class SnapShotCallBack;
class VideoRenderReceiveInterface;
typedef enum {
	VhallStreamNone = -1,
	VhallStreamOnlyAudio = 0,
	VhallStreamOnlyVideo = 1,
	VhallStreamAudioAndVideo = 2,
	VhallStreamScreen = 3,
	VhallStreamFile = 4,
	VhallStreamWindow = 5
} VhallStreamType;

enum VoiceChangeType {
	kVoiceTypeNone = 0,        //不变声
	kVoiceTypeCute = 1,        //变声
	kVoiceTypeCustom=2         //自定义音调  
};

class LiveVideoFilterParam {
public:
  int beautyLevel = 0; /* 仅对摄像头有效 */
  bool enableEdgeEnhance = true; /* 仅对桌面视频有效 */
  bool enableDenoise = false; /* 仅对摄像头有效 */
  bool enableBrighter = false; /* 仅对摄像头有效 */
};

class I420Data {
public:
  std::shared_ptr<unsigned char[]> mData = nullptr;
  int32_t width = 0;
  int32_t height = 0;
};

class MediaCaptureInfo {
public:
  std::shared_ptr<I420Data> mI420Data = nullptr;
  int mStreamType = -1;
  std::string VideoDevID = "";

  /* share region param */
  int32_t left = 0;
  int32_t top = 0;
  int32_t right = 0;
  int32_t bottom = 0;
};

class VHRoom;
class BaseStack;
class VHLogReport;
class SendAudioSsrc;
class SendVideoSsrc;
class ReceiveAudioSsrc;
class ReceiveVideoSsrc;
class Bwe;

typedef std::function<void(const std::string& resp)> VHEventCallback;
typedef std::function<void(const vhall::RespMsg& resp)> VHEventCB;

typedef void(*FileEventCallBack)(int, void *);

class VH_DLL VHStream
  : public EventDispatcher,
    public StreamMsg,
    public std::enable_shared_from_this<VHStream> {
public:
  VHStream(const StreamMsg &config);
  ~VHStream();
  void Destroy();
  void Init();
  void Getlocalstream(std::shared_ptr<MediaCaptureInfo> mediaInfo = nullptr);
  void InitPeer();
  void DeInitPeer();
  const char* GetID();
  void SetID(std::string id);
  void SetID(uint64_t id);
  const char* GetUserId();
  void play(HWND &wnd);
  void play(std::shared_ptr<VideoRenderReceiveInterface> receiver);
  void GetAttributes();
  void SetAttributes();
  void UpdateLocalAttributes();
  bool HasAudio();
  bool HasVideo();
  bool HasData();
  bool HasScreen();
  bool HasMedia();
  bool isExternal();
  void SendData(const std::string& data, const vhall::VHEventCB &callback = [](const RespMsg & resp)->void {}, uint64_t timeOut = 0);
  void OnPeerConnectionErr(std::string & msg);
  void stop();
  void close();
  void OnMessageFromErizo(const SignalingMessageVhallRespMsg &ErizoMsg);
  void processAnswer();
  void GetStreamStats();
  void StreamSubscribed();
  bool MuteAudio(bool isMuted);
  bool MuteVideo(bool isMuted);
  bool IsAudioMuted();
  bool IsVideoMuted();

  bool IsSupportBeauty();
  bool SetFilterParam(LiveVideoFilterParam filterParam);
  void SetAudioListener(AudioSendFrame* listener);
  // volume
  static std::atomic<uint32_t> mAudioOutVol;
  static std::atomic<uint32_t> mAudioInVol;
  uint32_t GetAudioInVolume();
  uint32_t GetAudioOutVolume();
  // about media device
  static bool SetAudioInDevice(uint32_t index, std::wstring LoopBackDeivce = L"");
  static bool SetAudioOutDevice(uint32_t index);
  static bool SetLoopBackVolume(const uint32_t volume);
  static bool SetMicrophoneVolume(const uint32_t volume);
  static bool SetSpeakerVolume(uint32_t volume);
  void SetVideoDevice(std::string& devId);
  //voice change
  static bool ChangeVoiceType(VoiceChangeType newType);
  static VoiceChangeType GetChangeType();
  //[-100,100]
  static bool ChangePitchValue(double value);
  // Desktop capture
  // Selects a source to be captured. Returns false in case of a failure (e.g.
  // if there is no source with the specified type and id.)
  bool SelectSource(int64_t id);

  bool ResetCapability(videoOptions& opt);

  bool UpdateRect(int32_t left, int32_t top, int32_t right, int32_t bottom);
  // about file
  bool FilePlay(const char *file);
  void FilePause();
  void FileResume();
  void FileStop();
  void FileSeek(const unsigned long long& seekTimeInMs);
  const long long FileGetMaxDuration();
  const long long FileGetCurrentDuration();
  bool SetFileVolume(uint32_t volume);
  uint32_t GetFileVolume();
  int GetPlayerState();
  void SetEventCallBack(FileEventCallBack cb, void *param);
  // states
  bool GetAudioPublishStates(struct SendAudioSsrc *pSsrc);
  bool GetVideoPublishStates(struct SendVideoSsrc *pSsrc);
  bool GetAudioSubscribeStates(struct ReceiveAudioSsrc *pSsrc);
  bool GetVideoSubscribeStates(struct ReceiveVideoSsrc *pSsrc);
  bool GetOverallBwe(struct Bwe *pBwe);

  static bool OpenUpStats(std::string url);
  static bool CloseUpStats();

  void RequestSnapShot(SnapShotCallBack* callBack, int width = 0, int height = 0, int quality = 80);
  void SetStreamState(VhallStreamState state);
  VhallStreamState GetStreamState();
  uint64_t GetSteadyTimeStamp();
  void SetRoom(std::shared_ptr<VHRoom> room);
  std::shared_ptr<VHRoom> GetRoom();
  static void SetReport(std::shared_ptr<VHLogReport> report);
  static std::shared_ptr<VHLogReport> GetReport();
private:
  void GetFrame();
  void ControlHandler();
  int SetBestCaptureFormat();
  friend class BaseStack;
public:
  VhallStreamState mState;
  std::string      mStreamId;
  uint64_t         upLogTime = 0;
  bool             showing;
  int              mDual = 1;

  std::mutex       mRoomMtx;
  std::mutex       mBaseMtx;
  std::shared_ptr<VHRoom>    mRoom;
  std::shared_ptr<BaseStack> mBaseStack;
private:
  VideoProfileList mVideoProfiles;
  int         attempted;
  uint32_t    mDesktopId;
  std::string VideoDevID;
  std::mutex  mDevMtx;

  std::mutex  mStateMtx;
  static std::mutex  mReportLogMtx;
  static std::shared_ptr<VHLogReport> reportLog;

  std::unique_ptr<SendAudioSsrc> mPublishAudio;
  std::unique_ptr<SendVideoSsrc> mPublishVideo;
  std::unique_ptr<ReceiveAudioSsrc> mSubscribeAudio;
  std::unique_ptr<ReceiveVideoSsrc> mSubscribeVideo;
  std::unique_ptr<Bwe> mOverallBwe;
  std::mutex ssrcMtx;
  friend class GetStatsObserver;
};

NS_VH_END

#endif /* ec_stream_h */
