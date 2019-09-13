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
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "lz4_unpacker_kernel.hpp"

#define GMEM_DWIDTH 512

#define NO_COMPRESS_BIT 128

#define BSIZE_NCOMP_64 1
#define BSIZE_NCOMP_256 4
#define BSIZE_NCOMP_1024 16
#define BSIZE_NCOMP_4096 64

// typedef ap_uint<GMEM_DWIDTH> uintMemWidth_t;

// Stream in_block_size, in_compress_size, block_start_idx to decompress kernel. And need to put Macro or use array
// based on number of compute units

extern "C" {
void xilLz4Unpacker(const xf::compression::uintMemWidth_t* in,
                    uint32_t* in_block_size,
                    uint32_t* in_compress_size,
                    uint32_t* block_start_idx,
                    uint32_t* no_blocks_per_cu,
                    uint64_t* original_size,
                    uint32_t* in_start_index,
                    uint32_t* no_blocks,
                    uint32_t block_size_in_kb,
                    uint8_t first_chunk,
                    uint8_t total_no_cu,
                    uint32_t num_blocks) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_compress_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = block_start_idx offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = no_blocks_per_cu offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = original_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_start_index offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = no_blocks offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_compress_size bundle = control
#pragma HLS INTERFACE s_axilite port = block_start_idx bundle = control
#pragma HLS INTERFACE s_axilite port = no_blocks_per_cu bundle = control
#pragma HLS INTERFACE s_axilite port = original_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_start_index bundle = control
#pragma HLS INTERFACE s_axilite port = no_blocks bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = first_chunk bundle = control
#pragma HLS INTERFACE s_axilite port = total_no_cu bundle = control
#pragma HLS INTERFACE s_axilite port = num_blocks bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS data_pack variable = in

    uint32_t block_size_in_bytes = block_size_in_kb * 1024;
    uint32_t max_no_blocks = num_blocks * total_no_cu;

    if (first_chunk) {
        xf::compression::uintMemWidth_t inTemp;
        /*Magic headers*/
        inTemp = in[0];
        uint8_t m1 = inTemp.range(7, 0);
        uint8_t m2 = inTemp.range(15, 8);
        uint8_t m3 = inTemp.range(23, 16);
        uint8_t m4 = inTemp.range(31, 24);

        /*Header checksum*/
        uint8_t hc = inTemp.range(39, 32);

        /*Block size*/
        block_size_in_kb = inTemp.range(47, 40);
        block_size_in_bytes = block_size_in_kb * 1024;

        /*Original file size*/
        original_size[0] = inTemp.range(111, 48);

        /*Calculate no of blocks based on original size of file*/
        no_blocks[0] = (original_size[0] - 1) / block_size_in_bytes + 1;

        /*Initialize start index for first chunk*/
        in_start_index[0] = 15;
    }

    uint32_t curr_no_blocks = (no_blocks[0] >= max_no_blocks) ? max_no_blocks : no_blocks[0];
    no_blocks[0] = no_blocks[0] - curr_no_blocks;

    const int c_byte_size = 8;
    uint64_t inIdx = in_start_index[0];
    uint32_t Idx1 = (inIdx * c_byte_size) / GMEM_DWIDTH;
    uint32_t Idx2 = (inIdx * c_byte_size) % GMEM_DWIDTH;
    uint32_t compressed_size = 0;
    uint32_t compressed_size1 = 0;

    for (uint32_t blkIdx = 0; blkIdx < curr_no_blocks; blkIdx++) {
        if (Idx2 + 32 <= GMEM_DWIDTH) {
            xf::compression::uintMemWidth_t inTemp;
            inTemp = in[Idx1];
            compressed_size = inTemp.range(Idx2 + 32 - 1, Idx2);
        } else {
            xf::compression::uintMemWidth_t inTemp;
            xf::compression::uintMemWidth_t inTemp1;
            ap_uint<32> ctemp;
            inTemp = in[Idx1];
            int c_word_size = GMEM_DWIDTH / c_byte_size;
            inTemp1 = in[Idx1 + 1];
            ctemp = (inTemp1.range(Idx2 + 32 - GMEM_DWIDTH - 1, 0), inTemp.range(GMEM_DWIDTH - 1, Idx2));
            compressed_size = ctemp;
        }
        inIdx = inIdx + 4;
        uint32_t tmp;
        tmp = compressed_size;
        tmp >>= 24;
        if (tmp == NO_COMPRESS_BIT) {
            uint8_t b1 = compressed_size;
            uint8_t b2 = compressed_size >> 8;
            uint8_t b3 = compressed_size >> 16;
            if (b3 == BSIZE_NCOMP_64 || b3 == BSIZE_NCOMP_4096 || b3 == BSIZE_NCOMP_256 || b3 == BSIZE_NCOMP_1024) {
                compressed_size = block_size_in_bytes;
            } else {
                uint32_t size = 0;
                size = b3;
                size <<= 16;
                uint32_t temp = b2;
                temp <<= 8;
                size |= temp;
                temp = b1;
                size |= temp;
                compressed_size = size;
            }
        }
        block_start_idx[blkIdx] = inIdx;
        in_compress_size[blkIdx] = compressed_size;
        in_block_size[blkIdx] = block_size_in_bytes;
        inIdx = inIdx + compressed_size;
        Idx1 = (inIdx * c_byte_size) / GMEM_DWIDTH;
        Idx2 = (inIdx * c_byte_size) % GMEM_DWIDTH;
    }
    in_start_index[0] = inIdx;

    if (no_blocks[0] == 0) {
        in_block_size[curr_no_blocks - 1] = original_size[0] % (block_size_in_bytes);
        // If original size is multiple of block size
        if (in_block_size[curr_no_blocks - 1] == 0) // If original size is multiple of block size
            in_block_size[curr_no_blocks - 1] = block_size_in_bytes;
    }
    for (int i = 0; i < total_no_cu; i++) {
        if (curr_no_blocks > num_blocks)
            no_blocks_per_cu[i] = num_blocks;
        else
            no_blocks_per_cu[i] = curr_no_blocks;
        curr_no_blocks = curr_no_blocks - num_blocks;
    }
}
}
