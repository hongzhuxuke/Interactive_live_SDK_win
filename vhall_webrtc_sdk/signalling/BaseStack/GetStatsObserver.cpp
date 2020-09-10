#include "BaseStack.h"
#include "GetStatsObserver.h"
#include "UpStats.h"
#include "../vh_uplog.h"
#include "../vh_uplog_define.h"
#include "json/json.h"

NS_VH_BEGIN
std::shared_ptr<UpStats> GetStatsObserver::mUpStats = nullptr;
std::mutex GetStatsObserver::mUpStatsMtx;

void ReceiveVideoSsrc::calc() {
  if (0 == CalcData.lastCalCTime) {
    CalcData.lastBytesReceived = BytesReceived;
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsReceived = PacketsReceived;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    LOGD("ReceivedBitrate:%lld", CalcData.Bitrate);
    return;
  }
  else {
    CalcData.duration = Utility::GetTimestampMs() - CalcData.lastCalCTime;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    
    CalcData.LastReceivedBytesSeg = BytesReceived - CalcData.lastBytesReceived;
    CalcData.lastBytesReceived = BytesReceived;
    CalcData.Bitrate = (CalcData.duration > 0 && CalcData.LastReceivedBytesSeg > 0) ? (CalcData.LastReceivedBytesSeg * 1000 * 8 / CalcData.duration) : 0;
    LOGD("ReceivedBitrate:%lld", CalcData.Bitrate);

    CalcData.LastPacketsLostSeg = PacketsLost - CalcData.LastPacketsLost;
    CalcData.LastPacketReceivedSeg = PacketsReceived - CalcData.LastPacketsReceived;
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsReceived = PacketsReceived;
    CalcData.PacketsLostRate = ((CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg) > 0 && CalcData.LastPacketsLostSeg > 0) ? ((double)CalcData.LastPacketsLostSeg / (CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg)) : 0;
    LOGD("ReceivedPacketsLostRate:%f, PacketsLost:%d", CalcData.PacketsLostRate, PacketsLost);
  }
}

void ReceiveAudioSsrc::calc() {
  if (0 == CalcData.lastCalCTime) {
    CalcData.lastBytesReceived = BytesReceived;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsReceived = PacketsReceived;
    LOGD("ReceivedBitrate:%lld", CalcData.Bitrate);
    return;
  }
  else {
    CalcData.duration = Utility::GetTimestampMs() - CalcData.lastCalCTime;
    CalcData.LastReceivedBytesSeg = BytesReceived - CalcData.lastBytesReceived;
    CalcData.lastBytesReceived = BytesReceived;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    CalcData.Bitrate = (CalcData.duration > 0 && CalcData.LastReceivedBytesSeg > 0) ? (CalcData.LastReceivedBytesSeg * 1000 * 8 / CalcData.duration) : 0;
    LOGD("ReceivedBitrate:%lld, BytesReceived:%lld", CalcData.Bitrate, BytesReceived);

    CalcData.LastPacketsLostSeg = PacketsLost - CalcData.LastPacketsLost;
    CalcData.LastPacketReceivedSeg = PacketsReceived - CalcData.LastPacketsReceived;
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsReceived = PacketsReceived;
    CalcData.PacketsLostRate = ((CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg) > 0 && CalcData.LastPacketsLostSeg > 0) ? ((double)CalcData.LastPacketsLostSeg / (CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg)) : 0;
    LOGD("ReceivedPacketsLostRate:%f, PacketsLost:%d", CalcData.PacketsLostRate, PacketsLost);
  }
}

void SendVideoSsrc::calc() {
  if (0 == CalcData.lastCalCTime) {
    CalcData.lastBytesSent = BytesSent;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsSent = PacketsSent;
    LOGD("SentBitrate:%lld", CalcData.Bitrate);
    return;
  }
  else {
    CalcData.duration = Utility::GetTimestampMs() - CalcData.lastCalCTime;
    CalcData.LastSentBytesSeg = BytesSent - CalcData.lastBytesSent;
    CalcData.lastBytesSent = BytesSent;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    CalcData.Bitrate = (CalcData.duration > 0 && CalcData.LastSentBytesSeg > 0) ? (CalcData.LastSentBytesSeg * 1000 * 8 / CalcData.duration) : 0;
    LOGD("SendVideoSsrc SentBitrate:%lld, BytesSent:%lld, CalcData.duration:%lld", CalcData.Bitrate, BytesSent, CalcData.duration);
    CalcData.LastPacketsLostSeg = PacketsLost - CalcData.LastPacketsLost;
    CalcData.LastPacketReceivedSeg = PacketsSent - CalcData.LastPacketsSent;
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsSent = PacketsSent;
    CalcData.PacketsLostRate = ((CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg) > 0 && CalcData.LastPacketsLostSeg > 0) ? ((double)CalcData.LastPacketsLostSeg / (CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg)) : 0;
    LOGD("SentPacketsLostRate:%f, PacketsLost:%d", CalcData.PacketsLostRate, PacketsLost);
  }
}

