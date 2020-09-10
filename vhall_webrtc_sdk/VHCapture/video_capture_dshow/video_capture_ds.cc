/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video_capture_ds.h"

#include <dvdmedia.h>  // VIDEOINFOHEADER2

#include "modules/video_capture/video_capture_config.h"
#include "modules/video_capture/windows/help_functions_ds.h"
#include "modules/video_capture/windows/sink_filter_ds.h"
#include "rtc_base/logging.h"

namespace vhall {
VideoCaptureDS::VideoCaptureDS()
    : _captureFilter(NULL),
      _graphBuilder(NULL),
      _mediaControl(NULL),
      _inputSendPin(NULL),
      _outputCapturePin(NULL),
      _dvFilter(NULL),
      _inputDvPin(NULL),
      _outputDvPin(NULL),
      m_pJpegDecoder(nullptr),
      mEvent(nullptr),
      mThreadDone(false), 
      m_pDecoderInput(nullptr),
      m_pDecoderOutput(nullptr),
      mNotify(NULL) {}

VideoCaptureDS::~VideoCaptureDS() {
  mThreadDone = true;
  if (mEventHandle) {
    SetEvent(mEventHandle);
  }
  if (mProcEventThread.joinable()) {
    mProcEventThread.join();
  }
  if (_mediaControl) {
    _mediaControl->Stop();
  }
  if (_graphBuilder) {
    if (sink_filter_)
      _graphBuilder->RemoveFilter(sink_filter_);
    if (_captureFilter)
      _graphBuilder->RemoveFilter(_captureFilter);
    if (m_pJpegDecoder)
      _graphBuilder->RemoveFilter(m_pJpegDecoder);
    if (_dvFilter)
      _graphBuilder->RemoveFilter(_dvFilter);
  }
  RELEASE_AND_CLEAR(_inputSendPin);
  RELEASE_AND_CLEAR(_outputCapturePin);

  RELEASE_AND_CLEAR(m_pDecoderInput);
  RELEASE_AND_CLEAR(m_pDecoderOutput);

  RELEASE_AND_CLEAR(_captureFilter);  // release the capture device
  RELEASE_AND_CLEAR(m_pJpegDecoder);
  RELEASE_AND_CLEAR(_dvFilter);

  RELEASE_AND_CLEAR(_mediaControl);

  RELEASE_AND_CLEAR(_inputDvPin);
  RELEASE_AND_CLEAR(_outputDvPin);

  RELEASE_AND_CLEAR(_graphBuilder);

  mEventHandle = nullptr;
  RELEASE_AND_CLEAR(mEvent);
}

int32_t VideoCaptureDS::Init(const char* deviceUniqueIdUTF8, webrtc::VideoCaptureCapability capability) {
  _requestedCapability = capability;
  const int32_t nameLength = (int32_t)strlen((char*)deviceUniqueIdUTF8);
  if (nameLength > webrtc::kVideoCaptureUniqueNameLength)
    return -1;

  // Store the device name
  _deviceUniqueId = new (std::nothrow) char[nameLength + 1];
  memcpy(_deviceUniqueId, deviceUniqueIdUTF8, nameLength + 1);

  if (_dsInfo.Init() != 0)
    return -1;

  _captureFilter = _dsInfo.GetDeviceFilter(deviceUniqueIdUTF8);
  if (!_captureFilter) {
    RTC_LOG(LS_INFO) << "Failed to create capture filter.";
    return -1;
  }

  HRESULT hr = CoCreateInstance(CLSID_MjpegDec, NULL, CLSCTX_INPROC, IID_IBaseFilter, reinterpret_cast<void**>(&m_pJpegDecoder));
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to create jpeg decoder filter!";
    return -1;
  }

