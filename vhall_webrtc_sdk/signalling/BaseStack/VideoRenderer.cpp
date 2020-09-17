#include "BaseStack.h"
#include "VideoRenderer.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "third_party/libyuv/include/libyuv/convert.h"

NS_VH_BEGIN

/* 视频track数据获取与渲染相关 */
VideoRenderer::VideoRenderer(HWND &wnd, std::shared_ptr<BaseStack> p) {
  LOGI("Create VideoRenderer");
  ::InitializeCriticalSection(&buffer_lock_);
  ZeroMemory(&bmi_, sizeof(bmi_));
  bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi_.bmiHeader.biPlanes = 1;
  bmi_.bmiHeader.biBitCount = 32;
  bmi_.bmiHeader.biCompression = BI_RGB;
  bmi_.bmiHeader.biWidth = 1;
  bmi_.bmiHeader.biHeight = -1;
  bmi_.bmiHeader.biSizeImage = 1 * 1 * (bmi_.bmiHeader.biBitCount >> 3);
  wnd_ = wnd;
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

VideoRenderer::VideoRenderer(std::shared_ptr<VideoRenderReceiveInterface> rec, std::shared_ptr<BaseStack> p) {
  LOGI("Create VideoRenderer");
  ::InitializeCriticalSection(&buffer_lock_);
  recv = rec;
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

VideoRenderer::~VideoRenderer() {
  LOGI("~VideoRenderer");
  ::DeleteCriticalSection(&buffer_lock_);
}

void VideoRenderer::SetSize(int width, int height) {
  // LOGD("SetSize width: %d, height: %d", width, height);
  AutoLock<VideoRenderer> lock(this);

  if (width == bmi_.bmiHeader.biWidth && -height == bmi_.bmiHeader.biHeight) {
    return;
  }

  bmi_.bmiHeader.biWidth = width;
  bmi_.bmiHeader.biHeight = -height;
  bmi_.bmiHeader.biSizeImage = width * height *
    (bmi_.bmiHeader.biBitCount >> 3);
  image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

void VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {
  // LOGD("OnFrame");
  {
    AutoLock<VideoRenderer> lock(this);

    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
      video_frame.video_frame_buffer()->ToI420());
    if (video_frame.rotation() != webrtc::kVideoRotation_0) {
      buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
    }
    
    /* 发送数据 */
    std::shared_ptr<VideoRenderReceiveInterface> receiver = recv.lock();
    if (receiver) {
      // yuv
      //receiver->ReceiveVideo(buffer->DataY(), buffer->width() * buffer->height() * 3 / 2, buffer->width(), buffer->height());

    //  // rgb
    //  std::shared_ptr<unsigned char[]> rgbDataBuffer(new unsigned char[buffer->width() * buffer->height() * 4]);
    //  libyuv::I420ToARGB(
    //    buffer->DataY(), buffer->StrideY(),
    //    buffer->DataU(), buffer->StrideU(),
    //    buffer->DataV(), buffer->StrideV(),
    //    rgbDataBuffer.get(),
    //    buffer->width() * 4,
    //    buffer->width(), buffer->height());
    //  receiver->ReceiveVideo(rgbDataBuffer.get(), buffer->width() * buffer->height() * 4, buffer->width(), buffer->height());
    //}

            /* 发送数据 */
       if (receiver) {
          std::shared_ptr<unsigned char[]> rgbDataBuffer(new unsigned char[buffer->width() * buffer->height() * 3]);
          libyuv::I420ToRGB24(
             buffer->DataY(), buffer->StrideY(),
             buffer->DataU(), buffer->StrideU(),
             buffer->DataV(), buffer->StrideV(),
             rgbDataBuffer.get(),
             buffer->width() * 3,
             buffer->width(), buffer->height());
          receiver->ReceiveVideo(rgbDataBuffer.get(), buffer->width() * buffer->height() * 3, buffer->width(), buffer->height());
       }

    /* 渲染 */
    if (wnd_) {
      SetSize(buffer->width(), buffer->height());

      RTC_DCHECK(image_.get() != NULL);
      libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(),
        buffer->DataU(), buffer->StrideU(),
        buffer->DataV(), buffer->StrideV(),
        image_.get(),
        bmi_.bmiHeader.biWidth *
        bmi_.bmiHeader.biBitCount / 8,
        buffer->width(), buffer->height());

      if (IsWindow(wnd_)) {
        // InvalidateRect(wnd_, NULL, FALSE);
        OnPaint();
      }
      else {
        LOGE("wnd_ is not window!");
      }
    }
  }
}

void VideoRenderer::OnPaint() {
  // LOGD("OnPaint");
  // PAINTSTRUCT ps;
  // ::BeginPaint(wnd_, &ps);

  RECT rc;
  ::GetClientRect(wnd_, &rc);


  const BITMAPINFO& bmi = this->bmi();
  int height = abs(bmi.bmiHeader.biHeight);
  int width = bmi.bmiHeader.biWidth;

  const uint8_t* image = this->image();
  if (image != NULL) {
    auto mWindowDC = ::GetDC(wnd_);
    HDC dc_mem = ::CreateCompatibleDC(mWindowDC);
    ::SetStretchBltMode(dc_mem, HALFTONE);

    // Set the map mode so that the ratio will be maintained for us.
    HDC all_dc[] = { mWindowDC, dc_mem };
    for (int i = 0; i < arraysize(all_dc); ++i) {
      SetMapMode(all_dc[i], MM_ISOTROPIC);
      SetWindowExtEx(all_dc[i], width, height, NULL);
      SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
    }

    HBITMAP bmp_mem = ::CreateCompatibleBitmap(mWindowDC, rc.right, rc.bottom);
    HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

    POINT logical_area = { rc.right, rc.bottom };
    DPtoLP(mWindowDC, &logical_area, 1);

    HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
    RECT logical_rect = { 0, 0, logical_area.x, logical_area.y };
    ::FillRect(dc_mem, &logical_rect, brush);
    ::DeleteObject(brush);

    int x = (logical_area.x / 2) - (width / 2);
    int y = (logical_area.y / 2) - (height / 2);

    StretchDIBits(dc_mem, x, y, width, height,
      0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);

    BitBlt(mWindowDC, 0, 0, logical_area.x, logical_area.y,
      dc_mem, 0, 0, SRCCOPY);

    // Cleanup.
    ::SelectObject(dc_mem, bmp_old);
    ::DeleteObject(bmp_mem);
    ::DeleteDC(dc_mem);
    ::ReleaseDC(wnd_, mWindowDC);
  }

  // ::EndPaint(wnd_, &ps);
}

NS_VH_END