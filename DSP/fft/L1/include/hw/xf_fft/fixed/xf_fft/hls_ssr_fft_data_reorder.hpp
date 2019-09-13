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

#ifndef HLS_SSR_FFT_DATA_REORDER_H
#define HLS_SSR_FFT_DATA_REORDER_H

#ifndef __SYNTHESIS__
#include <math.h>
#include <iostream>
#endif
#include <complex>
#include "xf_fft/hls_ssr_fft_utilities.hpp"
#include "xf_fft/hls_ssr_fft_read_barrel_shifter.hpp"
#include "xf_fft/hls_ssr_fft_output_reorder.hpp"
#include "xf_fft/fft_complex.hpp"

/*
 =========================================================================================
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-

 The digitReversedDataReOrder function is used to perform data re ordering at
 the output of final stage in SSR FFT. The data produced by final stage is
 shuffled in SSR dimension ( R or SSR streams coming from SSR FFT last stage)
 and also it needs re ordering in time dimension. The re or ordering is done
 using two PIPO buffers, in 4 phases.

 1- The input streams are rotated in SSR dimension ( R Streams)
 2- The input stream written to PIPO after rotation
 3- The ping pong memory is read
 4- The read data is shuffled and written to output
 This file defines functions for phases 1,2,3
 cacheData function : deals with phase 1 and 2
 WriteCacheData  function deals with phase  3,4
 and internally calls
 MemReadBarrelShifter::readMemAndBarrelShift
 Note : This function only deals with the cases when t_L is integer power of
 t_R , for the cases when t_L is not integer power of t_R :
 OutputDataReOrder<>::digitReversal2Phase<> is used which
 deals with it through different specializations.

 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 -_-                                                                                   -_-
 ========================================================================================
 */

