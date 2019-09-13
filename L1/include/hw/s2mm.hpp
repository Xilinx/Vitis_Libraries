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
 * This file is part of XF Compression Library.
 */

#include "common.h"

namespace xf {
namespace compression {
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
template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmEosNb(ap_uint<DATAWIDTH>* out,
               const uint32_t output_idx[PARALLEL_BLOCK],
               hls::stream<ap_uint<DATAWIDTH> > inStream[PARALLEL_BLOCK],
               hls::stream<bool> endOfStream[PARALLEL_BLOCK],
               hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK],
               STREAM_SIZE_DT output_size[PARALLEL_BLOCK]) {
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
                for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                    out[base_idx + j] = local_buffer[i][j];
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
template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmNb(ap_uint<DATAWIDTH>* out,
            const uint32_t output_idx[PARALLEL_BLOCK],
            hls::stream<ap_uint<DATAWIDTH> > inStream[PARALLEL_BLOCK],
            const STREAM_SIZE_DT input_size[PARALLEL_BLOCK]) {
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
                for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                    out[base_idx + j] = local_buffer[i][j];
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
void s2mmEosNbZlib(ap_uint<DATAWIDTH>* out,
                   const uint32_t output_idx[PARALLEL_BLOCK],
                   hls::stream<ap_uint<DATAWIDTH> >& inStream,
                   hls::stream<bool>& endOfStream,
                   STREAM_SIZE_DT output_size[PARALLEL_BLOCK],
                   uint32_t outSize) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = BURST_SIZE;

    ap_uint<NUM_BLOCKS> is_pending = 1;
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
        // printme("Running front done %d !!\n", done);
        for (; done == false;) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                if (!endOfStream.empty()) {
                    bool eos_flag = endOfStream.read();
                    local_buffer[pb][write_idx[pb]] = inStream.read();
                    if (eos_flag) {
                        // printme("eos_flag %d pb %d\n", eos_flag, pb);
                        end_of_stream.range(pb, pb) = 1;
                        done = true;
                    } else {
                        // printme("eos_flag %d pb %d\n", eos_flag, pb);
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

        // printme("Waiting for sometihg \n");

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
            // Write the data to global memory
            if ((read_size[i] > write_size[i]) && (read_size[i] - write_size[i]) >= burst_size[i]) {
                uint32_t base_addr = output_idx[i] / c_wordSize;
                uint32_t base_idx = base_addr + write_size[i];
                uint32_t burst_size_in_words = (burst_size[i]) ? burst_size[i] : 0;
                for (int j = 0; j < burst_size_in_words; j++) {
#pragma HLS PIPELINE II = 1
                    out[base_idx + j] = local_buffer[i][j];
                }
                write_size[i] += burst_size[i];
                write_idx[i] = 0;
            }
        }

        for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
            if (end_of_stream.range(i, i)) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
            // printme("i %d is_pending %d \n", i, (uint8_t)is_pending.range(i,i));
        }

        // printme("Running last!! %d\n", done);
    }

    output_size[0] = outSize;
}

} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_S2MM_HPP_
