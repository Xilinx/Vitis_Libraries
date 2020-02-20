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
#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include "stream_downsizer.hpp"

#define GET_DIFF_IF_BIG(x, y) (x > y) ? (x - y) : 0

#define STREAM_UTILS_MM2S_IF_NOT_FULL(bIdx, outStream, is_full, read_idx, write_idx, local_buffer) \
    is_full.range(bIdx, bIdx) = outStream.full();                                                  \
    if (!is_full.range(bIdx, bIdx) && (read_idx[bIdx] != write_idx[bIdx])) {                       \
        outStream << local_buffer[bIdx][read_idx[bIdx]];                                           \
        read_idx[bIdx] += 1;                                                                       \
    }

namespace xf {
namespace compression {

const int kGMemDWidth = 512;
typedef ap_uint<kGMemDWidth> uintMemWidth_t;

const int c_lTreeSize = 1024;
const int c_dTreeSize = 64;
const int c_bLTreeSize = 64;
const int c_maxCodeSize = 16;

namespace details {

template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNb(const ap_uint<DATAWIDTH>* in,
            const uint32_t _input_idx[NUM_BLOCKS],
            hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
            const uint32_t _input_size[NUM_BLOCKS]) {
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
     * @param _input_size input stream size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_idx[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<NUM_BLOCKS> pending;
    ap_uint<NUM_BLOCKS> is_full;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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
        ap_uint<NUM_BLOCKS> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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

template <int NUM_BLOCKS, int IN_DATAWIDTH, int OUT_DATAWIDTH, int BURST_SIZE>
void mm2multStream(const ap_uint<IN_DATAWIDTH>* in,
                   const uint32_t input_idx[NUM_BLOCKS],
                   hls::stream<ap_uint<OUT_DATAWIDTH> > outStream[NUM_BLOCKS],
                   const uint32_t _input_size[NUM_BLOCKS]) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output stream in 8-bit chunks. Writing to the multiple data streams is
     * non-blocking call which is done using is_full() API
     *
     * @tparam DATAWIDTH input width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam NUM_BLOCKS number of parallel blocks
     * @tparam OUT_DATAWIDTH output width of the data bus
     *
     * @param in input memory address
     * @param input_idx input index
     * @param outStream output stream
     * @param _input_size input stream size
     */

    // Array of Streams used as internal buffer.
    hls::stream<ap_uint<IN_DATAWIDTH> > outStreamBuffer[NUM_BLOCKS];
#pragma HLS STREAM variable = outStreamBuffer depth = 16
#pragma HLS RESOURCE variable = outStreamBuffer core = FIFO_SRL

    // Local buffer to store the input_size for all PARALLEL BLOCKS
    // for multiple reads
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = input_size complete dim = 1

    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
#pragma HLS UNROLL
        input_size[bIdx] = _input_size[bIdx];
    }

#pragma HLS DATAFLOW
    // Calling the mm2sNb module
    xf::compression::details::mm2sNb<IN_DATAWIDTH, BURST_SIZE, NUM_BLOCKS>(in, input_idx, outStreamBuffer, input_size);

// Calling the Downsizer parallelly for entire array of outStreams
parallel_downsizer:
    for (uint32_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
        xf::compression::details::streamDownsizer<uint32_t, IN_DATAWIDTH, OUT_DATAWIDTH>(outStreamBuffer[i],
                                                                                         outStream[i], input_size[i]);
    }
}

template <int NUM_BLOCKS, int DATAWIDTH, int BURST_SIZE>
void mm2multStreamSimple(const ap_uint<DATAWIDTH>* in,
                         hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
                         hls::stream<uint16_t> outSizeStream[NUM_BLOCKS],
                         const uint32_t input_idx[NUM_BLOCKS],
                         const uint32_t _input_size[NUM_BLOCKS]) {
    /**
         * @brief This module reads 512-bit data from memory interface and
         * writes to the output streams and output size streams
         *
         * @tparam DATAWIDTH input width of data bus
         * @tparam BURST_SIZE burst size of the data transfers
         * @tparam NUM_BLOCKS number of parallel blocks
         *
         * @param in input memory address
         * @param input_idx input index
         * @param outStream output stream
         * @param outSizeStream output size stream
         * @param _input_size input stream size
         */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;

    ap_uint<NUM_BLOCKS> is_pending;
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_size dim = 0 complete

    for (uint8_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS UNROLL
        read_idx[vid] = input_idx[vid] / c_wordSize;
        input_size[vid] = _input_size[vid];
        read_size[vid] = 0;
        is_pending.range(vid, vid) = 1;
    }

    while (is_pending) {
    parallel_ops:
        for (uint32_t vid = 0; vid < NUM_BLOCKS; vid++) {
            bool isFull = (outSizeStream[vid]).full();
            uint32_t pendingBytes = (input_size[vid] > read_size[vid]) ? (input_size[vid] - read_size[vid]) : 0;
            uint32_t sizeWrite = 0;
            if (pendingBytes && !isFull) {
                uint32_t pendingWords = (pendingBytes - 1) / c_wordSize + 1;
                uint32_t burstSize = (pendingWords > BURST_SIZE) ? BURST_SIZE : pendingWords;
                sizeWrite = burstSize * c_wordSize;
                outSizeStream[vid] << sizeWrite;
            gmem_read:
                for (uint32_t midx = 0; midx < burstSize; midx++) {
#pragma HLS PIPELINE II = 1
                    outStream[vid] << in[read_idx[vid] + midx];
                }
                read_idx[vid] += burstSize;
                read_size[vid] += sizeWrite;
            } else {
                is_pending.range(vid, vid) = 0;
            }
        }
    }
size_init:
    for (uint8_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS UNROLL
        outSizeStream[vid] << 0;
    }
}

template <int IN_DATAWIDTH, int OUT_DATAWIDTH>
void mm2multStreamDownSizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                            hls::stream<uint16_t>& inSizeStream,
                            hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream,
                            hls::stream<bool>& outStreamEoS) {
    /**
         * @brief This module reads 512-bit data from stream interface and
         * writes to the output stream in 8-bit chunks using the size stream.
         *
         * @tparam IN_DATAWIDTH input width of data bus
         * @tparam OUT_DATAWIDTH output width of the data bus
         *
         * @param inStream input stream
         * @param inSizeStream input size stream
         * @param outStream output stream
         * @param outStreamEos output end of stream
         */

    const int c_byteWidth = 8;
    const int c_inputWord = IN_DATAWIDTH / c_byteWidth;
    const int c_outWord = OUT_DATAWIDTH / c_byteWidth;
    const int factor = c_inputWord / c_outWord;
    ap_uint<IN_DATAWIDTH> inBuffer = 0;

downsizer_top:
    for (uint16_t inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        uint16_t outSizeV = (inSize - 1) / c_outWord + 1;
    downsizer_assign:
        for (uint16_t itr = 0; itr < outSizeV; itr++) {
#pragma HLS PIPELINE II = 1
            int idx = itr % factor;
            if (idx == 0) inBuffer = inStream.read();
            ap_uint<OUT_DATAWIDTH> tmpValue = inBuffer.range((idx + 1) * OUT_DATAWIDTH - 1, idx * OUT_DATAWIDTH);
            outStream << tmpValue;
            outStreamEoS << 0;
        }
    }
    outStreamEoS << 1;
}

template <int NUM_BLOCKS, int IN_DATAWIDTH, int OUT_DATAWIDTH, int BURST_SIZE>
void mm2multStreamNew(const ap_uint<IN_DATAWIDTH>* in,
                      const uint32_t input_idx[NUM_BLOCKS],
                      hls::stream<ap_uint<OUT_DATAWIDTH> > outStream[NUM_BLOCKS],
                      hls::stream<bool> outStreamEoS[NUM_BLOCKS],
                      const uint32_t _input_size[NUM_BLOCKS]) {
    /**
         * @brief This module reads 512-bit data from memory interface and
         * writes to the output streams in 8-bit chunks. Writing to the multiple data streams is
         * non-blocking call which is done using is_full() API
         *
         * @tparam NUM_BLOCKS number of parallel blocks
         * @tparam IN_DATAWIDTH input width of data bus
     * @tparam OUT_DATAWIDTH output width of the data bus
         * @tparam BURST_SIZE burst size of the data transfers
     *
         *
         * @param in input memory address
         * @param input_idx input index
         * @param outStream output stream
         * @param outStreamEos output end of stream
         * @param _input_size input stream size
         */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    // Array of Streams used as internal buffer.
    hls::stream<ap_uint<IN_DATAWIDTH> > outStreamV[NUM_BLOCKS];
    hls::stream<uint16_t> outStreamVSize[NUM_BLOCKS];
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 2
#pragma HLS RESOURCE variable = outStreamV core = FIFO_SRL
#pragma HLS RESOURCE variable = outStreamVSize core = FIFO_SRL

#pragma HLS DATAFLOW
    xf::compression::details::mm2multStreamSimple<NUM_BLOCKS, IN_DATAWIDTH, BURST_SIZE>(in, outStreamV, outStreamVSize,
                                                                                        input_idx, _input_size);
downsizer:
    for (uint8_t vid = 0; vid < NUM_BLOCKS; vid++) {
#pragma HLS UNROLL
        xf::compression::details::mm2multStreamDownSizer<IN_DATAWIDTH, OUT_DATAWIDTH>(
            outStreamV[vid], outStreamVSize[vid], outStream[vid], outStreamEoS[vid]);
    }
}

template <int DATAWIDTH, int BURST_SIZE>
void mm2SingleStream(const ap_uint<DATAWIDTH>* in,
                     hls::stream<ap_uint<DATAWIDTH> >& outStream,
                     hls::stream<uint16_t>& outSizeStream,
                     const uint32_t _input_size) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output streams and output size streams
     *
     * @tparam DATAWIDTH input width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     *
     * @param in input memory address
     * @param outStream output stream
     * @param outSizeStream output size stream
     * @param _input_size input stream size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const uint32_t c_burstSize = BURST_SIZE * c_wordSize;

