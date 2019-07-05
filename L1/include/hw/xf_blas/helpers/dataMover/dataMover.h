/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file utils.h
 * @brief common datatypes for L1 modules.
 * 
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_DATAMOVER_H
#define XF_BLAS_DATAMOVER_H

#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {

template<typename t_DataType>
void mem2stream(
  unsigned int p_n,
  t_DataType *p_in,
  hls::stream<t_DataType> &p_out
) {
  for (unsigned int i=0; i<p_n; ++i) {
  #pragma HLS PIPELINE
    t_DataType l_val = p_in[i];
    p_out.write(l_val);
  }
} //end mem2stream

template<typename t_DataType>
void stream2mem(
  unsigned int p_n,
  hls::stream<t_DataType> &p_in,
  t_DataType *p_out
) {
  for (unsigned int i=0; i<p_n; ++i) {
  #pragma HLS PIPELINE
    t_DataType l_val = p_in.read();
    p_out[i] = l_val;
  }
} //end stream2mem


template<
  typename t_DataType,
  unsigned int t_DataWidth,
  unsigned int t_ParEntries>
void readVec2Stream(
  t_DataType *p_in,
  unsigned int p_n,
  hls::stream<WideType<t_DataType, t_ParEntries, t_DataWidth > > &p_out
) {
  #ifndef __SYNTHESIS__
    assert ((p_n % t_ParEntries) == 0);
  #endif
  unsigned int l_parBlocks = p_n / t_ParEntries;
  for (unsigned int i=0; i<l_parBlocks; ++i) {
  #pragma HLS PIPELINE
    BitConv<t_DataType> l_bitConv;
    WideType<t_DataType, t_ParEntries, t_DataWidth > l_val;
    for (unsigned int j=0; j<t_ParEntries; ++j) {
      l_val[j] = p_in[i*t_ParEntries + j];
    }
    p_out.write(l_val);
  }
} //end readVec2Stream

template<
  typename t_DataType,
  unsigned int t_DataWidth,
  unsigned int t_ParEntries>
void writeStream2Vec(
  hls::stream<WideType<t_DataType, t_ParEntries, t_DataWidth > > &p_in,
  unsigned int p_n,
  t_DataType *p_out
) {
//#pragma HLS DATA_PACK variable=p_in
  #ifndef __SYNTHESIS__
    assert ((p_n % t_ParEntries) == 0);
  #endif
  unsigned int l_parBlocks = p_n / t_ParEntries;
  for (unsigned int i=0; i<l_parBlocks; ++i) {
  #pragma HLS PIPELINE
    BitConv<t_DataType> l_bitConv;
    WideType<t_DataType, t_ParEntries, t_DataWidth > l_val;
    l_val = p_in.read();
    for (unsigned int j=0; j<t_ParEntries; ++j) {
      p_out[i*t_ParEntries + j]=l_val[j];
    }
  }
} //end writeStream2Vec


}
}
}
#endif
