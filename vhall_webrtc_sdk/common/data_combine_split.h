//
//  data_combine_split.hpp
//  VhallLiveApi
//
//  Created by ilong on 2017/6/22.
//  Copyright © 2017年 vhall. All rights reserved.
//

#ifndef data_combine_split_h
#define data_combine_split_h

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <functional>

typedef std::function<void(const int8_t* audio_data, const int size)> OutputDataDelegate;

class BufferData {
   
public:
   BufferData(int buffer_size):mBufferSize(buffer_size),mDataSize(0),mData(NULL){
      mData.reset(new int8_t[buffer_size]);
      if (mData) {
        memset(mData.get(), buffer_size, 0);
      }
   }
   ~BufferData(){
     mData.reset();
   }
   void ClearData(){
      mDataSize = 0;
   }
   int  mBufferSize = 0;
   int  mDataSize = 0;
   std::shared_ptr<int8_t[] > mData = nullptr;
};

class DataCombineSplit {
   
public:
   DataCombineSplit();
   ~DataCombineSplit();
   
   /**
    设置输出数据回调

    @param delegate
    */
   void SetOutputDataDelegate(const OutputDataDelegate &delegate);
   
   /**
    清空buffer中的数据
    */
   void ClearBuffer();
   
   /**
    初始化对象

    @param output_size 输出数据的大小
    @return 0 是成功，非0失败
    */
   int Init(const int output_size);
   
   /**
    数据处理过程

    @param input_data 输入数据
    @param size 数据大小
    @return 0 是成功，非0失败
    */
   int DataCombineSplitProcess(const int8_t* input_data, const int size);
   
private:
   void Destory();
   std::shared_ptr<BufferData> mBufferData;
   OutputDataDelegate  mOutputDataDelegate;
};

#endif /* data_combine_split_hpp */