    uint32_t read_idx = 0;
    uint32_t read_size = 0;
    uint32_t input_size = _input_size;

mm2StreamSimple:
    for (uint32_t idx = 0; idx < input_size; idx += c_burstSize) {
        uint32_t pendingBytes = (input_size > read_size) ? (input_size - read_size) : 0;
        uint32_t sizeWrite = 0;
        uint32_t pendingWords = (pendingBytes - 1) / c_wordSize + 1;
        uint32_t burstSize = (pendingWords > BURST_SIZE) ? BURST_SIZE : pendingWords;
        sizeWrite = burstSize * c_wordSize;
        if (read_size + sizeWrite < input_size) {
            outSizeStream << sizeWrite;
            read_size += sizeWrite;
        } else {
            outSizeStream << (input_size - read_size);
            read_size = input_size;
        }
    gmem_read:
        for (uint32_t midx = 0; midx < burstSize; midx++) {
#pragma HLS PIPELINE II = 1
            outStream << in[read_idx + midx];
        }
        read_idx += burstSize;
    }
    outSizeStream << 0;
}

template <int IN_DATAWIDTH, int OUT_DATAWIDTH>
void mm2StreamDownSizer(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                        hls::stream<uint16_t>& inSizeStream,
                        hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream) {
    /**
     * @brief This module reads 512-bit data from stream interface and
     * writes to the output stream in 8-bit chunks using the size stream.
     *
     * @tparam IN_DATAWIDTH input width of data bus
     * @tparam OUT_DATAWIDTH output width of the data bus
     *
     * @param inStream input stream
     * @param inSizeStream input size stream
     * @param outStream output stream
     */

    const int c_byteWidth = 8;
    const int c_inputWord = IN_DATAWIDTH / c_byteWidth;
    const int c_outWord = OUT_DATAWIDTH / c_byteWidth;
    const int factor = c_inputWord / c_outWord;
    ap_uint<IN_DATAWIDTH> inBuffer = 0;

downsizer_top:
    for (uint16_t inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        uint16_t outSizeV = (inSize - 1) / c_outWord + 1;
    downsizer_assign:
        for (uint16_t itr = 0; itr < outSizeV; itr++) {
#pragma HLS PIPELINE II = 1
            int idx = itr % factor;
            if (idx == 0) {
                inBuffer = inStream.read();
            } else {
                inBuffer >>= OUT_DATAWIDTH;
            }
            outStream << inBuffer.range(OUT_DATAWIDTH - 1, 0);
        }
    }
}

template <int OUT_DATAWIDTH, int GMEM_DATAWIDTH = 512, int BURST_SIZE = 16>
void mm2Stream(const ap_uint<GMEM_DATAWIDTH>* in,
               hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream,
               const uint32_t _input_size) {
    /**
     * @brief This module reads 512-bit data from memory interface and
     * writes to the output streams in 8-bit chunks. Writing to the multiple data streams is
     * non-blocking call which is done using is_full() API
     *
     * @tparam IN_DATAWIDTH input width of data bus
     * @tparam OUT_DATAWIDTH output width of the data bus
     * @tparam BURST_SIZE burst size of the data transfers
     *
     *
     * @param in input memory address
     * @param outStream output stream
     * @param _input_size input stream size
     */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    // Array of Streams used as internal buffer.
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV;
    hls::stream<uint16_t> outStreamVSize;
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 1
#pragma HLS RESOURCE variable = outStreamV core = FIFO_SRL

#pragma HLS DATAFLOW
    xf::compression::details::mm2SingleStream<GMEM_DATAWIDTH, BURST_SIZE>(in, outStreamV, outStreamVSize, _input_size);
    xf::compression::details::mm2StreamDownSizer<GMEM_DATAWIDTH, OUT_DATAWIDTH>(outStreamV, outStreamVSize, outStream);
}

#ifndef PARALLEL_BLOCK
#define PARALLEL_BLOCK 8
#endif

template <int DATAWIDTH, int BURST_SIZE>
void mm2s_freq(const ap_uint<DATAWIDTH>* in,
               uint32_t* dyn_ltree_codes,
               uint32_t* dyn_ltree_blen,
               uint32_t* dyn_dtree_codes,
               uint32_t* dyn_dtree_blen,
               uint32_t* dyn_bltree_codes,
               uint32_t* dyn_bltree_blen,
               uint32_t* dyn_maxcodes,
               const uint32_t _input_idx[PARALLEL_BLOCK],
               hls::stream<ap_uint<DATAWIDTH> >& outStream_0,
#if PARALLEL_BLOCK > 1
               hls::stream<ap_uint<DATAWIDTH> >& outStream_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<ap_uint<DATAWIDTH> >& outStream_2,
               hls::stream<ap_uint<DATAWIDTH> >& outStream_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<ap_uint<DATAWIDTH> >& outStream_4,
               hls::stream<ap_uint<DATAWIDTH> >& outStream_5,
               hls::stream<ap_uint<DATAWIDTH> >& outStream_6,
               hls::stream<ap_uint<DATAWIDTH> >& outStream_7,
#endif
               hls::stream<uint32_t>& stream_ltree_codes_0,
#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_ltree_codes_1,
#endif

#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_ltree_codes_2,
               hls::stream<uint32_t>& stream_ltree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_ltree_codes_4,
               hls::stream<uint32_t>& stream_ltree_codes_5,
               hls::stream<uint32_t>& stream_ltree_codes_6,
               hls::stream<uint32_t>& stream_ltree_codes_7,
#endif
               hls::stream<uint32_t>& stream_ltree_blen_0,
#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_ltree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_ltree_blen_2,
               hls::stream<uint32_t>& stream_ltree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_ltree_blen_4,
               hls::stream<uint32_t>& stream_ltree_blen_5,
               hls::stream<uint32_t>& stream_ltree_blen_6,
               hls::stream<uint32_t>& stream_ltree_blen_7,
#endif
               hls::stream<uint32_t>& stream_dtree_codes_0,
#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_dtree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_dtree_codes_2,
               hls::stream<uint32_t>& stream_dtree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_dtree_codes_4,
               hls::stream<uint32_t>& stream_dtree_codes_5,
               hls::stream<uint32_t>& stream_dtree_codes_6,
               hls::stream<uint32_t>& stream_dtree_codes_7,
#endif
               hls::stream<uint32_t>& stream_dtree_blen_0,
#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_dtree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_dtree_blen_2,
               hls::stream<uint32_t>& stream_dtree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_dtree_blen_4,
               hls::stream<uint32_t>& stream_dtree_blen_5,
               hls::stream<uint32_t>& stream_dtree_blen_6,
               hls::stream<uint32_t>& stream_dtree_blen_7,
#endif
               hls::stream<uint32_t>& stream_bltree_codes_0,
#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_bltree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_bltree_codes_2,
               hls::stream<uint32_t>& stream_bltree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_bltree_codes_4,
               hls::stream<uint32_t>& stream_bltree_codes_5,
               hls::stream<uint32_t>& stream_bltree_codes_6,
               hls::stream<uint32_t>& stream_bltree_codes_7,
#endif
               hls::stream<uint32_t>& stream_bltree_blen_0,

#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_bltree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_bltree_blen_2,
               hls::stream<uint32_t>& stream_bltree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_bltree_blen_4,
               hls::stream<uint32_t>& stream_bltree_blen_5,
               hls::stream<uint32_t>& stream_bltree_blen_6,
               hls::stream<uint32_t>& stream_bltree_blen_7,
#endif
               hls::stream<uint32_t>& stream_maxcode_0,
#if PARALLEL_BLOCK > 1
               hls::stream<uint32_t>& stream_maxcode_1,
#endif
#if PARALLEL_BLOCK > 2
               hls::stream<uint32_t>& stream_maxcode_2,
               hls::stream<uint32_t>& stream_maxcode_3,
#endif
#if PARALLEL_BLOCK > 4
               hls::stream<uint32_t>& stream_maxcode_4,
               hls::stream<uint32_t>& stream_maxcode_5,
               hls::stream<uint32_t>& stream_maxcode_6,
               hls::stream<uint32_t>& stream_maxcode_7,
#endif
               uint32_t n_blocks,
               const uint32_t _input_size[PARALLEL_BLOCK]) {
    uint32_t lcl_ltree_codes[PARALLEL_BLOCK][c_lTreeSize];
    uint32_t lcl_ltree_blen[PARALLEL_BLOCK][c_lTreeSize];

    uint32_t lcl_dtree_codes[PARALLEL_BLOCK][c_dTreeSize];
    uint32_t lcl_dtree_blen[PARALLEL_BLOCK][c_dTreeSize];

    uint32_t lcl_bltree_codes[PARALLEL_BLOCK][c_bLTreeSize];
    uint32_t lcl_bltree_blen[PARALLEL_BLOCK][c_bLTreeSize];

    uint32_t lcl_maxcode[PARALLEL_BLOCK * c_maxCodeSize];

    ////printme("In mm2s start\n");

    for (uint32_t blk = 0; blk < n_blocks; blk++) {
        memcpy(lcl_ltree_codes[blk], &dyn_ltree_codes[blk * c_lTreeSize], c_lTreeSize * sizeof(uint32_t));
        memcpy(lcl_ltree_blen[blk], &dyn_ltree_blen[blk * c_lTreeSize], c_lTreeSize * sizeof(uint32_t));
        memcpy(lcl_dtree_codes[blk], &dyn_dtree_codes[blk * c_dTreeSize], c_dTreeSize * sizeof(uint32_t));
        memcpy(lcl_dtree_blen[blk], &dyn_dtree_blen[blk * c_dTreeSize], c_dTreeSize * sizeof(uint32_t));
        memcpy(lcl_bltree_codes[blk], &dyn_bltree_codes[blk * c_bLTreeSize], c_bLTreeSize * sizeof(uint32_t));
        memcpy(lcl_bltree_blen[blk], &dyn_bltree_blen[blk * c_bLTreeSize], c_bLTreeSize * sizeof(uint32_t));
    }

    memcpy(lcl_maxcode, &dyn_maxcodes[0], PARALLEL_BLOCK * c_maxCodeSize * sizeof(uint32_t));

    for (uint32_t blk = 0; blk < n_blocks; blk++) {
        if (blk == 0) {
            stream_maxcode_0 << lcl_maxcode[blk * 3];
            stream_maxcode_0 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_0 << lcl_maxcode[blk * 3 + 2];
        }
#if PARALLEL_BLOCK > 1
        if (blk == 1) {
            stream_maxcode_1 << lcl_maxcode[blk * 3];
            stream_maxcode_1 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_1 << lcl_maxcode[blk * 3 + 2];
        }
#endif
#if PARALLEL_BLOCK > 2
        if (blk == 2) {
            stream_maxcode_2 << lcl_maxcode[blk * 3];
            stream_maxcode_2 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_2 << lcl_maxcode[blk * 3 + 2];
        }
        if (blk == 3) {
            stream_maxcode_3 << lcl_maxcode[blk * 3];
            stream_maxcode_3 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_3 << lcl_maxcode[blk * 3 + 2];
        }
#endif

#if PARALLEL_BLOCK > 4
        if (blk == 4) {
            stream_maxcode_4 << lcl_maxcode[blk * 3];
            stream_maxcode_4 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_4 << lcl_maxcode[blk * 3 + 2];
        }
        if (blk == 5) {
            stream_maxcode_5 << lcl_maxcode[blk * 3];
            stream_maxcode_5 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_5 << lcl_maxcode[blk * 3 + 2];
        }
        if (blk == 6) {
            stream_maxcode_6 << lcl_maxcode[blk * 3];
            stream_maxcode_6 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_6 << lcl_maxcode[blk * 3 + 2];
        }
        if (blk == 7) {
            stream_maxcode_7 << lcl_maxcode[blk * 3];
            stream_maxcode_7 << lcl_maxcode[blk * 3 + 1];
            stream_maxcode_7 << lcl_maxcode[blk * 3 + 2];
        }
#endif
        // Literal Tree
        for (uint32_t i = 0; i < c_lTreeSize; i++) {
            if (blk == 0) {
                stream_ltree_codes_0 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_0 << lcl_ltree_blen[blk][i];
            }
#if PARALLEL_BLOCK > 1
            if (blk == 1) {
                stream_ltree_codes_1 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_1 << lcl_ltree_blen[blk][i];
            }
#endif
#if PARALLEL_BLOCK > 2
            if (blk == 2) {
                stream_ltree_codes_2 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_2 << lcl_ltree_blen[blk][i];
            }
            if (blk == 3) {
                stream_ltree_codes_3 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_3 << lcl_ltree_blen[blk][i];
            }
#endif

#if PARALLEL_BLOCK > 4
            if (blk == 4) {
                stream_ltree_codes_4 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_4 << lcl_ltree_blen[blk][i];
            }
            if (blk == 5) {
                stream_ltree_codes_5 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_5 << lcl_ltree_blen[blk][i];
            }
            if (blk == 6) {
                stream_ltree_codes_6 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_6 << lcl_ltree_blen[blk][i];
            }
            if (blk == 7) {
                stream_ltree_codes_7 << lcl_ltree_codes[blk][i];
                stream_ltree_blen_7 << lcl_ltree_blen[blk][i];
            }
#endif
        }

        // Distance Tree
        for (uint32_t i = 0; i < c_dTreeSize; i++) {
            if (blk == 0) {
                stream_dtree_codes_0 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_0 << lcl_dtree_blen[blk][i];
            }
#if PARALLEL_BLOCK > 1
            if (blk == 1) {
                stream_dtree_codes_1 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_1 << lcl_dtree_blen[blk][i];
            }
#endif
#if PARALLEL_BLOCK > 2
            if (blk == 2) {
                stream_dtree_codes_2 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_2 << lcl_dtree_blen[blk][i];
            }
            if (blk == 3) {
                stream_dtree_codes_3 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_3 << lcl_dtree_blen[blk][i];
            }
#endif

#if PARALLEL_BLOCK > 4
            if (blk == 4) {
                stream_dtree_codes_4 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_4 << lcl_dtree_blen[blk][i];
            }
            if (blk == 5) {
                stream_dtree_codes_5 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_5 << lcl_dtree_blen[blk][i];
            }
            if (blk == 6) {
                stream_dtree_codes_6 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_6 << lcl_dtree_blen[blk][i];
            }
            if (blk == 7) {
                stream_dtree_codes_7 << lcl_dtree_codes[blk][i];
                stream_dtree_blen_7 << lcl_dtree_blen[blk][i];
            }
#endif
        }

        // Bitlength  Tree
        for (uint32_t i = 0; i < c_bLTreeSize; i++) {
            if (blk == 0) {
                stream_bltree_codes_0 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_0 << lcl_bltree_blen[blk][i];
            }
#if PARALLEL_BLOCK > 1
            if (blk == 1) {
                stream_bltree_codes_1 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_1 << lcl_bltree_blen[blk][i];
            }
#endif

#if PARALLEL_BLOCK > 2
            if (blk == 2) {
                stream_bltree_codes_2 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_2 << lcl_bltree_blen[blk][i];
            }
            if (blk == 3) {
                stream_bltree_codes_3 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_3 << lcl_bltree_blen[blk][i];
            }
#endif

#if PARALLEL_BLOCK > 4
            if (blk == 4) {
                stream_bltree_codes_4 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_4 << lcl_bltree_blen[blk][i];
            }
            if (blk == 5) {
                stream_bltree_codes_5 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_5 << lcl_bltree_blen[blk][i];
            }
            if (blk == 6) {
                stream_bltree_codes_6 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_6 << lcl_bltree_blen[blk][i];
            }
            if (blk == 7) {
                stream_bltree_codes_7 << lcl_bltree_codes[blk][i];
                stream_bltree_blen_7 << lcl_bltree_blen[blk][i];
            }
#endif
        }
    }

    ////printme("In mm2s end\n");

    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
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
        for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
            uint32_t pending_bytes = GET_DIFF_IF_BIG(input_size[bIdx], read_size[bIdx]);
            if ((pending_bytes) && (read_idx[bIdx] == write_idx[bIdx])) {
                uint32_t pending_words = (pending_bytes - 1) / c_word_size + 1;
                uint32_t burst_size = (pending_words > BURST_SIZE) ? BURST_SIZE : pending_words;
                uint32_t mem_read_byte_idx = read_size[bIdx] + input_idx[bIdx];
                uint32_t mem_read_word_idx = (mem_read_byte_idx) ? ((mem_read_byte_idx - 1) / c_word_size + 1) : 0;
            gmem_rd:
                for (uint32_t i = 0; i < burst_size; i++) {
#pragma HLS PIPELINE II = 1
                    local_buffer[bIdx][i] = in[mem_read_word_idx + i];
                }
                pending.range(bIdx, bIdx) = 1;
                read_idx[bIdx] = 0;
                write_idx[bIdx] = burst_size;
                read_size[bIdx] += burst_size * c_word_size;
            }
        }
        ap_uint<PARALLEL_BLOCK> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            STREAM_UTILS_MM2S_IF_NOT_FULL(0, outStream_0, is_full, read_idx, write_idx, local_buffer);
#if PARALLEL_BLOCK > 1
            STREAM_UTILS_MM2S_IF_NOT_FULL(1, outStream_1, is_full, read_idx, write_idx, local_buffer);
#endif
#if PARALLEL_BLOCK > 2
            STREAM_UTILS_MM2S_IF_NOT_FULL(2, outStream_2, is_full, read_idx, write_idx, local_buffer);
            STREAM_UTILS_MM2S_IF_NOT_FULL(3, outStream_3, is_full, read_idx, write_idx, local_buffer);
#endif
#if PARALLEL_BLOCK > 4
            STREAM_UTILS_MM2S_IF_NOT_FULL(4, outStream_4, is_full, read_idx, write_idx, local_buffer);
            STREAM_UTILS_MM2S_IF_NOT_FULL(5, outStream_5, is_full, read_idx, write_idx, local_buffer);
            STREAM_UTILS_MM2S_IF_NOT_FULL(6, outStream_6, is_full, read_idx, write_idx, local_buffer);
            STREAM_UTILS_MM2S_IF_NOT_FULL(7, outStream_7, is_full, read_idx, write_idx, local_buffer);
#endif

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

template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNbFreq(const ap_uint<DATAWIDTH>* in,
                uint32_t* dyn_ltree_codes,
                uint32_t* dyn_ltree_blen,
                uint32_t* dyn_dtree_codes,
                uint32_t* dyn_dtree_blen,
                uint32_t* dyn_bltree_codes,
                uint32_t* dyn_bltree_blen,
                uint32_t* dyn_maxcodes,
                const uint32_t _input_idx[NUM_BLOCKS],
                hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
                hls::stream<uint32_t> stream_ltree_codes[NUM_BLOCKS],
                hls::stream<uint32_t> stream_ltree_blen[NUM_BLOCKS],
                hls::stream<uint32_t> stream_dtree_codes[NUM_BLOCKS],
                hls::stream<uint32_t> stream_dtree_blen[NUM_BLOCKS],
                hls::stream<uint32_t> stream_bltree_codes[NUM_BLOCKS],
                hls::stream<uint32_t> stream_bltree_blen[NUM_BLOCKS],
                hls::stream<uint32_t> stream_maxcode[NUM_BLOCKS],
                uint32_t n_blocks,
                const uint32_t _input_size[NUM_BLOCKS]) {
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
     * @param _input_size input stream size
     */

    uint32_t lcl_ltree_codes[NUM_BLOCKS][c_lTreeSize];
    uint32_t lcl_ltree_blen[NUM_BLOCKS][c_lTreeSize];

    uint32_t lcl_dtree_codes[NUM_BLOCKS][c_dTreeSize];
    uint32_t lcl_dtree_blen[NUM_BLOCKS][c_dTreeSize];

    uint32_t lcl_bltree_codes[NUM_BLOCKS][c_bLTreeSize];
    uint32_t lcl_bltree_blen[NUM_BLOCKS][c_bLTreeSize];

    uint32_t lcl_maxcode[NUM_BLOCKS * c_maxCodeSize];

    for (uint32_t blk = 0; blk < n_blocks; blk++) {
    cpy_ltree_codes:
        for (uint32_t ci = 0; ci < c_lTreeSize; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_ltree_codes[blk][ci] = dyn_ltree_codes[blk * c_lTreeSize + ci];
        }
    cpy_ltree_blen:
        for (uint32_t ci = 0; ci < c_lTreeSize; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_ltree_blen[blk][ci] = dyn_ltree_blen[blk * c_lTreeSize + ci];
        }
    cpy_dtree_codes:
        for (uint32_t ci = 0; ci < c_dTreeSize; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_dtree_codes[blk][ci] = dyn_dtree_codes[blk * c_dTreeSize + ci];
        }
    cpy_dtree_blen:
        for (uint32_t ci = 0; ci < c_dTreeSize; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_dtree_blen[blk][ci] = dyn_dtree_blen[blk * c_dTreeSize + ci];
        }
    cpy_bltree_codes:
        for (uint32_t ci = 0; ci < c_bLTreeSize; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_bltree_codes[blk][ci] = dyn_bltree_codes[blk * c_bLTreeSize + ci];
        }
    cpy_bltree_blen:
        for (uint32_t ci = 0; ci < c_bLTreeSize; ++ci) {
#pragma HLS PIPELINE II = 1
            lcl_bltree_blen[blk][ci] = dyn_bltree_blen[blk * c_bLTreeSize + ci];
        }
    }

cpy_maxcodes:
    for (uint32_t ci = 0; ci < NUM_BLOCKS * c_maxCodeSize; ++ci) {
#pragma HLS PIPELINE II = 1
        lcl_maxcode[ci] = dyn_maxcodes[ci];
    }

    for (uint32_t blk = 0; blk < n_blocks; blk++) {
        stream_maxcode[blk] << lcl_maxcode[blk * 3];
        stream_maxcode[blk] << lcl_maxcode[blk * 3 + 1];
        stream_maxcode[blk] << lcl_maxcode[blk * 3 + 2];

        // Literal Tree
        for (uint32_t i = 0; i < c_lTreeSize; i++) {
            stream_ltree_codes[blk] << lcl_ltree_codes[blk][i];
            stream_ltree_blen[blk] << lcl_ltree_blen[blk][i];
        }

        // Distance Tree
        for (uint32_t i = 0; i < c_dTreeSize; i++) {
            stream_dtree_codes[blk] << lcl_dtree_codes[blk][i];
            stream_dtree_blen[blk] << lcl_dtree_blen[blk][i];
        }

        // Bitlength  Tree
        for (uint32_t i = 0; i < c_bLTreeSize; i++) {
            stream_bltree_codes[blk] << lcl_bltree_codes[blk][i];
            stream_bltree_blen[blk] << lcl_bltree_blen[blk][i];
        }
    }

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_idx[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<NUM_BLOCKS> pending;
    ap_uint<NUM_BLOCKS> is_full;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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
        ap_uint<NUM_BLOCKS> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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

template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNbRoundOff(const ap_uint<DATAWIDTH>* in,
                    const uint32_t _input_idx[NUM_BLOCKS],
                    hls::stream<ap_uint<DATAWIDTH> > outStream[NUM_BLOCKS],
                    const uint32_t _input_size[NUM_BLOCKS]) {
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
     * @param _input_size input stream size
     * @param max_buffer_size_in_bytes Maximum buffer size for indexing
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> local_buffer[NUM_BLOCKS][BURST_SIZE];
#pragma HLS ARRAY_PARTITION variable = local_buffer dim = 1 complete
#pragma HLS RESOURCE variable = local_buffer core = RAM_2P_LUTRAM
    uint32_t read_idx[NUM_BLOCKS];
    uint32_t write_idx[NUM_BLOCKS];
    uint32_t read_size[NUM_BLOCKS];
    uint32_t input_idx[NUM_BLOCKS];
    uint32_t input_size[NUM_BLOCKS];
#pragma HLS ARRAY_PARTITION variable = read_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = write_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = read_size dim = 0 complete
    ap_uint<NUM_BLOCKS> pending;
    ap_uint<NUM_BLOCKS> is_full;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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
        ap_uint<NUM_BLOCKS> terminate_all;
        terminate_all = 1;
        bool terminate = 0;
    mm2s:
        for (int i = 0; (terminate == 0) && (terminate_all != 0); i++) {
#pragma HLS PIPELINE II = 1
            for (uint8_t pb = 0; pb < NUM_BLOCKS; pb++) {
#pragma HLS UNROLL
                is_full.range(pb, pb) = outStream[pb].full();
                if (!is_full.range(pb, pb) && (read_idx[pb] != write_idx[pb])) {
                    outStream[pb] << local_buffer[pb][read_idx[pb]];
                    read_idx[pb] += 1;
                }
            }
            terminate = 0;
            for (uint32_t bIdx = 0; bIdx < NUM_BLOCKS; bIdx++) {
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

template <int DATAWIDTH>
void mm2sSimple(const ap_uint<DATAWIDTH>* in, hls::stream<ap_uint<DATAWIDTH> >& outstream, uint32_t inputSize) {
    /**
     * @brief Read data from 512-bit wide axi memory interface and
     *        write to stream.
     *
     * @tparam DATAWIDTH    width of data bus
     *
     * @param in            pointer to input memory
     * @param outstream     output stream
     * @param inputSize     size of the data
     *
     */
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int inSize_gmemwidth = (inputSize - 1) / c_word_size + 1;

mm2s_simple:
    for (int i = 0; i < inSize_gmemwidth; i++) {
#pragma HLS PIPELINE II = 1
        outstream << in[i];
    }
}

template <int DATAWIDTH, int BURST_SIZE>
void mm2sSimple(const ap_uint<DATAWIDTH>* in, hls::stream<ap_uint<DATAWIDTH> >& outstream, uint32_t inputSize) {
    /**
     * @brief Read data from DATAWIDTH wide axi memory interface and
     *        write to stream.
     *
     * @tparam DATAWIDTH    width of data bus
     *
     * @param in            pointer to input memory
     * @param outstream     output stream
     * @param inputSize     size of the data
     *
     */
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int inSize_gmemwidth = (inputSize - 1) / c_word_size + 1;

    int allignedwidth = inSize_gmemwidth / BURST_SIZE;
    allignedwidth = ((inSize_gmemwidth - allignedwidth) > 0) ? allignedwidth + 1 : allignedwidth;

    int i = 0;
    ap_uint<DATAWIDTH> temp;
mm2s_simple:
    for (; i < allignedwidth * BURST_SIZE; i += BURST_SIZE) {
        for (uint32_t j = 0; j < BURST_SIZE; j++) {
#pragma HLS PIPELINE II = 1
            temp = in[i + j];
            if ((i + j) < inSize_gmemwidth) outstream << temp;
        }
    }
}

template <int DATAWIDTH, int BURST_SIZE>
void mm2s(const uintMemWidth_t* in,
          uintMemWidth_t* head_prev_blk,
          uintMemWidth_t* orig_input_data,
          hls::stream<ap_uint<DATAWIDTH> >& outStream,
          hls::stream<uint32_t>& outStreamSize,
          uint32_t* compressd_size,
          uint32_t* in_block_size,
          uint32_t no_blocks,
          uint32_t block_size_in_kb,
          uint32_t head_res_size,
          uint32_t offset) {
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    ap_uint<DATAWIDTH> buffer[BURST_SIZE];
#pragma HLS RESOURCE variable = buffer core = RAM_2P_LUTRAM

    uint32_t offset_gmem = offset ? offset / 64 : 0;

    // Handle header or residue here
    uint32_t block_stride = block_size_in_kb * 1024 / 64;

    uint32_t blkCompSize = 0;
    uint32_t origSize = 0;
    uint32_t sizeInWord = 0;
    uint32_t byteSize = 0;
    // Run over number of blocks
    for (int bIdx = 0; bIdx < no_blocks + 1; bIdx++) {
        if (bIdx == 0) {
            sizeInWord = head_res_size ? ((head_res_size - 1) / c_word_size + 1) : 0;
            byteSize = head_res_size;
        } else {
            blkCompSize = compressd_size[bIdx - 1];
            origSize = in_block_size[bIdx - 1];
            // Put compress block & input block
            // into streams for next block
            sizeInWord = (blkCompSize - 1) / c_word_size + 1;
            byteSize = blkCompSize;
        }

        // Send size in bytes
        outStreamSize << byteSize;

        // printf("[ %s ]blkCompSize %d origSize %d sizeInWord_512 %d offset %d head_res_size %d\n", __FUNCTION__,
        // blkCompSize, origSize, sizeInWord, offset, head_res_size);

        // Copy data from global memory to local
        // Put it into stream
        for (uint32_t i = 0; i < sizeInWord; i += BURST_SIZE) {
            uint32_t chunk_size = BURST_SIZE;

            if (i + BURST_SIZE > sizeInWord) chunk_size = sizeInWord - i;

            if (bIdx == 0) {
            memrd1:
                for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                    buffer[j] = head_prev_blk[(offset_gmem + i) + j];
                }
            } else if (blkCompSize == origSize) {
            memrd2:
                for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                    buffer[j] = orig_input_data[(block_stride * (bIdx - 1) + i) + j];
                }
            } else {
            memrd3:
                for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                    buffer[j] = in[(block_stride * (bIdx - 1) + i) + j];
                }
            }

        memrd4:
            for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                outStream << buffer[j];
            }
        }
    }
    // printf("%s Done \n", __FUNCTION__);
    outStreamSize << 0;
}
} // namespace details
} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_MM2S_HPP_
