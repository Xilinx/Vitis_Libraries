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

#define GET_DIFF_IF_BIG(x, y) (x > y) ? (x - y) : 0

#define STREAM_UTILS_S2MM_READ_SIZE(i, instream, end_of_stream) \
    if (!end_of_stream.range(i, i) && !instream.empty()) {      \
        uint16_t tmpValue = instream.read();                    \
        input_size[i] += tmpValue;                              \
        if (tmpValue == 0) end_of_stream.range(i, i) = 1;       \
    }

#define STREAM_UTILS_S2MM_IF_NOT_EMPTY(i, instream, burst_size, input_size, read_size, write_size, write_idx) \
    burst_size[i] = c_max_burst_size;                                                                         \
    if (end_of_stream.range(i, i) && ((input_size[i] - write_size[i]) < burst_size[i])) {                     \
        burst_size[i] = GET_DIFF_IF_BIG(input_size[i], write_size[i]);                                        \
    }                                                                                                         \
    if (((read_size[i] - write_size[i]) < burst_size[i]) && (input_size[i] > read_size[i])) {                 \
        bool is_empty = instream.empty();                                                                     \
        if (!is_empty) {                                                                                      \
            local_buffer[i][write_idx[i]] = instream.read();                                                  \
            write_idx[i] += 1;                                                                                \
            read_size[i] += 64;                                                                               \
            is_pending.range(i, i) = true;                                                                    \
        } else {                                                                                              \
            is_pending.range(i, i) = false;                                                                   \
        }                                                                                                     \
    } else {                                                                                                  \
        if (burst_size[i]) done = true;                                                                       \
        if (read_size[i] >= input_size[i]) is_pending.range(i, i) = false;                                    \
    }

