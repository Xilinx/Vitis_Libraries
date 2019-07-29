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
 * @file vecMoverB1.hpp
 * @brief common data movers for vectors used in BLAS L1 routines.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_VECMOVERB1_HPP
#define XF_BLAS_VECMOVERB1_HPP

#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {

template <unsigned int t_NumStreams, typename t_DataType>
void dupStream(unsigned int p_n,
               hls::stream<WideType<t_DataType, t_NumStreams> >& p_wideStream,
               hls::stream<WideType<t_DataType, 1> > p_stream[t_NumStreams]) {
    for (unsigned int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
        WideType<t_DataType, t_NumStreams> p_in = p_wideStream.read();
        for (unsigned int j = 0; j < t_NumStreams; j++) {
            p_stream[j].write(WideType<t_DataType, 1>(p_in[j]));
        }
    }
}

template <unsigned int t_NumStreams, typename t_DataType>
void mergeStream(unsigned int p_n,
                 hls::stream<WideType<t_DataType, 1> > p_stream[t_NumStreams],
                 hls::stream<WideType<t_DataType, t_NumStreams> >& p_wideStream) {
    for (unsigned int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
        WideType<t_DataType, t_NumStreams> p_out;
        for (unsigned int j = 0; j < t_NumStreams; j++) {
            p_out[j] = p_stream[j].read().getVal(0);
        }
        p_wideStream.write(p_out);
    }
}

template <typename t_DataType>
void mem2stream(unsigned int p_n, t_DataType* p_in, hls::stream<t_DataType>& p_out) {
    for (unsigned int i = 0; i < p_n; ++i) {
#pragma HLS PIPELINE
        t_DataType l_val = p_in[i];
        p_out.write(l_val);
    }
} // end mem2stream

template <typename t_DataType>
void stream2mem(unsigned int p_n, hls::stream<t_DataType>& p_in, t_DataType* p_out) {
    for (unsigned int i = 0; i < p_n; ++i) {
#pragma HLS PIPELINE
        t_DataType l_val = p_in.read();
        p_out[i] = l_val;
    }
} // end stream2mem

template <typename t_DataType, unsigned int t_ParEntries>
void readVec2Stream(t_DataType* p_in, unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries> >& p_out) {
#ifndef __SYNTHESIS__
    assert((p_n % t_ParEntries) == 0);
#endif
#pragma HLS DATA_PACK variable = p_out
    unsigned int l_parBlocks = p_n / t_ParEntries;
    for (unsigned int i = 0; i < l_parBlocks; ++i) {
#pragma HLS PIPELINE
        BitConv<t_DataType> l_bitConv;
        WideType<t_DataType, t_ParEntries> l_val;
        for (unsigned int j = 0; j < t_ParEntries; ++j) {
            l_val[j] = p_in[i * t_ParEntries + j];
        }
        p_out.write(l_val);
    }
} // end readVec2Stream

template <typename t_DataType, unsigned int t_ParEntries>
void writeStream2Vec(hls::stream<WideType<t_DataType, t_ParEntries> >& p_in, unsigned int p_n, t_DataType* p_out) {
#pragma HLS DATA_PACK variable = p_in
#ifndef __SYNTHESIS__
    assert((p_n % t_ParEntries) == 0);
#endif
    unsigned int l_parBlocks = p_n / t_ParEntries;
    for (unsigned int i = 0; i < l_parBlocks; ++i) {
#pragma HLS PIPELINE
        BitConv<t_DataType> l_bitConv;
        WideType<t_DataType, t_ParEntries> l_val;
        l_val = p_in.read();
        for (unsigned int j = 0; j < t_ParEntries; ++j) {
            p_out[i * t_ParEntries + j] = l_val[j];
        }
    }
} // end writeStream2Vec

} // namespace blas
} // namespace linear_algebra
} // namespace xf
#endif