namespace xf {
namespace dsp {
namespace fft {

template <int t_L, int t_R, typename T_dtype>
void cacheDataDR(std::complex<T_dtype> p_inData[t_R][t_L / t_R],
                 std::complex<T_dtype> p_digitReseversedOutputBuff[t_R][t_L / t_R]) {
#pragma HLS INLINE off
/*
 #pragma HLS ARRAY_PARTITION variable=p_inData complete dim=1
 #pragma HLS ARRAY_PARTITION variable=p_digitReseversedOutputBuff complete dim=1
 */

cacheDataDR_LOverRLooP:
    for (int r = 0; r < (t_L / t_R); r++) {
#pragma HLS PIPELINE II = 1 rewind

        std::complex<T_dtype> temp[t_R];
#pragma HLS ARRAY_PARTITION variable = temp complete dim = 1
#pragma HLS DATA_PACK variable = temp

    cacheDataDR_SSRLoop1:
        for (int c = 0; c < t_R; c++) {
            // int cdash = (c +  r / ( t_L / (t_R*t_R) )   )%t_R;
            // replaced//int cdash = (c +  (r*t_R*t_R) / ( t_L  )   )%t_R;
            int cdash = (c + ((r) >> (ssrFFTLog2<t_L / (t_R * t_R)>::val))) & (ssrFFTLog2BitwiseAndModMask<t_R>::val);
            // CHECK_COVEARAGE;
            temp[cdash] = p_inData[c][r]; // Read in Order :: Should be a stream
        }
    cacheDataDR_SSRLoop2:
        for (int c = 0; c < t_R; c++) {
            p_digitReseversedOutputBuff[c][r] = temp[c];
        }
    }
}

template <int t_L, int t_R, typename T_in, typename T_out>
void cacheDataDR(std::complex<T_in> p_inData[t_R][t_L / t_R],
                 std::complex<T_out> p_digitReseversedOutputBuff[t_R][t_L / t_R]) {
#pragma HLS INLINE off
    const unsigned int log2_radix = (ssrFFTLog2<t_R>::val);
cacheDataDR_LOverRLooP:
    for (int r = 0; r < (t_L / t_R); r++) {
#pragma HLS PIPELINE II = 1 rewind

        std::complex<T_in> temp[t_R];
#pragma HLS ARRAY_PARTITION variable = temp complete dim = 1
#pragma HLS DATA_PACK variable = temp

    cacheDataDR_SSRLoop1:
        for (int c = 0; c < t_R; c++) {
            // replaced//int cdash = (c +  (r*t_R*t_R) / ( t_L  )   )%t_R;
            int cdash = (c + (r >> (ssrFFTLog2<t_L / (t_R * t_R)>::val))) & (ssrFFTLog2BitwiseAndModMask<t_R>::val);
            // CHECK_COVEARAGE;

            temp[cdash] = p_inData[c][r]; // Read in Order :: Should be a stream
        }
    cacheDataDR_SSRLoop2:
        for (int c = 0; c < t_R; c++) {
            p_digitReseversedOutputBuff[c][r] = temp[c];
        }
    }
}

template <int t_L, int t_R, typename T_dtype>
void writeBackCacheDataDR(std::complex<T_dtype> p_digitReseversedOutputBuff[t_R][t_L / t_R],
                          std::complex<T_dtype> p_outData[t_R][t_L / t_R]) {
#pragma HLS INLINE off

    const unsigned int log2_radix = (ssrFFTLog2<t_R>::val);

writeBackCacheDataDR_LOverRLoop:
    for (int r = 0; r < (t_L / t_R); r++) {
#pragma HLS PIPELINE II = 1 rewind // This loop has rewind issue : VERIFIED

        std::complex<T_dtype> temp[t_R];
#pragma HLS ARRAY_PARTITION variable = temp complete dim = 1
#pragma HLS DATA_PACK variable = temp

        unsigned int lin_index = (r << log2_radix) | 0; // equivalent to : r*t_R + c;
        unsigned int bitReversedIndex = digitReversalFractionIsLSB<t_L, t_R>(lin_index);
        unsigned int out_r = bitReversedIndex >> log2_radix;             // equivalent to :  bitReversedIndex / t_R;
        unsigned int out_c = bitReversedIndex & ((1 << log2_radix) - 1); // equivalent to:bitReversedIndex % t_R;
        // int offset = (out_c  +  (out_r  /  ( t_L / (t_R*t_R) )    ) ) %t_R;//int out_cDash = (out_c  +  (out_r/t_R) )
        // %t_R; // ((r>>log2_radix) + c)%t_R;     //  int offset = (out_c  +  ( (out_r *t_R*t_R) /  ( t_L  )    ) )
        // %t_R;//int out_cDash = (out_c  +  (out_r/t_R) ) %t_R; // ((r>>log2_radix) + c)%t_R;     //  replaced//
        // int offset = (out_c  +  ( (out_r *t_R*t_R) /  ( t_L  )    ) ) %t_R;//int out_cDash = (out_c  +  (out_r/t_R) )
        // %t_R; // ((r>>log2_radix) + c)%t_R;     //
        int offset = (out_c + (out_r >> (ssrFFTLog2<t_L / (t_R * t_R)>::val))) &
                     (ssrFFTLog2BitwiseAndModMask<t_R>::val); // int out_cDash = (out_c  +  (out_r/t_R) ) %t_R; //
                                                              // ((r>>log2_radix) + c)%t_R;     //

        MemReadBarrelShifter<t_R> readBarrelShifterObj;
        readBarrelShifterObj.template readMemAndBarrelShift<t_R, t_L, std::complex<T_dtype> >(
            r, offset, p_digitReseversedOutputBuff, temp);
        for (int c = 0; c < t_R; c++) {
            p_outData[c][r] = temp[c]; // p_outData is written in order should be a stream
        }
    }
}

template <int t_L, int t_R, typename T_in, typename T_out>
void writeBackCacheDataDR(std::complex<T_in> p_digitReseversedOutputBuff[t_R][t_L / t_R],
                          std::complex<T_out> p_outData[t_R][t_L / t_R]) {
// CHECK_COVEARAGE;

#pragma HLS INLINE off

    const unsigned int log2_radix = (ssrFFTLog2<t_R>::val);

writeBackCacheDataDR_LOverRLoop:
    for (int r = 0; r < (t_L / t_R); r++) {
#pragma HLS PIPELINE II = 1 rewind // This loop has rewind issue

        std::complex<T_in> temp[t_R];
#pragma HLS ARRAY_PARTITION variable = temp complete dim = 1
#pragma HLS DATA_PACK variable = temp

        unsigned int lin_index = (r << log2_radix) | 0; // equivalent to : r*t_R + c;
        unsigned int bitReversedIndex = digitReversalFractionIsLSB<t_L, t_R>(lin_index);
        unsigned int out_r = bitReversedIndex >> log2_radix;             // equivalent to :  bitReversedIndex / t_R;
        unsigned int out_c = bitReversedIndex & ((1 << log2_radix) - 1); // equivalent to:bitReversedIndex % t_R;
        // int offset = (out_c  +  (out_r  /  ( t_L / (t_R*t_R) )    ) ) %t_R;//int out_cDash = (out_c  +  (out_r/t_R) )
        // %t_R; // ((r>>log2_radix) + c)%t_R;     //  int offset = (out_c  +  ( (out_r*t_R*t_R)/( t_L )    ) )
        // %t_R;//int out_cDash = (out_c  +  (out_r/t_R) ) %t_R; // ((r>>log2_radix) + c)%t_R;     //  replaced// int
        // offset = (out_c  +  ( (out_r *t_R*t_R) /  ( t_L  )    ) ) %t_R;//int out_cDash = (out_c  +  (out_r/t_R) )
        // %t_R; // ((r>>log2_radix) + c)%t_R;     //
        int offset = (out_c + (out_r >> (ssrFFTLog2<t_L / (t_R * t_R)>::val))) &
                     (ssrFFTLog2BitwiseAndModMask<t_R>::val); // int out_cDash = (out_c  +  (out_r/t_R) ) %t_R; //
                                                              // ((r>>log2_radix) + c)%t_R;     //
        // MemReadBarrelShifter<t_R> readBarrelShifterObj;
        // readBarrelShifterObj.template readMemAndBarrelShift< t_R,t_L,std::complex<T_in>
        // >(r,offset,p_digitReseversedOutputBuff, temp );
        for (int c = 0; c < t_R; c++) {
#pragma HLS UNROLL
            unsigned int lin_index1 = (r << log2_radix) | ((t_R + c - offset) % t_R); // equivalent to : r*t_R + c;
            unsigned int bitReversedIndex1 = digitReversal<t_L, t_R>(lin_index1);
            unsigned int out_r = bitReversedIndex1 >> log2_radix; // equivalent to :  bitReversedIndex / t_R;
            // replaced//out[c]= in[(c+(stage-1))%t_R][out_r];
            temp[(t_R + c - offset) % t_R] = p_digitReseversedOutputBuff[c][out_r];
        }
        //			CHECK_COVEARAGE;
        for (int c = 0; c < t_R; c++) {
            p_outData[c][r] = temp[c]; // p_outData is written in order should be a stream
        }
    }
}

template <int t_L, int t_R, typename T_in, typename T_out>
void digitReversedDataReOrder(std::complex<T_in> p_inData[t_R][t_L / t_R],
                              std::complex<T_out> p_outData[t_R][t_L / t_R]) {
#pragma HLS INLINE

    const unsigned int log2_radix = (ssrFFTLog2<t_R>::val);

    std::complex<T_in> digitReverseBuff[t_R][t_L / t_R];
#pragma HLS ARRAY_PARTITION variable = digitReverseBuff complete dim = 1
#pragma HLS DATA_PACK variable = digitReverseBuff
    //#pragma HLS STREAM variable=digitReverseBuff off depth=4 dim=2

    cacheDataDR<t_L, t_R, T_in, T_in>(p_inData, digitReverseBuff);
    writeBackCacheDataDR<t_L, t_R, T_in, T_out>(digitReverseBuff, p_outData);
}

template <int t_L, int t_R, typename T_in, typename T_out>
void digitReversalSimulationModel(T_in p_inData[t_R][t_L / t_R], T_out p_outData[t_R][t_L / t_R]) {
    unsigned int ind1 = 0;
    unsigned int revind = 0;

    for (int i = 0; i < t_R; i++) {
        for (int j = 0; j < t_L / t_R; j++) {
            ind1 = j * (t_R) + i;
            revind = digitReversalFractionIsLSB<t_L, t_R>(ind1);
            p_outData[revind % t_R][revind / t_R] = p_inData[i][j];
        }
    }
}

#if 0 // Initial simulation model for SSR FFT data commutors
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
  
