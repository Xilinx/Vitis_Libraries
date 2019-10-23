#ifndef _XFCOMPRESSION_LZ4_P2P_DECOMPRESS_KERNEL_HPP_
#define _XFCOMPRESSION_LZ4_P2P_DECOMPRESS_KERNEL_HPP_
/**********
 * Copyright (c) 2017, Xilinx, Inc.
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * **********/

/**
 * @file lz4_p2p_decompress_kernel.hpp
 * @brief Header for LZ4 P2P decompression kernel.
 *
 * This file is part of XF Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "lz_decompress.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_downsizer.hpp"
#include "stream_upsizer.hpp"
#include "lz4_decompress.hpp"
#include "lz4_p2p.hpp"
#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16

// Kernel top functions
extern "C" {

/**
 * @brief LZ4 P2P decompression kernel.
 *
 * @param in input stream width
 * @param out output stream width
 * @param in_block_size intput size
 * @param in_compress_size output size
 * @param block_start_idx start index of block
 * @param no_blocks number of blocks for each compute unit
 * @param block_size_in_kb block input size
 * @param compute_unit particular compute unit
 * @param total_no_cu number of compute units
 * @param num_blocks number of blocks base don host buffersize
 */
void xilLz4P2PDecompress(const xf::compression::uintMemWidth_t* in,
                         xf::compression::uintMemWidth_t* out,
                         dt_blockInfo* bObj,
                         dt_chunkInfo* cObj,
                         uint32_t block_size_in_kb,
                         uint32_t compute_unit,
                         uint8_t total_no_cu,
                         uint32_t num_blocks);
}

#endif
