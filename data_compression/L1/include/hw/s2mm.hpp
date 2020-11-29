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
#include "iostream"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "stream_upsizer.hpp"

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

template <int NUM_BLOCK = 4, int DATAWIDTH = 512, int BURST_SIZE = 16, int BLOCK_SIZE = 32768>
void multStream2MMNB(hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCK],
                     hls::stream<bool> inStreamEos[NUM_BLOCK],
                     hls::stream<uint32_t> inSizeStream[NUM_BLOCK],
                     hls::stream<uint32_t> inCmpBlkSizeStream[NUM_BLOCK],
                     hls::stream<uint32_t> inBaseIdx[NUM_BLOCK],
                     ap_uint<DATAWIDTH>* out,
                     uint32_t* compressedSize) {
    constexpr int c_byteSize = 8;
    constexpr int c_wordSize = DATAWIDTH / c_byteSize;
    constexpr int c_idx_fctr = BLOCK_SIZE / c_wordSize;

    ap_uint<NUM_BLOCK> is_pending;
    uint32_t write_size[NUM_BLOCK];
    uint32_t base_addr[NUM_BLOCK];
    uint8_t tmpVal[NUM_BLOCK];
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = base_addr dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = tmpVal dim = 0 complete

    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        is_pending.range(i, i) = 1;
        write_size[i] = 0;
        base_addr[i] = inBaseIdx[i].read();
        tmpVal[i] = 0;
    }

    while (is_pending) {
        for (int i = 0; i < NUM_BLOCK; i++) {
            uint32_t readSizeBytes = 0;
            if (!inSizeStream[i].empty()) {
                readSizeBytes = inSizeStream[i].read();
                is_pending.range(i, i) = (readSizeBytes > 0) ? 1 : 0;
            }
            if (readSizeBytes > 0) {
                uint32_t readSize = (readSizeBytes - 1) / c_wordSize + 1;
                uint32_t base_idx = base_addr[i] + write_size[i];
                bool eos = false;
                if (write_size[i] + readSize <= c_idx_fctr) {
                gmem_write:
                    for (int j = 0; j < readSize; j++) {
#pragma HLS PIPELINE II = 1
                        out[base_idx + j] = inStream[i].read();
                        eos = inStreamEos[i].read();
                    }
                } else {
                leftover_read:
                    for (int j = 0; j < readSize; j++) {
#pragma HLS PIPELINE II = 1
                        ap_uint<DATAWIDTH> tmp = inStream[i].read();
                        eos = inStreamEos[i].read();
                    }
                }
                write_size[i] += readSize;
                if (eos) {
                    base_addr[i] += c_idx_fctr;
                    tmpVal[i]++;
                    write_size[i] = 0;
                }
            }
        }
    }

    uint32_t idx = 0;
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
        for (uint8_t j = 0; j < tmpVal[i]; j++) {
#pragma HLS PIPELINE
            if (!inCmpBlkSizeStream[i].empty()) compressedSize[idx++] = inCmpBlkSizeStream[i].read();
        }
    }
}

template <int IN_DATAWIDTH, int NUM_BLOCK = 4, int GMEM_DATAWIDTH = 512, int BURST_SIZE = 16, int BLOCK_SIZE = 32768>
void multStream2MMBlockReceiver(hls::stream<ap_uint<IN_DATAWIDTH> > inStream[NUM_BLOCK],
                                hls::stream<bool> inStreamEos[NUM_BLOCK],
                                hls::stream<bool> inFileEos[NUM_BLOCK],
                                hls::stream<uint32_t> totalOutSizeStream[NUM_BLOCK],
                                hls::stream<uint32_t> inBaseIdx[NUM_BLOCK],
                                ap_uint<GMEM_DATAWIDTH>* out,
                                uint32_t* outSize) {
#pragma HLS DATAFLOW
    constexpr int c_depthOutStreamV = 2 * BURST_SIZE;

    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV[NUM_BLOCK];
    hls::stream<bool> outStreamVEos[NUM_BLOCK];
    hls::stream<uint32_t> outStreamVSize[NUM_BLOCK];
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVEos depth = c_depthOutStreamV

parallel_upsizer:
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        xf::compression::details::simpleUpsizer<IN_DATAWIDTH, GMEM_DATAWIDTH, BURST_SIZE>(
            inStream[i], inStreamEos[i], inFileEos[i], outStreamV[i], outStreamVEos[i], outStreamVSize[i]);
    }

    multStream2MMNB<NUM_BLOCK, GMEM_DATAWIDTH, BURST_SIZE, BLOCK_SIZE>(outStreamV, outStreamVEos, outStreamVSize,
                                                                       totalOutSizeStream, inBaseIdx, out, outSize);
}

