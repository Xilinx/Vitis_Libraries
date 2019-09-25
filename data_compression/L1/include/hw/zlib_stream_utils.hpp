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
#ifndef _XFCOMPRESSION_ZLIB_STREAM_UTILS_HPP_
#define _XFCOMPRESSION_ZLIB_STREAM_UTILS_HPP_

#include "common.h"
#include <string.h>

#define GET_DIFF_IF_BIG(x, y) (x > y) ? (x - y) : 0

#define STREAM_UTILS_MM2S_IF_NOT_FULL(bIdx, outStream, is_full, read_idx, write_idx, local_buffer) \
    is_full.range(bIdx, bIdx) = outStream.full();                                                  \
    if (!is_full.range(bIdx, bIdx) && (read_idx[bIdx] != write_idx[bIdx])) {                       \
        outStream << local_buffer[bIdx][read_idx[bIdx]];                                           \
        read_idx[bIdx] += 1;                                                                       \
    }

#define STREAM_UTILS_S2MM_READ_SIZE(i, instream, end_of_stream) \
    if (!end_of_stream.range(i, i) && !instream.empty()) {      \
        uint16_t tmpValue = instream.read();                    \
        input_size[i] += tmpValue;                              \
        if (tmpValue == 0) end_of_stream.range(i, i) = 1;       \
    }

#define STREAM_UTILS_S2MM_IF_NOT_EMPTY(i, instream, burst_size, input_size, read_size, write_size, write_idx) \
    burst_size[i] = c_maxBurstSize;                                                                           \
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

#define STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(i, instream, burst_size, input_size, read_size, write_size, write_idx) \
    burst_size[i] = c_maxBurstSize;                                                                               \
    if (((input_size[i] - write_size[i]) < burst_size[i])) {                                                      \
        burst_size[i] = GET_DIFF_IF_BIG(input_size[i], write_size[i]);                                            \
    }                                                                                                             \
    if (((read_size[i] - write_size[i]) < burst_size[i]) && (input_size[i] > read_size[i])) {                     \
        bool is_empty = instream.empty();                                                                         \
        if (!is_empty) {                                                                                          \
            local_buffer[i][write_idx[i]] = instream.read();                                                      \
            write_idx[i] += 1;                                                                                    \
            read_size[i] += 64;                                                                                   \
            is_pending.range(i, i) = true;                                                                        \
        } else {                                                                                                  \
            is_pending.range(i, i) = false;                                                                       \
        }                                                                                                         \
    } else {                                                                                                      \
        if (burst_size[i]) done = true;                                                                           \
    }

