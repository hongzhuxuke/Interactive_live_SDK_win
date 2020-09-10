#include "video_profile.h"

BroadCastProfile::BroadCastProfile(BroadCastVideoProfileIndex profileIndex, int ratioW, int ratioH, int width, int height, int frameRate, int bitRate) {
  mIndex = profileIndex;
  mWidth = width;
  mHeight = height;
  mRatioW = ratioW;
  mRatioH = ratioH;
  mFrameRate = frameRate;
  mBitRate = bitRate;
};

BroadCastProfileList::BroadCastProfileList() {
  AddItem(BROADCAST_VIDEO_PROFILE_480P_4x3_5F, 4, 3, 640, 480, 5, 550);
  AddItem(BROADCAST_VIDEO_PROFILE_480P_4x3_15F, 4, 3, 640, 480, 15, 450);
  AddItem(BROADCAST_VIDEO_PROFILE_480P_4x3_25F, 4, 3, 640, 480, 25, 700);
  AddItem(BROADCAST_VIDEO_PROFILE_480P_16x9_5F, 16, 9, 848, 480, 5, 650);
  AddItem(BROADCAST_VIDEO_PROFILE_480P_16x9_15F, 16, 9, 848, 480, 15, 500);
  AddItem(BROADCAST_VIDEO_PROFILE_480P_16x9_25F, 16, 9, 848, 480, 25, 750);
  AddItem(BROADCAST_VIDEO_PROFILE_540P_4x3_5F, 4, 3, 720, 540, 5, 800);
  AddItem(BROADCAST_VIDEO_PROFILE_540P_4x3_15F, 4, 3, 720, 540, 15, 650);
  AddItem(BROADCAST_VIDEO_PROFILE_540P_4x3_25F, 4, 3, 720, 540, 25, 950);
  AddItem(BROADCAST_VIDEO_PROFILE_540P_16x9_5F, 16, 9, 960, 540, 5, 950);
  AddItem(BROADCAST_VIDEO_PROFILE_540P_16x9_15F, 16, 9, 960, 540, 15, 700);
  AddItem(BROADCAST_VIDEO_PROFILE_540P_16x9_25F, 16, 9, 960, 540, 25, 1150);
  AddItem(BROADCAST_VIDEO_PROFILE_720P_4x3_5F, 4, 3, 960, 720, 5, 900);
  AddItem(BROADCAST_VIDEO_PROFILE_720P_4x3_15F, 4, 3, 960, 720, 15, 1150);
  AddItem(BROADCAST_VIDEO_PROFILE_720P_4x3_25F, 4, 3, 960, 720, 25, 1400);
  AddItem(BROADCAST_VIDEO_PROFILE_720P_16x9_5F, 16, 9, 1280, 720, 5, 1050);
  AddItem(BROADCAST_VIDEO_PROFILE_720P_16x9_15F, 16, 9, 1280, 720, 15, 1350);
  AddItem(BROADCAST_VIDEO_PROFILE_720P_16x9_25F, 16, 9, 1280, 720, 25, 1600);
  AddItem(BROADCAST_VIDEO_PROFILE_810P_16x9_5F, 16, 9, 1440, 810, 5, 1100);
  AddItem(BROADCAST_VIDEO_PROFILE_810P_16x9_15F, 16, 9, 1440, 810, 15, 1350);
  AddItem(BROADCAST_VIDEO_PROFILE_810P_16x9_25F, 16, 9, 1440, 810, 25, 1750);
  AddItem(BROADCAST_VIDEO_PROFILE_960P_4x3_5F, 4, 3, 1280, 960, 5, 1200);
  AddItem(BROADCAST_VIDEO_PROFILE_960P_4x3_15F, 4, 3, 1280, 960, 15, 1400);
  AddItem(BROADCAST_VIDEO_PROFILE_960P_4x3_25F, 4, 3, 1280, 960, 25, 1600);
  AddItem(BROADCAST_VIDEO_PROFILE_960P_16x9_5F, 16, 9, 1712, 960, 5, 1400);
  AddItem(BROADCAST_VIDEO_PROFILE_960P_16x9_15F, 16, 9, 1712, 960, 15, 1600);
  AddItem(BROADCAST_VIDEO_PROFILE_960P_16x9_25F, 16, 9, 1712, 960, 25, 1900);
  AddItem(BROADCAST_VIDEO_PROFILE_1080P_4x3_5F, 4, 3, 1440, 1080, 5, 1300);
  AddItem(BROADCAST_VIDEO_PROFILE_1080P_4x3_15F, 4, 3, 1440, 1080, 15, 1550);
  AddItem(BROADCAST_VIDEO_PROFILE_1080P_4x3_25F, 4, 3, 1440, 1080, 25, 1800);
  AddItem(BROADCAST_VIDEO_PROFILE_1080P_16x9_5F, 16, 9, 1920, 1080, 5, 1600);
  AddItem(BROADCAST_VIDEO_PROFILE_1080P_16x9_15F, 16, 9, 1920, 1080, 15, 1900);
  AddItem(BROADCAST_VIDEO_PROFILE_1080P_16x9_25F, 16, 9, 1920, 1080, 25, 2200);
};