  template < int t_L, int t_R,int STAGE,typename T_inType, typename T_outType>
  void reOrderDataStatic(T_inType p_inData[t_R][t_L/t_R], T_outType p_outData[t_R][t_L/t_R])
  { 
    ////#pragma HLS LOOP_MERGE
    
    //#pragma HLS INLINE off
    
    const int no_of_fft_in_stage = ssrFFTPow<t_R,STAGE>::val;//(int)pow((double)(t_R), double(STAGE));
    const int fft_len_in_stage = t_L / no_of_fft_in_stage;
    const int no_bflys_per_fft = fft_len_in_stage / t_R;
    const int no_samples_per_bfly = t_R;
    //static_assert(STAGE - 1 <= ((_log2<LEN>::val) - _log2<SSR>::val), "Stage no. should be less or equal of log Radix of FFT length minus 1");
    
    //assert(STAGE <_log2<LEN>::val/ _log2<SSR>::val);
    FFTs_LOOP:
    for (int f = 0;f < no_of_fft_in_stage;f++)
    { 
      BFLYs_LOOP:
      for (int bf = 0;bf < no_bflys_per_fft;bf++)

      //#pragma HLS PIPELINE II=1
      { 
        RADIX_LOOP:
        for (int r = 0;r < no_samples_per_bfly;r++)

        //#pragma HLS UNROLL
        { 
          // butterfly number r sample;
          int in_index = (fft_len_in_stage*f + bf*t_R + r);
          int idx1 = in_index / (t_R);
          int idx2 = in_index % t_R;
          int out_index = (bf + r*(fft_len_in_stage / t_R) + (fft_len_in_stage*f));
          int odx1 = out_index / ( t_R);
          int odx2 = out_index % t_R;

#ifdef FUNCTION_INTERNALS
          std::cout << "in_index:" << in_index << std::endl;
          std::cout << "Out_index:" << out_index << std::endl;
#endif // FUNCTION_INTERNALS
          
          p_outData[idx2][idx1] = p_inData[odx2][odx1];
          /*out[idx1][idx2].real = in[odx1][odx2].real;
           out[idx1][idx2].imag = in[odx1][odx2].imag;*/
        }
      }

    }
  }

