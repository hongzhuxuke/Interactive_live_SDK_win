#include "vh_tools.h"

#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/audio_device_impl.h"

//#include "custom_capture/video_capture/desktop_capture_impl.h"
#include "modules/desktop_capture/desktop_and_cursor_composer.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "media/base/video_common.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/video_capture/video_capture.h"
//#include "media/engine/webrtcvideocapturer.h"
//#include "vhall_custom_video_capturer/custom_video_capture.h"
#include "tool/audio_device_detect.h"

#include "tool/DeviceObtain.h"
#include "DShowPlugin.h"
#include "vh_uplog.h"
#include "json/json.h"
#include "vh_stream.h"

NS_VH_BEGIN

VideoProfileList VHTools::mVideoProfiles;
VHTools::VHTools() {
  m_pDeviceObtain.reset(new(std::nothrow) CDeviceObtain());
  if (NULL == m_pDeviceObtain || !m_pDeviceObtain->Create()) {
    LOGE("create CDeviceObtain failed");
    assert(FALSE);
  }
}

VHTools::~VHTools() {

}

uint32_t VHTools::GetVideoDevices() {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
    webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    return 0;
  }
  uint32_t num_devices = info->NumberOfDevices();
  return num_devices;
}

uint32_t VHTools::GetAudioInDevices() {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_ = webrtc::AudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
  if (!audio_device_) {
    return 0;
  }
  audio_device_->Init();
  return audio_device_->RecordingDevices();
}

uint32_t VHTools::GetAudioOutDevices() {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_ = webrtc::AudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
  if (!audio_device_) {
    return 0;
  }
  audio_device_->Init();
  uint32_t num_device = audio_device_->PlayoutDevices();
  audio_device_->Terminate();
  return num_device;
}

int32_t VHTools::GetAudioInDevName(uint32_t index, char *deviceName, uint32_t NameLength) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_ = webrtc::AudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
  if (!audio_device_) {
    return 0;
  }
  audio_device_->Init();
  const uint32_t kSize = 256;
  char id[kSize] = { 0 };
  return audio_device_->RecordingDeviceName(index, deviceName, id);
}

int32_t VHTools::GetAudioInDevName(uint32_t index, char *deviceName, uint32_t NameLength, char *deviceId, uint32_t IdLength) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_ = webrtc::AudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
  if (!audio_device_) {
    return 0;
  }
  audio_device_->Init();
  return audio_device_->RecordingDeviceName(index, deviceName, deviceId);
}

const std::vector<std::shared_ptr<AudioDevProperty>>& VHTools::AudioRecordingDevicesList() {
  mRecordingList.clear();
  AudioDeviceDetect::RecordingDevicesList(mRecordingList);

  /* Get DShow audio device */
  DeviceList micList;
  if (m_pDeviceObtain) {
    m_pDeviceObtain->GetAudioDevices(micList, eCapture);
  }
  auto itor = micList.begin();
  for (; itor != micList.end(); itor++) {
    if (itor->m_sDeviceType != TYPE_COREAUDIO) {
      char name[kAdmMaxDeviceNameSize] = { 0 };
      char guid[kAdmMaxGuidSize] = { 0 };
      std::shared_ptr<AudioDevProperty> audioDevice = std::make_shared<AudioDevProperty>();
      if (audioDevice) {
        audioDevice->mDevGuid = itor->m_sDeviceName;
        audioDevice->mDevName = itor->m_sDeviceDisPlayName;
        audioDevice->m_sDeviceType = (TYPE_RECODING_SOURCE)itor->m_sDeviceType;
        mRecordingList.push_back(audioDevice);
      }
    }
  }
  return mRecordingList;
}

int32_t VHTools::GetVideoDevName(uint32_t index, char *deviceName, uint32_t NameLength) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
    webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    return -1;
  }

  const uint32_t kSize = 256;
  char id[kSize] = { 0 };
  return info->GetDeviceName(index, deviceName, NameLength, id, kSize);
}

int32_t VHTools::GetVideoDevName(uint32_t index, char *deviceName, uint32_t NameLength, char *deviceId, uint32_t IdLength) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
    webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    return -1;
  }

  return info->GetDeviceName(index, deviceName, NameLength, deviceId, IdLength);
}

