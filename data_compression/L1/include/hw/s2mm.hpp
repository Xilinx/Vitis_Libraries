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
#ifndef _XFCOMPRESSION_S2MM_HPP_
#define _XFCOMPRESSION_S2MM_HPP_

/**
 * @file s2mm.hpp
 * @brief Header for modules used for streaming to memory mapped interface conversion.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

namespace xf {
namespace compression {
template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmEosNb(ap_uint<DATAWIDTH>* out,
               const uint32_t output_idx[PARALLEL_BLOCK],
               hls::stream<ap_uint<DATAWIDTH> > inStream[PARALLEL_BLOCK],
               hls::stream<bool> endOfStream[PARALLEL_BLOCK],
               hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK],
               STREAM_SIZE_DT output_size[PARALLEL_BLOCK]) {
    /**
     * @brief This module reads DATAWIDTH data from stream until end of
     * stream happens and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API.
     *
     * @tparam STREAM_SIZE_DT Stream size class instance
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param endOfStream end flag for stream
     * @param compressedSize size of compressed stream
     * @param output_size output size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = BURST_SIZE;

    ap_uint<PARALLEL_BLOCK> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[PARALLEL_BLOCK][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    ap_uint<PARALLEL_BLOCK> end_of_stream = 0;
    uint32_t read_size[PARALLEL_BLOCK];
    uint32_t write_size[PARALLEL_BLOCK];
    uint32_t write_idx[PARALLEL_BLOCK];
    uint32_t burst_size[PARALLEL_BLOCK];
#pragma HLS ARRAY PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY PARTITION variable = burst_size dim = 0 complete

    for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        read_size[i] = 0;
        write_size[i] = 0;
        write_idx[i] = 0;
    }

    bool done = false;
    uint32_t loc = 0;
    uint32_t remaining_data = 0;

    while (is_pending != 0) {
        done = false;
        for (; done == false;) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                if (!endOfStream[pb].empty()) {
                    bool eos_flag = endOfStream[pb].read();
                    local_buffer[pb][write_idx[pb]] = inStream[pb].read();
                    if (eos_flag) {
                        end_of_stream.range(pb, pb) = 1;
                        done = true;
                    } else {
                        read_size[pb] += 1;
                        write_idx[pb] += 1;
                    }
                    if (read_size[pb] >= BURST_SIZE) {
                        done = true;
                    }
                    burst_size[pb] = c_maxBurstSize;
                    if (end_of_stream.range(pb, pb) && (read_size[pb] - write_size[pb]) < burst_size[pb]) {
                        burst_size[pb] = (read_size[pb] > write_size[pb]) ? (read_size[pb] - write_size[pb]) : 0;
                    }
                }
            }
        }

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
            // Write the data to global memory
            if ((read_size[i] > write_size[i]) && (read_size[i] - write_size[i]) >= burst_size[i]) {
                uint32_t base_addr = output_idx[i] / c_wordSize;
                uint32_t base_idx = base_addr + write_size[i];
                uint32_t burst_size_in_words = (burst_size[i]) ? burst_size[i] : 0;

                if (burst_size_in_words > 0) {
                    for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                        out[base_idx + j] = local_buffer[i][j];
                    }
                }

                write_size[i] += burst_size[i];
                write_idx[i] = 0;
            }
        }

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
            if (end_of_stream.range(i, i)) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }

    for (uint8_t i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS PIPELINE II = 1
        output_size[i] = compressedSize[i].read();
    }
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmEosNbFreq(ap_uint<DATAWIDTH>* out,
                   const uint32_t output_idx[PARALLEL_BLOCK],
                   hls::stream<ap_uint<DATAWIDTH> > inStream[PARALLEL_BLOCK],
                   hls::stream<bool> endOfStream[PARALLEL_BLOCK],
                   hls::stream<uint32_t> inStreamTree[PARALLEL_BLOCK],
                   hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK],
                   STREAM_SIZE_DT output_size[PARALLEL_BLOCK],
                   const STREAM_SIZE_DT input_size[PARALLEL_BLOCK],
                   STREAM_SIZE_DT* dyn_ltree_freq,
                   STREAM_SIZE_DT* dyn_dtree_freq) {
    /**
     * @brief This module reads DATAWIDTH data from stream until end of
     * stream happens and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API. It also
     * collects the huffman codes and bit length data and copies to DDR.
     *
     * @tparam STREAM_SIZE_DT Stream size class instance
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param out output global memory address
     * @param output_idx output index
     * @param inStream input stream contains final output data
     * @param endOfStream input stream indicates end of the input stream
     * @param inStreamTree input stream contains huffman codes and bitlength data
     * @param compressedSize output stream contains compressed size of each block
     * @param output_size output global memory address that holds compress sizes
     * @param input_size input global memory address indicates block size
     * @param dyn_ltree_freq output literal frequency data
     * @param dyn_dtree_freq output distance frequency data
     *
     */
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = BURST_SIZE;

    const int c_lTreeSize = 1024;
    const int c_dTreeSize = 64;
    const int c_bLTreeSize = 64;
    const int c_maxCodeSize = 16;

    ap_uint<PARALLEL_BLOCK> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[PARALLEL_BLOCK][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    ap_uint<PARALLEL_BLOCK> end_of_stream = 0;
    uint32_t read_size[PARALLEL_BLOCK];
    uint32_t write_size[PARALLEL_BLOCK];
    uint32_t write_idx[PARALLEL_BLOCK];
    uint32_t burst_size[PARALLEL_BLOCK];
#pragma HLS ARRAY PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY PARTITION variable = burst_size dim = 0 complete

    for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        read_size[i] = 0;
        write_size[i] = 0;
        write_idx[i] = 0;
    }

    bool done = false;
    uint32_t loc = 0;
    uint32_t remaining_data = 0;

    while (is_pending != 0) {
        done = false;
        for (; done == false;) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                if (!endOfStream[pb].empty()) {
                    bool eos_flag = endOfStream[pb].read();
                    local_buffer[pb][write_idx[pb]] = inStream[pb].read();
                    if (eos_flag) {
                        end_of_stream.range(pb, pb) = 1;
                        done = true;
                    } else {
                        read_size[pb] += 1;
                        write_idx[pb] += 1;
                    }
                    if (read_size[pb] >= BURST_SIZE) {
                        done = true;
                    }
                    burst_size[pb] = c_maxBurstSize;
                    if (end_of_stream.range(pb, pb) && (read_size[pb] - write_size[pb]) < burst_size[pb]) {
                        burst_size[pb] = (read_size[pb] > write_size[pb]) ? (read_size[pb] - write_size[pb]) : 0;
                    }
                }
            }
        }

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
            // Write the data to global memory
            if ((read_size[i] > write_size[i]) && (read_size[i] - write_size[i]) >= burst_size[i]) {
                uint32_t base_addr = output_idx[i] / c_wordSize;
                uint32_t base_idx = base_addr + write_size[i];
                uint32_t burst_size_in_words = (burst_size[i]) ? burst_size[i] : 0;

                if (burst_size_in_words > 0) {
                    for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                        out[base_idx + j] = local_buffer[i][j];
                    }
                }
                write_size[i] += burst_size[i];
                write_idx[i] = 0;
            }
        }

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
            if (end_of_stream.range(i, i)) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }

    for (uint8_t i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS PIPELINE II = 1
        output_size[i] = compressedSize[i].read();
    }

    uint32_t val = 0;
    for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
        bool dist_read = true;
    s2mm_ltree_freq:
        for (uint32_t i = 0; i < c_lTreeSize; i++) {
            val = inStreamTree[bIdx].read();
            if (val == 9999) {
                i = c_lTreeSize;
                dist_read = false;
            } else {
                dyn_ltree_freq[bIdx * c_lTreeSize + i] = val;
            }
        }

    s2mm_dtree_freq:
        for (uint32_t j = 0; ((j < c_dTreeSize) && dist_read); j++) {
            val = inStreamTree[bIdx].read();
            dyn_dtree_freq[bIdx * c_dTreeSize + j] = val;
        }
    }
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmNb(ap_uint<DATAWIDTH>* out,
            const uint32_t output_idx[PARALLEL_BLOCK],
            hls::stream<ap_uint<DATAWIDTH> > inStream[PARALLEL_BLOCK],
            const STREAM_SIZE_DT input_size[PARALLEL_BLOCK]) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR. Reading data from
     * multiple data streams is non-blocking which is done using empty() API.
     *
     * @tparam STREAM_SIZE_DT Stream size class instance
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param input_size input size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = c_wordSize * BURST_SIZE;
    uint32_t read_size[PARALLEL_BLOCK];
    uint32_t write_size[PARALLEL_BLOCK];
    uint32_t burst_size[PARALLEL_BLOCK];
    uint32_t write_idx[PARALLEL_BLOCK];
