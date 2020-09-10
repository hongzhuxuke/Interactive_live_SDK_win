#include "JpegEncode.h"
#include "common/vhall_log.h"
#include "common_types.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "third_party/libjpeg_turbo/jpeglib.h"
#include "third_party/libyuv/include/libyuv.h"

namespace vhall {

  void InitDestination(j_compress_ptr cinfo);
  boolean EmptyOutputBuffer(j_compress_ptr cinfo);
  void TermDestination(j_compress_ptr cinfo);
  METHODDEF(void) ErrorExit(j_common_ptr cinfo);
  METHODDEF(void) OutputMessage(j_common_ptr cinfo);
  /* jpeg图像处理异常类 */
  class JpegException :public std::logic_error {
  public:
    using std::logic_error::logic_error;
  };

  char JpegErrBuffer[JMSG_LENGTH_MAX] = { 0 };
  std::vector<unsigned char> mJpegBuffer;
  size_t mBlockSize;


  JpegEncode::JpegEncode() {

  }

  std::vector<unsigned char> JpegEncode::Encode(std::shared_ptr<uint8_t[]> &src, int width, int height, int quality) {
    const int kColorPlanes = 3;  // R, G and B.
    size_t rgb_len = height * width * kColorPlanes;
    std::unique_ptr<uint8_t[]> dst(new uint8_t[rgb_len]);
    if (!src || !dst) {
      mJpegBuffer.clear();
      LOGE("Jpeg data buffer failed.");
      return mJpegBuffer;
    }
    /* kRGB24 actually corresponds to FourCC 24BG which is 24-bit BGR */
    int size = width * height;
    libyuv::ConvertFromI420(
      src.get(), width,
      src.get() + size, width / 2,
      src.get() + size + size / 4, width / 2,
      dst.get(), 0, width, height,
      ConvertVideoType(webrtc::VideoType::kRGB24));
    /* Invoking LIBJPEG */
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    cinfo.err = jpeg_std_error(&jerr);
    /* 异常处理 */
    jerr.error_exit = ErrorExit;
    jerr.output_message = OutputMessage;

    try {
      jpeg_create_compress(&cinfo);

      jpeg_stdio_dest(&cinfo, nullptr);
      mJpegBuffer.clear();
      mBlockSize = rgb_len;
      cinfo.dest->free_in_buffer = 0;
      cinfo.dest->init_destination = InitDestination;
      cinfo.dest->empty_output_buffer = EmptyOutputBuffer;
      cinfo.dest->term_destination = TermDestination;

      cinfo.image_width = width;
      cinfo.image_height = height;
      cinfo.input_components = kColorPlanes;
      cinfo.in_color_space = JCS_EXT_BGR;
      jpeg_set_defaults(&cinfo);
      jpeg_set_quality(&cinfo, quality, TRUE);

      jpeg_start_compress(&cinfo, TRUE);
      int row_stride = width * 3;
      while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &dst.get()[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
      }

      jpeg_finish_compress(&cinfo);
      jpeg_destroy_compress(&cinfo);
    }
    catch (JpegException& e) {
      LOGE("Jpeg encode failed.");
      mJpegBuffer.clear();
      return mJpegBuffer;
    }
    return mJpegBuffer;
  }

  void InitDestination(j_compress_ptr cinfo) {
    mJpegBuffer.resize(mBlockSize);
    cinfo->dest->next_output_byte = &mJpegBuffer[0];
    cinfo->dest->free_in_buffer = mJpegBuffer.size();
  }

  boolean EmptyOutputBuffer(j_compress_ptr cinfo) {
    size_t oldsize = mJpegBuffer.size();
    mJpegBuffer.resize(oldsize + mBlockSize);
    cinfo->dest->next_output_byte = &mJpegBuffer[oldsize];
    cinfo->dest->free_in_buffer = mJpegBuffer.size() - oldsize;
    return true;
  }

  void TermDestination(j_compress_ptr cinfo) {
    LOGI("free_in_buffer: %lu, mJpegBuffer.size():%lu", cinfo->dest->free_in_buffer, mJpegBuffer.size());
    if (cinfo->dest->free_in_buffer > 0 && (mJpegBuffer.size() > cinfo->dest->free_in_buffer)) {
      mJpegBuffer.resize(mJpegBuffer.size() - cinfo->dest->free_in_buffer);
    }
    cinfo->dest->free_in_buffer = 0;
  }

  METHODDEF(void) ErrorExit(j_common_ptr cinfo) {
    /* Always display the message */
    (*cinfo->err->output_message) (cinfo);

    /* Let the memory manager delete any temp files before we die */
    jpeg_destroy(cinfo);

    throw JpegException(JpegErrBuffer);
  }
  METHODDEF(void) OutputMessage(j_common_ptr cinfo) {
    memset(JpegErrBuffer, sizeof(JpegErrBuffer), 0);
    /* Create the message */
    (*cinfo->err->format_message) (cinfo, JpegErrBuffer);
    LOGE("Jpeg encode err: %s\n", JpegErrBuffer);
  }
}