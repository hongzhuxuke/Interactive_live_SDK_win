#ifndef VH_GETSTATSOBSERVER_H
#define VH_GETSTATSOBSERVER_H

#include <mutex>
#include "common/vhall_define.h"
#include "common/vhall_utility.h"
#include "api/peer_connection_interface.h"
#include "statsInfo.h"

namespace webrtc {
  StatsReport;
  StatsReports;
}

NS_VH_BEGIN
class UpStats;
class BaseStack;
class VHStream;

class GetStatsObserver : public webrtc::StatsObserver {
public:
  GetStatsObserver(std::shared_ptr<BaseStack> p);
  ~GetStatsObserver();

  void OnComplete(const webrtc::StatsReports& reports);

  bool called() const { return called_; }

  struct SendAudioSsrc GetAuidoPublishStates() {
    std::unique_lock<std::mutex> lock(SsrcMtx);
    //mPublishAudio.calcBitRate();
    return mPublishAudio; }
  struct SendVideoSsrc GetVideoPublishStates() {
    std::unique_lock<std::mutex> lock(SsrcMtx);
    //mPublishVideo.calcBitRate();
    return mPublishVideo; }
  struct ReceiveAudioSsrc GetAudioSubscribeState() {
    std::unique_lock<std::mutex> lock(SsrcMtx);
    //mSubscribeAudio.calcBitRate();
    return mSubscribeAudio; }
  struct ReceiveVideoSsrc GetVideoSubscribeState() { 
    std::unique_lock<std::mutex> lock(SsrcMtx);
    //mSubscribeVideo.calcBitRate();
    return mSubscribeVideo; }
  struct Bwe GetOverallBwe() { return mOverallBwe; }

  static bool OpenUpStats(std::string url);
  static bool CloseUpStats();
private:
  static std::shared_ptr<UpStats> mUpStats;
  static std::mutex mUpStatsMtx;
  std::string ReportToString(const webrtc::StatsReports& reports, std::string mediaType); // type: "audio" or "video"
  bool GetIntValue(const webrtc::StatsReport* report, webrtc::StatsReport::StatsValueName name, int* value);

  bool GetInt64Value(const webrtc::StatsReport* report, webrtc::StatsReport::StatsValueName name, int64_t* value);

  bool GetStringValue(const webrtc::StatsReport* report, webrtc::StatsReport::StatsValueName name, std::string* value);

  bool GetBoolValue(const webrtc::StatsReport* report, webrtc::StatsReport::StatsValueName name, bool* value);

  bool GetFloatValue(const webrtc::StatsReport* report, webrtc::StatsReport::StatsValueName name, float* value);

  bool called_;

  std::weak_ptr<BaseStack> mPtrBaseStack;
  std::weak_ptr<VHStream> mVhStream;

  StreamCommonAttr mCommonAttr;
  SendAudioSsrc mPublishAudio;
  SendVideoSsrc mPublishVideo;
  ReceiveAudioSsrc mSubscribeAudio;
  ReceiveVideoSsrc mSubscribeVideo;
  std::mutex SsrcMtx;
  Bwe mOverallBwe;
};

NS_VH_END

#endif /* ec_stream_h */