#pragma HLS ARRAY PARTITION variable = input_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY PARTITION variable = burst_size dim = 0 complete
    ap_uint<PARALLEL_BLOCK> end_of_stream = 0;
    ap_uint<PARALLEL_BLOCK> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[PARALLEL_BLOCK][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    // printme("%s:Started\n", __FUNCTION__);
    for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        read_size[i] = 0;
        write_size[i] = 0;
        write_idx[i] = 0;
        // printme("%s:Indx=%d out_idx=%d\n",__FUNCTION__,i , output_idx[i]);
    }
    bool done = false;
    uint32_t loc = 0;
    uint32_t remaining_data = 0;
    while (is_pending != 0) {
        done = false;
        for (int i = 0; (is_pending != 0) && (done == 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                burst_size[pb] = c_maxBurstSize;
                if (((input_size[pb] - write_size[pb]) < burst_size[pb])) {
                    burst_size[pb] = (input_size[pb] > write_size[pb]) ? (input_size[pb] - write_size[pb]) : 0;
                }
                if (((read_size[pb] - write_size[pb]) < burst_size[pb]) && (input_size[pb] > read_size[pb])) {
                    bool is_empty = inStream[pb].empty();
                    if (!is_empty) {
                        local_buffer[pb][write_idx[pb]] = inStream[pb].read();
                        write_idx[pb] += 1;
                        read_size[pb] += c_wordSize;
                        is_pending.range(pb, pb) = true;
                    } else {
                        is_pending.range(pb, pb) = false;
                    }
                } else {
                    if (burst_size[pb]) done = true;
                }
            }
        }

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
            // Write the data to global memory
            if ((read_size[i] > write_size[i]) && (read_size[i] - write_size[i]) >= burst_size[i]) {
                uint32_t base_addr = output_idx[i] + write_size[i];
                uint32_t base_idx = base_addr / c_wordSize;
                uint32_t burst_size_in_words = (burst_size[i]) ? ((burst_size[i] - 1) / c_wordSize + 1) : 0;

                if (burst_size_in_words > 0) {
                    for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                        out[base_idx + j] = local_buffer[i][j];
                    }
                }
                write_size[i] += burst_size[i];
                write_idx[i] = 0;
            }
        }
        for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
            if (done == true && (write_size[i] >= input_size[i])) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmEosSimple(ap_uint<DATAWIDTH>* out,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream,
                   hls::stream<bool>& endOfStream,
                   hls::stream<uint32_t>& outSize,
                   uint32_t* output_size) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR.
     *
     * @tparam STREAM_SIZE_DT Stream size class instance
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCKS number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param endOfStream stream to indicate end of data stream
     * @param outSize output data size
     */

    uint32_t out_idx = 0;
