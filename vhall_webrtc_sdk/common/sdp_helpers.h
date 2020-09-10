//
//  sdp_helpers.hpp
//  VhallSignalingDynamic
//
//  Created by ilong on 2018/10/29.
//  Copyright © 2018 ilong. All rights reserved.
//

#ifndef sdp_helpers_h
#define sdp_helpers_h

#include <cstdio>
#include "vhall_log.h"
#include "vhall_define.h"
#include <string>
#include <vector>

NS_VH_BEGIN

class SdpHelpers {
   
public:
   template<typename T>
   static T StringTo(std::string str);
   
   template<typename T>
   static std::string ToString(T arg);
   
   // return sdp
   static std::string EnableSimulcast(const std::string &sdp, const int numSpatialLayers);
   
private:
   SdpHelpers();
   ~SdpHelpers();
   static std::vector<std::string> GetFID(const std::string &sdp);
   static std::string GetSsrcTagValue(const std::string &sdp, uint64_t fid, std::string &tag);
   static std::string AddSim(std::vector<uint64_t> &spatialLayers);
   static std::string AddGroup(uint64_t spatialLayerId, uint64_t spatialLayerIdRtx);
   static std::string AddSpatialLayer(std::string &cname, std::string &msid, std::string &mslabel, std::string &label, uint64_t spatialLayerId, uint64_t spatialLayerIdRtx);
   static void StrReplace(std::string &baseString, std::string &src, std::string &dst);
   static std::string RemoveSsrcLine(std::string &sdp ,uint64_t fid);
   static void StringReplace( std::string &strBig, const std::string &strsrc, const std::string &strdst);
};

NS_VH_END

#endif /* sdp_helpers_hpp */
