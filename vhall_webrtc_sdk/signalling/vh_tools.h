#ifndef vh_tools_h
#define vh_tools_h

#include <cstdio>
#include <cstdint>
#include "vh_data_message.h"
#include "vh_events.h"
#include "common/vhall_define.h"
#include "common/vhall_timer.h"
#include "common/video_profile.h"
#include "tool/vhall_dev_format.h"

namespace webrtc {
  class TaskQueueFactory;
}
namespace cricket {
  class VideoFormat;
}
class CDeviceObtain;
NS_VH_BEGIN

class VH_DLL VHTools
  : public EventDispatcher,
  public std::enable_shared_from_this<VHTools> {
public:
  VHTools();
  ~VHTools();

  uint32_t GetVideoDevices();
  int32_t GetVideoDevName(uint32_t index, char *deviceName, uint32_t NameLength);
  int32_t GetVideoDevName(uint32_t index, char *deviceName, uint32_t NameLength, char *deviceId, uint32_t IdLength);
 
  uint32_t GetAudioInDevices();
  int32_t GetAudioInDevName(uint32_t index, char *deviceName, uint32_t NameLength);
  int32_t GetAudioInDevName(uint32_t index, char *deviceName, uint32_t NameLength, char *deviceId, uint32_t IdLength);
  uint32_t GetAudioOutDevices();
  int32_t GetAudioOutDevName(uint32_t index, char *deviceName, uint32_t NameLength);
  int32_t GetAudioOutDevName(uint32_t index, char *deviceName, uint32_t NameLength, char *deviceId, uint32_t IdLength);

  // 3:Screen,5:window
  static std::vector<DesktopCaptureSource> GetDesktops(int streamType);

  /* get video dev caps*/
  const std::vector<std::shared_ptr<VideoFormat>>& GetVideoDevCapabilities(const char* deviceUniqueIdUTF8);
 
  /* Dev list */
  const std::vector<std::shared_ptr<VideoDevProperty>>& VideoInDeviceList();
  const std::vector<std::shared_ptr<AudioDevProperty>>& AudioRecordingDevicesList();
  const std::vector<std::shared_ptr<AudioDevProperty>>& AudioPlayoutDevicesList();
  /* check dev supported */
  bool IsSupported(std::shared_ptr<VideoDevProperty>& dev, VideoProfileIndex index);
  bool IsSupported(std::shared_ptr<AudioDevProperty>& dev);
  static std::unordered_map<VideoProfileIndex, bool> GetSupportIndex(const std::vector<std::shared_ptr<VideoFormat>> vecVideoPro);
private:
  void UpLogVideoDevice(const std::vector<std::shared_ptr<VideoDevProperty>>& cameraList);
private:
  static VideoProfileList mVideoProfiles;
  std::vector<std::shared_ptr<VideoDevProperty>> mCameraList;
  std::vector<std::shared_ptr<AudioDevProperty>> mRecordingList;
  std::vector<std::shared_ptr<AudioDevProperty>> mPlayoutList;
  std::vector<std::shared_ptr<VideoFormat>> mVideoFormats;
  std::shared_ptr<CDeviceObtain> m_pDeviceObtain;
  std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;
};

NS_VH_END

#endif /* ec_stream_h */