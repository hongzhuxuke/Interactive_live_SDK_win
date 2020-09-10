#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace vhall {

  class JpegEncode {
  public:
    JpegEncode();
    virtual ~JpegEncode() {};
    /* src: I420 */
    std::vector<unsigned char> Encode(std::shared_ptr<uint8_t[]> &src, int width, int height, int quality = 50);
  };

}