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
#ifndef _XFCOMPRESSION_LZ4_PACKER_KERNEL_HPP_
#define _XFCOMPRESSION_LZ4_PACKER_KERNEL_HPP_

/**
 * @file lz4_packer_mm.hpp
 * @brief Header for LZ4 packer kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"
#include <ap_int.h>

#include "lz4_packer.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_downsizer.hpp"
#include "stream_upsizer.hpp"

#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16
#define BLOCK_PARITION 1024
#define MARKER 255
#define MAX_LIT_COUNT 4096
#define PACK_WIDTH 64  // packer width
#define PARLLEL_BYTE 8 // byte length

#define MAGIC_BYTE_1 4
#define MAGIC_BYTE_2 34
#define MAGIC_BYTE_3 77
#define MAGIC_BYTE_4 24
#define FLG_BYTE 104

// Below are the codes as per LZ4 standard for
// various maximum block sizes supported.
#define BSIZE_STD_64KB 64
#define BSIZE_STD_256KB 80
#define BSIZE_STD_1024KB 96
#define BSIZE_STD_4096KB 112

const int c_gmem_burst_size = (2 * GMEM_BURST_SIZE);
const int c_size_stream_depth = 8;
const int max_literal_count = MAX_LIT_COUNT;

typedef ap_uint<GMEM_DWIDTH> uint512_t;
typedef ap_uint<PACK_WIDTH> uintV_t; // 64bit input stream

// Kernel top functions
extern "C" {
/**
 * @brief LZ4 packer kernel takes the raw data as input and compresses the data
 * in block based fashion and writes the output to global memory.
 *
 * @param in input raw data
 * @param out output compressed data
 * @param compressd_size compressed output size of each block
 * @param in_block_size input block size of each block
 * @param encode_size encoded size of each block
 * @param orig_input_data raw input data
 * @param head_res_size size of the header
 * @param offset offset
 * @param block_size_in_kb input block size in bytes
 * @param no_blocks number of input blocks
 * @param tail_bytes remaining bytes for the last block
 */
void xilLz4Packer(const uint512_t* in,
                  uint512_t* out,
                  uint512_t* head_prev_blk,
                  uint32_t* compressd_size,
                  uint32_t* in_block_size,
                  uint32_t* encoded_size,
                  uint512_t* orig_input_data,
                  uint32_t head_res_size,
                  uint32_t offset,
                  uint32_t block_size_in_kb,
                  uint32_t no_blocks,
                  uint32_t tail_bytes);
}
#endif
