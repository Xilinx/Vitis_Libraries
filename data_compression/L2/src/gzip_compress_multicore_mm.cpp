/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
 * @file gzip_compress_multicore_mm.cpp
 * @brief Source for Gzip compression multicore kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "gzip_compress_multicore_mm.hpp"

extern "C" {
/**
 * @brief Gzip mulicore compression kernel.
 *
 * @param in input stream width
 * @param out output stream width
 * @param compressd_size output size
 * @param input_size input size
 */

void xilGzipMultiCoreCompressFull(const ap_uint<GMEM_DWIDTH>* in,
                                  ap_uint<GMEM_DWIDTH>* out,
                                  uint64_t* compressd_size,
                                  uint64_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem max_read_burst_length = 128
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem max_write_burst_length = 128
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    hls::stream<ap_uint<GMEM_DWIDTH> > mm2sStream;
    hls::stream<uint64_t> mm2sSizeStream;
    hls::stream<ap_uint<GMEM_DWIDTH> > outStream;
    hls::stream<bool> outEos;
    hls::stream<uint64_t> outSizeStream;

#pragma HLS STREAM variable = mm2sStream depth = 256
#pragma HLS STREAM variable = mm2sSizeStream depth = 4
#pragma HLS STREAM variable = outStream depth = 256
#pragma HLS STREAM variable = outEos depth = 256
#pragma HLS STREAM variable = outSizeStream depth = 4

#pragma HLS BIND_STORAGE variable = mm2sOutStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = mm2sSizeStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outSizeStream type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::details::mm2sSimple<GMEM_DWIDTH, GMEM_BURST_SIZE>(in, mm2sStream, input_size, mm2sSizeStream);
    xf::compression::gzipMulticoreCompression<uint64_t, NUM_CORES, GMEM_DWIDTH, GZIP_BLOCK_SIZE>(
        mm2sStream, mm2sSizeStream, outStream, outEos, outSizeStream);
    xf::compression::details::s2mmEosSimple<GMEM_DWIDTH, GMEM_BURST_SIZE, uint64_t>(out, outStream, outEos,
                                                                                    outSizeStream, compressd_size);
}
}
