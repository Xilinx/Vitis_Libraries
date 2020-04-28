/**********
 * Copyright (c) 2019, Xilinx, Inc.
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
#ifndef _XFCOMPRESSION_LZ4_P2P_HPP_
#define _XFCOMPRESSION_LZ4_P2P_HPP_

/**
 * @file lz4_p2p.hpp
 * @brief Block and Chunk data required for LZ4 P2P.
 *
 * This file is part of XF Compression Library.
 */

#include <stdint.h>
#define GMEM_DATAWIDTH 512

// structure size explicitly made equal to 64Bytes so that it will match
// to Kernel Global Memory datawidth (512bit).
typedef struct unpackerBlockInfo {
    uint32_t compressedSize;
    uint32_t blockSize;
    uint32_t blockStartIdx;
    uint32_t padding[(GMEM_DATAWIDTH / 32) - 3];
} dt_blockInfo;

// structure size explicitly made equal to 64Bytes so that it will match
// to Kernel Global Memory datawidth (512bit).
typedef struct unpackerChunkInfo {
    uint32_t inStartIdx;
    uint32_t originalSize;
    uint32_t numBlocks;
    uint32_t numBlocksPerCU[2];
    uint32_t padding[(GMEM_DATAWIDTH / 32) - 5];
} dt_chunkInfo;

#endif // _XFCOMPRESSION_LZ4_P2P_HPP_