template <int IN_WIDTH, int OUT_WIDTH, int BURST_SIZE>
void stream2mmUpsizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                      hls::stream<bool>& inStreamEos,
                      hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                      hls::stream<uint16_t>& outSizeStream) {
    /**
     * @brief This module reads IN_WIDTH data from stream until end of
     * stream happens and transfers OUT_WIDTH data into stream along with the
     * size of the chunk.
     *
     * @tparam IN_WIDTH width of input data bus
     * @tparam OUT_WIDTH width of output data bus
     * @tparam BURST_SIZE burst size
     *
     * @param inStream input stream
     * @param inStreamEos end flag for stream
     * @param outStream output stream
     * @param outSizeStream size stream for data stream
     */

    const int c_byteWidth = 8;
    const int c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    const int c_wordSize = OUT_WIDTH / c_byteWidth;
    const int c_size = BURST_SIZE * c_wordSize;

    ap_uint<OUT_WIDTH> outBuffer = 0;
    uint32_t byteIdx = 0;
    uint16_t sizeWrite = 0;
    ap_uint<IN_WIDTH> inValue = inStream.read();
stream_upsizer:
    for (bool eos_flag = inStreamEos.read(); eos_flag == false; eos_flag = inStreamEos.read()) {
#pragma HLS PIPELINE II = 1
        if (byteIdx == c_upsizeFactor) {
            outStream << outBuffer;
            sizeWrite++;
            if (sizeWrite == BURST_SIZE) {
                outSizeStream << c_size;
                sizeWrite = 0;
            }
            byteIdx = 0;
        }
        outBuffer.range((byteIdx + 1) * IN_WIDTH - 1, byteIdx * IN_WIDTH) = inValue;
        byteIdx++;
        inValue = inStream.read();
    }

    if (byteIdx) {
        outStream << outBuffer;
        sizeWrite++;
        outSizeStream << (sizeWrite * c_wordSize);
    }
    outSizeStream << 0;
}

template <int DATAWIDTH>
void singleStream2mm(hls::stream<ap_uint<DATAWIDTH> >& inStream,
                     hls::stream<uint16_t>& inSizeStream,
                     hls::stream<uint64_t>& outSizeStream,
                     ap_uint<DATAWIDTH>* out,
                     ap_uint<DATAWIDTH>* outSize) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on size stream
     * and writes the data to DDR. Writing data into multiple
     * data streams is non-blocking.
     *
     * @tparam DATAWIDTH width of data bus
     *
     * @param inStream input stream
     * @param inSizeStream size of input stream
     * @param outSizeStream size of output stream
     * @param out output memory address
     * @param output_size output size
     */

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
               hls::stream<uint64_t>& outSizeStream,
               ap_uint<GMEM_DATAWIDTH>* out,
               ap_uint<GMEM_DATAWIDTH>* outSize) {
    /**
     * @brief This module reads IN_DATAWIDTH data from stream until end of
     * stream happens and writes the data to DDR. Writing data into multiple
     * data streams is non-blocking.
     *
     * @tparam IN_WIDTH width of input data bus
     * @tparam OUT_WIDTH width of output data bus
     * @tparam BURST_SIZE burst size
     *
     * @param inStream input stream
     * @param inStreamEos input end flag for stream
     * @param outSizeStream size of output stream
     * @param out output memory address
     * @param output_size output size
     */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV;
    hls::stream<uint16_t> outStreamVSize;
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 2
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

#pragma HLS DATAFLOW
    xf::compression::details::stream2mmUpsizer<IN_DATAWIDTH, GMEM_DATAWIDTH, BURST_SIZE>(inStream, inStreamEos,
                                                                                         outStreamV, outStreamVSize);
    xf::compression::details::singleStream2mm<GMEM_DATAWIDTH>(outStreamV, outStreamVSize, outSizeStream, out, outSize);
}

