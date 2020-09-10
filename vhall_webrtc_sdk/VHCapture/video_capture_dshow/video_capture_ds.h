/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

/* too many modifications in this module to replace it info vhall_webrtc_sdk */
#pragma once
#include <thread>
#include <atomic>
#include <mutex>

#include "api/scoped_refptr.h"
#include "modules/video_capture/I_DshowDeviceEvent.h"
#include "./video_capture_impl.h"
#include "modules/video_capture/windows/device_info_ds.h"
#include "sink_filter_ds.h"

#define CAPTURE_FILTER_NAME L"VideoCaptureFilter"
#define SINK_FILTER_NAME L"SinkFilter"

namespace vhall {
// Forward declaraion

class VideoCaptureDS : public webrtc::videocapturemodule::VHVideoCaptureImpl {
 public:
  VideoCaptureDS();

  virtual int32_t Init(const char* deviceUniqueIdUTF8, webrtc::VideoCaptureCapability capability);

  /*************************************************************************
   *
   *   Start/Stop
   *
   *************************************************************************/
  int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) override;
  int32_t StopCapture() override;

  /**************************************************************************
   *
   *   Properties of the set device
   *
   **************************************************************************/

  bool CaptureStarted() override;
  int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) override;

  virtual bool SetDhowDeviceNotify(vhall::I_DShowDevieEvent* notify);
 protected:
  ~VideoCaptureDS() override;

  // Help functions

  int32_t SetCameraOutput(const webrtc::VideoCaptureCapability& requestedCapability);
  int32_t DisconnectGraph();
  HRESULT ConnectDVCamera();

  webrtc::videocapturemodule::DeviceInfoDS _dsInfo;

  IBaseFilter* _captureFilter;
  IGraphBuilder* _graphBuilder;
  IMediaControl* _mediaControl;
  rtc::scoped_refptr<vhall::videocapturemodule::CaptureSinkFilter> sink_filter_;
  IPin* _inputSendPin;
  IPin* _outputCapturePin;

  // Microsoft DV interface (external DV cameras)
  IBaseFilter* _dvFilter;
  IPin* _inputDvPin;
  IPin* _outputDvPin;

  /* device event monitoring */
  void ProcEventHandle();
  HANDLE            mEventHandle;
  IMediaEventEx*    mEvent;
  std::thread       mProcEventThread;
  std::atomic<bool> mThreadDone;
  vhall::I_DShowDevieEvent* mNotify = nullptr;
  std::mutex        mNotifyMtx;

  /* support mjpeg stream capture */
  IPin* m_pDecoderInput;
  IPin* m_pDecoderOutput;
  IBaseFilter* m_pJpegDecoder;
};
}  // namespace vhall
