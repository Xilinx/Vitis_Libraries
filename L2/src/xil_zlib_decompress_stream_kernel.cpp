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
/**
 * @file xil_zlib_decompress_stream_kernel.cpp
 * @brief Source for zlib decompression stream kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "xil_zlib_decompress_stream_kernel.hpp"

// LZ specific Defines
#define MIN_OFFSET 1
#define MIN_MATCH 4
#define LZ_MAX_OFFSET_LIMIT 32768
#define HISTORY_SIZE LZ_MAX_OFFSET_LIMIT
#define LOW_OFFSET 10

void xil_inflate(hls::stream<xf::compression::hStream16b_t>& inaxistream,
                 hls::stream<xf::compression::hStream8b_t>& outaxistream,
                 hls::stream<xf::compression::hStream32b_t>& encodedsize,
                 uint32_t input_size) {
    hls::stream<ap_uint<16> > outDownStream("outDownStream");
    hls::stream<xf::compression::streamDt> uncompOutStream("unCompOutStream");
    hls::stream<bool> byte_eos("byteEndOfStream");
    hls::stream<xf::compression::compressd_dt> bitUnPackStream("bitUnPackStream");
    hls::stream<bool> bitEndOfStream("bitEndOfStream");

#pragma HLS STREAM variable = outDownStream depth = 16
#pragma HLS STREAM variable = uncompOutStream depth = 32
#pragma HLS STREAM variable = byte_eos depth = 32
#pragma HLS STREAM variable = bitUnPackStream depth = 32
#pragma HLS STREAM variable = bitEndOfStream depth = 32

#pragma HLS RESOURCE variable = outDownStream core = FIFO_SRL
#pragma HLS RESOURCE variable = uncompOutStream core = FIFO_SRL
#pragma HLS RESOURCE variable = byte_eos core = FIFO_SRL
#pragma HLS RESOURCE variable = bitUnPackStream core = FIFO_SRL
#pragma HLS RESOURCE variable = bitEndOfStream core = FIFO_SRL

    hls::stream<uint32_t> outsize_val("outsize_val");

#pragma HLS dataflow
    xf::compression::axis2hlsStream<16>(inaxistream, outDownStream);

    bitUnPacker(outDownStream, bitUnPackStream, bitEndOfStream, input_size);

    xf::compression::lzDecompressZlibEos_new<HISTORY_SIZE, LOW_OFFSET>(bitUnPackStream, bitEndOfStream, uncompOutStream,
                                                                       byte_eos, outsize_val);

    xf::compression::hlsStream2axis(uncompOutStream, byte_eos, outaxistream, outsize_val, encodedsize);
}

extern "C" {
void xilZlibDecompressStream(hls::stream<xf::compression::hStream16b_t>& inaxistream,
                             hls::stream<xf::compression::hStream8b_t>& outaxistream,
                             hls::stream<xf::compression::hStream32b_t>& encodedsize,
                             uint32_t input_size) {
#pragma HLS interface axis port = inaxistream
#pragma HLS interface axis port = outaxistream
#pragma HLS interface axis port = encodedsize
#pragma HLS interface s_axilite port = input_size bundle = control
#pragma HLS interface s_axilite port = return bundle = control

    // printf("In decompress kernel \n");
    xil_inflate(inaxistream, outaxistream, encodedsize, input_size);
}
}
