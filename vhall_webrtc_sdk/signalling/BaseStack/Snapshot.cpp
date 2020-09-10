#include "Snapshot.h"
#include "BaseStack.h"
#include "third_party/libyuv/include/libyuv/scale.h"
#include "../vh_uplog.h"
#include "common/arithmetic.h"
#include "../tool/JpegEncode.h"
#include "SnapShotCallBack.h"

namespace vhall {
  vhall::Snapshot::Snapshot(std::shared_ptr<BaseStack> p) {
    mPtrBaseStack = p;
    mVhStream = p->mVhStream;
    mDstWidth = 160;
    mDstHeight = 120;
    mQuality = 50;
    mInterval = 15;
    mLastFrameTs = 0;
    mEnable = false;
    mRequestCount = 0;
    mRequestHeight = 0;
    mRequestWidth = 0;
    mRequestQuality = 80;
  }

  vhall::Snapshot::~Snapshot() {
    
  }

  void vhall::Snapshot::OnFrame(const webrtc::VideoFrame & frame) {
    /* usr request */
    RequestFrame(frame);

    if (!mEnable) {
      return;
    }
    std::shared_ptr<BaseStack> ptrBaseStack = mPtrBaseStack.lock();
    if (!ptrBaseStack) {
      LOGE("BaseStack is reset!");
      return;
    }
    std::shared_ptr<VHStream> vhStream = mVhStream.lock();
    if (!vhStream) {
      LOGE("VHStream is reset!");
      return;
    }

    if (vhStream->mLocal == false) {
      return;
    }

    std::unique_lock<std::mutex> lock(mMtx);

    /* encode every 15s */
    if (mLastFrameTs && (frame.timestamp_us() - mLastFrameTs) < mInterval * 1000000) {
      return;
    }
    
    mLastFrameTs = frame.timestamp_us();
    rtc::scoped_refptr<webrtc::I420BufferInterface> src_buffer(frame.video_frame_buffer()->ToI420());
    if (frame.rotation() != webrtc::kVideoRotation_0) {
      src_buffer = webrtc::I420Buffer::Rotate(*src_buffer, frame.rotation());
    }
    rtc::scoped_refptr<webrtc::I420Buffer> dst_buffer = webrtc::I420Buffer::Create(mDstWidth, mDstHeight);
    if (src_buffer && dst_buffer) {
      int size = mDstWidth * mDstHeight;
      std::shared_ptr<uint8_t[]> temp_buf(new uint8_t[mDstWidth * mDstHeight * 3 / 2]);
      if (temp_buf) {
        libyuv::I420Scale(
          src_buffer->DataY(), src_buffer->StrideY(),
          src_buffer->DataU(), src_buffer->StrideU(),
          src_buffer->DataV(), src_buffer->StrideV(),
          src_buffer->width(), src_buffer->height(),
          temp_buf.get(), mDstWidth,
          temp_buf.get() + size, mDstWidth / 2,
          temp_buf.get() + size + size / 4, mDstWidth / 2,
          mDstWidth, mDstHeight,
          libyuv::FilterMode::kFilterNone);
        LOGD("start Jpeg encode");
        JpegEncode codec;
        std::vector<uint8_t> mJpegBuffer = codec.Encode(temp_buf, mDstWidth, mDstHeight, mQuality);
        std::unique_ptr<uint8_t[]> JpegData(new uint8_t[mJpegBuffer.size()]);
        if (mJpegBuffer.size() <= 0) {
          return;
        }
        memcpy(JpegData.get(), &mJpegBuffer[0], mJpegBuffer.size());
        std::unique_ptr<uint8_t[]> up_load(new uint8_t[mJpegBuffer.size() * 2]);
        LOGD("Jpeg encode done");
        if (up_load && JpegData) {
          char* src = (char*)JpegData.get();
          char* dst = (char*)up_load.get();
          encode_base64(dst, src, mJpegBuffer.size());
          /* uplog */
          std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
          if (reportLog) {
            LOGD("uplog Jpeg snapshot");
            reportLog->upLog(uploadScreenShot, ptrBaseStack->GetStreamID(), (char*)up_load.get());
            LOGD("uplog Jpeg snapshot done");
          }
        }
      }
    }
  }

  void Snapshot::SetEncodeParam(int width, int height, int quality)
  {
    std::unique_lock<std::mutex> lock(mMtx);
    mDstWidth = width;
    mDstHeight = height;
    mQuality = quality;
  }

  void Snapshot::RequestSnapShot(SnapShotCallBack* callBack, int width, int height, int quality) {
    mRequestCount += 1;
    mRequestHeight = height;
    mRequestWidth = width;
    mRequestQuality = quality;
    std::unique_lock<std::mutex> lock(mListenMtx);
    mSnapShotListener = callBack;
    lock.unlock();
  }

  void Snapshot::RequestFrame(const webrtc::VideoFrame & frame) {
    if (mRequestCount <= 0) {
      return;
    }
    std::shared_ptr<VHStream> vhStream = mVhStream.lock();
    if (!vhStream) {
      LOGE("VHStream is reset!");
      return;
    }
    /* check encode param */
    int dstWidth = mRequestWidth;
    int dstHeight = mRequestHeight;
    int quality = mRequestQuality;
    if (dstWidth <= 0 || dstHeight <= 0) {
      dstWidth = frame.width();
      dstHeight = frame.height();
    }
    if (quality <= 0 || quality > 100) {
      quality = 80;
    }

    rtc::scoped_refptr<webrtc::I420BufferInterface> src_buffer(frame.video_frame_buffer()->ToI420());
    if (frame.rotation() != webrtc::kVideoRotation_0) {
      src_buffer = webrtc::I420Buffer::Rotate(*src_buffer, frame.rotation());
    }
    rtc::scoped_refptr<webrtc::I420Buffer> dst_buffer = webrtc::I420Buffer::Create(dstWidth, dstHeight);
    if (src_buffer && dst_buffer) {
      int size = dstWidth * dstHeight;
      std::shared_ptr<uint8_t[]> temp_buf(new uint8_t[dstWidth * dstHeight * 3 / 2]);
      if (temp_buf) {
        libyuv::I420Scale(
          src_buffer->DataY(), src_buffer->StrideY(),
          src_buffer->DataU(), src_buffer->StrideU(),
          src_buffer->DataV(), src_buffer->StrideV(),
          src_buffer->width(), src_buffer->height(),
          temp_buf.get(), dstWidth,
          temp_buf.get() + size, dstWidth / 2,
          temp_buf.get() + size + size / 4, dstWidth / 2,
          dstWidth, dstHeight,
          libyuv::FilterMode::kFilterNone);
        JpegEncode codec;
        std::vector<uint8_t> mJpegBuffer = codec.Encode(temp_buf, dstWidth, dstHeight, quality);
        std::unique_ptr<uint8_t[]> JpegData(new uint8_t[mJpegBuffer.size()]);
        memcpy(JpegData.get(), &mJpegBuffer[0], mJpegBuffer.size());
        std::unique_lock<std::mutex> lock(mListenMtx);
        if (mJpegBuffer.size() > 0 && nullptr != mSnapShotListener) {
          /* data callback */
          mSnapShotListener->OnSnapShot(&mJpegBuffer[0], mJpegBuffer.size(), dstWidth, dstHeight, vhStream);
        }
        lock.unlock();
      }
    }
    mRequestCount -= 1;
    return;
  }

} // end vhall
