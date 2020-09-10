//
//  sdp_helpers.cpp
//  VhallSignalingDynamic
//
//  Created by ilong on 2018/10/29.
//  Copyright © 2018 ilong. All rights reserved.
//

#include "sdp_helpers.h"
#include <sstream>
#include <regex>

NS_VH_BEGIN

SdpHelpers::SdpHelpers(){
   
}

SdpHelpers::~SdpHelpers(){
   
}

template <typename T>
T SdpHelpers::StringTo(std::string str) {
   T tmp;
   std::stringstream ss;
   ss << str;
   ss >> tmp;
   return tmp;
}

template<typename T>
std::string SdpHelpers::ToString(T arg){
   std::stringstream ss;
   ss << arg;
   return ss.str();
}

std::string SdpHelpers::EnableSimulcast(const std::string &sdp, const int numSpatialLayers){
   std::string sdpStr = sdp;
   uint64_t baseSsrc = 0, baseSsrcRtx = 0;
   std::vector<std::string> results = SdpHelpers::GetFID(sdp);
   if (results.size()>2) {
      baseSsrc = SdpHelpers::StringTo<uint64_t>(results[1]);
      baseSsrcRtx = SdpHelpers::StringTo<uint64_t>(results[2]);
   }else{
      LOGW("fid is empty.");
      return "";
   }
   std::string tag = "cname";
   std::string cname = SdpHelpers::GetSsrcTagValue(sdp, baseSsrc, tag);
   if (cname.length()<=0) {
      LOGW("cname length is 0.");
      return "";
   }
   tag = "msid";
   std::string msid = SdpHelpers::GetSsrcTagValue(sdp, baseSsrc, tag);
   if (msid.length()<=0) {
      LOGW("msid length is 0.");
      return "";
   }
   tag = "mslabel";
   std::string mslabel = SdpHelpers::GetSsrcTagValue(sdp, baseSsrc, tag);
   if (mslabel.length()<=0) {
      LOGW("mslabel length is 0.");
      return "";
   }
   tag = "label";
   std::string label = SdpHelpers::GetSsrcTagValue(sdp, baseSsrc, tag);
   if (label.length()<=0) {
      LOGW("label length is 0.");
      return "";
   }
   
   sdpStr = SdpHelpers::RemoveSsrcLine(sdpStr, baseSsrc);
   sdpStr = SdpHelpers::RemoveSsrcLine(sdpStr, baseSsrcRtx);
   
   std::vector<uint64_t> spatialLayers;
   spatialLayers.push_back(baseSsrc);
   std::vector<uint64_t> spatialLayersRtx;
   spatialLayersRtx.push_back(baseSsrcRtx);
   for (int i = 1; i<numSpatialLayers; i++) {
      spatialLayers.push_back(baseSsrc+i*1000);
      spatialLayersRtx.push_back(baseSsrcRtx+i*1000);
   }
   
   std::string result = SdpHelpers::AddSim(spatialLayers);
   for (int i = 0; i < spatialLayers.size(); i++) {
      result += SdpHelpers::AddGroup(spatialLayers[i], spatialLayersRtx[i]);
   }
   
   for (int i = 0; i < spatialLayers.size(); i++) {
      result += SdpHelpers::AddSpatialLayer(cname, msid, mslabel, label, spatialLayers[i], spatialLayersRtx[i]);
   }
   result += "a=x-google-flag:conference\r\n";
   SdpHelpers::StrReplace(sdpStr, results[0], result);
   return sdpStr;
}

std::vector<std::string> SdpHelpers::GetFID(const std::string &sdp){
   std::string pattern{ "a=ssrc-group:FID ([0-9]*) ([0-9]*)\r?\n" }; // id card
   std::regex re(pattern);
   std::smatch results;
   std::vector<std::string> res;
   res.clear();
   bool ret = std::regex_search(sdp, results, re);
   if (ret) {
      for (auto item:results) {
         res.push_back(item.str());
      }
   }
   return res;
}

std::string SdpHelpers::GetSsrcTagValue(const std::string &sdp, uint64_t fid, std::string &tag){
   std::stringstream ss;
   ss << "a=ssrc:" << fid <<" "<< tag <<":(.*)\r?\n";
   std::regex re(ss.str());
   std::smatch results;
   bool ret = std::regex_search(sdp, results, re);
   if (ret) {
      return results[1].str();
   }
   return "";
}

std::string SdpHelpers::AddSim(std::vector<uint64_t> &spatialLayers){
   std::string line{"a=ssrc-group:SIM"};
   for (auto item:spatialLayers) {
      line+=" "+SdpHelpers::ToString(item);
   }
   return line+"\r\n";
}

std::string SdpHelpers::AddGroup(uint64_t spatialLayerId, uint64_t spatialLayerIdRtx){
   return "a=ssrc-group:FID "+SdpHelpers::ToString(spatialLayerId)+" "+SdpHelpers::ToString(spatialLayerIdRtx)+"\r\n";
}

std::string SdpHelpers::AddSpatialLayer(std::string &cname, std::string &msid, std::string &mslabel, std::string &label, uint64_t spatialLayerId, uint64_t spatialLayerIdRtx){
   return "a=ssrc:"+SdpHelpers::ToString(spatialLayerId)+" cname:"+cname+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerId)+" msid:"+msid+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerId)+" mslabel:"+mslabel+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerId)+" label:"+label+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerIdRtx)+" cname:"+cname+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerIdRtx)+" msid:"+msid+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerIdRtx)+" mslabel:"+mslabel+"\r\n"+
          "a=ssrc:"+SdpHelpers::ToString(spatialLayerIdRtx)+" label:"+label+"\r\n";
}

void SdpHelpers::StrReplace(std::string &baseString, std::string &src, std::string &dst){
   SdpHelpers::StringReplace(baseString, src, dst);
}

void SdpHelpers::StringReplace( std::string &strBig, const std::string &strsrc, const std::string &strdst){
   std::string::size_type pos = 0;
   std::string::size_type srclen = strsrc.size();
   std::string::size_type dstlen = strdst.size();
   
   while( (pos=strBig.find(strsrc, pos)) != std::string::npos ){
      strBig.replace( pos, srclen, strdst );
      pos += dstlen;
   }
}

std::string SdpHelpers::RemoveSsrcLine(std::string &sdp ,uint64_t fid){
   std::string retStr = sdp;
   std::stringstream ss;
   ss << "a=ssrc:" << fid <<".*\r?\n";
   std::regex re(ss.str());
   std::smatch results;
   //多次匹配
   while (std::regex_search(retStr, results, re)) {
      for (auto item:results) {
         SdpHelpers::StringReplace(retStr, item.str(), std::string(""));
      }
   }
   return retStr;
}

NS_VH_END