namespace xf {
namespace compression {

const int c_lTreeSize = 1024;
const int c_dTreeSize = 64;
const int c_bLTreeSize = 64;
const int c_maxCodeSize = 16;

template <int DATAWIDTH, int BURST_SIZE>
void gzMm2s(const ap_uint<DATAWIDTH>* in,
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
        for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
            uint32_t pending_bytes = GET_DIFF_IF_BIG(input_size[bIdx], read_size[bIdx]);
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

template <int DATAWIDTH, int BURST_SIZE>
void mm2sFreq(const ap_uint<DATAWIDTH>* in,
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

    // printf("In mm2s start\n");

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

    // printf("In mm2s end\n");

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
        for (uint32_t bIdx = 0; bIdx < PARALLEL_BLOCK; bIdx++) {
            uint32_t pending_bytes = GET_DIFF_IF_BIG(input_size[bIdx], read_size[bIdx]);
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

template <class SIZE_DT, int IN_WIDTH, int OUT_WIDTH>
void upsizerSizeStream(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                       hls::stream<SIZE_DT>& inStreamSize,
                       hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                       hls::stream<SIZE_DT>& outStreamSize) {
    // Constants
    const int c_byteWidth = 8; // 8bit is each BYTE
    const int c_upsizeFactor = OUT_WIDTH / c_byteWidth;
    const int c_inSize = IN_WIDTH / c_byteWidth;

    ap_uint<2 * OUT_WIDTH> outBuffer = 0; // Declaring double buffers
    uint32_t byteIdx = 0;
    // printme("%s: factor=%d\n",__FUNCTION__,c_upsizeFactor);
    for (SIZE_DT size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // rounding off the output size
        uint16_t outSize = ((size + byteIdx) / c_upsizeFactor) * c_upsizeFactor;
        if (outSize) {
            outStreamSize << outSize;
        }
    ////printme("%s: reading next data=%d outSize=%d c_inSize=%d\n ",__FUNCTION__, size,outSize,c_inSize);
    stream_upsizer:
        for (int i = 0; i < size; i += c_inSize) {
#pragma HLS PIPELINE II = 1
            int chunk_size = c_inSize;
            if (chunk_size + i > size) chunk_size = size - i;
            ap_uint<IN_WIDTH> tmpValue = inStream.read();
            outBuffer.range((byteIdx + c_inSize) * c_byteWidth - 1, byteIdx * c_byteWidth) = tmpValue;
            byteIdx += chunk_size;
            ////printme("%s: value=%c, chunk_size = %d and byteIdx=%d\n",__FUNCTION__,(char)tmpValue,
            /// chunk_size,byteIdx);
            if (byteIdx >= c_upsizeFactor) {
                outStream << outBuffer.range(OUT_WIDTH - 1, 0);
                outBuffer >>= OUT_WIDTH;
                byteIdx -= c_upsizeFactor;
            }
        }
    }
    if (byteIdx) {
        outStreamSize << byteIdx;
        ////printme("sent outSize %d \n", byteIdx);
        outStream << outBuffer.range(OUT_WIDTH - 1, 0);
    }
    // end of block
    outStreamSize << 0;
    // printme("%s:Ended \n",__FUNCTION__);
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH>
void s2mmCompressFreq(ap_uint<DATAWIDTH>* out,
                      uint32_t* dyn_ltree_freq,
                      uint32_t* dyn_dtree_freq,
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
                      hls::stream<uint32_t>& inStreamTree_0,
#if PARALLEL_BLOCK > 1
                      hls::stream<uint32_t>& inStreamTree_1,
#endif
#if PARALLEL_BLOCK > 2
                      hls::stream<uint32_t>& inStreamTree_2,
                      hls::stream<uint32_t>& inStreamTree_3,
#endif
#if PARALLEL_BLOCK > 4
                      hls::stream<uint32_t>& inStreamTree_4,
                      hls::stream<uint32_t>& inStreamTree_5,
                      hls::stream<uint32_t>& inStreamTree_6,
                      hls::stream<uint32_t>& inStreamTree_7,
#endif
                      STREAM_SIZE_DT output_size[PARALLEL_BLOCK]) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = c_wordSize * BURST_SIZE;
    uint32_t input_size[PARALLEL_BLOCK];
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

    uint32_t val = 0;

    for (uint32_t blk = 0; blk < PARALLEL_BLOCK; blk++) {
        bool dist_read = true;
    s2mm_ltree_freq:
        for (uint32_t i = 0; i < c_lTreeSize; i++) {
            if (blk == 0) val = inStreamTree_0.read();
#if PARALLEL_BLOCK > 1
            if (blk == 1) val = inStreamTree_1.read();
#endif
#if PARALLEL_BLOCK > 2
            if (blk == 2) val = inStreamTree_2.read();
            if (blk == 3) val = inStreamTree_3.read();
#endif
#if PARALLEL_BLOCK > 4
            if (blk == 4) val = inStreamTree_4.read();
            if (blk == 5) val = inStreamTree_5.read();
            if (blk == 6) val = inStreamTree_6.read();
            if (blk == 7) val = inStreamTree_7.read();
#endif
            if (val == 9999) {
                i = c_lTreeSize;
                dist_read = false;
            } else {
                dyn_ltree_freq[blk * c_lTreeSize + i] = val;
            }
        }

    s2mm_dtree_freq:
        for (uint32_t j = 0; ((j < c_dTreeSize) && dist_read); j++) {
            if (blk == 0) val = inStreamTree_0.read();
#if PARALLEL_BLOCK > 1
            if (blk == 1) val = inStreamTree_1.read();
#endif
#if PARALLEL_BLOCK > 2
            if (blk == 2) val = inStreamTree_2.read();
            if (blk == 3) val = inStreamTree_3.read();
#endif
#if PARALLEL_BLOCK > 4
            if (blk == 4) val = inStreamTree_4.read();
            if (blk == 5) val = inStreamTree_5.read();
            if (blk == 6) val = inStreamTree_6.read();
            if (blk == 7) val = inStreamTree_7.read();
#endif
            dyn_dtree_freq[blk * c_dTreeSize + j] = val;
        }
    }
}

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH>
void s2mmCompress(ap_uint<DATAWIDTH>* out,
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
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int c_maxBurstSize = c_wordSize * BURST_SIZE;
    uint32_t input_size[PARALLEL_BLOCK];
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

template <class STREAM_SIZE_DT, int BURST_SIZE, int DATAWIDTH>
void s2mmDecompress(ap_uint<DATAWIDTH>* out,
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
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(0, inStream_0, burst_size, input_size, read_size, write_size, write_idx);
#if PARALLEL_BLOCK > 1
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(1, inStream_1, burst_size, input_size, read_size, write_size, write_idx);
#endif
#if PARALLEL_BLOCK > 2
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(2, inStream_2, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(3, inStream_3, burst_size, input_size, read_size, write_size, write_idx);
#endif
#if PARALLEL_BLOCK > 4
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(4, inStream_4, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(5, inStream_5, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(6, inStream_6, burst_size, input_size, read_size, write_size, write_idx);
            STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY(7, inStream_7, burst_size, input_size, read_size, write_size, write_idx);
#endif
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

} // namespace compression
} // namespace xf

#undef GET_DIFF_IF_BIG
#undef STREAM_UTILS_MM2S_IF_NOT_FULL
#undef STREAM_UTILS_S2MM_READ_SIZE
#undef STREAM_UTILS_S2MM_IF_NOT_EMPTY
#undef STREAM_UTILS_S2MM_DEC_IF_NOT_EMPTY

#endif // _XFCOMPRESSION_ZLIB_STREAM_UTILS_HPP_