const std::vector<std::shared_ptr<VideoFormat>>& VHTools::GetVideoDevCapabilities(const char* deviceUniqueIdUTF8) {
  mVideoFormats.clear();
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
    webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    return mVideoFormats;
  }
  int32_t num_caps = info->NumberOfCapabilities(deviceUniqueIdUTF8);
  for (int32_t i = 0; i < num_caps; ++i) {
    webrtc::VideoCaptureCapability cap;
    if (info->GetCapability(deviceUniqueIdUTF8, i, cap) != -1) {
      std::shared_ptr<VideoFormat> format = std::make_shared<VideoFormat>();
      format->height = cap.height;
      format->width = cap.width;
      format->maxFPS = cap.maxFPS;
      format->videoType = (int)cap.videoType;
      format->interlaced = cap.interlaced;
      mVideoFormats.push_back(format);
    }
  }
  return mVideoFormats;
}

const std::vector<std::shared_ptr<VideoDevProperty>>& VHTools::VideoInDeviceList() {
  mCameraList.clear();
  uint32_t devNum = GetVideoDevices();
  if (devNum > 0) {
    for (uint32_t i = 0; i < devNum; i++) {
      const uint32_t kSize = 256;
      char devName[kSize] = { 0 };
      char devId[kSize] = { 0 };
      if (-1 != GetVideoDevName(i, devName, kSize, devId, kSize)) {
        std::shared_ptr<VideoDevProperty> camera = std::make_shared<VideoDevProperty>();
        camera->mIndex = i;
        camera->mDevName = devName;
        camera->mDevId = devId;
        camera->mDevFormatList = GetVideoDevCapabilities(devId);
        if (camera->mDevFormatList.size() <= 0) {
          LOGE("Get video device formats fail");
          camera->state = VideoDevProperty::GetFormatInfoError;
        }
        mCameraList.push_back(camera);
      }
    }
  }
  UpLogVideoDevice(mCameraList);
  return mCameraList;
};

int32_t VHTools::GetAudioOutDevName(uint32_t index, char *deviceName, uint32_t NameLength) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_ = webrtc::AudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
  if (!audio_device_) {
    return 0;
  }
  audio_device_->Init();
  const uint32_t kSize = 256;
  char id[kSize] = { 0 };
  return audio_device_->PlayoutDeviceName(index, deviceName, id);
}

int32_t VHTools::GetAudioOutDevName(uint32_t index, char *deviceName, uint32_t NameLength, char *deviceId, uint32_t IdLength) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_ = webrtc::AudioDeviceModuleImpl::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory_.get());
  if (!audio_device_) {
    return 0;
  }
  audio_device_->Init();
  return audio_device_->PlayoutDeviceName(index, deviceName, deviceId);
}

const std::vector<std::shared_ptr<AudioDevProperty>>& VHTools::AudioPlayoutDevicesList() {
  mPlayoutList.clear();
  AudioDeviceDetect::PlayoutDevicesList(mPlayoutList);
  return mPlayoutList;
}

std::vector<DesktopCaptureSource> VHTools::GetDesktops(int type) {
	webrtc::DesktopCaptureOptions options(webrtc::DesktopCaptureOptions::CreateDefault());
	options.set_allow_directx_capturer(true);
	std::unique_ptr<webrtc::DesktopCapturer> dc;
	std::vector<DesktopCaptureSource> ret;
	if (type==3){
		dc = webrtc::DesktopCapturer::CreateScreenCapturer(options);
	}else if(type==5){
		dc = webrtc::DesktopCapturer::CreateWindowCapturer(options);
	}else{
		return ret;
	}
	webrtc::DesktopCapturer::SourceList screens;
	if (dc->GetSourceList(&screens)) {
		for (auto &screen : screens) {
			DesktopCaptureSource source;
			source.id = screen.id;
			source.title = screen.title;
			ret.push_back(source);
		}
	}
	return ret;
}