  // Get the interface for DirectShow's GraphBuilder
  hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                                IID_IGraphBuilder, (void**)&_graphBuilder);
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to create graph builder.";
    return -1;
  }

  /* dshow event listen */
  hr = _graphBuilder->QueryInterface(IID_IMediaEventEx, (void**)&mEvent);
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to create media IMediaEventEx.";
    return -1;
  }
  hr = mEvent->GetEventHandle((OAEVENT*)&mEventHandle);
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to Get EventHandle.";
    return -1;
  }
  /* Dhsow graph monitor thread */
  mProcEventThread = std::thread(std::bind(&VideoCaptureDS::ProcEventHandle, this));

  hr = _graphBuilder->QueryInterface(IID_IMediaControl, (void**)&_mediaControl);
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to create media control builder.";
    return -1;
  }
  hr = _graphBuilder->AddFilter(_captureFilter, CAPTURE_FILTER_NAME);
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to add the capture device to the graph.";
    return -1;
  }

  _outputCapturePin = webrtc::videocapturemodule::GetOutputPin(_captureFilter, PIN_CATEGORY_CAPTURE);
  if (!_outputCapturePin) {
    RTC_LOG(LS_INFO) << "Failed to get output capture pin";
    return -1;
  }

  // Create the sink filte used for receiving Captured frames.
  sink_filter_ = new webrtc::videocapturemodule::ComRefCount<vhall::videocapturemodule::CaptureSinkFilter>(this);

  hr = _graphBuilder->AddFilter(m_pJpegDecoder, L"Mjpeg Decoder");
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to add the Mjpeg decode filter to the graph.";
    return -1;
  }
  /* get mjpegDecoder inputPin */
  m_pDecoderInput = webrtc::videocapturemodule::GetInputPin(m_pJpegDecoder);

  /* get mjpegDecoder outPutPin */
  {
    HRESULT hr;
    IEnumPins* pEnum = NULL;
    m_pJpegDecoder->EnumPins(&pEnum);
    if (pEnum == NULL) {
      RTC_LOG(LS_INFO) << "Failed to get Mjpeg decode filter EnumPins";
      return -1;
    }
    pEnum->Reset();
    pEnum->Skip(1);
    m_pDecoderOutput = NULL;
    hr = pEnum->Next(1, &m_pDecoderOutput, NULL);
    pEnum->Release();
    if (FAILED(hr)) {
      RTC_LOG(LS_INFO) << "Failed to get Mjpeg decode filter outputPin";
      return -1;
    }
  }

  hr = _graphBuilder->AddFilter(sink_filter_, SINK_FILTER_NAME);
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to add the send filter to the graph.";
    return -1;
  }

  _inputSendPin = webrtc::videocapturemodule::GetInputPin(sink_filter_);
  if (!_inputSendPin) {
    RTC_LOG(LS_INFO) << "Failed to get input send pin";
    return -1;
  }

  // Temporary connect here.
  // This is done so that no one else can use the capture device.
  if (SetCameraOutput(_requestedCapability) != 0) {
    return -1;
  }
  hr = _mediaControl->Pause();
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO)
        << "Failed to Pause the Capture device. Is it already occupied? " << hr;
    return -1;
  }
  RTC_LOG(LS_INFO) << "Capture device '" << deviceUniqueIdUTF8
                   << "' initialized.";
  return 0;
}

int32_t VideoCaptureDS::StartCapture(const webrtc::VideoCaptureCapability& capability) {
  rtc::CritScope cs(&_apiCs);

  if (capability != _requestedCapability) {
    DisconnectGraph();

    if (SetCameraOutput(capability) != 0) {
      return -1;
    }
  }
  HRESULT hr = _mediaControl->Run();
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to start the Capture device.";
    return -1;
  }
  return 0;
}

int32_t VideoCaptureDS::StopCapture() {
  rtc::CritScope cs(&_apiCs);

  HRESULT hr = _mediaControl->Pause();
  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to stop the capture graph. " << hr;
    return -1;
  }
  return 0;
}

bool VideoCaptureDS::CaptureStarted() {
  OAFilterState state = 0;
  HRESULT hr = _mediaControl->GetState(1000, &state);
  if (hr != S_OK && hr != VFW_S_CANT_CUE) {
    RTC_LOG(LS_INFO) << "Failed to get the CaptureStarted status";
  }
  RTC_LOG(LS_INFO) << "CaptureStarted " << state;
  return state == State_Running;
}

int32_t VideoCaptureDS::CaptureSettings(webrtc::VideoCaptureCapability& settings) {
  settings = _requestedCapability;
  return 0;
}

