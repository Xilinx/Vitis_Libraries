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
#ifndef _XFCOMPRESSION_LZ4_PACKER_HPP_
#define _XFCOMPRESSION_LZ4_PACKER_HPP_

/**
 * @file lz4_packer.hpp
 * @brief Header for module used in LZ4 packer kernel.
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

/**
 * @brief Lz4 packer modules that packs the compressed data.
 *
 * @param inStream input data
 * @param outStream output data
 * @param inStreamSize size of the data in input stream
 * @param outStreamSize ouput data size
 * @param block_size_in_kb block size
 * @param no_blocks number of input blocks
 * @param head_res_size header size
 * @param tail_bytes enabled for last block to handle final data
 */
template <int PACK_WIDTH, int PARLLEL_BYTE>
uint32_t lz4Packer(hls::stream<ap_uint<PACK_WIDTH> >& inStream,
                   hls::stream<ap_uint<PACK_WIDTH> >& outStream,
                   hls::stream<uint32_t>& inStreamSize,
                   hls::stream<uint32_t>& outStreamSize,
                   uint32_t block_size_in_kb,
                   uint32_t no_blocks,
                   uint32_t head_res_size,
                   uint32_t tail_bytes) {
    // 16 bytes can be held in on shot
    ap_uint<2 * PACK_WIDTH> lcl_buffer;
    uint32_t lbuf_idx = 0;

    uint32_t cBlen = 4;

    uint32_t endSizeCnt = 0;
    uint32_t sizeOutput = 0;
    uint32_t word_head = 0;
    uint32_t over_size = 0;
    uint32_t flag = 0;
    uint32_t size_cntr = 0;
// Core packer logic
packer:
    for (int blkIdx = 0; blkIdx < no_blocks + 1; blkIdx++) {
        uint32_t size = inStreamSize.read();
        // printf("lbuf %d \n", lbuf_idx);
        // Find out the compressed size header
        // This value is sent by mm2s module
        // by using compressed_size[] buffer picked from
        // LZ4 compression kernel
        if (blkIdx == 0) {
            uint32_t word_head;
            if (head_res_size < 8)
                word_head = 1;
            else
                word_head = head_res_size / 8;
            sizeOutput = word_head;
            endSizeCnt += head_res_size;
        } else {
            // Size is nothing but 8bytes * 8 gives original input
            over_size = lbuf_idx + cBlen + size;
            endSizeCnt += cBlen + size;
            // 64bit size value including headers etc
            sizeOutput = over_size / 8;
        }
        // printf("%s - trailSize %d size %d lbuf %d word_64size %d over_size_pblick %d \n", __FUNCTION__, trailSize,
        // size, lbuf_idx, sizeOutput, over_size);
        // Send the size of output to next block
        outStreamSize << sizeOutput;

        if (blkIdx != 0) {
            // Update local buffer with compress size of current block - 4Bytes
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size;
            lbuf_idx++;
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size >> 8;
            lbuf_idx++;
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size >> 16;
            lbuf_idx++;
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size >> 24;
            lbuf_idx++;
        }

        if (lbuf_idx >= 8) {
            outStream << lcl_buffer.range(63, 0);
            lcl_buffer >>= PACK_WIDTH;
            lbuf_idx -= 8;
        }
        // printf("%s size %d sizeOutput %d \n", __FUNCTION__, size, sizeOutput);

        uint32_t chunk_size = 0;
    // Packer logic
    // Read compressed data of a block
    // Stream it to upsizer
    pack_post:
        for (int i = 0; i < size; i += PARLLEL_BYTE) {
#pragma HLS PIPELINE II = 1

            if (i + chunk_size > size)
                chunk_size = size - i;
            else
                chunk_size = PARLLEL_BYTE;

            // Update local buffer with new set of data
            // Update local buffer index
            lcl_buffer.range((lbuf_idx * 8) + PACK_WIDTH - 1, lbuf_idx * 8) = inStream.read();
            lbuf_idx += chunk_size;

            if (lbuf_idx >= 8) {
                outStream << lcl_buffer.range(63, 0);
                lcl_buffer >>= PACK_WIDTH;
                lbuf_idx -= 8;
            }
        } // End of main packer loop
    }

    // printf("End of packer \n");
    if (tail_bytes) {
        // Trailing bytes based on LZ4 standard
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
    }
    // printf("flag %d lbuf_idx %d\n", flag, lbuf_idx);

    uint32_t extra_size = (lbuf_idx - 1) / 8 + 1;
    if (lbuf_idx) outStreamSize << extra_size;

    while (lbuf_idx) {
        // printf("Ssent the data \n");
        outStream << lcl_buffer.range(63, 0);

        if (lbuf_idx >= 8) {
            lcl_buffer >>= PACK_WIDTH;
            lbuf_idx -= 8;
        } else {
            lbuf_idx = 0;
        }
    }

    // printf("%s Done \n", __FUNCTION__);
    // Termination condition of
    // next block
    outStreamSize << 0;
    // printf("endsizeCnt %d \n", endSizeCnt);
    return endSizeCnt;
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ4_COMPRESS_HPP_
