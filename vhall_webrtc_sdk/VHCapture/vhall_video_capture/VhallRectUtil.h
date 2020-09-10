#pragma once
class VhallRectUtil
{
public:
  class CGRect
  {
  public:
    float x;
    float y;
    float width;
    float height;

    CGRect(float x, float y, float width, float height) {
      this->x = x;
      this->y = y;
      this->width = width;
      this->height = height;
    };

    CGRect() {
      x = 0.0;
      y = 0.0;
      width = 0.0;
      height = 0.0;
    };

    bool isValid() {
      if (width != 0 || height != 0) {
        return true;
      }
      else {
        return false;
      }
    }

    void toInt() {
      x = (int)x;
      y = (int)y;
      width = (int)width;
      height = (int)height;
    }

    ~CGRect() {};
  };

  static CGRect adjustFitScreenFrame(CGRect frame, CGRect superFrame) {
    if (superFrame.width != 0.0F && superFrame.height != 0.0F) {
      float wScale = frame.width / superFrame.width;
      float hScale = frame.height / superFrame.height;
      float width;
      float height;
      if (wScale > hScale) {
        width = superFrame.width;
        height = superFrame.width / frame.width * frame.height;
        return  CGRect(0.0, -(height - superFrame.height) / 2.0, width, height);
      }
      else if (wScale < hScale) {
        width = superFrame.height / frame.height * frame.width;
        height = superFrame.height;
        return  CGRect(-(width - superFrame.width) / 2.0F, 0.0F, width, height);
      }
      else {
        superFrame.x = 0.0F;
        superFrame.y = 0.0F;
        return superFrame;
      }
    }
    else {
      return CGRect(0.0F, 0.0F, 0.0F, 0.0F);
    }
  }

  static CGRect adjustFillScreenFrame(CGRect frame, CGRect superFrame) {
    if (superFrame.width != 0.0F && superFrame.height != 0.0F) {
      float wScale = frame.width / superFrame.width;
      float hScale = frame.height / superFrame.height;
      float width;
      float height;
      if (wScale > hScale) {
        width = superFrame.height / frame.height * frame.width;
        height = superFrame.height;
        return CGRect(-(width - superFrame.width) / 2.0F, 0.0F, width, height);
      }
      else if (wScale < hScale) {
        width = superFrame.width;
        height = superFrame.width / frame.width * frame.height;
        return CGRect(0.0F, -(height - superFrame.height) / 2.0F, width, height);
      }
      else {
        superFrame.x = 0.0F;
        superFrame.y = 0.0F;
        return superFrame;
      }
    }
    else {
      return CGRect(0.0F, 0.0F, 0.0F, 0.0F);
    }
  }


private:
  VhallRectUtil() {};
  ~VhallRectUtil() {};
};