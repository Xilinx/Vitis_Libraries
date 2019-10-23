#ifndef _XFCOMPRESSION_UNPACKER_KERNEL_HPP_
#define _XFCOMPRESSION_UNPACKER_KERNEL_HPP_
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
 * @file unpacker_kernel.hpp
 * @brief Header for Unapcker kernel.
 *
 * This file is part of XF Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_downsizer.hpp"
#include "stream_upsizer.hpp"
#include "lz4_p2p.hpp"
#define GMEM_DWIDTH 512

// Kernel top functions
extern "C" {

/**
 * @brief Unapcker kernel.
 *
 * @param in input stream width
 * @param in_block_size input block size
 * @param in_compress_size input compress size
 * @param block_start_idx start index of each input block
 * @param no_blocks_per_cu number of blocks for each compute unit
 * @param original_size original file size
 * @param in_start_index input start index
 * @param no_blocks number of blocks
 * @param block_size_in_kb size of each block
 * @param first_chunk first chunk to determine header
 * @param total_no_cu number of decompress compute units
 * @param num_blocks number of blocks based on host buffersize
 */
void xilLz4Unpacker(const xf::compression::uintMemWidth_t* in,
                    dt_blockInfo* bObj,
                    dt_chunkInfo* cObj,
                    uint32_t block_size_in_kb,
                    uint8_t first_chunk,
                    uint8_t total_no_cu,
                    uint32_t num_blocks);
}

#endif