void SendAudioSsrc::calc() {
  if (0 == CalcData.lastCalCTime) {
    CalcData.lastBytesSent = BytesSent;
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsSent = PacketsSent;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    LOGD("SentBitrate:%lld", CalcData.Bitrate);
    return;
  }
  else {
    CalcData.duration = Utility::GetTimestampMs() - CalcData.lastCalCTime;
    CalcData.LastSentBytesSeg = BytesSent - CalcData.lastBytesSent;
    CalcData.lastBytesSent = BytesSent;
    CalcData.lastCalCTime = Utility::GetTimestampMs();
    CalcData.Bitrate = (CalcData.duration > 0 && CalcData.LastSentBytesSeg > 0) ? (CalcData.LastSentBytesSeg * 1000 * 8 / CalcData.duration) : 0;
    CalcData.lastBytesSent = BytesSent;
    LOGD("SentBitrate:%lld, BytesSent:%lld", CalcData.Bitrate, BytesSent);
    CalcData.LastPacketsLostSeg = PacketsLost - CalcData.LastPacketsLost;
    CalcData.LastPacketReceivedSeg = PacketsSent - CalcData.LastPacketsSent;
    CalcData.LastPacketsLost = PacketsLost;
    CalcData.LastPacketsSent = PacketsSent;
    CalcData.PacketsLostRate = ((CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg) > 0 && CalcData.LastPacketsLostSeg > 0) ? ((double)CalcData.LastPacketsLostSeg / (CalcData.LastPacketReceivedSeg + CalcData.LastPacketsLostSeg)) : 0;
    LOGD("SentPacketsLostRate:%f, PacketsLost:%d", CalcData.PacketsLostRate, PacketsLost);
  }
}

GetStatsObserver::GetStatsObserver(std::shared_ptr<BaseStack> p):
called_(false), mSubscribeVideo(), mSubscribeAudio(),
mPublishVideo(), mPublishAudio()
{
  mPtrBaseStack = p;
  mVhStream = p->mVhStream;
}

GetStatsObserver::~GetStatsObserver() {
  LOGI("~GetStatsCallback");
}