template <int BURST_SIZE, int DATAWIDTH, int NUM_BLOCK>
void multStream2mmSize(hls::stream<ap_uint<DATAWIDTH> > inStream[NUM_BLOCK],
                       hls::stream<uint16_t> inSizeStream[NUM_BLOCK],
                       hls::stream<uint32_t> totalOutSizeStream[NUM_BLOCK],
                       const uint32_t output_idx[NUM_BLOCK],
                       ap_uint<DATAWIDTH>* out,
                       uint32_t outSize[NUM_BLOCK]) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on the size
     * stream and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API.
     *
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam DATAWIDTH width of data bus
     * @tparam NUM_BLOCK number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param inSizeStream size flag for input stream
     * @param totalOutSizeStream size of output stream
     * @param output_size output size
     */

    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;

    uint32_t write_size[NUM_BLOCK];
    uint32_t base_addr[NUM_BLOCK];
#pragma HLS ARRAY_PARTITION variable = write_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = base_addr dim = 0 complete
    ap_uint<NUM_BLOCK> is_pending;

    for (int vid = 0; vid < NUM_BLOCK; vid++) {
#pragma HLS UNROLL
        write_size[vid] = 0;
        base_addr[vid] = output_idx[vid] / c_wordSize;
        is_pending.range(vid, vid) = 1;
    }

    while (is_pending) {
        for (int i = 0; i < NUM_BLOCK; i++) {
            uint32_t readSizeBytes = 0;
            if (!inSizeStream[i].empty()) {
                readSizeBytes = inSizeStream[i].read();
                is_pending.range(i, i) = (readSizeBytes > 0) ? 1 : 0;
            }
            if (readSizeBytes > 0) {
                uint32_t readSize = (readSizeBytes - 1) / c_wordSize + 1;
                uint32_t base_idx = base_addr[i] + write_size[i];
            gmem_write:
                for (int j = 0; j < readSize; j++) {
#pragma HLS PIPELINE II = 1
                    out[base_idx + j] = inStream[i].read();
                }
                write_size[i] += readSize;
            }
        }
    }

    for (uint8_t pb = 0; pb < NUM_BLOCK; pb++) {
#pragma HLS PIPELINE II = 1
        outSize[pb] = totalOutSizeStream[pb].read();
    }
}

