#include "file_audio_device.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread.h"
#include "system_wrappers/include/sleep.h"

namespace webrtc {

#define MAX_AUDIO_BUFFER_S  5  //5s        

  unsigned int mAudioChannels;
  unsigned int mSamplesPerSec;
  unsigned int mBitsPerSample;
  unsigned int mRecAudioFrameSize;

  VHFileAudioDevice::VHFileAudioDevice(std::weak_ptr<IMediaOutput> &audio_output)
    : _ptrAudioBuffer(NULL),
    mRecordingBuffer(NULL),
    mRecordingBufferSizeIn10MS(0),
    mRecordingFramesIn10MS(0),
    mRecordingBufferFrameIndex(0),
    mRecordingBufferFrameCount(0),
    mIsRecording(false),
    _lastCallRecordMillis(0),
    mIsRecordingInitialized(false),
    mAudioMediaOutput(audio_output),
    mAudioChannels(0),
    mSamplesPerSec(0),
    mBitsPerSample(0),
    mRecAudioFrameSize(0) {
  }

  VHFileAudioDevice::~VHFileAudioDevice() {
    RTC_LOG(LS_INFO) << __FUNCTION__ << " destroyed";
    Terminate();
    _ptrAudioBuffer = NULL;
  }

  int32_t VHFileAudioDevice::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const {
    return -1;
  }

  AudioDeviceGeneric::InitStatus VHFileAudioDevice::Init() {
    //获取audio参数
    if (mIsRecordingInitialized == true) {
      RTC_LOG(INFO) << "file audio device initialized already.";
      InitStatus::RECORDING_ERROR;
    }

    mIsRecordingInitialized = true;
    return InitStatus::OK;
  }

  int32_t VHFileAudioDevice::Terminate() {
    mIsRecordingInitialized = false;

    return 0;
  }

  bool VHFileAudioDevice::Initialized() const {

    return true;
  }

  int32_t VHFileAudioDevice::RecordingDeviceName(uint16_t index,
    char name[kAdmMaxDeviceNameSize],
    char guid[kAdmMaxGuidSize]) {
    const char* kName = "dummy_device";
    const char* kGuid = "dummy_device_unique_id";
    if (index < 1) {
      memset(name, 0, kAdmMaxDeviceNameSize);
      memset(guid, 0, kAdmMaxGuidSize);
      memcpy(name, kName, strlen(kName));
      memcpy(guid, kGuid, strlen(guid));
      return 0;
    }
    return -1;
  }

  int32_t VHFileAudioDevice::RecordingIsAvailable(bool& available) {
    if (mRecordingBufferFrameIndex == 0) {
      available = true;
      return (int32_t)mRecordingBufferFrameIndex;
    }
    available = false;
    return -1;
  }

  int32_t VHFileAudioDevice::InitRecording() {
    rtc::CritScope lock(&mCritSect_);
    if (mIsRecording) {
      RTC_LOG(INFO) << "file audio device is Recording, don't init again..";
      return -1;
    }

    //get audio param
    std::shared_ptr<IMediaOutput> media_output(mAudioMediaOutput.lock());
    if (media_output) {
      media_output->GetAudioParam(mAudioChannels, mSamplesPerSec, mBitsPerSample);
      mRecordingFramesIn10MS = static_cast<size_t>(mSamplesPerSec / 100);
      if (_ptrAudioBuffer) {
        _ptrAudioBuffer->SetRecordingSampleRate(mSamplesPerSec);
        _ptrAudioBuffer->SetRecordingChannels(mAudioChannels);
      }
      mRecAudioFrameSize = mBitsPerSample / 8 * mAudioChannels;
      return 0;
    }
    return -1;
  }

  bool VHFileAudioDevice::RecordingIsInitialized() const {
    return mRecordingFramesIn10MS != 0;
  }

  int32_t VHFileAudioDevice::StartRecording() {
    if (mThreadRecPtr) {
      RTC_LOG(INFO) << "had rec thread .";
      return 0;
    }

    if (mIsRecording) {
      RTC_LOG(INFO) << "file audio device is Recording, don't init again..";
      return 0;
    }

    rtc::CritScope lock(&mCritSect_);
    mIsRecording = true;
    // Make sure we only create the buffer once.
    mRecordingBufferSizeIn10MS = mRecordingFramesIn10MS * mRecAudioFrameSize;
    if (!mRecordingBuffer) {
      mRecordingBuffer = new int8_t[mRecordingBufferSizeIn10MS*MAX_AUDIO_BUFFER_S * 100];
    }
    if (!mRecordingBuffer) {
      RTC_LOG(INFO) << "memory is not enough, new failed.";
      return -1;
    }
    memset(mRecordingBuffer, 0, mRecordingBufferSizeIn10MS*MAX_AUDIO_BUFFER_S * 100);

    mThreadRecPtr.reset(new rtc::PlatformThread(RecThreadFunc, this, "webrtc_audio_module_capture_thread", rtc::kRealtimePriority));

    if (mThreadRecPtr) {
      mThreadRecPtr->Start();
    }

    RTC_LOG(LS_INFO) << "Started recording from input file: ";

    return 0;
  }