s2mm_eos_simple:
    for (bool eos = endOfStream.read(); eos == false; eos = endOfStream.read())
#pragma HLS PIPELINE II = 1
        out[out_idx++] = inStream.read();

    ap_uint<DATAWIDTH> val = inStream.read();
    output_size[0] = outSize.read();
}

template <int DATAWIDTH>
void s2mmSimple(ap_uint<DATAWIDTH>* out, hls::stream<ap_uint<DATAWIDTH> >& inStream, uint32_t output_size) {
    /**
     * @brief This module reads N-bit data from stream based on
     * size stream and writes the data to DDR. N is template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH Width of the input data stream
     *
     * @param out output memory address
     * @param inStream input hls stream
     * @param output_size output data size
     */

    uint8_t factor = DATAWIDTH / 8;
    uint32_t itrLim = 1 + ((output_size - 1) / factor);
s2mm_simple:
    for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        out[i] = inStream.read();
    }
}

template <int DATAWIDTH>
void s2mm(hls::stream<ap_uint<DATAWIDTH> >& inStream, ap_uint<DATAWIDTH>* out, hls::stream<uint32_t>& inStreamSize) {
    const int c_byte_size = 8;
    const int c_factor = DATAWIDTH / c_byte_size;

    uint32_t outIdx = 0;
    uint32_t size = 1;
    uint32_t sizeIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
    mwr:
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
            out[outIdx + i] = inStream.read();
        }
        outIdx += size;
    }
}

} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_S2MM_HPP_