namespace xf {
namespace compression {
namespace details {

template <int IN_DATAWIDTH, int OUT_DATAWIDTH, int BURST_SIZE>
void stream2mmUpsizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                      hls::stream<bool>& inStreamEos,
                      hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream,
                      hls::stream<uint16_t>& outSizeStream) {
    const int c_byteWidth = 8;
    const int c_nobytes = OUT_DATAWIDTH / c_byteWidth;
    const int c_upsizeFactor = OUT_DATAWIDTH / IN_DATAWIDTH;
    const int c_size = BURST_SIZE * c_nobytes;

    ap_uint<IN_DATAWIDTH> inValue = inStream.read();
    ap_uint<OUT_DATAWIDTH> outBuffer = 0;
    uint32_t i = 0, sizeWrite = 0;
    bool skip_first = true;
    bool out_flg = false;
upsizerTop:
    for (bool eos_flag = inStreamEos.read(); eos_flag == false; i++) {
#pragma HLS PIPELINE II = 1
        if ((i % c_upsizeFactor == 0) && (skip_first == false)) {
            outStream << outBuffer;
            sizeWrite++;
            if (sizeWrite == BURST_SIZE) {
                outSizeStream << c_size;
                sizeWrite = 0;
            }
            i = 0;
        }
        skip_first = false;
        outBuffer >>= IN_DATAWIDTH;
        outBuffer.range(OUT_DATAWIDTH - 1, OUT_DATAWIDTH - IN_DATAWIDTH) = inValue;
        inValue = inStream.read();
        eos_flag = inStreamEos.read();
    }

    if ((i % c_upsizeFactor == 0) && (skip_first == false)) {
        outStream << outBuffer;
        sizeWrite++;
        if (sizeWrite == BURST_SIZE) {
            outSizeStream << c_size;
            sizeWrite = 0;
        }
        i = 0;
    }

    for (; i % c_upsizeFactor > 0; i++) {
#pragma HLS PIPELINE II = 1
        outBuffer >>= IN_DATAWIDTH;
        out_flg = true;
    }
    if (out_flg) {
        outStream << outBuffer;
        sizeWrite++;
        outSizeStream << (sizeWrite * c_nobytes);
    }
    outSizeStream << 0;
}

template <int DATAWIDTH>
void singleStream2mm(hls::stream<ap_uint<DATAWIDTH> >& inStream,
                     hls::stream<uint16_t>& inSizeStream,
                     hls::stream<uint32_t>& outSizeStream,
                     ap_uint<DATAWIDTH>* out,
                     ap_uint<DATAWIDTH>* outSize) {
    const int c_byteWidth = 8;
    const int c_nobytes = DATAWIDTH / c_byteWidth;
    uint32_t memIdx = 0;
s2mm_simple:
    for (uint16_t inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        uint16_t outSizeV = (inSize - 1) / c_nobytes + 1;
        for (uint32_t j = 0; j < outSizeV; j++) {
#pragma HLS PIPELINE II = 1
            out[memIdx + j] = inStream.read();
        }
        memIdx += outSizeV;
    }
    outSize[0] = outSizeStream.read();
}

template <int IN_DATAWIDTH, int GMEM_DATAWIDTH = 512, int BURST_SIZE = 16>
void stream2MM(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
               hls::stream<bool>& inStreamEos,
               hls::stream<uint32_t>& outSizeStream,
               ap_uint<GMEM_DATAWIDTH>* out,
               ap_uint<GMEM_DATAWIDTH>* outSize) {
    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV;
    hls::stream<uint16_t> outStreamVSize;
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 1
#pragma HLS RESOURCE variable = outStreamV core = FIFO_SRL

#pragma HLS DATAFLOW
    xf::compression::details::stream2mmUpsizer<IN_DATAWIDTH, GMEM_DATAWIDTH, BURST_SIZE>(inStream, inStreamEos,
                                                                                         outStreamV, outStreamVSize);
    xf::compression::details::singleStream2mm<GMEM_DATAWIDTH>(outStreamV, outStreamVSize, outSizeStream, out, outSize);
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmEosNb(ap_uint<DATAWIDTH>* out,
               const uint32_t output_idx[NUM_BLOCKS],
               hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCKS],
               hls::stream<bool> endOfStream[NUM_BLOCKS],
               hls::stream<uint32_t> compressedSize[NUM_BLOCKS],
               STREAM_SIZE_DT output_size[NUM_BLOCKS]) {
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

    ap_uint<NUM_BLOCKS> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    ap_uint<NUM_BLOCKS> end_of_stream = 0;
    uint32_t read_size[NUM_BLOCKS];
    uint32_t write_size[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t burst_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = burst_size dim = 0 complete

    for (int i = 0; i < NUM_BLOCKS; i++) {
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

        for (int i = 0; i < NUM_BLOCKS; i++) {
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

        for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
            if (end_of_stream.range(i, i)) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }

    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS PIPELINE II = 1
        output_size[i] = compressedSize[i].read();
    }
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmEosNbFreq(ap_uint<DATAWIDTH>* out,
                   const uint32_t output_idx[NUM_BLOCKS],
                   hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCKS],
                   hls::stream<bool> endOfStream[NUM_BLOCKS],
                   hls::stream<uint32_t> inStreamTree[NUM_BLOCKS],
                   hls::stream<uint32_t> compressedSize[NUM_BLOCKS],
                   STREAM_SIZE_DT output_size[NUM_BLOCKS],
                   const STREAM_SIZE_DT input_size[NUM_BLOCKS],
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

    ap_uint<NUM_BLOCKS> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    ap_uint<NUM_BLOCKS> end_of_stream = 0;
    uint32_t read_size[NUM_BLOCKS];
    uint32_t write_size[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t burst_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = burst_size dim = 0 complete

    for (int i = 0; i < NUM_BLOCKS; i++) {
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

        for (int i = 0; i < NUM_BLOCKS; i++) {
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

        for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
            if (end_of_stream.range(i, i)) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }

    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS PIPELINE II = 1
        output_size[i] = compressedSize[i].read();
    }

    uint32_t val = 0;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH>
void s2mm_compress(ap_uint<DATAWIDTH>* out,
                   const uint32_t output_idx[PARALLEL_BLOCK],
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_0,
#if PARALLEL_BLOCK > 1
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_1,
#endif
#if PARALLEL_BLOCK > 2
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_2,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_3,
#endif
#if PARALLEL_BLOCK > 4
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_4,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_5,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_6,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream_7,
#endif
                   hls::stream<uint16_t>& inStreamSize_0,
#if PARALLEL_BLOCK > 1
                   hls::stream<uint16_t>& inStreamSize_1,
#endif
#if PARALLEL_BLOCK > 2
                   hls::stream<uint16_t>& inStreamSize_2,
                   hls::stream<uint16_t>& inStreamSize_3,
#endif
#if PARALLEL_BLOCK > 4
                   hls::stream<uint16_t>& inStreamSize_4,
                   hls::stream<uint16_t>& inStreamSize_5,
                   hls::stream<uint16_t>& inStreamSize_6,
                   hls::stream<uint16_t>& inStreamSize_7,
#endif
                   STREAM_SIZE_DT output_size[PARALLEL_BLOCK]) {
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int c_max_burst_size = c_word_size * BURST_SIZE;
    uint32_t input_size[PARALLEL_BLOCK];
    uint32_t read_size[PARALLEL_BLOCK];
    uint32_t write_size[PARALLEL_BLOCK];
    uint32_t burst_size[PARALLEL_BLOCK];
    uint32_t write_idx[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = burst_size dim = 0 complete
    ap_uint<PARALLEL_BLOCK> end_of_stream = 0;
    ap_uint<PARALLEL_BLOCK> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[PARALLEL_BLOCK][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    // printme("%s:Started\n", __FUNCTION__);
    for (int i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        input_size[i] = 0;
        read_size[i] = 0;
        write_size[i] = 0;
        write_idx[i] = 0;
        // printme("%s:Indx=%d out_idx=%d\n",__FUNCTION__,i , output_idx[i]);
    }
    bool done = false;
    uint32_t loc = 0;
    uint32_t remaining_data = 0;
    while (is_pending != 0) {
        STREAM_UTILS_S2MM_READ_SIZE(0, inStreamSize_0, end_of_stream);
#if PARALLEL_BLOCK > 1
        STREAM_UTILS_S2MM_READ_SIZE(1, inStreamSize_1, end_of_stream);
#endif
#if PARALLEL_BLOCK > 2
        STREAM_UTILS_S2MM_READ_SIZE(2, inStreamSize_2, end_of_stream);
        STREAM_UTILS_S2MM_READ_SIZE(3, inStreamSize_3, end_of_stream);
#endif
#if PARALLEL_BLOCK > 4
        STREAM_UTILS_S2MM_READ_SIZE(4, inStreamSize_4, end_of_stream);
        STREAM_UTILS_S2MM_READ_SIZE(5, inStreamSize_5, end_of_stream);
        STREAM_UTILS_S2MM_READ_SIZE(6, inStreamSize_6, end_of_stream);
        STREAM_UTILS_S2MM_READ_SIZE(7, inStreamSize_7, end_of_stream);
#endif
        done = false;
        for (int i = 0; (is_pending != 0) && (done == 0); i++) {
#pragma HLS PIPELINE II = 1
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(0, inStream_0, burst_size, input_size, read_size, write_size, write_idx);
#if PARALLEL_BLOCK > 1
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(1, inStream_1, burst_size, input_size, read_size, write_size, write_idx);
#endif
#if PARALLEL_BLOCK > 2
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(2, inStream_2, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(3, inStream_3, burst_size, input_size, read_size, write_size, write_idx);
#endif
#if PARALLEL_BLOCK > 4
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(4, inStream_4, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(5, inStream_5, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(6, inStream_6, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_IF_NOT_EMPTY(7, inStream_7, burst_size, input_size, read_size, write_size, write_idx);
#endif
        }

        for (int i = 0; i < PARALLEL_BLOCK; i++) {
            // Write the data to global memory
            if ((read_size[i] > write_size[i]) && (read_size[i] - write_size[i]) >= burst_size[i]) {
                uint32_t base_addr = output_idx[i] + write_size[i];
                uint32_t base_idx = base_addr / c_word_size;
                uint32_t burst_size_in_words = (burst_size[i]) ? ((burst_size[i] - 1) / c_word_size + 1) : 0;
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
            if (end_of_stream.range(i, i) && (write_size[i] >= input_size[i])) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }
    // printme("%s:Ended \n", __FUNCTION__);
    for (int i = 0; i < PARALLEL_BLOCK; i++) {
        output_size[i] = input_size[i];
    }
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmNb(ap_uint<DATAWIDTH>* out,
            const uint32_t output_idx[NUM_BLOCKS],
            hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCKS],
            const STREAM_SIZE_DT input_size[NUM_BLOCKS]) {
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
    uint32_t read_size[NUM_BLOCKS];
    uint32_t write_size[NUM_BLOCKS];
    uint32_t burst_size[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = input_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = burst_size dim = 0 complete
    ap_uint<NUM_BLOCKS> end_of_stream = 0;
    ap_uint<NUM_BLOCKS> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    // printme("%s:Started\n", __FUNCTION__);
    for (int i = 0; i < NUM_BLOCKS; i++) {
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

        for (int i = 0; i < NUM_BLOCKS; i++) {
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
        for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
            if (done == true && (write_size[i] >= input_size[i])) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }
}

template <int BURST_SIZE, int DATAWIDTH, int NUM_BLOCKS>
void s2mmNb(ap_uint<DATAWIDTH>* out,
            const uint32_t output_idx[NUM_BLOCKS],
            hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCKS],
            hls::stream<bool> endOfStream[NUM_BLOCKS],
            const uint32_t input_size[NUM_BLOCKS]) {
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
    uint32_t read_size[NUM_BLOCKS];
    uint32_t write_size[NUM_BLOCKS];
    uint32_t burst_size[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = input_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = burst_size dim = 0 complete
    ap_uint<NUM_BLOCKS> end_of_stream = 0;
    ap_uint<NUM_BLOCKS> is_pending = 1;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM

    // printme("%s:Started\n", __FUNCTION__);
    for (int i = 0; i < NUM_BLOCKS; i++) {
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
                        bool eos = endOfStream[pb].read();
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

        for (int i = 0; i < NUM_BLOCKS; i++) {
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
        for (int i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
            if (done == true && (write_size[i] >= input_size[i])) {
                is_pending.range(i, i) = 0;
            } else {
                is_pending.range(i, i) = 1;
            }
        }
    }
    ap_uint<DATAWIDTH> tmp = inStream[0].read();
    bool eos = endOfStream[0].read();
}

template <int DATAWIDTH, int BURST_SIZE>
void s2mmEosSimple(ap_uint<DATAWIDTH>* out,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream,
                   hls::stream<bool>& endOfStream,
                   hls::stream<uint32_t>& outSize,
                   uint32_t* output_size) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR.
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param endOfStream stream to indicate end of data stream
     * @param outSize output data size
     */
    bool eos = false;
    ap_uint<DATAWIDTH> dummy = 0;
s2mm_eos_simple:
    for (int j = 0; eos == false; j += BURST_SIZE) {
        for (int i = 0; i < BURST_SIZE; i++) {
#pragma HLS PIPELINE II = 1
            out[j + i] = (eos == true) ? dummy : inStream.read();
            eos = (eos == true) ? true : endOfStream.read();
        }
    }
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
} // namespace details
} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_S2MM_HPP_
