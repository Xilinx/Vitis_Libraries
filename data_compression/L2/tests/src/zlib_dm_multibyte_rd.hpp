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

#ifndef _XFCOMPRESSION_ZLIB_DM_RD_HPP_
#define _XFCOMPRESSION_ZLIB_DM_RD_HPP_

/**
 * @file zlib_dm_rd.hpp
 * @brief Header for zlib data reader kernel which reads data from zlib decompression streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "ap_axi_sdata.h"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "s2mm.hpp"
#include "zlib_specs.hpp"

typedef ap_uint<MULTIPLE_BYTES * 8> uintMemWidth_t;

extern "C" {
/**
 * @brief Zlib data mover:reader kernel top function. It reads data from zlib decompression kernel stream interface
 *  and writes it to memory.
 *
 * @param out output memory pointer
 * @param encoded_size decompressed size output
 * @param read_block_size Block size to be read
 * @param outstreamk output axi kernel stream (512-bit wide data stream read by this kernel)
 * @param sizestreamk output data size axi kernel stream (size valid in 512-bits read by this kernel)
 *
 */

void xilZlibDmReader(uintMemWidth_t* out,
                     uint32_t* encoded_size,
                     uint32_t* status_flag,
                     uint32_t read_block_size,
                     hls::stream<ap_axiu<MULTIPLE_BYTES * 8, 0, 0, 0> >& outstreamk,
                     hls::stream<ap_axiu<64, 0, 0, 0> >& sizestreamk);
}
#endif // _XFCOMPRESSION_ZLIB_DM_RD_HPP_
