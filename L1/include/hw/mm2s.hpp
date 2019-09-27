/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef _XFCOMPRESSION_MM2S_HPP_
#define _XFCOMPRESSION_MM2S_HPP_

/**
 * @file mm2s.hpp
 * @brief Header for modules used for memory mapped to streaming interface conversion.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "common.h"

namespace xf {
namespace compression {
/**
 * @brief This module reads 512bit data from memory interface and
 * writes to the stream. Writing to the multiple data streams is
 * non-blocking call which is done using is_full() API
 *
 * @tparam DATAWIDTH width of data bus
 * @tparam BURST_SIZE burst size of the data transfers
 * @tparam NUM_BLOCKS number of blocks
 *
 * @param in input memory address
 * @param _input_idx input index
 * @param outStream output stream
 * @param _input_size intput stream size
 */
template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNb(const ap_uint<DATAWIDTH>* in,
            const uint32_t _input_idx[PARALLEL_BLOCK],
            hls::stream<ap_uint<DATAWIDTH> > outStream[PARALLEL_BLOCK],
            const uint32_t _input_size[PARALLEL_BLOCK]) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[PARALLEL_BLOCK][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM
    uint32_t read_idx[PARALLEL_BLOCK];
    uint32_t write_idx[PARALLEL_BLOCK];
    uint32_t read_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t input_size[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<PARALLEL_BLOCK> pending;
    ap_uint<PARALLEL_BLOCK> is_full;
    for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
#pragma HLS UNROLL
        read_idx[bIdx] = 0;
        write_idx[bIdx] = 0;
        read_size[bIdx] = 0;
        input_idx[bIdx] = _input_idx[bIdx];
        input_size[bIdx] = _input_size[bIdx];
        pending.range(bIdx, bIdx) = 1;
    }
    while (pending) {
        pending = 0;
        for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
            uint32_t pending_bytes = (input_size[bIdx] > read_size[bIdx]) ? (input_size[bIdx] - read_size[bIdx]) : 0;
            if ((pending_bytes) && (read_idx[bIdx] == write_idx[bIdx])) {
                uint32_t pending_words = (pending_bytes - 1) / c_wordSize + 1;
                uint32_t burst_size = (pending_words > BURST_SIZE) ? BURST_SIZE : pending_words;
                uint32_t mem_read_byte_idx = read_size[bIdx] + input_idx[bIdx];
                uint32_t mem_read_word_idx = (mem_read_byte_idx) ? ((mem_read_byte_idx - 1) / c_wordSize + 1) : 0;
            gmem_rd:
                for (uint32_t i = 0; i < burst_size; i++) {
#pragma HLS PIPELINE II = 1
                    local_buffer[bIdx][i] = in[mem_read_word_idx + i];
                }
                pending.range(bIdx, bIdx) = 1;
                read_idx[bIdx] = 0;
                write_idx[bIdx] = burst_size;
                read_size[bIdx] += burst_size * c_wordSize;
            }
        }
        ap_uint<PARALLEL_BLOCK> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < PARALLEL_BLOCK; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
#pragma HLS UNROLL
                if (read_idx[bIdx] == write_idx[bIdx]) {
                    terminate_all.range(bIdx, bIdx) = 0;
                    if (read_size[bIdx] < input_size[bIdx]) {
                        terminate = 1;
                    }
                } else {
                    terminate_all.range(bIdx, bIdx) = 1;
                    pending.range(bIdx, bIdx) = 1;
                }
            }
        }
    }
}

/**
 * @brief This module is same as mm2sNb API but with an extra handling
 * rounding off the indexing to maximum buffer size for P2P decompression.
 *
 * @tparam DATAWIDTH width of data bus
 * @tparam BURST_SIZE burst size of the data transfers
 * @tparam NUM_BLOCKS number of blocks
 *
 * @param in input memory address
 * @param _input_idx input index
 * @param outStream output stream
 * @param _input_size intput stream size
 * @param max_buffer_size_in_bytes Maximum buffer size for indexing
 */