int32_t VideoCaptureDS::SetCameraOutput(
    const webrtc::VideoCaptureCapability& requestedCapability) {
  // Get the best matching capability
  webrtc::VideoCaptureCapability capability;
  int32_t capabilityIndex;

  // Store the new requested size
  _requestedCapability = requestedCapability;
  // Match the requested capability with the supported.
  if ((capabilityIndex = _dsInfo.GetBestMatchedCapability(
           _deviceUniqueId, _requestedCapability, capability)) < 0) {
    return -1;
  }
  // Reduce the frame rate if possible.
  if (capability.maxFPS > requestedCapability.maxFPS) {
    capability.maxFPS = requestedCapability.maxFPS;
  } else if (capability.maxFPS <= 0) {
    capability.maxFPS = 30;
  }

  // Convert it to the windows capability index since they are not nexessary
  // the same
  webrtc::videocapturemodule::VideoCaptureCapabilityWindows windowsCapability;
  if (_dsInfo.GetWindowsCapability(capabilityIndex, windowsCapability) != 0) {
    return -1;
  }

  IAMStreamConfig* streamConfig = NULL;
  AM_MEDIA_TYPE* pmt = NULL;
  VIDEO_STREAM_CONFIG_CAPS caps;

  HRESULT hr = _outputCapturePin->QueryInterface(IID_IAMStreamConfig,
                                                 (void**)&streamConfig);
  if (hr) {
    RTC_LOG(LS_INFO) << "Can't get the Capture format settings.";
    return -1;
  }

  // Get the windows capability from the capture device
  bool isDVCamera = false;
  hr = streamConfig->GetStreamCaps(windowsCapability.directShowCapabilityIndex,
                                   &pmt, reinterpret_cast<BYTE*>(&caps));
  RTC_LOG(LS_INFO) << "GetStreamCaps hr: " << hr;
  if (hr == S_OK) {
    if (pmt->formattype == FORMAT_VideoInfo2) {
      VIDEOINFOHEADER2* h = reinterpret_cast<VIDEOINFOHEADER2*>(pmt->pbFormat);
      if (capability.maxFPS > 0 && windowsCapability.supportFrameRateControl) {
        h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.maxFPS);
      }
    } else {
      VIDEOINFOHEADER* h = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
      if (capability.maxFPS > 0 && windowsCapability.supportFrameRateControl) {
        h->AvgTimePerFrame = REFERENCE_TIME(10000000.0 / capability.maxFPS);
      }
    }

    // Set the sink filter to request this capability
    sink_filter_->SetRequestedCapability(capability);
    // Order the capture device to use this capability
    hr += streamConfig->SetFormat(pmt);
    RTC_LOG(LS_INFO) << "SetFormat " << hr;

    // Check if this is a DV camera and we need to add MS DV Filter
    if (pmt->subtype == MEDIASUBTYPE_dvsl ||
        pmt->subtype == MEDIASUBTYPE_dvsd || pmt->subtype == MEDIASUBTYPE_dvhd)
      isDVCamera = true;  // This is a DV camera. Use MS DV filter
  }
  RELEASE_AND_CLEAR(streamConfig);

  if (FAILED(hr)) {
    RTC_LOG(LS_INFO) << "Failed to set capture device output format";
    return -1;
  }

  if (isDVCamera) {
    hr = ConnectDVCamera();
  } else {
    if (capability.videoType == webrtc::VideoType::kMJPEG) {
      /* set mjpeg decode inputPin/outputPin */
      hr = _graphBuilder->ConnectDirect(_outputCapturePin, m_pDecoderInput, NULL);
      RTC_LOG(LS_INFO) << "graph ConnectDirect pin" << hr;
      hr = _graphBuilder->ConnectDirect(m_pDecoderOutput, _inputSendPin, NULL);
      RTC_LOG(LS_INFO) << "graph ConnectDirect pin" << hr;
    }
    else {
      hr = _graphBuilder->ConnectDirect(_outputCapturePin, _inputSendPin, NULL);
      RTC_LOG(LS_INFO) << "graph ConnectDirect pin" << hr;
    }
  }
  if (hr != S_OK) {
    RTC_LOG(LS_INFO) << "Failed to connect the Capture graph " << hr;
    return -1;
  }
  return 0;
}

