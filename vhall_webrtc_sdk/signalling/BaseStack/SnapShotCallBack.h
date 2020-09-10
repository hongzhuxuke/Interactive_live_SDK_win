#pragma once
#include <iostream>
#include "../vh_stream.h"
namespace vhall {
  class SnapShotCallBack {
  public:
    SnapShotCallBack() {};
    virtual ~SnapShotCallBack() {};
    virtual void OnSnapShot(unsigned char* data, size_t length, int width, int height, std::shared_ptr<VHStream> stream) = 0;
  };
}