void GetStatsObserver::OnComplete(const webrtc::StatsReports& reports) {
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
  LOGD("Get Stream Stats, id: %s", ptrBaseStack->GetStreamID());
  called_ = true;

  // for debug
  //for (const auto* r : reports) {
  //  RTC_LOG(INFO) << __FUNCTION__ << " Type: " << r->type();
  //  for (auto v : r->values()) {
  //    RTC_LOG(INFO) << __FUNCTION__ << " StatsValueName: " << v.first << " StatsValue: " << v.second->ToString() << " Type: " << v.second->type();
  //  }
  //}

  std::unique_lock<std::mutex> upStatsLock(mUpStatsMtx);
  if (mUpStats) {
    std::string VideoReportMessage = ReportToString(reports, "video");
    std::string AudioReportMessage = ReportToString(reports, "audio");
    mUpStats->DoUpStats(VideoReportMessage);
    mUpStats->DoUpStats(AudioReportMessage);
  }
  upStatsLock.unlock();

  bool isLocal = vhStream->mLocal;
  for (const auto* r : reports) {
    if (r->type() == webrtc::StatsReport::kStatsReportTypeCandidatePair) {
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameLocalAddress, &mCommonAttr.mLocalAddr);
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameRemoteAddress, &mCommonAttr.mRemoteAddr);

      if (isLocal) {
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameRtt, &mPublishVideo.Rtt);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameRtt, &mPublishAudio.Rtt);
      }
      else {
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameRtt, &mSubscribeVideo.Rtt);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameRtt, &mSubscribeAudio.Rtt);
      }
    }

    if (r->type() == webrtc::StatsReport::kStatsReportTypeIceLocalCandidate) {
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameCandidateNetworkType, &mCommonAttr.mNetworkType);
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameCandidateType, &mCommonAttr.mCadidateType);
    }

    if (r->type() == webrtc::StatsReport::kStatsReportTypeSsrc) {
      std::string MediaType;
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &MediaType);
      if (isLocal && MediaType == "video") {/* publish */
        std::unique_lock<std::mutex> lock(SsrcMtx);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameBytesSent, &mPublishVideo.BytesSent);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameCodecImplementationName, &mPublishVideo.CodecImplementationName);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFramesEncoded, &mPublishVideo.FramesEncoded);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &mPublishVideo.MediaType);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsLost, &mPublishVideo.PacketsLost);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsSent, &mPublishVideo.PacketsSent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameQpSum, &mPublishVideo.QpSum);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameSsrc, &mPublishVideo.Ssrc);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTransportId, &mPublishVideo.TransportId);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameAdaptationChanges, &mPublishVideo.AdaptationChanges);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameAvgEncodeMs, &mPublishVideo.AvgEncodeMs);
        GetBoolValue(r, webrtc::StatsReport::kStatsValueNameBandwidthLimitedResolution, &mPublishVideo.BandwidthLimitedResolution);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameCodecName, &mPublishVideo.CodecName);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameContentType, &mPublishVideo.ContentType);
        GetBoolValue(r, webrtc::StatsReport::kStatsValueNameCpuLimitedResolution, &mPublishVideo.CpuLimitedResolution);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameEncodeUsagePercent, &mPublishVideo.EncodeUsagePercent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFirsReceived, &mPublishVideo.FirsReceived);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameHeightInput, &mPublishVideo.FrameHeightInput);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameHeightSent, &mPublishVideo.FrameHeightSent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameRateInput, &mPublishVideo.FrameRateInput);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameRateSent, &mPublishVideo.FrameRateSent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameWidthInput, &mPublishVideo.FrameWidthInput);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameWidthSent, &mPublishVideo.FrameWidthSent);
        GetBoolValue(r, webrtc::StatsReport::kStatsValueNameHasEnteredLowResolution, &mPublishVideo.HasEnteredLowResolution);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameMinPlayoutDelayMs, &mPublishVideo.MinPlayoutDelayMs);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameNacksReceived, &mPublishVideo.NacksReceived);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePlisReceived, &mPublishVideo.PliSReceived);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameRetransmitBitrate, &mPublishVideo.RetransmitBitrate);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTimingFrameInfo, &mPublishVideo.TimingFrameInfo);
      } else if (isLocal && MediaType == "audio") {
        std::unique_lock<std::mutex> lock(SsrcMtx);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameAudioInputLevel, &mPublishAudio.AudioInputLevel);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameBytesSent, &mPublishAudio.BytesSent);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &mPublishAudio.MediaType);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsLost, &mPublishAudio.PacketsLost);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsSent, &mPublishAudio.PacketsSent);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameSsrc, &mPublishAudio.Ssrc);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameTotalAudioEnergy, &mPublishAudio.TotalAudioEnergy);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameTotalSamplesDuration, &mPublishAudio.TotalSamplesDuration);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTransportId, &mPublishAudio.TransportId);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameCodecName, &mPublishAudio.CodecName);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameJitterBufferMs, &mPublishAudio.JitterBufferMs);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameRetransmitBitrate, &mPublishAudio.RetransmitBitrate);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTimingFrameInfo, &mPublishAudio.TimingFrameInfo);
        GetBoolValue(r, webrtc::StatsReport::kStatsValueNameTypingNoiseState, &mPublishAudio.TypingNoiseState);
      } else if (!isLocal && MediaType == "video") {/* Subscribe */
        std::unique_lock<std::mutex> lock(SsrcMtx);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameBytesReceived, &mSubscribeVideo.BytesReceived);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameCodecImplementationName, &mSubscribeVideo.CodecImplementationName);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFramesDecoded, &mSubscribeVideo.FramesDecoded);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &mSubscribeVideo.MediaType);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsLost, &mSubscribeVideo.PacketsLost);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsReceived, &mSubscribeVideo.PacketsReceived);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameQpSum, &mSubscribeVideo.QpSum);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameSsrc, &mSubscribeVideo.Ssrc);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTransportId, &mSubscribeVideo.TransportId);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameCaptureStartNtpTimeMs, &mSubscribeVideo.CaptureStartNtpTimeMs);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameCodecName, &mSubscribeVideo.CodecName);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameContentType, &mSubscribeVideo.ContentType);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameCurrentDelayMs, &mSubscribeVideo.CurrentDelayMs);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodeMs, &mSubscribeVideo.DecodeMs);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFirsSent, &mSubscribeVideo.FirsSent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameHeightReceived, &mSubscribeVideo.FrameHeightReceived);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameRateDecoded, &mSubscribeVideo.FrameRateDecoded);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameRateOutput, &mSubscribeVideo.FrameRateOutput);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameRateReceived, &mSubscribeVideo.FrameRateReceived);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameFrameWidthReceived, &mSubscribeVideo.FrameWidthReceived);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameInitiator, &mSubscribeVideo.Initiator);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameIssuerId, &mSubscribeVideo.IssuerId);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameLocalCertificateId, &mSubscribeVideo.LocalCertificateId);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameMaxDecodeMs, &mSubscribeVideo.MaxDecodeMs);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameNacksSent, &mSubscribeVideo.NacksSent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePlisSent, &mSubscribeVideo.PlisSent);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameRemoteCertificateId, &mSubscribeVideo.RemoteCertificateId);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameSrtpCipher, &mSubscribeVideo.SrtpCipher);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTimingFrameInfo, &mSubscribeVideo.TimingFrameInfo);
      } else if (!isLocal && MediaType == "audio") {
        std::unique_lock<std::mutex> lock(SsrcMtx);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameAudioOutputLevel, &mSubscribeAudio.AudioOutputLevel);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameBytesReceived, &mSubscribeAudio.BytesReceived);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &mSubscribeAudio.MediaType);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsLost, &mSubscribeAudio.PacketsLost);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNamePacketsReceived, &mSubscribeAudio.PacketsReceived);
        GetInt64Value(r, webrtc::StatsReport::kStatsValueNameSsrc, &mSubscribeAudio.Ssrc);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameTotalAudioEnergy, &mSubscribeAudio.TotalAudioEnergy);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameTotalSamplesDuration, &mSubscribeAudio.TotalSamplesDuration);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTransportId, &mSubscribeAudio.TransportId);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameAccelerateRate, &mSubscribeAudio.AccelerateRate);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameCodecName, &mSubscribeAudio.CodecName);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameCurrentDelayMs, &mSubscribeAudio.CurrentDelayMs);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingCNG, &mSubscribeAudio.DecodingCNG);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingCTN, &mSubscribeAudio.DecodingCTN);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingCTSG, &mSubscribeAudio.DecodingCTSG);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingMutedOutput, &mSubscribeAudio.DecodingMutedOutput);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingNormal, &mSubscribeAudio.DecodingNormal);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingPLC, &mSubscribeAudio.DecodingPLC);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameDecodingPLCCNG, &mSubscribeAudio.DecodingPLCCNG);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameExpandRate, &mSubscribeAudio.ExpandRate);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameIssuerId, &mSubscribeAudio.IssuerId);
        GetIntValue(r, webrtc::StatsReport::kStatsValueNameJitterBufferMs, &mSubscribeAudio.JitterBufferMs);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNamePlisSent, &mSubscribeAudio.PlisSent);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNamePreemptiveExpandRate, &mSubscribeAudio.PreemptiveExpandRate);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameSecondaryDecodedRate, &mSubscribeAudio.SecondaryDecodedRate);
        GetFloatValue(r, webrtc::StatsReport::kStatsValueNameSendPacketsDiscarded, &mSubscribeAudio.SendPacketsDiscarded);
        GetStringValue(r, webrtc::StatsReport::kStatsValueNameTimingFrameInfo, &mSubscribeAudio.TimingFrameInfo);
      }
    }
    else if (r->type() == webrtc::StatsReport::kStatsReportTypeBwe && vhStream->mLocal) {
      std::string MediaType;
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &MediaType);
      GetInt64Value(r, webrtc::StatsReport::kStatsValueNameTransmitBitrate, &mPublishVideo.TransmitBitrate);
      GetIntValue(r, webrtc::StatsReport::kStatsValueNameActualEncBitrate, &mOverallBwe.ActualEncBitrate);
      GetIntValue(r, webrtc::StatsReport::kStatsValueNameAvailableReceiveBandwidth, &mOverallBwe.AvailableReceiveBandwidth);
      GetIntValue(r, webrtc::StatsReport::kStatsValueNameAvailableSendBandwidth, &mOverallBwe.AvailableSendBandwidth);
      GetInt64Value(r, webrtc::StatsReport::kStatsValueNameBucketDelay, &mOverallBwe.BucketDelay);
    }
    else if (r->type() == webrtc::StatsReport::kStatsReportTypeTrack) {
      webrtc::StatsReport::StatsType t = r->type();
      GetIntValue(r, webrtc::StatsReport::kStatsValueNameAnaUplinkPacketLossFraction, &mOverallBwe.AnaUplinkPacketLossFraction);
      GetIntValue(r, webrtc::StatsReport::kStatsValueNameTargetDelayMs, &mOverallBwe.TargetDelayMs);
      GetIntValue(r, webrtc::StatsReport::kStatsValueNameTrackId, &mOverallBwe.TrackId);
    }
  }
  
  if (ptrBaseStack->uplogIceCompleted) { /* uplog ice complete */
    ptrBaseStack->uplogIceCompleted = false;
    
    std::shared_ptr<VHLogReport> reportLog = ptrBaseStack->GetReport();
    if (reportLog) {
      Json::Value localAddr(Json::ValueType::objectValue);
      localAddr["ip"] = Json::Value(mCommonAttr.mLocalAddr);
      localAddr["port"] = Json::Value(mCommonAttr.mLocalAddr);
      localAddr["nType"] = Json::Value(mCommonAttr.mNetworkType);
      localAddr["cType"] = Json::Value(mCommonAttr.mCadidateType);

      Json::Value remoteAddr(Json::ValueType::objectValue);
      remoteAddr["ip"] = Json::Value(mCommonAttr.mRemoteAddr);
      remoteAddr["port"] = Json::Value(mCommonAttr.mRemoteAddr);
      remoteAddr["nType"] = Json::Value(mCommonAttr.mNetworkType);
      remoteAddr["cType"] = Json::Value(mCommonAttr.mCadidateType);

      Json::Value dictIce(Json::ValueType::objectValue);
      dictIce["local"] = localAddr;
      dictIce["remote"] = remoteAddr;

      Json::FastWriter root;
      std::string msg = root.write(dictIce);

      std::string key = isLocal ? ulLocalIceState : ulRemoteIceState;
      reportLog->upLog(key, vhStream->mStreamId, msg);
    }
  }

  /* update stream property */
  std::unique_lock<std::mutex> lock_ssrc(vhStream->ssrcMtx);
  if (vhStream->mPublishAudio)
    *vhStream->mPublishAudio = mPublishAudio;
  if (vhStream->mPublishVideo)
    *vhStream->mPublishVideo = mPublishVideo;
  if (vhStream->mSubscribeAudio)
    *vhStream->mSubscribeAudio = mSubscribeAudio;
  if (vhStream->mSubscribeVideo)
    *vhStream->mSubscribeVideo = mSubscribeVideo;
  if (vhStream->mOverallBwe)
    *vhStream->mOverallBwe = mOverallBwe;
  lock_ssrc.unlock();
  LOGD("Finished for Getting Stream Stats, id: %s", ptrBaseStack->GetStreamID());
}

