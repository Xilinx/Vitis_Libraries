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

namespace xf {
namespace compression {

const int kGMemDWidth = 512;
typedef ap_uint<kGMemDWidth> uintMemWidth_t;

const int c_lTreeSize = 1024;
const int c_dTreeSize = 64;
const int c_bLTreeSize = 64;
const int c_maxCodeSize = 16;

template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNb(const ap_uint<DATAWIDTH>* in,
            const uint32_t _input_idx[PARALLEL_BLOCK],
            hls::stream<ap_uint<DATAWIDTH> > outStream[PARALLEL_BLOCK],
            const uint32_t _input_size[PARALLEL_BLOCK]) {
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

template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNbFreq(const ap_uint<DATAWIDTH>* in,
                uint32_t* dyn_ltree_codes,
                uint32_t* dyn_ltree_blen,
                uint32_t* dyn_dtree_codes,
                uint32_t* dyn_dtree_blen,
                uint32_t* dyn_bltree_codes,
                uint32_t* dyn_bltree_blen,
                uint32_t* dyn_maxcodes,
                const uint32_t _input_idx[PARALLEL_BLOCK],
                hls::stream<ap_uint<DATAWIDTH> > outStream[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_ltree_codes[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_ltree_blen[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_dtree_codes[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_dtree_blen[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_bltree_codes[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_bltree_blen[PARALLEL_BLOCK],
                hls::stream<uint32_t> stream_maxcode[PARALLEL_BLOCK],
                uint32_t n_blocks,
                const uint32_t _input_size[PARALLEL_BLOCK]) {
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

    uint32_t lcl_ltree_codes[PARALLEL_BLOCK][c_lTreeSize];
    uint32_t lcl_ltree_blen[PARALLEL_BLOCK][c_lTreeSize];

    uint32_t lcl_dtree_codes[PARALLEL_BLOCK][c_dTreeSize];
    uint32_t lcl_dtree_blen[PARALLEL_BLOCK][c_dTreeSize];

    uint32_t lcl_bltree_codes[PARALLEL_BLOCK][c_bLTreeSize];
    uint32_t lcl_bltree_blen[PARALLEL_BLOCK][c_bLTreeSize];

    uint32_t lcl_maxcode[PARALLEL_BLOCK * c_maxCodeSize];

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
    for (uint32_t ci = 0; ci < PARALLEL_BLOCK * c_maxCodeSize; ++ci) {
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

template <int DATAWIDTH, int BURST_SIZE, int NUM_BLOCKS>
void mm2sNbRoundOff(const ap_uint<DATAWIDTH>* in,
                    const uint32_t _input_idx[PARALLEL_BLOCK],
                    hls::stream<ap_uint<DATAWIDTH> > outStream[PARALLEL_BLOCK],
                    const uint32_t _input_size[PARALLEL_BLOCK]) {
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

template <int DATAWIDTH, int BURST_SIZE>
void mm2sSimple(const uintMemWidth_t* in, hls::stream<uintMemWidth_t>& outstream, uint32_t inputSize) {
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

        memrd1:
            for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                if (bIdx == 0)
                    buffer[j] = head_prev_blk[(offset_gmem + i) + j];
                else if (blkCompSize == origSize)
                    buffer[j] = orig_input_data[(block_stride * (bIdx - 1) + i) + j];
                else
                    buffer[j] = in[(block_stride * (bIdx - 1) + i) + j];
            }

        memrd2:
            for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                outStream << buffer[j];
            }
        }
    }
    // printf("%s Done \n", __FUNCTION__);
    outStreamSize << 0;
}

} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_MM2S_HPP_