  template <typename T_dtype, int t_L, int t_R>
  void reOrderDataDynamic(std::complex<T_dtype> p_inData[t_R][t_L/t_R], std::complex<T_dtype> p_outData[t_R][t_L/t_R], int p_stage)
  { 
    int no_of_fft_in_stage = (int)pow((double)(t_R), double(p_stage));
    int fft_len_in_stage = t_L / no_of_fft_in_stage;
    int no_bflys_per_fft = fft_len_in_stage / t_R;
    int no_samples_per_bfly = t_R;
    //static_assert(p_stage - 1 <= ((_log2<LEN>::val) - _log2<SSR>::val), "Stage no. should be less or equal of log Radix of FFT length minus 1");
    
    //assert(p_stage <_log2<LEN>::val/ _log2<SSR>::val);
    FFTs_LOOP:
    for (int f = 0;f < no_of_fft_in_stage;f++)
    { 
      BFLYs_LOOP:
      for (int bf = 0;bf < no_bflys_per_fft;bf++)
      { 
        RADIX_LOOP:
        for (int r = 0;r < no_samples_per_bfly;r++)
        { 
          // butterfly number r sample;
          int in_index = (fft_len_in_stage*f + bf*t_R + r);
          int idx1 = in_index / (t_R);
          int idx2 = in_index % t_R;
          int out_index = (bf + r*(fft_len_in_stage / t_R) + (fft_len_in_stage*f));
          int odx1 = out_index / ( t_R);
          int odx2 = out_index % t_R;

#ifdef FUNCTION_INTERNALS
          std::cout << "in_index:" << in_index << std::endl;
          std::cout << "Out_index:" << out_index << std::endl;
#endif // FUNCTION_INTERNALS
          
          p_outData[idx2][idx1] = p_inData[odx2][odx1];
        }
      }

    }
  }

