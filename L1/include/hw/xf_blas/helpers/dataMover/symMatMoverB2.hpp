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
 * @file symMatMoverB2.hpp
 * @brief data movers for symmetric matrices and corresponding vectors.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_SYMMATMOVERB2_HPP
#define XF_BLAS_SYMMATMOVERB2_HPP

#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {
/**
 * @brief readSymMat2Stream function that read the symmetric matrix from memory to stream
 *
 * @tparam t_DataType the data type of the matrix entries
 * @tparam t_ParEntries the number of parallelly processed entries in the matrix
 *
 * @param p_n number of rows/cols in a symmetric matrix
 * @param p_a memory location of a p_n x p_n symmetric matrix
 * @param p_outSymTransp the stream of matrix entries to transpSymMatBlocks
 * @param p_outTransp the stream of matrix entries to transpMatBlocks
 * @param p_out the streams of matrix entries to be directly forwarded
 */
template <typename t_DataType, unsigned int t_ParEntries>
void readSymUpMat2Stream(unsigned int p_n,
                         t_DataType* p_a,
                         hls::stream<WideType<t_DataType, t_ParEntries> >& p_outSymUpTransp,
                         hls::stream<WideType<t_DataType, t_ParEntries> >& p_outTransp,
                         hls::stream<WideType<t_DataType, t_ParEntries> >& p_outForward) {
    uint26_t l_blocks = p_n / t_ParEntries;
    t_DataType* l_aAddr = p_a;
    for (unsigned int i = 0; i < l_blocks; ++i) {
        for (unsigned int j = 0; j < l_blocks; ++j) {
            t_DataType* l_aBlockAddr = l_aAddr + j * t_ParEntries;
            for (unsigned int br = 0; br < t_ParEntries; ++br) {
#pragma HLS PIPELINE
                WideType<t_DataType, t_ParEntries> l_val;
#pragma HLS ARRAY_PARTITION variable = l_val complete
                for (unsigned int bl = 0; bl < t_ParEntries; ++bl) {
                    l_val[bl] = l_aBlockAddr[bl];
                }
                if (i == j) {
                    p_outSymUpTransp.write(l_val);
                } else if (i < j) {
                    p_outTransp.write(l_val);
                } else {
                    p_outForward.write(l_val);
                }
                l_aBlockAddr += p_n;
            }
        }
        l_aAddr += p_n * t_ParEntries;
    }

    template <typename t_DataType, unsigned int t_ParEntries>
    void mergeSymUpMat(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries> >& p_inSymUpTransp,
                       hls::stream<WideType<t_DataType, t_ParEntries> >& p_inTransp,
                       hls::stream<WideType<t_DataType, t_ParEntries> >& p_inForward,
                       hls::stream<WideType<t_DataType, t_ParEntries> >& p_out) {
        unsigned int l_blocks = p_n / t_ParEntries;
        for (unsigned int i = 0; i < l_blocks; ++i) {
            for (unsigned int j = 0; j < l_blocks; ++j) {
                for (unsigned int br = 0; br < t_ParEntries; ++br) {
#pragma HLS PIPELINE
                    WideType<t_DataType, t_ParEntries> l_val;
                    if (i == j) {
                        l_val = p_inSymUpTransp.read();
                    } else if (i > j) {
                        l_val = p_inForward.read();
                    } else {
                        l_val = p_inTransp.read();
                    }
                    p_out.write(l_val);
                }
            }
        }
    }

    template <typename t_DataType, unsigned int t_ParEntries>
    void readVec2SymStream(unsigned int p_n, t_DataType* p_x, hls::stream<WideType<t_DataType, t_ParEntries> >& p_out) {
        unsigned int l_blocks = p_n / t_ParEntries;
        for (unsigned int i = 0; i < l_blocks; ++i) {
            for (unsigned int j = 0; j < l_blocks; ++j) {
                for (unsigned int br = 0; br < t_ParEntries; ++br) {
                    WideType<t_DataType, t_ParEntries> l_val;
#pragma HLS ARRAY_PARTITION variable = l_val complete
                    for (unsigned int bl = 0; bl < t_ParEntries; ++bl) {
                        l_val[bl] = p_x[j * t_ParEntries + bl];
                    }
                    p_out.write(l_val);
                }
            }
        }
    }

} // namespace blas
} // namespace blas
} // namespace linear_algebra
#endif