template <int IN_DATAWIDTH, int NUM_BLOCK = 8, int GMEM_DATAWIDTH = 512, int BURST_SIZE = 16>
void multStream2MM(hls::stream<ap_uint<IN_DATAWIDTH> > inStream[NUM_BLOCK],
                   hls::stream<bool> inStreamEos[NUM_BLOCK],
                   hls::stream<uint32_t> totalOutSizeStream[NUM_BLOCK],
                   const uint32_t output_idx[NUM_BLOCK],
                   ap_uint<GMEM_DATAWIDTH>* out,
                   uint32_t outSize[NUM_BLOCK]) {
    /**
     * @brief This module reads IN_DATAWIDTH data from stream based on the end
     * flag stream and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API.
     *
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam IN_DATAWIDTH width of input data bus
     * @tparam GMEM_DATAWIDTH width of output data bus
     * @tparam NUM_BLOCK number of blocks
     *
     * @param out output memory address
     * @param output_idx output index
     * @param inStream input stream
     * @param inSizeStream size flag for input stream
     * @param totalOutSizeStream size of output stream
     * @param output_size output size
     */

    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV[NUM_BLOCK];
    hls::stream<uint16_t> outStreamVSize[NUM_BLOCK];
#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 2
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

#pragma HLS DATAFLOW
parallel_upsizer:
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        xf::compression::details::stream2mmUpsizer<IN_DATAWIDTH, GMEM_DATAWIDTH, BURST_SIZE>(
            inStream[i], inStreamEos[i], outStreamV[i], outStreamVSize[i]);
    }
    xf::compression::details::multStream2mmSize<BURST_SIZE, GMEM_DATAWIDTH, NUM_BLOCK>(
        outStreamV, outStreamVSize, totalOutSizeStream, output_idx, out, outSize);
}

template <class STREAM_SIZE_DT, int IN_DATAWIDTH, int NUM_BLOCK = 8, int GMEM_DATAWIDTH = 512, int BURST_SIZE = 16>
void multStream2MMFreq(hls::stream<ap_uint<IN_DATAWIDTH> > inStream[NUM_BLOCK],
                       hls::stream<bool> inStreamEos[NUM_BLOCK],
                       hls::stream<uint32_t> totalOutSizeStream[NUM_BLOCK],
                       hls::stream<uint32_t> inStreamTree[NUM_BLOCK],
                       const uint32_t output_idx[NUM_BLOCK],
                       ap_uint<GMEM_DATAWIDTH>* out,
                       STREAM_SIZE_DT outSize[NUM_BLOCK],
                       STREAM_SIZE_DT* dyn_ltree_freq,
                       STREAM_SIZE_DT* dyn_dtree_freq) {
    /**
     * @brief This module reads IN_DATAWIDTH data from stream until end of
     * stream happens and writes the data to DDR. Reading data from multiple
     * data streams is non-blocking which is done using empty() API. It also
     * collects the huffman codes and bit length data and copies to DDR.
     *
     * @tparam STREAM_SIZE_DT Stream size class instance
     * @tparam BURST_SIZE burst size of the data transfers
     * @tparam IN_DATAWIDTH width of input data bus
     * @tparam GMEM_DATAWIDTH width of output data bus
     * @tparam NUM_BLOCK number of blocks
     *
     * @param out output global memory address
     * @param output_idx output index
     * @param inStream input stream contains final output data
     * @param inStreamEos input stream indicates end of the input stream
     * @param inStreamTree input stream contains huffman codes and bitlength data
     * @param totalOutSizeStream output stream contains compressed size of each block
     * @param output_size output global memory address that holds compress sizes
     * @param input_size input global memory address indicates block size
     * @param dyn_ltree_freq output literal frequency data
     * @param dyn_dtree_freq output distance frequency data
     *
     */
    const int c_byteSize = 8;
    const uint32_t c_depthOutStreamV = 2 * BURST_SIZE;
    hls::stream<ap_uint<GMEM_DATAWIDTH> > outStreamV[NUM_BLOCK];
    hls::stream<uint16_t> outStreamVSize[NUM_BLOCK];

    const int c_lTreeSize = 286;
    const int c_dTreeSize = 30;
    const int c_lTreeIdx = 1024;
    const int c_dTreeIdx = 64;

#pragma HLS STREAM variable = outStreamV depth = c_depthOutStreamV
#pragma HLS STREAM variable = outStreamVSize depth = 2
#pragma HLS BIND_STORAGE variable = outStreamV type = FIFO impl = SRL

#pragma HLS DATAFLOW
parallel_upsizer:
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        xf::compression::details::stream2mmUpsizer<IN_DATAWIDTH, GMEM_DATAWIDTH, BURST_SIZE>(
            inStream[i], inStreamEos[i], outStreamV[i], outStreamVSize[i]);
    }
    xf::compression::details::multStream2mmSize<BURST_SIZE, GMEM_DATAWIDTH, NUM_BLOCK>(
        outStreamV, outStreamVSize, totalOutSizeStream, output_idx, out, outSize);

    uint32_t val = 0;
    for (uint32_t bIdx = 0; bIdx < NUM_BLOCK; bIdx++) {
        val = inStreamTree[bIdx].read();
        if (val != 9999) {
            uint32_t lIndex = bIdx * c_lTreeIdx;
            uint32_t dIndex = bIdx * c_dTreeIdx;
        s2mm_ltree_freq:
            for (uint16_t i = 0; i < c_lTreeSize; i++) {
                dyn_ltree_freq[lIndex + i] = val;
                val = inStreamTree[bIdx].read();
            }

            uint8_t j = 0;
        s2mm_dtree_freq:
            for (; j < (c_dTreeSize - 1); j++) {
                dyn_dtree_freq[dIndex + j] = val;
                val = inStreamTree[bIdx].read();
            }
            dyn_dtree_freq[dIndex + j] = val;
        }
    }
}

