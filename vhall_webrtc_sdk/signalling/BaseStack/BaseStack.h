#ifndef VH_BaseStack_h
#define VH_BaseStack_h

#include <cstdio>
#include <atomic>

#include "signalling/vh_data_message.h"
#include "signalling/vh_events.h"
#include "signalling/vh_stream.h"
#include "signalling/vh_room.h"
#include "common/vhall_define.h"
#include "common/vhall_timer.h"
#include "common/message_type.h"
#include "modules/video_capture/I_DshowDeviceEvent.h"
#include "CreatePeerConnectionObserver.h"
#include "CreateOfferObserver.h"
#include "SetLocalDescriptionObserver.h"
#include "SetAnswerObserver.h"
#include "VideoRenderer.h"
#include "GetStatsObserver.h"
#include "Snapshot.h"
#include "audioExport.h"

#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "media/base/video_common.h"
#include "modules/audio_device/audio_device_impl.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/strings/json.h"
#include "pc/peer_connection_factory.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"

#include "rtc_base/ssl_adapter.h"
#include "rtc_base/win32_socket_init.h"
#include "rtc_base/win32_socket_server.h"
#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"
#include "rtc_base/arraysize.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "sdk/media_constraints.h"
#include "../tool/AudioSendFrame.h"
#include "3rdparty/media_reader/MediaReader.h"
#include "../../VHCapture/vhall_video_capture/vhall_video_capturer.h"

/* all stream use common thread in baseStack, only file stream in uses independent peerconnectionfactory*/
namespace webrtc {
  class CapturerTrackSource;
}
NS_VH_BEGIN
class VHTools;
class SnapShotCallBack;
class DesktopCaptureImpl;
class AudioDataFromCapture;
class BaseAudioDeviceModule;
class AudioDevProperty;
class VHBeautifyRender;

class BaseMessage : public rtc::MessageData {
public:
  uint32_t AudioDevID = 0;
  std::wstring LoopBackDeivce = L"";
  AudioSendFrame* mListener = nullptr;
  uint32_t        mVolume = 0;

  // render
  HWND wnd = nullptr;
  std::weak_ptr<VideoRenderReceiveInterface> receiver;

  /* snapShot */
  SnapShotCallBack * mSnapShotListener = nullptr;
  int                mSnapWidth = 0;
  int                mSnapHeight = 0;
  int                mSnapQuality = 0;

  videoOptions       mOpt;

  /* share region rect */
  webrtc::DesktopRect        mRect;
  std::shared_ptr<MediaCaptureInfo> mCaptureInfo = nullptr;

  LiveVideoFilterParam mFilterParam;
};