  int32_t VHFileAudioDevice::StopRecording() {
    {
      rtc::CritScope lock(&mCritSect_);
      mIsRecording = false;
    }

    if (mThreadRecPtr) {
      mThreadRecPtr->Stop();
      mThreadRecPtr.reset();
    }

    rtc::CritScope lock(&mCritSect_);
    if (mRecordingBuffer) {
      delete[] mRecordingBuffer;
      mRecordingBuffer = NULL;
    }

    RTC_LOG(LS_INFO) << "Stopped recording from input file: ";
    return 0;
  }

  bool VHFileAudioDevice::Recording() const {
    return mIsRecording;
  }

  void VHFileAudioDevice::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
    rtc::CritScope lock(&mCritSect_);

    _ptrAudioBuffer = audioBuffer;

    // Inform the AudioBuffer about default settings for this implementation.
    // Set all values to zero here since the actual settings will be done by
    // InitPlayout and InitRecording later.
    _ptrAudioBuffer->SetRecordingSampleRate(0);
    _ptrAudioBuffer->SetRecordingChannels(0);

    _ptrAudioBuffer->SetPlayoutSampleRate(0);
    _ptrAudioBuffer->SetPlayoutChannels(0);
  }

  void VHFileAudioDevice::RecThreadFunc(void* pThis) {
    VHFileAudioDevice* device = static_cast<VHFileAudioDevice*>(pThis);
    while (device->RecThreadProcess()) {
    };
  }

  bool VHFileAudioDevice::RecThreadProcess() {
    if (!mIsRecording) {
      return false;
    }
    bool ret = false;
    int64_t currentTime = rtc::TimeMillis();
    unsigned int numFrames = 0;
    unsigned long long timestamp = 0;
    int8_t* newData = 0;
    std::shared_ptr<IMediaOutput> media_output(mAudioMediaOutput.lock());
    if (media_output) {
      ret = media_output->GetNextAudioBuffer((void **)&newData, &numFrames, &timestamp);
      if (ret && newData) {
        if ((mRecordingBufferFrameCount + numFrames)* mRecAudioFrameSize <= mRecordingBufferSizeIn10MS * MAX_AUDIO_BUFFER_S * 100) {
          CopyMemory(mRecordingBuffer + mRecordingBufferFrameCount * mRecAudioFrameSize, newData,
            numFrames * mRecAudioFrameSize);
          mRecordingBufferFrameCount += numFrames;
          mCritSect_.Enter();
          //note!, not calculate lastest GetNextAudioBuffer
          int recDelay = (int)((mRecordingBufferFrameCount * 10) / mRecordingFramesIn10MS - 10);

          while (mRecordingBufferFrameCount >= mRecordingFramesIn10MS) {
            if (_ptrAudioBuffer) {
              _ptrAudioBuffer->SetRecordedBuffer(mRecordingBuffer + mRecordingBufferFrameIndex * mRecAudioFrameSize,
                mRecordingFramesIn10MS);

              _ptrAudioBuffer->SetVQEData(0, recDelay);
              //_ptrAudioBuffer->SetTypingStatus(KeyPressed());    
              _ptrAudioBuffer->DeliverRecordedData();
            }
            mRecordingBufferFrameCount -= mRecordingFramesIn10MS;
            mRecordingBufferFrameIndex += mRecordingFramesIn10MS;
            recDelay -= 10;
          }

          // store remaining data which was not able to deliver as 10ms segment
          MoveMemory(mRecordingBuffer,
            mRecordingBuffer + mRecordingBufferFrameIndex * mRecAudioFrameSize,
            mRecordingBufferFrameCount * mRecAudioFrameSize);

          mRecordingBufferFrameIndex = 0;
          mCritSect_.Leave();
          free(newData);
          newData = NULL;
        }
        else {
          RTC_LOG(LERROR) << "memory enough.";
        }
      }
      else {
        webrtc::SleepMs(5);
      }
    }

    int64_t deltaTimeMillis = rtc::TimeMillis() - currentTime;
    if (deltaTimeMillis < 10) {
      SleepMs((int)(10 - deltaTimeMillis));
    }
    return true;
  }
}
