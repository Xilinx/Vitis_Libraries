/*
 * Copyright 2019 Xilinx, Inc.
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
 */
#include "hls_stream.h"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "snappy_decompress.hpp"
#include "lz_decompress.hpp"
#include "lz_optional.hpp"

#define BIT 8

typedef ap_uint<32> compressd_dt;
typedef ap_uint<BIT> uintV_t;

/**
 * @brief Snappy decompress engine that run the snappy core modules
 *
 *
 * @param inStream input stream
 * @param snappyOut output stream
 * @param _input_size input data size
 * @param _output_size output data size
 */

template <int READ_STATE, int MATCH_STATE, int LOW_OFFSET_STATE, int LOW_OFFSET, int HISTORY_SIZE>
void snappy_decompress_engine(hls::stream<uintV_t>& inStream,
                              hls::stream<uintV_t>& snappyOut,
                              const uint32_t _input_size,
                              const uint32_t _output_size) {
    uint32_t input_size = _input_size;
    uint32_t output_size = _output_size;
    hls::stream<compressd_dt> decompressd_stream("decompressd_stream");
#pragma HLS STREAM variable = decompressd_stream depth = 8
#pragma HLS RESOURCE variable = decompressd_stream core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::snappyDecompress(inStream, decompressd_stream, input_size);
    xf::compression::lzDecompress<HISTORY_SIZE, READ_STATE, MATCH_STATE, LOW_OFFSET_STATE, LOW_OFFSET>(
        decompressd_stream, snappyOut, output_size);
}
