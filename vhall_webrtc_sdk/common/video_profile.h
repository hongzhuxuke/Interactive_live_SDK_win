#pragma once
#include <string>
#include <memory>
#include <unordered_map>

enum BroadCastVideoProfileIndex {
  BROADCAST_VIDEO_PROFILE_UNDEFINED = -1,
  BROADCAST_VIDEO_PROFILE_480P_4x3_5F, //	  640x480	5	550
  BROADCAST_VIDEO_PROFILE_480P_4x3_15F, // 	640x480	15	450
  BROADCAST_VIDEO_PROFILE_480P_4x3_25F, //	640x480	25	700
  BROADCAST_VIDEO_PROFILE_480P_16x9_5F, //	848x480	5	650
  BROADCAST_VIDEO_PROFILE_480P_16x9_15F, //	848x480	15	500
  BROADCAST_VIDEO_PROFILE_480P_16x9_25F, //	848x480	25	750
  BROADCAST_VIDEO_PROFILE_540P_4x3_5F, /*	  720X540	5	800	此分辨率在涉及PC端观看的业务中使用时请充分考虑场景特点，细节辨识可能有难度 */
  BROADCAST_VIDEO_PROFILE_540P_4x3_15F, //	720X540	15	650
  BROADCAST_VIDEO_PROFILE_540P_4x3_25F, //	720X540	25	950
  BROADCAST_VIDEO_PROFILE_540P_16x9_5F, //	960X540	5	950
  BROADCAST_VIDEO_PROFILE_540P_16x9_15F, //	960X540	15	700
  BROADCAST_VIDEO_PROFILE_540P_16x9_25F, //	960X540	25	1150
  BROADCAST_VIDEO_PROFILE_720P_4x3_5F, /*	  960X720	5	900	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_720P_4x3_15F, //	960X720	15	1150
  BROADCAST_VIDEO_PROFILE_720P_4x3_25F, //	960X720	25	1400
  BROADCAST_VIDEO_PROFILE_720P_16x9_5F, /*	1280X720	5	1050	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_720P_16x9_15F, //	1280X720	15	1350
  BROADCAST_VIDEO_PROFILE_720P_16x9_25F, //	1280X720	25	1600
  BROADCAST_VIDEO_PROFILE_810P_16x9_5F, /*	1440X810	5	1100	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_810P_16x9_15F, //	1440X810	15	1350
  BROADCAST_VIDEO_PROFILE_810P_16x9_25F, //	1440X810	25	1750
  BROADCAST_VIDEO_PROFILE_960P_4x3_5F, /*	  1280X960	5	1200	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_960P_4x3_15F, //	1280X960	15	1400
  BROADCAST_VIDEO_PROFILE_960P_4x3_25F, //	1280X960	25	1600
  BROADCAST_VIDEO_PROFILE_960P_16x9_5F, /*	1712X960	5	1400	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_960P_16x9_15F, //	1712X960	15	1600
  BROADCAST_VIDEO_PROFILE_960P_16x9_25F, //	1712X960	25	1900
  BROADCAST_VIDEO_PROFILE_1080P_4x3_5F, /*	1440X1080	5	1300	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_1080P_4x3_15F, //	1440X1080	15	1550
  BROADCAST_VIDEO_PROFILE_1080P_4x3_25F, //	1440X1080	25	1800
  BROADCAST_VIDEO_PROFILE_1080P_16x9_5F, /*	1920X1080	5	1600	应用于只关注桌面共享内容的情况 */
  BROADCAST_VIDEO_PROFILE_1080P_16x9_15F, //1920X1080	15	1900
  BROADCAST_VIDEO_PROFILE_1080P_16x9_25F, //1920X1080	25	2200
};

class __declspec(dllexport)BroadCastProfile {
public:
  BroadCastProfile(BroadCastVideoProfileIndex profileIndex,int ratioW, int ratioH, int width, int height, int frameRate, int bitRate);
  BroadCastVideoProfileIndex mIndex;
  int mWidth;
  int mHeight;
  int mRatioW;
  int mRatioH;
  int mFrameRate;
  int mBitRate;
};

struct DesktopCaptureSource {
	// The unique id to represent a Source of current DesktopCapturer.
	int64_t id;
	// Title of the window or screen in UTF-8 encoding, maybe empty. This field
	// should not be used to identify a source.
	std::string title;
};

class __declspec(dllexport)BroadCastProfileList {
public:
  BroadCastProfileList();
  std::shared_ptr<BroadCastProfile> GetProfile(BroadCastVideoProfileIndex index);
private:
  void AddItem(BroadCastVideoProfileIndex profileIndex, int ratioW, int ratioH, int width, int height, int frameRate, int bitRate);
  std::unordered_map<BroadCastVideoProfileIndex, std::shared_ptr<BroadCastProfile>> mList;
};