#ifndef PARALLEL_BLOCK
#define PARALLEL_BLOCK 8
#endif

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
#pragma HLS BIND_STORAGE variable = local_buffer type = RAM_2P impl = LUTRAM

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
#pragma HLS BIND_STORAGE variable = local_buffer type = RAM_2P impl = LUTRAM

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
#pragma HLS BIND_STORAGE variable = local_buffer type = RAM_2P impl = LUTRAM

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
void s2mmEosStreamSimple(ap_uint<DATAWIDTH>* out,
                         hls::stream<ap_uint<DATAWIDTH> >& inStream,
                         hls::stream<bool>& endOfStream) {
    /**
     * @brief This module reads DATAWIDTH data from stream based on
     * size stream and writes the data to DDR.
     *
     * @tparam DATAWIDTH width of data bus
     * @tparam BURST_SIZE burst size of the data transfers
     * @param out output memory address
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
            ap_uint<DATAWIDTH> tmp = (eos == true) ? dummy : inStream.read();
            bool eos_tmp = (eos == true) ? true : endOfStream.read();
            out[j + i] = tmp;
            eos = eos_tmp;
        }
    }
}

template <int DATAWIDTH, int BURST_SIZE, class OUTSIZE_DT = uint32_t>
void s2mmEosSimple(ap_uint<DATAWIDTH>* out,
                   hls::stream<ap_uint<DATAWIDTH> >& inStream,
                   hls::stream<bool>& endOfStream,
                   hls::stream<OUTSIZE_DT>& outSize,
                   OUTSIZE_DT* output_size) {
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
            ap_uint<DATAWIDTH> tmp = (eos == true) ? dummy : inStream.read();
            bool eos_tmp = (eos == true) ? true : endOfStream.read();
            out[j + i] = tmp;
            eos = eos_tmp;
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
void s2mmStreamSimple(ap_uint<DATAWIDTH>* out,
                      hls::stream<ap_uint<DATAWIDTH> >& inStream,
                      hls::stream<bool>& inStreamEoS) {
    /**
     * @brief This module reads N-bit data from stream based on
     * end of stream and writes the data to DDR. N is template parameter DATAWIDTH.
     *
     * @tparam DATAWIDTH Width of the input data stream
     *
     * @param out output memory address
     * @param inStream input hls stream
     * @param inStreamEoS input end of stream
     */

    uint32_t i = 0;
    bool eosFlag = inStreamEoS.read();
s2mm_simple:
    for (; eosFlag == false; eosFlag = inStreamEoS.read(), i++) {
#pragma HLS PIPELINE II = 1
        out[i] = inStream.read();
    }

    ap_uint<DATAWIDTH> dummy = inStream.read();
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