  template < int t_L, int t_R, int STAGE,typename T_dtype>
  void reverseReOrderDataStatic(T_dtype p_inData[t_R][t_L/t_R], T_dtype p_outData[t_R][t_L/t_R])
  { 
    ////#pragma HLS LOOP_MERGE
    //#pragma HLS INLINE off
    
    const int no_of_fft_in_stage =ssrFFTPow<t_R,STAGE>::val;// (int)pow((double)(t_R), double(STAGE));
    const int fft_len_in_stage = (t_L/t_R) / no_of_fft_in_stage;
    const int no_bflys_per_fft = fft_len_in_stage / t_R;
    const int no_samples_per_bfly = t_R;
    //static_assert(STAGE - 1 <= ((_log2<LEN>::val) - _log2<SSR>::val), "Stage no. should be less or equal of log Radix of FFT length minus 1");
    
    //assert(STAGE <_log2<LEN>::val/ _log2<SSR>::val);
    FFTs_LOOP:
    for (int f = 0;f < no_of_fft_in_stage;f++)
    { 
      BFLYs_LOOP:
      for (int bf = 0;bf < no_bflys_per_fft;bf++)
      { 
        //#pragma HLS PIPELINE II=1
        RADIX_LOOP:
        for (int r = 0;r < no_samples_per_bfly;r++)
        { 
          //#pragma HLS UNROLL
          // butterfly number r sample;
          int idx1 = (fft_len_in_stage*f + bf*t_R + r) / (t_R);
          int idx2 = (fft_len_in_stage*f + bf*t_R + r) % t_R;
          int temp = (bf + r*(fft_len_in_stage / t_R) + (fft_len_in_stage*f));
          int odx1 = temp / (t_R);
          int odx2 = temp % (t_R);
          //std::cout << "temp:" << temp << std::endl;;
          //std::cout << idx1 << ":" << idx2 << std::endl;;
          //out[idx1][idx2] = in[odx1][odx2];
          p_outData[odx2][odx1] = p_inData[idx2][idx1];
        }
      }
    }
  }

  template <typename T_dtype, int t_L, int t_R>
  void reverseReOrderDataDynamic(std::complex<T_dtype> p_inData[t_R][t_L/t_R], std::complex<T_dtype> p_outData[t_R][t_L/t_R], int p_stage)
  { 
    int no_of_fft_in_stage = (int)pow((double)(t_R), double(p_stage));
    int fft_len_in_stage = t_L / no_of_fft_in_stage;
    int no_bflys_per_fft = fft_len_in_stage / t_R;
    int no_samples_per_bfly = t_R;
    //static_assert(p_stage - 1 <= ((_log2<LEN>::val) - _log2<SSR>::val), "p_stage no. should be less or equal of log Radix of FFT length minus 1");
    
    //assert(p_stage <_log2<LEN>::val/ _log2<SSR>::val);
    FFTs_LOOP:
    for (int f = 0;f < no_of_fft_in_stage;f++)
    { 
      BFLYs_LOOP:
      for (int bf = 0;bf < no_bflys_per_fft;bf++)
      { 
        RADIX_LOOP:
        for (int r = 0;r < no_samples_per_bfly;r++)
        { 
          // butterfly number r sample;
          int idx1 = (fft_len_in_stage*f + bf*t_R + r) / (t_R);
          int idx2 = (fft_len_in_stage*f + bf*t_R + r) % t_R;
          int temp = (bf + r*(fft_len_in_stage / t_R) + (fft_len_in_stage*f));
          int odx1 = temp / (t_R);
          int odx2 = temp % (t_R);
          //std::cout << "temp:" << temp << std::endl;;
          //std::cout << idx1 << ":" << idx2 << std::endl;;
          //out[idx1][idx2] = in[odx1][odx2];
          p_outData[odx2][odx1] = p_inData[idx2][idx1];
        }
      }
    }

  }
  template <typename T_dtype, int t_L, int t_R>
  int verifyDataReversal(std::complex<T_dtype> p_Data[t_R][t_L/t_R])
  { 
    int flag = 0;
    for (int i = 0; i < t_L/t_R; i++)
    { 
      for (int j = 0; j < t_R; j++)
      { 
        int index = i*t_R + j;
        if (p_Data[j][i].real() != index || p_Data[j][i].imag() != index)
        flag++;
      }
    }
    return flag;
  }
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
} // end namespace fft
} // end namespace dsp
} // end namespace xf

#endif // !HLS_SSR_FFT_DATA_REORDER_H