bool VHTools::IsSupported(std::shared_ptr<VideoDevProperty>& dev, VideoProfileIndex index) {
  std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(index);
  if (!profile || !profile.get()) {
    return false;
  }
  for (size_t i = 0; i < dev->mDevFormatList.size(); i++) {
    std::shared_ptr<VideoFormat> format = dev->mDevFormatList[i];
    if (format->height == profile->mMaxHeight && format->width == profile->mMaxWidth) {
      return true;
    }
  }
  return false;
}

void VHTools::UpLogVideoDevice(const std::vector<std::shared_ptr<VideoDevProperty>>& cameraList) {
  int devNum = cameraList.size();
  if (cameraList.empty()) {
    LOGE("cannot find video device");
    return;
  }
  Json::Value dictdata(Json::ValueType::arrayValue);
  for (int i = 0; i < devNum; i++) {
    Json::Value dictDev(Json::ValueType::objectValue);
    std::shared_ptr<VideoDevProperty> devProp = cameraList.at(i);
    Json::Value dictAttr(Json::ValueType::arrayValue);
    std::vector<std::shared_ptr<VideoFormat>> devFormatList = devProp->mDevFormatList;
    for (int fmtIndex = 0; fmtIndex < devFormatList.size(); fmtIndex++) {
      Json::Value dictFmt(Json::ValueType::objectValue);
      std::shared_ptr<VideoFormat> fmt = devFormatList.at(fmtIndex);
      dictFmt["width"] = Json::Value(fmt->width);
      dictFmt["height"] = Json::Value(fmt->height);
      dictFmt["maxFPS"] = Json::Value(fmt->maxFPS);
      dictFmt["videoType"] = Json::Value(fmt->videoType);
      dictAttr[fmtIndex] = dictFmt;
    }
    dictDev["deviceName"] = Json::Value(devProp->mDevName);
    dictDev["deviceId"] = Json::Value(devProp->mDevId);
    dictDev["resolutions"] = dictAttr;
    dictdata[i] = dictDev;
  }
  Json::FastWriter root;
  std::string msg = root.write(dictdata);
  std::shared_ptr<VHLogReport> reportLog = VHStream::GetReport();
  if (reportLog) {
    reportLog->upLogVideoDevice(ulCameraList, "", msg, std::string("cameraList: ") + msg);
  }
}

std::unordered_map<VideoProfileIndex, bool> VHTools::GetSupportIndex(const std::vector<std::shared_ptr<VideoFormat>> vecVideoPro) {
  std::unordered_map<VideoProfileIndex, bool> mapVideoPros;
  for (int vProIndex = 0; vProIndex < vecVideoPro.size(); vProIndex++) {
    std::shared_ptr<VideoFormat> videoPro = vecVideoPro[vProIndex];
    std::shared_ptr<VideoProfile> pro = mVideoProfiles.GetProfile(static_cast<VideoProfileIndex>(RTC_VIDEO_PROFILE_1080P_4x3_L));
    int diffArea = abs(videoPro->height * videoPro->width - pro->mMaxWidth * pro->mMaxHeight);

    int bestIndex = RTC_VIDEO_PROFILE_1080P_4x3_L;
    for (int index = RTC_VIDEO_PROFILE_1080P_4x3_L; index < RTC_VIDEO_PROFILE_90P_16x9_H; index++) {
      std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(static_cast<VideoProfileIndex>(index));
      if (!profile) {
        continue;
      }
      if (profile->mMaxWidth * profile->mMaxHeight < videoPro->height * videoPro->width && videoPro->maxFPS >= profile->mMaxFrameRate) {
        mapVideoPros[static_cast<VideoProfileIndex>(index)] = true;
      }
      /*if (abs(profile->mWidth*profile->mHeight - videoPro->height * videoPro->width) < diffArea) {
        diffArea = abs(profile->mWidth*profile->mHeight - videoPro->height * videoPro->width);
        bestIndex = index;
      }*/
    } // end VIDEO_PROFILE_1080P_DESKTOP
    //mapVideoPros[static_cast<VideoProfileIndex>(bestIndex)] = true;

  } // end vecVideoPro.size()

  return mapVideoPros;
}

NS_VH_END
