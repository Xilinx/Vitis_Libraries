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
 * @file xil_zlib_decompress_stream_k2k_kernel.cpp
 * @brief Source for zlib decompression streaming(kernel to kernel) kernel.
 *
 * This file is part of XF Compression Library.
 */

#include "zlib_decompress_stream_k2k_kernel.hpp"

// LZ specific Defines
#define MIN_OFFSET 1
#define MIN_MATCH 4
#define LZ_MAX_OFFSET_LIMIT 32768
#define HISTORY_SIZE LZ_MAX_OFFSET_LIMIT
#define LOW_OFFSET 10

/**
 * @brief kStreamReadZlibDecomp Read 16-bit wide data from internal streams output by compression modules
 *                              and write to output axi stream.
 *
 * @param inKStream     input kernel stream
 * @param readStream    internal stream to be read for processing
 *
 */
void kStreamReadZlibDecomp(hls::stream<xf::compression::kStream16b_t>& inKStream,
                           hls::stream<ap_uint<16> >& readStream,
                           uint32_t input_size) {
    for (int i = 0; i < input_size; i += 2) {
#pragma HLS PIPELINE II = 1
        xf::compression::kStream16b_t tmp = inKStream.read();
        readStream << tmp.data;
    }
}

/**
 * @brief kStreamWriteZlibDecomp Read 16-bit wide data from internal streams output by compression modules
 *                                and write to output axi stream.
 *
 * @param outKStream    output kernel stream
 * @param outDataStream output data stream from internal modules
 * @param byteEos       internal stream which indicates end of data stream
 * @param dataSize      size of data in streams
 *
 */
void kStreamWriteZlibDecomp(hls::stream<xf::compression::kStream8b_t>& outKStream,
                            hls::stream<ap_uint<8> >& outDataStream,
                            hls::stream<bool>& byteEos,
                            hls::stream<uint32_t>& dataSize) {
    uint32_t outSize = 0;
    bool lastByte = false;
    ap_uint<8> tmp;

    do {
#pragma HLS PIPELINE II = 1
        tmp = outDataStream.read();
        lastByte = byteEos.read();
        xf::compression::kStream8b_t t1;
        t1.data = tmp;
        t1.last = lastByte;
        outKStream.write(t1);
    } while (!lastByte);
    outSize = dataSize.read();
}

void xil_inflate(hls::stream<xf::compression::kStream16b_t>& inaxistream,
                 hls::stream<xf::compression::kStream8b_t>& outaxistream,
                 uint32_t input_size) {
    // printf("Inflate: input_size %d \n", input_size);
    hls::stream<ap_uint<16> > outdownstream("outDownStream");
    hls::stream<ap_uint<8> > uncompoutstream("unCompOutStream");
    hls::stream<bool> byte_eos("byteEndOfStream");

    hls::stream<xf::compression::compressd_dt> bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");
#pragma HLS STREAM variable = bitunpackstream depth = 32
#pragma HLS STREAM variable = bitendofstream depth = 32

#pragma HLS STREAM variable = outdownstream depth = 32
#pragma HLS STREAM variable = uncompoutstream depth = 32
#pragma HLS STREAM variable = byte_eos depth = 32

    hls::stream<uint32_t> outsize_val("outsize_val");

#pragma HLS dataflow

    kStreamReadZlibDecomp(inaxistream, outdownstream, input_size);

    bitUnPacker(outdownstream, bitunpackstream, bitendofstream, input_size);

    xf::compression::lzDecompressZlibEos_new<HISTORY_SIZE, LOW_OFFSET>(bitunpackstream, bitendofstream, uncompoutstream,
                                                                       byte_eos, outsize_val);

    kStreamWriteZlibDecomp(outaxistream, uncompoutstream, byte_eos, outsize_val);
}

extern "C" {
void xilDecompressStreamk2k(uint32_t input_size,
                            hls::stream<xf::compression::kStream16b_t>& inaxistreamd,
                            hls::stream<xf::compression::kStream8b_t>& outaxistreamd) {
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS interface axis port = inaxistreamd
#pragma HLS interface axis port = outaxistreamd
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // Call for parallel compression
    xil_inflate(inaxistreamd, outaxistreamd, input_size);
}
}
