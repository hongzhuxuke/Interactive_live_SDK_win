/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_MAIN_WND_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_MAIN_WND_H_

#include <map>
#include <memory>
#include <string>

#include "api/mediastreaminterface.h"
#include "api/video/video_frame.h"
#include "media/base/mediachannel.h"
#include "media/base/videocommon.h"
#if defined(WEBRTC_WIN)
#include "rtc_base/win32.h"
#endif  // WEBRTC_WIN

#ifdef WIN32

class MainWnd {
 public:
  static const wchar_t kClassName[];

  enum WindowMessages {
    UI_THREAD_CALLBACK = WM_APP + 1,
  };

  MainWnd();
  ~MainWnd();

  bool Create();
  bool Destroy();
  bool PreTranslateMessage(MSG* msg);

  HWND wnd_;

 protected:
  enum ChildWindowID {
    EDIT_ID = 1,
    BUTTON_ID,
    LABEL1_ID,
    LABEL2_ID,
    LISTBOX_ID,
  };


  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  static bool RegisterWindowClass();

 private:
 
  
  DWORD ui_thread_id_;
  HWND edit1_;
  HWND edit2_;
  HWND label1_;
  HWND label2_;
  HWND button_;
  HWND listbox_;
  bool destroyed_;
  void* nested_msg_;
  static ATOM wnd_class_;
  std::string server_;
  std::string port_;
  bool auto_connect_;
  bool auto_call_;
};
#endif  // WIN32

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_MAIN_WND_H_