class BaseStack:
  public std::enable_shared_from_this<BaseStack>,
  public rtc::MessageHandler,
  public I_DShowDevieEvent
{
public:
  BaseStack(std::shared_ptr<VHStream> p);
  void Init();
  ~BaseStack();
  void Destroy();
  void ReceiveAnswer(const SignalingMessageVhallRespMsg &ErizoMsg);

  //set resolution and fps
  void ResetCapability(videoOptions& opt);
  //desktop capture
  //webrtc::DesktopCapturer::SourceList GetSourceList();

  // Selects a source to be captured. Returns false in case of a failure (e.g.
  // if there is no source with the specified type and id.)
  bool SelectSource(int64_t id);
  bool UpdateRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

  //voice change
  static bool SetChangeVoiceType(int newType);
  static int GetChangeType();
  static bool ChangePitchValue(double value);

  // beautify level
  void SetFilterParam(LiveVideoFilterParam filterParam);
  // about file player
  bool FilePlay(const char *file);
  void FilePause();
  void FileResume();
  void FileStop();
  void FileSeek(const unsigned long long& seekTimeInMs);
  const long long FileGetMaxDuration();
  const long long FileGetCurrentDuration();
  void FileSetVolumn(unsigned int volumn);
  int FileGetVolumn();
  int GetPlayerState();
  // void SetEventCallBack(IMediaEventCallBack cb, void *param);
  void SetEventCallBack(FileEventCallBack cb, void *param);

  /* snapShot */
  void RequestSnapShot(SnapShotCallBack* callBack, int width = 0, int height = 0, int quality = 80);

  const char* GetStreamID();
  virtual void OnMessage(rtc::Message* msg);
  /* uplog */
  void SetReport(std::shared_ptr<VHLogReport> report);
  std::shared_ptr<VHLogReport> GetReport();
  bool PostTask(E_BASE_TASK task, BaseMessage* data, int delay = 0);
  virtual void OnAudioNotify(long eventCode, long param1, long param2) {};
  virtual void OnVideoNotify(long eventCode, long param1, long param2);
  virtual void OnVideoCaptureFail();
  static bool SetAudioInDevice(uint32_t index, std::wstring LoopBackDeivce = L"");
  static bool SetAudioOutDevice(uint32_t index);
  /* uplog Device */
  static void UpLogSpeakerDevice(std::vector<std::shared_ptr<AudioDevProperty>>& vecAuidoDevice);
  static void UpLogMircophoneDevie(std::vector<std::shared_ptr<AudioDevProperty>>& vecAuidoDevice);
  static void UpLogSetMicroPhoneDevcie(std::shared_ptr<AudioDevProperty> dev, std::wstring LoopBackDeivce, float volume, std::string result, std::string message);
  static void UpLogSetSpeakerDevice(std::shared_ptr<AudioDevProperty> dev, std::string result, std::string message);
  static bool SetMicrophoneVolume(const uint32_t volume);
  static bool SetLoopBackVolume(const uint32_t volume);
  static bool SetSpeakerVolume(uint32_t volume);
private:
  void OnSetFilterParam(LiveVideoFilterParam filterParam);
  // Get Video Capture
  std::unique_ptr<cricket::VideoCapturer> OpenVideoCaptureDevice(std::string devId);
  // 3:DesktopCapture,5:WindowCapturer defualt:3
  std::unique_ptr<cricket::VideoCapturer> CreateDesktopCapture(int type = 3);
  std::unique_ptr<cricket::VideoCapturer> CreateFileCapture();
  std::unique_ptr<cricket::VideoCapturer> CreatePictureCapture(std::shared_ptr<I420Data>& picture, const webrtc::VideoCaptureCapability & capability);
  // call webrtc api
  bool CreatePeerConnectionFactory();
  void ClosePeerConnectionFactory();
  bool CreatePeerConnection();
  void ClosePeerConnection();
  void GetUserMedia(std::shared_ptr<MediaCaptureInfo> mediaInfo = nullptr);
  void CreateOfferToErizo();
  void processAnswer();
  void ReceiveRemoteStream();
  void GetTrackStatsTimer();
  void GetTrackStats();
  void RenderView();
  void SetAudioListener(AudioSendFrame* listener);
  void stopStream();
  void closeStream();
  void MuteStream();
  void OnRequestSnapShot(SnapShotCallBack* callBack, int width = 0, int height = 0, int quality = 80);
  bool OnUpdateRect(webrtc::DesktopRect &_rect);
  void OnResetCapability(videoOptions& opt);
  void SetBitRate();
  std::string ListStreamParams();// for local stream uplog
public:
  // PeerConnectionObserver
  CreatePeerConnectionObserver *mCreatePeerConnectionObserver;
  // SetSessionDescriptionObserver
  rtc::scoped_refptr<SetLocalDescriptionObserver> mSetLocalDescriptionObserver;
  // SetSessionDescriptionObserver
  rtc::scoped_refptr<SetAnswerObserver> mSetAnswerObserver;
  // SetSessionDescriptionObserver
  rtc::scoped_refptr<CreateOfferObserver> mCreateOfferObserver;
  // StatsObserver
  rtc::scoped_refptr<GetStatsObserver> mGetStatsObserver;

  std::weak_ptr<VHStream> mVhStream;
  bool remoteDescriptionSet;
  std::list<SignalingMessageMsg> listIceCandidate;
  //rtc::scoped_refptr<webrtc::AudioDeviceModule> mAudioDevice = nullptr;
  rtc::scoped_refptr<webrtc::AudioDeviceModule> mAudioFileDevice = nullptr;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_ = nullptr;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_file = nullptr;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  std::unique_ptr<VideoRenderer> stream_renderer_;
  std::unique_ptr<Snapshot> snapshot_;
  std::atomic_bool is_rendering_;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track;
  rtc::scoped_refptr<webrtc::CapturerTrackSource> video_device;
  std::string video_label_ = "video_label";

  std::shared_ptr<SignalingMessageVhallRespMsg> ptrAnswer;

  HWND wnd = nullptr;
  //VideoRenderReceiveInterface* receiver = nullptr;
  std::weak_ptr<VideoRenderReceiveInterface> receiver;
  // Devices
  std::string VideoDevID;
  uint32_t DesktopID;

  bool audioMuted = false;
  bool videoMuted = false;
  uint32_t mAudioInVol;
  uint32_t mAudioOutVol;

  std::atomic<uint64_t> sampling;
  std::string mStreamId;
  std::shared_ptr<rtc::Thread> mLocalThread;
  std::shared_ptr<BaseAudioDeviceModule> mBaseAdm = nullptr;
  std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;

  bool uplogIceCompleted = false;
private:
  std::shared_ptr<VHBeautifyRender> mBeautifyFilter;
  LiveVideoFilterParam              mFilterParam;
  std::mutex                   mLogMtx;
  std::atomic<bool>            mStopFlag;
  std::shared_ptr<VHLogReport> mReportLog = nullptr;
  std::unique_ptr<SendVideoSsrc> mPublishVideo;
  std::shared_ptr<VHTools>     mDevTool;
  std::mutex                   mRenderingMutex;
  std::shared_ptr<AudioExport> mAudioExport = nullptr;
  AudioSendFrame*              mListener = nullptr;
  double                       mPitchValue;
  VideoProfileList             mVideoProfiles;
  std::atomic_bool             mEnableSnap;

  //std::shared_ptr<DesktopCaptureImpl> mCaptureModule;
  //std::shared_ptr<FileVideoCaptureImpl> fvc = nullptr;
  //std::shared_ptr<PictureCaptureImpl> mPictureModule = nullptr;
};

NS_VH_END
#endif /* ec_stream_h */