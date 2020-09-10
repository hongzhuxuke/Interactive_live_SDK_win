//
//  data_combine_split.cpp
//  VhallLiveApi
//
//  Created by ilong on 2017/6/22.
//  Copyright © 2017年 vhall. All rights reserved.
//
#include<string>
#include "data_combine_split.h"

DataCombineSplit::DataCombineSplit():mBufferData(NULL){
   
}

DataCombineSplit::~DataCombineSplit(){
   Destory();
}

void DataCombineSplit::ClearBuffer(){
   if (mBufferData) {
      mBufferData->ClearData();
   }
}

void DataCombineSplit::SetOutputDataDelegate(const OutputDataDelegate &delegate){
   mOutputDataDelegate = delegate;
}

int DataCombineSplit::Init(const int output_size){
  mBufferData.reset();
  mBufferData.reset(new BufferData(output_size));
  return 0;
}

void DataCombineSplit::Destory(){
  mBufferData.reset();
}

int DataCombineSplit::DataCombineSplitProcess(const int8_t*input_data,const int size){
   if (mBufferData==NULL) {
      return -1;
   }
   int dataSzie = size;
   int readPosition = 0; 
   while (true) {
      int dataSpareSize = mBufferData->mBufferSize-mBufferData->mDataSize;
      if (dataSzie>=dataSpareSize) {
        memcpy(mBufferData->mData.get() + mBufferData->mDataSize, input_data + readPosition, dataSpareSize);
         readPosition += dataSpareSize;
         dataSzie -= dataSpareSize;
         mBufferData->mDataSize += dataSpareSize;
         if (mOutputDataDelegate) {
            mOutputDataDelegate(mBufferData->mData.get(), mBufferData->mDataSize);
         }
         mBufferData->mDataSize = 0;
      }else{
        memcpy(mBufferData->mData.get() + mBufferData->mDataSize, input_data + readPosition, dataSzie);
         readPosition -= dataSzie;
         mBufferData->mDataSize += dataSzie;
         dataSzie = 0;
      }
      if (dataSzie<=0) {
         break;
      }
   }
   return 0;
}
