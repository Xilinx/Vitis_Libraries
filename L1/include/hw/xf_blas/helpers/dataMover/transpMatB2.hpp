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
 * @file transpMatB2.hpp
 * @brief datamovers for symmetric matrices and vectors used in BLAS L2 routines.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_TRANSPMATB2_HPP
#define XF_BLAS_TRANSPMATB2_HPP

#include "hls_stream.h"
#include "ap_int.h"
#include "ap_shift_reg.h"

namespace xf {
namespace linear_algebra {
namespace blas {

/**
 * @brief transpSymUpMat function that mirros the super-diagonals in a matrix block to sub-diagonals
 *
 * @tparam t_DataType the data type of the matrix entries
 * @tparam t_ParEntries number of parallelly processed entries in the matrix
 *
 * @param p_blocks number of t_ParEntries x t_ParEntries matrix blocks
 * @param p_in input stream of matrix blocks
 * @param p_out output stream of symmetric matrix blocks
 */
template <typename t_DataType, unsigned int t_ParEntries>
void transpSymUpMatBlocks(unsigned int p_blocks,
                          hls::stream<WideType<t_DataType, t_ParEntries> >& p_in,
                          hls::stream<WideType<t_DataType, t_ParEntries> >& p_out) {
    t_DataType l_buf[t_ParEntries][t_ParEntries];
#pragma HLS ARRAY_PARTITION variable = l_buf complete dim = 2
    for (unsigned int l_block = 0; l_block < p_blocks; ++l_block) {
#pragma HLS PIPELINE II = t_ParEntries
        // shuffle and store
        for (unsigned int i = 0; i < t_ParEntries; ++i) {
            WideType<t_DataType, t_ParEntries> l_val;
#pragma HLS ARRAY_PARTITION variable = l_val complete
            l_val = p_in.read();
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_buf[i][j] = l_val[(t_ParEntries - i + j) % t_ParEntries];
            }
        }

        for (unsigned int i = 0; i < t_ParEntries; ++i) {
            WideType<t_DataType, t_ParEntries> l_val;
#pragma HLS ARRAY_PARTITION variable = l_val complete
            WideType<t_DataType, t_ParEntries> l_out;
#pragma HLS ARRAY_PARTITION variable = l_out complete
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_val[j] = ((t_ParEntries - i + j) % t_ParEntries < i) ? l_buf[(t_ParEntries - i + j) % t_ParEntries][j]
                                                                       : l_buf[i][j];
            }
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_out[j] = l_val[(i + j) % t_ParEntries];
            }
            p_out.write(l_out);
        }
    }
}

/**
 * @brief transpMat function transposes matrix blocks
 *
 * @tparam t_DataType the data type of the matrix entries
 * @tparam t_ParEntries the number of parallely processed entries in the matrix
 *
 * @param p_blocks number of matrix blocks
 * @param p_in input stream of matrix blocks
 * @param p_out output stream of matrix blocks
 */
template <typename t_DataType, unsigned int t_ParEntries>
void transpMatBlocks(unsigned int p_blocks,
                     hls::stream<WideType<t_DataType, t_ParEntries> >& p_in,
                     hls::stream<WideType<t_DataType, t_ParEntries> >& p_out) {
    t_DataType l_buf[t_ParEntries][t_ParEntries];
#pragma HLS ARRAY_PARTITION variable = l_buf complete dim = 2
    for (unsigned int l_block = 0; l_block < p_blocks; ++l_block) {
#pragma HLS PIPELINE II = t_ParEntries
        // shuffle and store
        for (unsigned int i = 0; i < t_ParEntries; ++i) {
            WideType<t_DataType, t_ParEntries> l_val;
#pragma HLS ARRAY_PARTITION variable = l_val complete
            l_val = p_in.read();
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_buf[i][j] = l_val[(i + j) % t_ParEntries];
            }
        }

        for (unsigned int i = 0; i < t_ParEntries; ++i) {
            WideType<t_DataType, t_ParEntries> l_val;
#pragma HLS ARRAY_PARTITION variable = l_val complete
            WideType<t_DataType, t_ParEntries> l_out;
#pragma HLS ARRAY_PARTITION variable = l_out complete
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_val[j] = l_buf[(t_ParEntries - j + i) % t_ParEntries][j];
            }
            for (unsigned int j = 0; j < t_ParEntries; ++j) {
                l_out[j] = l_val[(t_ParEntries - j + i) % t_ParEntries];
            }
            p_out.write(l_out);
        }
    }
}

} // namespace blas
} // namespace linear_algebra
} // namespace xf
#endif