template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNbRoundOff(const ap_uint<DATAWIDTH>* in,
                    const uint32_t _input_idx[PARALLEL_BLOCK],
                    hls::stream<ap_uint<DATAWIDTH> > outStream[PARALLEL_BLOCK],
                    const uint32_t _input_size[PARALLEL_BLOCK]) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[PARALLEL_BLOCK][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM
    uint32_t read_idx[PARALLEL_BLOCK];
    uint32_t write_idx[PARALLEL_BLOCK];
    uint32_t read_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t input_size[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<PARALLEL_BLOCK> pending;
    ap_uint<PARALLEL_BLOCK> is_full;
    for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
#pragma HLS UNROLL
        read_idx[bIdx] = 0;
        write_idx[bIdx] = 0;
        read_size[bIdx] = 0;
        input_idx[bIdx] = _input_idx[bIdx];
        input_size[bIdx] = _input_size[bIdx] + (input_idx[bIdx] % c_wordSize);
        pending.range(bIdx, bIdx) = 1;
    }
    while (pending) {
        pending = 0;
        for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
            uint32_t pending_bytes = (input_size[bIdx] > read_size[bIdx]) ? (input_size[bIdx] - read_size[bIdx]) : 0;
            if ((pending_bytes) && (read_idx[bIdx] == write_idx[bIdx])) {
                uint32_t pending_words = (pending_bytes - 1) / c_wordSize + 1;
                uint32_t burst_size = (pending_words > BURST_SIZE) ? BURST_SIZE : pending_words;
                uint32_t mem_read_byte_idx = read_size[bIdx] + input_idx[bIdx];
                uint32_t mem_read_word_idx = 0;
                if (mem_read_byte_idx)
                    mem_read_word_idx = (mem_read_byte_idx % c_wordSize) ? (mem_read_byte_idx - 1) / c_wordSize
                                                                         : ((mem_read_byte_idx - 1) / c_wordSize + 1);
                else
                    mem_read_word_idx = 0;

            gmem_rd:
                for (uint32_t i = 0; i < burst_size; i++) {
#pragma HLS PIPELINE II = 1
                    local_buffer[bIdx][i] = in[mem_read_word_idx + i];
                }
                pending.range(bIdx, bIdx) = 1;
                read_idx[bIdx] = 0;
                write_idx[bIdx] = burst_size;
                read_size[bIdx] += burst_size * c_wordSize;
            }
        }
        ap_uint<PARALLEL_BLOCK> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < PARALLEL_BLOCK; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
#pragma HLS UNROLL
                if (read_idx[bIdx] == write_idx[bIdx]) {
                    terminate_all.range(bIdx, bIdx) = 0;
                    if (read_size[bIdx] < input_size[bIdx]) {
                        terminate = 1;
                    }
                } else {
                    terminate_all.range(bIdx, bIdx) = 1;
                    pending.range(bIdx, bIdx) = 1;
                }
            }
        }
    }
}

/**
 * @brief Read data from 512-bit wide axi memory interface and
 *        write to stream.
 *
 * @tparam DATAWIDTH    width of data bus
 * @tparam BURST_SIZE   burst size of the data transfers
 *
 * @param in            pointer to input memory
 * @param outstream     output stream
 * @param inputSize     size of the data
 *
 */
template <int DATAWIDTH, int BURST_SIZE>
void mm2sSimple(const uintMemWidth_t* in, hls::stream<uintMemWidth_t>& outstream, uint32_t inputSize) {
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int inSize_gmemwidth = (inputSize - 1) / c_word_size + 1;

mm2s_simple:
    for (int i = 0; i < inSize_gmemwidth; i++) {
#pragma HLS PIPELINE II = 1
        outstream << in[i];
    }
}

} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_MM2S_HPP_