enum VideoProfileIndex {
  VIDEO_PROFILE_UNDEFINED = -1,
  RTC_VIDEO_PROFILE_1080P_4x3_L, // 960X720	1440X1080	5	5	1100 1600 1300
  RTC_VIDEO_PROFILE_1080P_4x3_M, // 640X480	1440X1080	10	15	600	1850	1500
  RTC_VIDEO_PROFILE_1080P_4x3_H, //	640X480	1440X1080	15	25	750	2200	1750
  RTC_VIDEO_PROFILE_1080P_16x9_L, // 1280X720	1920X1080	5	5	1200	1800	1450
  RTC_VIDEO_PROFILE_1080P_16x9_M, //	848X480	1920X1080	10	15	700	2100	1700
  RTC_VIDEO_PROFILE_1080P_16x9_H, //	848X480	1920X1080	15	25	850	2500	2000
  RTC_VIDEO_PROFILE_810P_16x9_L, //	1280X720	1440X810	5	5	850	1350	1100
  RTC_VIDEO_PROFILE_720P_4x3_L, //	720X540	960X720	5	5	750	1100	900
  RTC_VIDEO_PROFILE_720P_4x3_M, //	480X360	960X720	10	15	450	1300	1050
  RTC_VIDEO_PROFILE_720P_4x3_H, //	480X360	960X720	15	25	550	1600	1300
  RTC_VIDEO_PROFILE_720P_16x9_L, //	1280X720	1280X720	5	5	800	1250	1000
  RTC_VIDEO_PROFILE_720P_16x9_M, //	640X360	1280X720	10	15	500	1500	1200
  RTC_VIDEO_PROFILE_720P_16x9_H, //	640X360	1280X720	15	25	600	1800	1450
  RTC_VIDEO_PROFILE_540P_4x3_L, //	720X540	720X540	5	5	450	700	550
  RTC_VIDEO_PROFILE_540P_4x3_M, //	640X480	720X540	10	15	450	900	700
  RTC_VIDEO_PROFILE_540P_4x3_H, //	640X480	720X540	15	25	550	1100	900
  RTC_VIDEO_PROFILE_540P_16x9_L, //	960X540	960X540	5	5	550	800	650
  RTC_VIDEO_PROFILE_540P_16x9_M, //	848X480	960X540	10	15	500	1000	800
  RTC_VIDEO_PROFILE_540P_16x9_H, //	848X480	960X540	15	25	600	1200	950
  RTC_VIDEO_PROFILE_480P_4x3_L, //	480X360	640X480	5	  5	  300	650	550
  RTC_VIDEO_PROFILE_480P_4x3_M, //	480X360	640X480	10	15	360	720	600
  RTC_VIDEO_PROFILE_480P_4x3_H, //	480X360	640X480	15	25	425	850	700
  RTC_VIDEO_PROFILE_480P_16x9_L, //	640X360	848X480	5	  5	  400	700	600
  RTC_VIDEO_PROFILE_480P_16x9_M, //	640X360	848X480	10	15	450	900	750
  RTC_VIDEO_PROFILE_480P_16x9_H, // 640X360	848X480	15	25	550	1100	900
  RTC_VIDEO_PROFILE_432P_4x3_M, //	570X432	570X432	10	15	185	500	400
  RTC_VIDEO_PROFILE_360P_4x3_L, //	480X360	480X360	5	  5	  200	375	350
  RTC_VIDEO_PROFILE_360P_4x3_M, //	320X240	480X360	10	15	200	400	350
  RTC_VIDEO_PROFILE_360P_4x3_H, //	320X240	480X360	15	25	250	500	450
  RTC_VIDEO_PROFILE_360P_16x9_L, //	640X360	640X360	5	  5	  220	400	400
  RTC_VIDEO_PROFILE_360P_16x9_M, //	424X240	640X360	10	15	240	480	450
  RTC_VIDEO_PROFILE_360P_16x9_H, //	424X240	640X360	15	25	300	600	500
  RTC_VIDEO_PROFILE_240P_4x3_L,  //	320X240	320X240	5	  5	  175	250	250
  RTC_VIDEO_PROFILE_240P_4x3_M, //	320X240	320X240	10	15	200	300	300
  RTC_VIDEO_PROFILE_240P_4x3_H, //	320X240	320X240	15	25	250	380	350
  RTC_VIDEO_PROFILE_240P_16x9_L, //	424X240	424X240	5	  5	  200	300	300
  RTC_VIDEO_PROFILE_240P_16x9_M, //	424X240	424X240	10	15	220	350	300
  RTC_VIDEO_PROFILE_240P_16x9_H, //	424X240	424X240	15	25	300	450	400
  RTC_VIDEO_PROFILE_160P_4x3_M, //  240X160 240X160	10	15	190	290	290
  RTC_VIDEO_PROFILE_160P_4x3_H, //	240X160	240X160	15	25	220	350	300
  RTC_VIDEO_PROFILE_180P_16x9_M, // 320X180	320X180	10	15	190	290	290
  RTC_VIDEO_PROFILE_180P_16x9_H, // 320X180	320X180	15	25	220	350	300
  RTC_VIDEO_PROFILE_144P_4x3_M, //  192X144	192X144	10	15	150	200	200
  RTC_VIDEO_PROFILE_120P_4x3_M, //  160X120 160X120 10	15	150	200	200
  RTC_VIDEO_PROFILE_120P_4x3_H, //  160X120 160X120 15	25	210	280	280
  RTC_VIDEO_PROFILE_90P_16x9_L, //	160X90	160X90	5	  5	  100	150	150
  RTC_VIDEO_PROFILE_90P_16x9_M, //  160X90	160X90	10	15	120	180	180
  RTC_VIDEO_PROFILE_90P_16x9_H, //  160X90	160X90	15	25	160	250	250
};
class __declspec(dllexport)VideoProfile {
public:
  VideoProfile();
  VideoProfile(
    VideoProfileIndex index,
    int ratioW, int ratioH,
    int minWdth, int maxWidth,
    int minHeight, int maxHeight,
    int minFps, int maxFps,
    int minBitRate, int maxBitRate, int startBitRate);

