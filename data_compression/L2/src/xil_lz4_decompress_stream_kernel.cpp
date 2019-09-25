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

#include "lz4_decompress_stream_kernel.hpp"

#define LZ_MAX_OFFSET 65536
#define LZ_HISTORY_SIZE LZ_MAX_OFFSET
#define LZ_READ_STATE 0
#define LZ_MATCH_STATE 1
#define LZ_LOW_OFFSET_STATE 2
#define LZ_LOW_OFFSET 8 // This should be bigger than Pipeline Depth to handle inter dependency false case

extern "C" {
void xilLz4DecompressStream(hls::stream<xf::compression::hStream8b_t>& inaxistream,
                            hls::stream<xf::compression::hStream8b_t>& outaxistream,
                            uint32_t inputSize,
                            uint32_t outputSize) {
#pragma HLS interface axis port = inaxistream
#pragma HLS interface axis port = outaxistream
#pragma HLS interface s_axilite port = inputSize bundle = control
#pragma HLS interface s_axilite port = outputSize bundle = control
#pragma HLS interface s_axilite port = return bundle = control

    hls::stream<uintVt> instreamV("instreamV");
    hls::stream<xf::compression::compressd_dt> decompressdStream("decompressdStream");
    hls::stream<uintVt> decompressedStream("decompressedStream");
    hls::stream<xf::compression::streamDt> inStream("inStream");
    hls::stream<xf::compression::streamDt> outStream("outStream");

#pragma HLS STREAM variable = inStream depth = 2
#pragma HLS STREAM variable = outStream depth = 2
#pragma HLS STREAM variable = instreamV depth = 8
#pragma HLS STREAM variable = decompressdStream depth = 8
#pragma HLS STREAM variable = decompressedStream depth = 8

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream core = FIFO_SRL
#pragma HLS RESOURCE variable = instreamV core = FIFO_SRL
#pragma HLS RESOURCE variable = decompressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = decompressedStream core = FIFO_SRL

#pragma HLS dataflow

    // printf("Input : %d bytes\t", inputSize);
    xf::compression::axis2hlsStreamFixedSize(inaxistream, inStream, inputSize);
    xf::compression::lz4Decompress(inStream, decompressdStream, inputSize);
    xf::compression::lzDecompress<LZ_HISTORY_SIZE, LZ_READ_STATE, LZ_MATCH_STATE, LZ_LOW_OFFSET_STATE, LZ_LOW_OFFSET>(
        decompressdStream, outStream, outputSize);
    xf::compression::hlsStream2axiStreamFixedSize(outStream, outaxistream, outputSize);
    // printf("Done\n");
}
}
