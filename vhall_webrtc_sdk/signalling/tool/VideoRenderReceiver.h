#pragma once

namespace vhall {
  class VideoRenderReceiveInterface {
  public:
    virtual void ReceiveVideo(const unsigned char* video, int length, int width, int height) = 0;
  };

}