bool GetStatsObserver::OpenUpStats(std::string url) {
  std::unique_lock<std::mutex> lock(mUpStatsMtx);
  if (mUpStats) {
    LOGW("UpStats initialized already");
    return true;
  }
  mUpStats = std::make_shared<UpStats>();
  if (nullptr == mUpStats) {
    LOGE("create UpStats failed");
    return false;
  }
  mUpStats->SetUrl(url);
  mUpStats->Init();
  return true;
}

bool GetStatsObserver::CloseUpStats() {
  std::unique_lock<std::mutex> lock(mUpStatsMtx);
  if (mUpStats) {
    mUpStats->Destroy();
    mUpStats.reset();
  }
  return true;
}

std::string GetStatsObserver::ReportToString(const webrtc::StatsReports & reports, std::string mediaType) {
  std::shared_ptr<VHStream> vhStream = mVhStream.lock();
  if (!vhStream) {
    LOGE("VHStream is reset!");
    return "";
  }
  Json::FastWriter root;
  Json::Value dictdata(Json::ValueType::objectValue);
  dictdata["streamId"] = Json::Value(vhStream->GetID());
  dictdata["userId"] = Json::Value(vhStream->mUser.mId);

  std::chrono::time_point<std::chrono::steady_clock> ts = std::chrono::steady_clock::now();
  int64_t curTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(ts.time_since_epoch()).count() / 1000;
  dictdata["ts"] = curTimeInMs;
  //typedef std::vector<const StatsReport*> StatsReports;
  //std::vector<const StatsReport*>::const_iterator r = reports.begin();
  //for (; r != reports.end(); ++r) {
  for(const auto& r: reports) {
    if (r) {
      bool dropReport = true;
      std::string MediaType = "";
      GetStringValue(r, webrtc::StatsReport::kStatsValueNameMediaType, &MediaType);

      webrtc::StatsReport::StatsType type = (r)->type();
      switch (type) {
      case webrtc::StatsReport::StatsType::kStatsReportTypeBwe:
      case webrtc::StatsReport::StatsType::kStatsReportTypeTransport:
      case webrtc::StatsReport::StatsType::kStatsReportTypeTrack:
        MediaType == mediaType;
        dropReport = false;
        break;
      case webrtc::StatsReport::StatsType::kStatsReportTypeSsrc:
      case webrtc::StatsReport::StatsType::kStatsReportTypeRemoteSsrc:
        dropReport = false;
        break;
      default:
        break;
      }
      if (dropReport || MediaType != mediaType) {
        continue;
      }
      std::map<webrtc::StatsReport::StatsValueName, rtc::scoped_refptr<webrtc::StatsReport::Value>>::const_iterator v = ((r)->values()).begin();
      
      for (; v != (r)->values().end(); ++v) {
        if (v->second) {
          dictdata[v->second->display_name()] = Json::Value(v->second->ToString());
        }
      }
    }
  }
  return root.write(dictdata);
}

