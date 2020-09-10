#pragma once
#include <iostream>
#include <string>

namespace vhall {
  class __declspec(dllexport) StreamCommonAttr {
  public:
    std::string mLocalAddr;
    std::string mRemoteAddr;

    std::string mNetworkType;
    std::string mCadidateType;

  };
  class __declspec(dllexport)SsrcCalcData {
  public:
    int64_t duration = 0; // calc duration in ms
    /* bitrate */
    int64_t Bitrate = 0;
    int64_t lastCalCTime = 0;
    int64_t lastBytesReceived = 0;
    int64_t LastReceivedBytesSeg = 0;
    int64_t LastSentBytesSeg = 0;
    int64_t lastBytesSent = 0;
    /* packetsLostRate */
    double PacketsLostRate = 0;
    int LastPacketReceivedSeg = 0;
    int LastPacketsLostSeg = 0;
    int LastPacketsLost = 0;
    int LastPacketsReceived = 0;
    int LastPacketsSent = 0;
  };

  struct __declspec(dllexport)ReceiveVideoSsrc {
  public:
    ReceiveVideoSsrc() {};
    int64_t BytesReceived = 0;
    std::string CodecImplementationName = "";
    int FramesDecoded = 0;
    std::string MediaType = "";
    int PacketsLost = 0;
    int PacketsReceived = 0;
    int64_t QpSum = 0;
    int64_t Ssrc = 0;
    std::string TransportId = "";
    int64_t CaptureStartNtpTimeMs = 0;
    std::string CodecName = "";
    std::string ContentType = "";
    int CurrentDelayMs = 0;
    int DecodeMs = 0;
    int FirsSent = 0;
    int FrameHeightReceived = 0;
    int FrameRateDecoded = 0;
    int FrameRateOutput = 0;
    int FrameRateReceived = 0;
    int FrameWidthReceived = 0;
    int64_t Initiator = 0;
    int IssuerId = 0;
    int LocalCertificateId = 0;
    int MaxDecodeMs = 0;
    int NacksSent = 0;
    int PlisSent = 0;
    int RemoteCertificateId = 0;
    int SrtpCipher = 0;
    float Rtt = 0;
    std::string TimingFrameInfo = "";
    SsrcCalcData CalcData;
  public:
    void calc();
  };

  struct __declspec(dllexport)ReceiveAudioSsrc {
  public:
    int AudioOutputLevel = 0;
    int64_t BytesReceived = 0;
    std::string MediaType = "";
    int PacketsLost = 0;
    int PacketsReceived = 0;
    int64_t Ssrc = 0;
    float TotalAudioEnergy = 0;
    float TotalSamplesDuration = 0;
    std::string TransportId = "";
    float AccelerateRate = 0;
    std::string CodecName = "";
    int CurrentDelayMs = 0;
    int DecodingCNG = 0;
    int DecodingCTN = 0;
    int DecodingCTSG = 0;
    int DecodingMutedOutput = 0;
    int DecodingNormal = 0;
    int DecodingPLC = 0;
    int DecodingPLCCNG = 0;
    float ExpandRate = 0;
    int IssuerId = 0;
    int JitterBufferMs = 0;
    float PlisSent = 0;
    float PreemptiveExpandRate = 0;
    float Rtt = 0;
    float SecondaryDecodedRate = 0;
    float SendPacketsDiscarded = 0;
    std::string TimingFrameInfo = "";
    SsrcCalcData CalcData;
  public:
    void calc();
  };

  struct __declspec(dllexport)SendVideoSsrc {
  public:
    SendVideoSsrc() {};
    int64_t BytesSent = 0;
    std::string CodecImplementationName = "";
    int FramesEncoded = 0;
    std::string MediaType = "";
    int PacketsLost = 0;
    int PacketsSent = 0;
    int QpSum = 0;
    int64_t Ssrc = 0;
    std::string TransportId = "";
    int AdaptationChanges = 0;
    int AvgEncodeMs = 0;
    bool BandwidthLimitedResolution = 0;
    std::string CodecName = "";
    std::string ContentType = "";
    bool CpuLimitedResolution = 0;
    int EncodeUsagePercent = 0;
    int FrameHeightInput = 0;
    int FrameHeightSent = 0;
    int FrameRateInput = 0;
    int FrameRateSent = 0;
    int FrameWidthInput = 0;
    int FrameWidthSent = 0;
    bool HasEnteredLowResolution = 0;
    int MinPlayoutDelayMs = 0;
    int NacksReceived = 0;
    int FirsReceived = 0;
    int PliSReceived = 0;
    int64_t RetransmitBitrate = 0;
    int64_t TransmitBitrate = 0;
    float Rtt = 0;
    std::string TimingFrameInfo = "";
    SsrcCalcData CalcData;
  public:
    void calc();
  };

  struct __declspec(dllexport)SendAudioSsrc {
  public:
    SendAudioSsrc() {};
    int AudioInputLevel = 0;
    int64_t BytesSent = 0;
    std::string MediaType = "";
    int PacketsLost = 0;
    int PacketsSent = 0;
    int64_t Ssrc = 0;
    float TotalAudioEnergy = 0;
    float TotalSamplesDuration = 0;
    std::string TransportId = "";
    std::string CodecName = "";
    int JitterBufferMs = 0;
    int64_t RetransmitBitrate = 0;
    float Rtt = 0;
    int64_t TransmitBitrate = 0;
    std::string TimingFrameInfo = "";
    bool TypingNoiseState = 0;
    SsrcCalcData CalcData;
  public:
    void calc();
  };

  struct __declspec(dllexport)Bwe {
  public:
    Bwe() {};
    int ActualEncBitrate = 0;
    int AvailableReceiveBandwidth = 0;
    int AvailableSendBandwidth = 0;
    int64_t BucketDelay = 0;
    int AnaUplinkPacketLossFraction = 0;
    int TargetDelayMs = 0;
    int TrackId = 0;
  };
}