std::shared_ptr<BroadCastProfile> BroadCastProfileList::GetProfile(BroadCastVideoProfileIndex index) {
  return mList[index];
}

void BroadCastProfileList::AddItem(BroadCastVideoProfileIndex profileIndex, int ratioW, int ratioH, int width, int height, int frameRate, int bitRate) {
  std::shared_ptr<BroadCastProfile> proifle = std::make_shared<BroadCastProfile>(profileIndex, ratioW, ratioH, width, height, frameRate, bitRate);
  mList[profileIndex] = proifle;
}

VideoProfile::VideoProfile() {
  mIndex = VIDEO_PROFILE_UNDEFINED; // BROADCAST_VIDEO_PROFILE_UNDEFINED
  mMaxWidth = 0;
  mMinWidth = 0;
  mMaxHeight = 0;
  mMinHeight = 0;
  mRatioW = 0;
  mRatioH = 0;
  mMaxFrameRate = 0;
  mMinFrameRate = 0;
  mMaxBitRate = 0;
  mMinBitRate = 0;
  mStartBitRate = 0;
}

VideoProfile::VideoProfile(
  VideoProfileIndex index,
  int ratioW, int ratioH,
  int minWdth, int maxWidth,
  int minHeight, int maxHeight,
  int minFps, int maxFps,
  int minBitRate, int maxBitRate, int startBitRate) {
  mIndex = index;
  mMaxWidth = maxWidth;
  mMinWidth = minWdth;
  mMaxHeight = maxHeight;
  mMinHeight = minHeight;
  mRatioW = ratioW;
  mRatioH = ratioH;
  mMaxFrameRate = maxFps;
  mMinFrameRate = minFps;
  mMaxBitRate = maxBitRate;
  mMinBitRate = minBitRate;
  mStartBitRate = startBitRate;
};
VideoProfileList::VideoProfileList() {
  AddItem(RTC_VIDEO_PROFILE_1080P_4x3_L, 4, 3, 960, 1440, 720, 1080, 5, 5, 1100, 1600, 1300);
  AddItem(RTC_VIDEO_PROFILE_1080P_4x3_M, 4, 3, 640, 1440, 480, 1080, 10, 15, 600, 1850, 1500);
  AddItem(RTC_VIDEO_PROFILE_1080P_4x3_H, 4, 3, 640, 1440, 480, 1080, 15, 25, 750, 2200, 1750);
  AddItem(RTC_VIDEO_PROFILE_1080P_16x9_L, 16, 9, 1280, 1920, 720, 1080, 5, 5, 1200, 1800, 1450);
  AddItem(RTC_VIDEO_PROFILE_1080P_16x9_M, 16, 9, 848, 1920, 480, 1080, 10, 15, 700, 2100, 1700);
  AddItem(RTC_VIDEO_PROFILE_1080P_16x9_H, 16, 9, 848, 1920, 480, 1080, 15, 25, 850, 2500, 2000);
  AddItem(RTC_VIDEO_PROFILE_810P_16x9_L, 16, 9, 1280, 1440, 720, 810, 5, 5, 850, 1350, 1100);
  AddItem(RTC_VIDEO_PROFILE_720P_4x3_L, 4, 3, 720, 960, 540, 720, 5, 5, 750, 1100, 900);
  AddItem(RTC_VIDEO_PROFILE_720P_4x3_M, 4, 3, 480, 960, 360, 720, 10, 15, 450, 1300, 1050);
  AddItem(RTC_VIDEO_PROFILE_720P_4x3_H, 4, 3, 480, 960, 360, 720, 15, 25, 550, 1600, 1300);
  AddItem(RTC_VIDEO_PROFILE_720P_16x9_L, 16, 9, 1280, 1280, 720, 720, 5, 5, 800, 1250, 1000);
  AddItem(RTC_VIDEO_PROFILE_720P_16x9_M, 16, 9, 640, 1280, 360, 720, 10, 15, 500, 1500, 1200);
  AddItem(RTC_VIDEO_PROFILE_720P_16x9_H, 16, 9, 640, 1280, 360, 720, 15, 25, 600, 1800, 1450);
  AddItem(RTC_VIDEO_PROFILE_540P_4x3_L, 4, 3, 720, 720, 540, 540, 5, 5, 450, 700, 550);
  AddItem(RTC_VIDEO_PROFILE_540P_4x3_M, 4, 3, 640, 720, 480, 540, 10, 15, 450, 900, 700);
  AddItem(RTC_VIDEO_PROFILE_540P_4x3_H, 4, 3, 640, 720, 480, 540, 15, 25, 550, 1100, 900);
  AddItem(RTC_VIDEO_PROFILE_540P_16x9_L, 16, 9, 960, 960, 540, 540, 5, 5, 550, 800, 650);
  AddItem(RTC_VIDEO_PROFILE_540P_16x9_M, 16, 9, 848, 960, 480, 540, 10, 15, 500, 1000, 800);
  AddItem(RTC_VIDEO_PROFILE_540P_16x9_H, 16, 9, 848, 960, 480, 540, 15, 25, 600, 1200, 950);
  AddItem(RTC_VIDEO_PROFILE_480P_4x3_L, 4, 3, 480, 640, 360, 480, 5, 5, 600, 650, 550);
  AddItem(RTC_VIDEO_PROFILE_480P_4x3_M, 4, 3, 480, 640, 360, 480, 10, 15, 360, 720, 600);
  AddItem(RTC_VIDEO_PROFILE_480P_4x3_H, 4, 3, 480, 640, 360, 480, 15, 25, 425, 850, 700);
  AddItem(RTC_VIDEO_PROFILE_480P_16x9_L, 16, 9, 640, 848, 360, 480, 5, 5, 400, 700, 600);
  AddItem(RTC_VIDEO_PROFILE_480P_16x9_M, 16, 9, 640, 848, 360, 480, 10, 15, 450, 900, 750);
  AddItem(RTC_VIDEO_PROFILE_480P_16x9_H, 16, 9, 640, 848, 360, 480, 15, 25, 550, 1100, 900);
  AddItem(RTC_VIDEO_PROFILE_432P_4x3_M, 4, 3, 570, 570, 432, 432, 10, 15, 185, 500, 400);
  AddItem(RTC_VIDEO_PROFILE_360P_4x3_L, 4, 3, 320, 480, 240, 360, 5, 5, 200, 375, 350);
  AddItem(RTC_VIDEO_PROFILE_360P_4x3_M, 4, 3, 320, 480, 240, 360, 10, 15, 200, 400, 350);
  AddItem(RTC_VIDEO_PROFILE_360P_4x3_H, 4, 3, 320, 480, 240, 360, 15, 25, 250, 500, 450);
  AddItem(RTC_VIDEO_PROFILE_360P_16x9_L, 16, 9, 424, 640, 240, 360, 5, 5, 220, 400, 400);
  AddItem(RTC_VIDEO_PROFILE_360P_16x9_M, 16, 9, 424, 640, 240, 360, 10, 15, 240, 480, 450);
  AddItem(RTC_VIDEO_PROFILE_360P_16x9_H, 16, 9, 424, 640, 240, 360, 15, 25, 300, 600, 500);
  AddItem(RTC_VIDEO_PROFILE_240P_4x3_L, 4, 3, 320, 320, 240, 240, 5, 5, 175, 250, 250);
  AddItem(RTC_VIDEO_PROFILE_240P_4x3_M, 4, 3, 320, 320, 240, 240, 10, 15, 200, 300, 300);
  AddItem(RTC_VIDEO_PROFILE_240P_4x3_H, 4, 3, 320, 320, 240, 240, 15, 25, 250, 380, 350);
  AddItem(RTC_VIDEO_PROFILE_240P_16x9_L, 16, 9, 424, 424, 240, 240, 5, 5, 200, 300, 300);
  AddItem(RTC_VIDEO_PROFILE_240P_16x9_M, 16, 9, 424, 424, 240, 240, 10, 15, 220, 350, 300);
  AddItem(RTC_VIDEO_PROFILE_240P_16x9_H, 16, 9, 424, 424, 240, 240, 15, 25, 300, 450, 400);
  AddItem(RTC_VIDEO_PROFILE_160P_4x3_M, 4, 3, 240, 240, 160, 160, 10, 15, 190, 290, 290);
  AddItem(RTC_VIDEO_PROFILE_160P_4x3_H, 4, 3, 240, 240, 160, 160, 15, 25, 220, 350, 300);
  AddItem(RTC_VIDEO_PROFILE_180P_16x9_M, 16, 9, 320, 320, 180, 180, 10, 15, 190, 290, 290);
  AddItem(RTC_VIDEO_PROFILE_180P_16x9_H, 16, 9, 320, 320, 180, 180, 15, 25, 220, 350, 300);
  AddItem(RTC_VIDEO_PROFILE_144P_4x3_M, 4, 3, 192, 192, 144, 144, 10, 15, 150, 200, 200);
  AddItem(RTC_VIDEO_PROFILE_120P_4x3_M, 4, 3, 160, 160, 120, 120, 10, 15, 150, 200, 200);
  AddItem(RTC_VIDEO_PROFILE_120P_4x3_H, 4, 3, 160, 160, 120, 120, 15, 25, 210, 280, 280);
  AddItem(RTC_VIDEO_PROFILE_90P_16x9_L, 16, 9, 160, 160, 90, 90, 5, 5, 100, 150, 150);
  AddItem(RTC_VIDEO_PROFILE_90P_16x9_M, 16, 9, 160, 160, 90, 90, 10, 15, 120, 180, 180);
  AddItem(RTC_VIDEO_PROFILE_90P_16x9_H, 16, 9, 160,160,90,90, 15, 25, 160, 250, 250);
}

std::shared_ptr<VideoProfile> VideoProfileList::GetProfile(VideoProfileIndex index) {
  return mProfileList[index];
}

void VideoProfileList::AddItem(VideoProfileIndex index, int ratioW, int ratioH, int minWdth, int maxWidth, int minHeight, int maxHeight, int minFps, int maxFps, int minBitRate, int maxBitRate, int startBitRate) {
  std::shared_ptr<VideoProfile> profile = std::make_shared<VideoProfile>(index, ratioW, ratioH, minWdth, maxWidth, minHeight, maxHeight, minFps, maxFps, minBitRate, maxBitRate, startBitRate);
  mProfileList[index] = profile;
}
