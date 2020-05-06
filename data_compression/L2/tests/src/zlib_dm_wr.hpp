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

#ifndef _XFCOMPRESSION_ZLIB_DM_WR_HPP_
#define _XFCOMPRESSION_ZLIB_DM_WR_HPP_

/**
 * @file zlib_dm_wr.hpp
 * @brief Header for zlib data writer kernel which streams data to zlib decompression streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "ap_axi_sdata.h"

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ap_int.h>
#include "hls_stream.h"
#include "mm2s.hpp"
#include "zlib_specs.hpp"

const int kGMemDWidth = 32;
typedef ap_uint<kGMemDWidth> uintMemWidth_t;

extern "C" {
/**
 * @brief Zlib data mover:Writer kernel top function. It reads data from memory and streams it
 *  to zlib decompression kernel.
 *
 * @param in input memory pointer
 * @param inputSize input size
 * @param instreamk input axi kernel stream (written by this kernel)
 *
 */

void xilZlibDmWriter(uintMemWidth_t* in, uint32_t inputSize, hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& instreamk);
}
#endif // _XFCOMPRESSION_ZLIB_DM_WR_HPP_
