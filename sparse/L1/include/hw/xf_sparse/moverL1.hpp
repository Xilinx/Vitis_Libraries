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
 * @file moverL1.hpp
 * @brief SPARSE Level 1 template function implementation for moving and buffering data
 *
 * This file is part of Vitis SPARSE Library.
 */

#ifndef XF_SPARSE_MOVERL1_HPP
#define XF_SPARSE_MOVERL1_HPP

#ifndef __cplusplus
#error "SPARSE Library only works with C++."
#endif

#include <cstdint>
#include "ap_int.h"
#include "hls_stream.h"
#include "xf_blas.hpp"
#ifndef __SYNTHESIS__
#include <iostream>
#endif

using namespace xf::blas;
namespace xf {
namespace sparse {

/**
 * @brief dispColVec function that forward and copy input column vector and parameter streams
 *
 * @tparam t_MaxColParBlocks the maximum number of parallel processed column vectors buffered in on-chip memory
 * @tparam t_HbmChannels number of HBM channels
 * @tparam t_ParEntries parallelly process entries
 * @tparam t_DataBits number of bits used to store each entry
 *
 * @param t_chId constant HBM channel ID
 * @param p_paramStr 32-bit input parameter stream
 * @param p_datStr input vector stream
 * @param p_paramOutStr an forwarded output 32-bit parameter streams
 * @param p_datOutStr an forwarded column vector streams
 * @param p_paramOutStr an copied output 32-bit parameter streams
 * @param p_datOutStr an copied column vector streams
 */
// parallel cscmv related data movers
template <unsigned int t_MaxColParBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_DataBits>
void dispColVec(const unsigned int t_chId,
                hls::stream<ap_uint<32> >& p_paramStr,
                hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datStr,
                hls::stream<ap_uint<32> >& p_paramFwdStr,
                hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datFwdStr,
                hls::stream<ap_uint<32> >& p_paramOutStr,
                hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datOutStr) {
    // local buffer for colVec
    ap_uint<t_DataBits * t_ParEntries> l_vecStore[t_MaxColParBlocks];
    unsigned int l_vecBlocks = 0;
    unsigned int l_chBlocks = 0;
    unsigned int l_minIdx = 0;
    unsigned int l_maxIdx = 0;

    l_vecBlocks = p_paramStr.read();
    l_chBlocks = p_paramStr.read();
    l_minIdx = p_paramStr.read();
    l_maxIdx = p_paramStr.read();

    p_paramFwdStr.write(l_vecBlocks);
    for (unsigned int i = 1; i < t_HbmChannels - t_chId; ++i) {
#pragma HLS PIPELINE II = 3
        unsigned int l_nextChBlocks = p_paramStr.read();
        unsigned int l_nextMinIdx = p_paramStr.read();
        unsigned int l_nextMaxIdx = p_paramStr.read();
        p_paramFwdStr.write(l_nextChBlocks);
        p_paramFwdStr.write(l_nextMinIdx);
        p_paramFwdStr.write(l_nextMaxIdx);
    }
    // store and forward vector
    for (unsigned int i = 0; i < l_vecBlocks; ++i) {
#pragma HLS PIPELINE
        ap_uint<t_DataBits* t_ParEntries> l_dat = p_datStr.read();
        l_vecStore[i] = l_dat;
        p_datFwdStr.write(l_dat);
    }

    p_paramOutStr.write(l_chBlocks);
    // select vector values for this channel
    uint16_t l_minIdxBlock = l_minIdx / t_ParEntries;
    uint8_t l_minIdxMod = l_minIdx % t_ParEntries;
    uint16_t l_maxIdBlock = l_maxIdx / t_ParEntries;
    uint8_t l_maxIdMod = l_maxIdx % t_ParEntries;

    ap_uint<t_DataBits * t_ParEntries> l_valFirst;
    ap_uint<t_DataBits * t_ParEntries> l_valNext;
    ap_uint<t_DataBits * t_ParEntries> l_valOut;
    uint16_t l_curBlock = l_minIdxBlock;

    l_valNext = l_vecStore[l_curBlock];
    l_curBlock++;

    while (l_chBlocks != 0) {
#pragma HLS PIPELINE
        l_valFirst = l_valNext;
        l_valNext = (!(l_curBlock > l_maxIdBlock)) ? l_vecStore[l_curBlock] : (ap_uint<t_DataBits * t_ParEntries>)0;
        for (unsigned int i = 0; i < t_ParEntries; ++i) {
#pragma HLS UNROLL
            uint8_t l_id = l_minIdxMod + i;
            l_valOut.range((i + 1) * t_DataBits - 1, i * t_DataBits) =
                (l_id < t_ParEntries)
                    ? l_valFirst.range((l_id + 1) * t_DataBits - 1, l_id * t_DataBits)
                    : l_valNext.range((l_id + 1 - t_ParEntries) * t_DataBits - 1, (l_id - t_ParEntries) * t_DataBits);
        }
        p_datOutStr.write(l_valOut);
        l_chBlocks--;
        l_curBlock++;
    }
}

template <unsigned int t_MaxColParBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_DataBits>
void dispColVecSink(hls::stream<ap_uint<32> >& p_paramStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datStr,
                    hls::stream<ap_uint<32> >& p_paramOutStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datOutStr) {
    // local buffer for colVec
    ap_uint<t_DataBits * t_ParEntries> l_vecStore[t_MaxColParBlocks];
    unsigned int l_vecBlocks = 0;
    unsigned int l_chBlocks = 0;
    unsigned int l_minIdx = 0;
    unsigned int l_maxIdx = 0;

    l_vecBlocks = p_paramStr.read();
    l_chBlocks = p_paramStr.read();
    l_minIdx = p_paramStr.read();
    l_maxIdx = p_paramStr.read();

    // store vector
    for (unsigned int i = 0; i < l_vecBlocks; ++i) {
#pragma HLS PIPELINE
        ap_uint<t_DataBits* t_ParEntries> l_dat = p_datStr.read();
        l_vecStore[i] = l_dat;
    }
    p_paramOutStr.write(l_chBlocks);
    // select vector values for this channel
    uint16_t l_minIdxBlock = l_minIdx / t_ParEntries;
    uint8_t l_minIdxMod = l_minIdx % t_ParEntries;
    uint16_t l_maxIdBlock = l_maxIdx / t_ParEntries;
    uint8_t l_maxIdMod = l_maxIdx % t_ParEntries;

    ap_uint<t_DataBits * t_ParEntries> l_valFirst;
    ap_uint<t_DataBits * t_ParEntries> l_valNext;
    ap_uint<t_DataBits * t_ParEntries> l_valOut;
    uint16_t l_curBlock = l_minIdxBlock;

    l_valNext = l_vecStore[l_curBlock];
    l_curBlock++;

    while (l_chBlocks != 0) {
#pragma HLS PIPELINE
        l_valFirst = l_valNext;
        l_valNext = (!(l_curBlock > l_maxIdBlock)) ? l_vecStore[l_curBlock] : (ap_uint<t_DataBits * t_ParEntries>)0;
        for (unsigned int i = 0; i < t_ParEntries; ++i) {
#pragma HLS UNROLL
            uint8_t l_id = l_minIdxMod + i;
            l_valOut.range((i + 1) * t_DataBits - 1, i * t_DataBits) =
                (l_id < t_ParEntries)
                    ? l_valFirst.range((l_id + 1) * t_DataBits - 1, l_id * t_DataBits)
                    : l_valNext.range((l_id + 1 - t_ParEntries) * t_DataBits - 1, (l_id - t_ParEntries) * t_DataBits);
        }
        p_datOutStr.write(l_valOut);
        l_chBlocks--;
        l_curBlock++;
    }
}

/**
 * @brief dispCol function that dispatchs input column vectors accross parallel cscmv engines
 *
 * @tparam t_MaxColParBlocks the maximum number of parallel processed column vectors buffered in each cscmv engine
 * @tparam t_HbmChannels number of HBM channels
 * @tparam t_ParEntries parallelly process entries
 * @tparam t_DataBits number of bits used to store each entry
 *
 * @param p_paramStr 32-bit input parameter stream
 * @param p_datStr input vector stream
 * @param p_paramOutStr an output array of 32-bit parameter streams
 * @param p_datOutStr an output array of column vector streams
 */
template <unsigned int t_MaxColParBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_DataBits>
void dispCol(hls::stream<ap_uint<32> >& p_paramStr,
             hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datStr,
             hls::stream<ap_uint<32> > p_paramOutStr[t_HbmChannels],
             hls::stream<ap_uint<t_DataBits * t_ParEntries> > p_datOutStr[t_HbmChannels]) {
#if (t_HbmChannels == 1)
    dispColVecSink<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(p_paramStr, p_datStr, p_paramOutStr[0],
                                                                               p_datOutStr[0]);
#else
    const unsigned int t_FwdChannels = t_HbmChannels - 1;
    hls::stream<ap_uint<32> > l_paramFwdStr[t_FwdChannels];
    hls::stream<ap_uint<t_DataBits * t_ParEntries> > l_datFwdStr[t_FwdChannels];
#pragma HLS DATAFLOW
    dispColVec<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(
        0, p_paramStr, p_datStr, l_paramFwdStr[0], l_datFwdStr[0], p_paramOutStr[0], p_datOutStr[0]);
    for (unsigned int i = 1; i < t_FwdChannels; ++i) {
#pragma HLS UNROLL
        dispColVec<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(
            i, l_paramFwdStr[i - 1], l_datFwdStr[i - 1], l_paramFwdStr[i], l_datFwdStr[i], p_paramOutStr[i],
            p_datOutStr[i]);
    }
    dispColVecSink<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(
        l_paramFwdStr[t_FwdChannels - 1], l_datFwdStr[t_FwdChannels - 1], p_paramOutStr[t_FwdChannels],
        p_datOutStr[t_FwdChannels]);

#endif
}

template <unsigned int t_MaxColParBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_DataBits>
void dispNnzColStep(const unsigned int t_chId,
                    hls::stream<ap_uint<32> >& p_paramStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datStr,
                    hls::stream<ap_uint<32> >& p_paramFwdStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datFwdStr,
                    hls::stream<ap_uint<32> >& p_paramOutStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datOutStr) {
    // local buffer for colVec
    const unsigned int t_Channels = t_HbmChannels - t_chId;
    ap_uint<t_DataBits * t_ParEntries> l_nnzColStore[t_MaxColParBlocks];
    unsigned int l_chBlocks[t_HbmChannels];

    for (unsigned int i = 0; i < t_Channels; ++i) {
        l_chBlocks[i] = p_paramStr.read();
    }

    for (unsigned int i = 1; i < t_Channels; ++i) {
#pragma HLS PIPELINE
        p_paramFwdStr.write(l_chBlocks[i]);
    }
    // store and forward vector
    unsigned int l_blocks = l_chBlocks[0];
    for (unsigned int i = 0; i < l_blocks; ++i) {
#pragma HLS PIPELINE
        ap_uint<t_DataBits* t_ParEntries> l_dat = p_datStr.read();
        l_nnzColStore[i] = l_dat;
    }
    for (unsigned int ch = 1; ch < t_Channels; ++ch) {
        l_blocks = l_chBlocks[ch];
        for (unsigned int i = 0; i < l_blocks; ++i) {
#pragma HLS PIPELINE
            ap_uint<t_DataBits* t_ParEntries> l_dat = p_datStr.read();
            p_datFwdStr.write(l_dat);
        }
    }
    l_blocks = l_chBlocks[0];
    p_paramOutStr.write(l_blocks);
    for (unsigned int i = 0; i < l_blocks; ++i) {
        ap_uint<t_DataBits * t_ParEntries> l_val;
        l_val = l_nnzColStore[i];
        p_datOutStr.write(l_val);
    }
}

template <unsigned int t_MaxColParBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_DataBits>
void dispNnzColSink(hls::stream<ap_uint<32> >& p_paramStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datStr,
                    hls::stream<ap_uint<32> >& p_paramOutStr,
                    hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datOutStr) {
    // local buffer for colVec
    ap_uint<t_DataBits * t_ParEntries> l_nnzColStore[t_MaxColParBlocks];
    unsigned int l_chBlocks;

    l_chBlocks = p_paramStr.read();

    // store vector
    for (unsigned int i = 0; i < l_chBlocks; ++i) {
#pragma HLS PIPELINE
        ap_uint<t_DataBits* t_ParEntries> l_dat = p_datStr.read();
        l_nnzColStore[i] = l_dat;
    }
    p_paramOutStr.write(l_chBlocks);
    for (unsigned int i = 0; i < l_chBlocks; ++i) {
        ap_uint<t_DataBits * t_ParEntries> l_val;
        l_val = l_nnzColStore[i];
        p_datOutStr.write(l_val);
    }
}

/**
 * @brief dispNnzCol function that dispatchs NNZ Col pointer vectors accross parallel cscmv engines
 *
 * @tparam t_MaxColParBlocks the maximum number of parallel processed column vectors buffered in each cscmv engine
 * @tparam t_HbmChannels number of HBM channels
 * @tparam t_ParEntries parallelly process entries
 * @tparam t_DataBits number of bits used to store each entry
 *
 * @param p_paramStr 32-bit input parameter stream
 * @param p_datStr input vector stream
 * @param p_paramOutStr an output array of 32-bit parameter streams
 * @param p_datOutStr an output array of vector streams
 */
template <unsigned int t_MaxColParBlocks,
          unsigned int t_HbmChannels,
          unsigned int t_ParEntries,
          unsigned int t_DataBits>
void dispNnzCol(hls::stream<ap_uint<32> >& p_paramStr,
                hls::stream<ap_uint<t_DataBits * t_ParEntries> >& p_datStr,
                hls::stream<ap_uint<32> > p_paramOutStr[t_HbmChannels],
                hls::stream<ap_uint<t_DataBits * t_ParEntries> > p_datOutStr[t_HbmChannels]) {
#if (t_HbmChannels == 1)
    dispNnzColSink<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(p_paramStr, p_datStr, p_paramOutStr[0],
                                                                               p_datOutStr[0]);
#else
    const unsigned int t_FwdChannels = t_HbmChannels - 1;
    hls::stream<ap_uint<32> > l_paramFwdStr[t_FwdChannels];
    hls::stream<ap_uint<t_DataBits * t_ParEntries> > l_datFwdStr[t_FwdChannels];
#pragma HLS DATAFLOW
    dispNnzColStep<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(
        0, p_paramStr, p_datStr, l_paramFwdStr[0], l_datFwdStr[0], p_paramOutStr[0], p_datOutStr[0]);
    for (unsigned int i = 1; i < t_FwdChannels; ++i) {
#pragma HLS UNROLL
        dispNnzColStep<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(
            i, l_paramFwdStr[i - 1], l_datFwdStr[i - 1], l_paramFwdStr[i], l_datFwdStr[i], p_paramOutStr[i],
            p_datOutStr[i]);
    }
    dispNnzColSink<t_MaxColParBlocks, t_HbmChannels, t_ParEntries, t_DataBits>(
        l_paramFwdStr[t_FwdChannels - 1], l_datFwdStr[t_FwdChannels - 1], p_paramOutStr[t_FwdChannels],
        p_datOutStr[t_FwdChannels]);

#endif
}
} // end namespace sparse
} // end namespace xf
#endif