int32_t VideoCaptureDS::DisconnectGraph() {
  HRESULT hr = _mediaControl->Stop();
  if (_outputCapturePin) {
    hr += _graphBuilder->Disconnect(_outputCapturePin);
  }
  if (m_pDecoderInput) {
    hr = _graphBuilder->Disconnect(m_pDecoderInput);
  }
  if (m_pDecoderOutput) {
    hr = _graphBuilder->Disconnect(m_pDecoderOutput);
  }
  if (_inputSendPin) {
    hr += _graphBuilder->Disconnect(_inputSendPin);
  }

  // if the DV camera filter exist
  if (_dvFilter) {
    hr = _graphBuilder->Disconnect(_inputDvPin);
    hr = _graphBuilder->Disconnect(_outputDvPin);
  }
  if (hr != S_OK) {
    RTC_LOG(LS_ERROR)
        << "Failed to Stop the Capture device for reconfiguration " << hr;
    return -1;
  }
  return 0;
}

bool VideoCaptureDS::SetDhowDeviceNotify(vhall::I_DShowDevieEvent* notify) {
  std::unique_lock<std::mutex> lock(mNotifyMtx);
  mNotify = (vhall::I_DShowDevieEvent*)(notify);
  lock.unlock();
  return true;
}
void VideoCaptureDS::ProcEventHandle() {
  while (!mThreadDone) {
    long evCode = 0;
    long long param1 = 0, param2 = 0;
    if (WAIT_OBJECT_0 == WaitForSingleObject(mEventHandle, 100)) {
      while (S_OK == mEvent->GetEvent(&evCode, &param1, &param2, 0)) {
        RTC_LOG(LS_WARNING) << "Event code:" << evCode << "Params: " << param1 << ", " << param2;
        mEvent->FreeEventParams(evCode, param1, param2);
        switch (evCode) {
        case EC_COMPLETE:
        case EC_USERABORT:
        case EC_ERRORABORT:
          mThreadDone = true;
          RTC_LOG(LS_WARNING) << "ProcEventHandle loop done";
          break;
        case EC_DEVICE_LOST:
          // As a result, windows will process device lost event first.
          // We needn't process this event in DirectShow any more.
        {
          std::unique_lock<std::mutex> lock(mNotifyMtx);
          if (mNotify) {
            mNotify->OnVideoNotify(evCode, param1, param2);
          }
          lock.unlock();
        }
        break;
        default:
          break;
        }
        /* reset param */
        evCode = 0;
        param1 = 0;
        param2 = 0;
      }
    }
  }
}

HRESULT VideoCaptureDS::ConnectDVCamera() {
  HRESULT hr = S_OK;

  if (!_dvFilter) {
    hr = CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC,
                          IID_IBaseFilter, (void**)&_dvFilter);
    if (hr != S_OK) {
      RTC_LOG(LS_INFO) << "Failed to create the dv decoder: " << hr;
      return hr;
    }
    hr = _graphBuilder->AddFilter(_dvFilter, L"VideoDecoderDV");
    if (hr != S_OK) {
      RTC_LOG(LS_INFO) << "Failed to add the dv decoder to the graph: " << hr;
      return hr;
    }
    _inputDvPin = webrtc::videocapturemodule::GetInputPin(_dvFilter);
    if (_inputDvPin == NULL) {
      RTC_LOG(LS_INFO) << "Failed to get input pin from DV decoder";
      return -1;
    }
    _outputDvPin = webrtc::videocapturemodule::GetOutputPin(_dvFilter, GUID_NULL);
    if (_outputDvPin == NULL) {
      RTC_LOG(LS_INFO) << "Failed to get output pin from DV decoder";
      return -1;
    }
  }
  hr = _graphBuilder->ConnectDirect(_outputCapturePin, _inputDvPin, NULL);
  if (hr != S_OK) {
    RTC_LOG(LS_INFO) << "Failed to connect capture device to the dv devoder: "
                     << hr;
    return hr;
  }

  hr = _graphBuilder->ConnectDirect(_outputDvPin, _inputSendPin, NULL);
  if (hr != S_OK) {
    if (hr == HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES)) {
      RTC_LOG(LS_INFO) << "Failed to connect the capture device, busy";
    } else {
      RTC_LOG(LS_INFO) << "Failed to connect capture device to the send graph: "
                       << hr;
    }
  }
  return hr;
}
}  // namespace vhall