  VideoProfileIndex mIndex;
  int mMaxWidth;
  int mMinWidth;
  int mMaxHeight;
  int mMinHeight;
  int mRatioW;
  int mRatioH;
  int mMaxFrameRate;
  int mMinFrameRate;
  int mMaxBitRate;
  int mMinBitRate;
  int mStartBitRate;
};

class __declspec(dllexport)VideoProfileList {
public:
  VideoProfileList();
  std::shared_ptr<VideoProfile> GetProfile(VideoProfileIndex index);
private:
  void AddItem(
    VideoProfileIndex index,
    int ratioW, int ratioH,
    int minWdth, int maxWidth,
    int minHeight, int maxHeight,
    int minFps, int maxFps,
    int minBitRate, int maxBitRate, int startBitRate);
  std::unordered_map<VideoProfileIndex, std::shared_ptr<VideoProfile>> mProfileList;
};

enum LayoutMode {
  /* 均匀表格布局 */
  CANVAS_LAYOUT_PATTERN_GRID_1 = 0,    // 一人铺满
  CANVAS_LAYOUT_PATTERN_GRID_2_H,      // 左右两格
  CANVAS_LAYOUT_PATTERN_GRID_3_E,      // 正品字
  CANVAS_LAYOUT_PATTERN_GRID_3_D,      // 倒品字
  CANVAS_LAYOUT_PATTERN_GRID_4_M,      // 2行x2列
  CANVAS_LAYOUT_PATTERN_GRID_5_D,      // 2行，上3下2
  CANVAS_LAYOUT_PATTERN_GRID_6_E,      // 2行x3列
  CANVAS_LAYOUT_PATTERN_GRID_9_E,      // 3行x3列
  /* 主次屏浮窗布局 */
  CANVAS_LAYOUT_PATTERN_FLOAT_2_1DR,   // 大屏铺满，小屏悬浮右下角
  CANVAS_LAYOUT_PATTERN_FLOAT_2_1DL,   // 大屏铺满，小屏悬浮左下角
  CANVAS_LAYOUT_PATTERN_FLOAT_3_2DL,   // 大屏铺满，2小屏悬浮左下角
  CANVAS_LAYOUT_PATTERN_FLOAT_6_5D,    // 大屏铺满，一行5个悬浮于下面
  CANVAS_LAYOUT_PATTERN_FLOAT_6_5T,    // 大屏铺满，一行5个悬浮于上面
  /* 主次屏平铺布局 */
  CANVAS_LAYOUT_PATTERN_TILED_5_1T4D,  // 主次平铺，一行4个位于底部
  CANVAS_LAYOUT_PATTERN_TILED_5_1D4T,  // 主次平铺，一行4个位于顶部
  CANVAS_LAYOUT_PATTERN_TILED_5_1L4R,  // 主次平铺，一列4个位于右边
  CANVAS_LAYOUT_PATTERN_TILED_5_1R4L,  // 主次平铺，一列4个位于左边
  CANVAS_LAYOUT_PATTERN_TILED_6_1T5D,  // 主次平铺，一行5个位于底部
  CANVAS_LAYOUT_PATTERN_TILED_6_1D5T,  // 主次平铺，一行5个位于顶部
  CANVAS_LAYOUT_PATTERN_TILED_9_1L8R,  // 主次平铺，右边为（2列x4行=8个块）
  CANVAS_LAYOUT_PATTERN_TILED_9_1R8L,  // 主次平铺，左边为（2列x4行=8个块）
  CANVAS_LAYOUT_PATTERN_TILED_13_1L12R,  // 主次平铺，右边为（3列x4行=12个块）
  CANVAS_LAYOUT_PATTERN_TILED_17_1TL16GRID,  // 主次平铺，1V16，主屏在左上角
  CANVAS_LAYOUT_PATTERN_TILED_9_1D8T,  // 主次平铺，主屏在下，8个（2行x4列）在上
  CANVAS_LAYOUT_PATTERN_TILED_13_1TL12GRID,  // 主次平铺，主屏在左上角，其余12个均铺于其他剩余区域
  CANVAS_LAYOUT_PATTERN_TILED_17_1TL16GRID_E,  // 主次平铺，主屏在左上角，其余16个均铺于其他剩余区域
  /* 自定义坐标布局 */
  CANVAS_LAYOUT_PATTERN_CUSTOM        // 自定义，当使用坐标布局接口时，请使用此
};