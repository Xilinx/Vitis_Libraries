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
 * @file sbMatMover.hpp
 * @brief data movers for symmetric banded matrix.
 * 
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_SBMATMOVER_HPPPP
#define XF_BLAS_SBMATMOVER_HPPPP

#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {
  template<
    typename t_DataType,
    unsigned int t_ParEntries
  >
  void processUpSbMatStream(
    unsigned int p_n,
    unsigned int p_k,
    hls::stream<WideType<t_DataType, t_ParEntries > > &p_in,
    hls::stream<WideType<t_DataType, t_ParEntries > > &p_out
  ){
    unsigned int l_parBlocks = p_n / t_ParEntries;
    for (unsigned int i=p_k; i>0; --i) {
      uint16_t l_numPaddings = i % t_ParEntries;
      uint16_t l_numFirstEnt = t_ParEntries - l_numPaddings;
      uint16_t j=0;
      uint16_t l_numOut = 0;
      WideType<t_DataType, t_ParEntries> l_intVal;
      #pragma HLS ARRAY_PARTITION variable=l_intVal complete dim=1
      WideType<t_DataType, t_ParEntries> l_out;
      #pragma HLS ARRAY_PARTITION variable=l_out complete dim=1
      
      do {//skip the paddings
      #pragma HLS PIPELINE
        WideType<t_DataType, t_ParEntries> l_val = p_in.read();
        l_intVal = l_val;
        j++;
      } while (i >= (j*t_ParEntries));

      do {//output superdiagonals without paddings
      #pragma HLS PIPELINE
        WideType<t_DataType, t_ParEntries> l_val = p_in.read();
        #pragma HLS ARRAY_PARTITION variable=l_val complete dim=1
        for (unsigned int b=0; b<t_ParEntries; ++b) {
          l_out[b] = (b < l_numFirstEnt)? l_intVal[t_ParEntries-1-b] : l_val[b-l_numFirstEnt];
        }
        p_out.write(l_out);
        l_numOut++;
        l_intVal = l_val;
        j++;
      } while (j<l_parBlocks);

      do {
        for (unsigned int b=0; b<t_ParEntries; ++b){
          l_out[b] = (b < l_numFirstEnt)? l_intVal[t_ParEntries-1-b]: 0;
        }
        p_out.write(l_out);
        l_intVal = 0;
        l_numOut++;
      } while (l_numOut < l_parBlocks);
    }
    for (unsigned int i=0; i<=p_k; ++i) {
      for (unsigned int j=0; j<l_parBlocks; ++j) {
      #pragma HLS PIPELINE
        WideType<t_DataType, t_ParEntries> l_val = p_in.read();
        p_out.write(l_val);
      }
    }
  }

  template<
    typename t_DataType,
    unsigned int t_ParEntries
  >
  void readUpSbMat2Stream(
    unsigned int p_n,
    unsigned int p_k,
    t_DataType *p_a,
    hls::stream<WideType<t_DataType, t_ParEntries > > &p_out
  ) {
    unsigned int l_parBlocks = p_n * (p_k+1) / t_ParEntries;
    unsigned int l_upParBlocks = p_n * p_k / t_ParEntries;
    for (unsigned int i=0; i<l_parBlocks; ++i) {
    #pragma HLS PIPELINE
      WideType<t_DataType, t_ParEntries> l_val;
      #pragma HLS ARRAY_PARTITION variable=l_val complete dim=1 
      for (unsigned int b=0; b < t_ParEntries; ++b) {
        l_val[b] = p_a[i*t_ParEntries+b];
      }
      p_out.write(l_val);
    }
    for (unsigned int i=0; i<l_upParBlocks; ++i) {
    #pragma HLS PIPELINE
      WideType<t_DataType, t_ParEntries> l_val;
      #pragma HLS ARRAY_PARTITION variable=l_val complete dim=1 
      for (unsigned int b=0; b < t_ParEntries; ++b) {
        l_val[b] = p_a[i*t_ParEntries+b];
      }
      p_out.write(l_val);
    }
  } 
  
  template<
    typename t_DataType,
    unsigned int t_ParEntries
  >
  void readVec2SbStream(
    unsigned int p_n,
    unsigned int p_k,
    t_DataType *p_x,
    hls::stream<WideType<t_DataType, t_ParEntries > > &p_out
  ) {
    unsigned int l_parBlocks = p_n / t_ParEntries;
    for (unsigned int i=0; i<= p_k*2; ++i) {
      for (unsigned int j=0; j<l_parBlocks; ++j) {
      #pragma HLS PIPELINE
        WideType<t_DataType, t_ParEntries> l_val;
        for (unsigned int b=0; b < t_ParEntries; ++b) {
          l_val[b] = p_x[j*t_ParEntries+b];
        }
        p_out.write(l_val);
      }
    } 
  } 

/**
 * @brief sbmSuper2Stream function that moves symmetric banded matrix with super diagonals from memory to stream
 *
 * @tparam t_DataType the data type of the matrix entries
 * @tparam t_ParEntries number of parallelly processed entries in the matrix
 * @tparam t_ParBlocks number of t_ParEntries, p_n must be multiple t_ParEntries * t_ParBlocks
 *
 * @param p_n number of rows/cols in a square matrix
 * @param p_k number of superdiagonals 
 * @param p_a a p_n x p_n symmetric banded matrix with on-chip column-major storage and corresponding 0 paddings
 * @param p_out output stream, which is row-aligned with 0 paddings along subdiagonals
 */
template<
  typename t_DataType,
  unsigned int t_ParEntries,
  unsigned int t_ParBlocks=1>
void sbmSuper2Stream(
  unsigned int p_n,
  unsigned int p_k,
  t_DataType *p_a,
  hls::stream<WideType<t_DataType, t_ParEntries > > &p_out
) {
  #ifndef __SYNTHESIS__
    assert ((p_n % (t_ParEntries*t_ParBlocks)) == 0);
  #endif
  #pragma HLS DATAFLOW
    hls::stream<WideType<t_DataType, t_ParEntries > > l_str;
    #pragma HLS DATA_PACK variable=l_str
    readUpSbMat2Stream<t_DataType, t_ParEntries>(p_n, p_k, p_a, l_str);
    processUpSbMatStream<t_DataType, t_ParEntries>(p_n, p_k, l_str, p_out); 
  }//end sbmSuper2Stream
   
/**
 * @brief vec2SbMatStream function that moves vector from memory to stream that matches the sbMat2Stream outputs
 *
 * @tparam t_DataType the data type of the matrix entries
 * @tparam t_ParEntries number of parallelly processed entries in the matrix
 *
 * @param p_n number of rows/cols in a square matrix
 * @param p_k number of superdiagonals 
 * @param p_x vector input
 * @param p_out output stream, which matches the outputs of sbMat2Stream
 */
template<
  typename t_DataType,
  unsigned int t_ParEntries>
void vec2SbMatStream(
  unsigned int p_n,
  unsigned int p_k,
  t_DataType *p_x,
  hls::stream<WideType<t_DataType, t_ParEntries> > &p_out
) {
  #ifndef __SYNTHESIS__
    assert ((p_n % t_ParEntries) == 0);
  #endif
  #pragma HLS DATAFLOW
    hls::stream<WideType<t_DataType, t_ParEntries> > l_str;
    #pragma HLS DATA_PACK variable=l_str
    readVec2SbStream<t_DataType, t_ParEntries>(p_n, p_k, p_x, l_str);
    processUpSbMatStream<t_DataType, t_ParEntries>(p_n, p_k, l_str, p_out); 
} //end vec2SbStream
}
}
}
#endif