bool GetStatsObserver::GetIntValue(const webrtc::StatsReport* report,
  webrtc::StatsReport::StatsValueName name,
  int* value) {
  const webrtc::StatsReport::Value* v = report->FindValue(name);
  if (v) {
    // TODO(tommi): We should really just be using an int here :-/
    *value = rtc::FromString<int>(v->ToString());
  }
  return v != nullptr;
}

bool GetStatsObserver::GetInt64Value(const webrtc::StatsReport* report,
  webrtc::StatsReport::StatsValueName name,
  int64_t* value) {
  const webrtc::StatsReport::Value* v = report->FindValue(name);
  if (v) {
    // TODO(tommi): We should really just be using an int here :-/
    *value = rtc::FromString<int64_t>(v->ToString());
  }
  return v != nullptr;
}

bool GetStatsObserver::GetStringValue(const webrtc::StatsReport* report,
  webrtc::StatsReport::StatsValueName name,
  std::string* value) {
  const webrtc::StatsReport::Value* v = report->FindValue(name);
  if (v)
    *value = v->ToString();
  return v != nullptr;
}

bool GetStatsObserver::GetBoolValue(const webrtc::StatsReport* report,
  webrtc::StatsReport::StatsValueName name,
  bool* value) {
  const webrtc::StatsReport::Value* v = report->FindValue(name);
  if (v) {
    // TODO(tommi): We should really just be using an int here :-/
    *value = rtc::FromString<bool>(v->ToString());
  }
  return v != nullptr;
}

bool GetStatsObserver::GetFloatValue(const webrtc::StatsReport* report,
  webrtc::StatsReport::StatsValueName name,
  float* value) {
  const webrtc::StatsReport::Value* v = report->FindValue(name);
  if (v) {
    // TODO(tommi): We should really just be using an int here :-/
    *value = rtc::FromString<float>(v->ToString());
  }
  return v != nullptr;
}

NS_VH_END