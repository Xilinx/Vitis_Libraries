/*
 * (c) Copyright 2019-2021 Xilinx, Inc. All rights reserved.
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

void xilGzipCompBlock(const ap_uint<GMEM_DWIDTH>* in,
                      ap_uint<GMEM_DWIDTH>* out,
                      uint32_t* compressd_size,
                      uint32_t* checksumData,
                      uint32_t input_size,
                      bool checksumType) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem max_read_burst_length = 128
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem max_write_burst_length = 128
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = checksumData offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = checksumData bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = checksumType bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS INTERFACE ap_ctrl_chain port = return

    hls::stream<ap_uint<GMEM_DWIDTH> > mm2sStream;
    hls::stream<uint32_t> mm2sSizeStream;
    hls::stream<ap_uint<32> > checksumInitStream;
    hls::stream<ap_uint<2> > checksumTypeStream;

    hls::stream<ap_uint<32> > checksumOutStream;
    hls::stream<bool> checksumOutEos;
    hls::stream<ap_uint<GMEM_DWIDTH> > outStream;
    hls::stream<bool> outEos;
    hls::stream<uint32_t> outSizeStream;

#pragma HLS STREAM variable = mm2sStream depth = 4
#pragma HLS STREAM variable = mm2sSizeStream depth = 4
#pragma HLS STREAM variable = checksumInitStream depth = 4
#pragma HLS STREAM variable = checksumTypeStream depth = 4
#pragma HLS STREAM variable = outStream depth = 4
#pragma HLS STREAM variable = outEos depth = 4
#pragma HLS STREAM variable = checksumOutStream depth = 4
#pragma HLS STREAM variable = checksumOutEos depth = 4
#pragma HLS STREAM variable = outSizeStream depth = 4

#pragma HLS BIND_STORAGE variable = mm2sStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = mm2sSizeStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outSizeStream type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::details::mm2Stream<GMEM_DWIDTH, GMEM_BURST_SIZE>(
        in, mm2sStream, checksumInitStream, checksumData, input_size, mm2sSizeStream, checksumType, checksumTypeStream);

    xf::compression::gzipMulticoreCompression<NUM_CORES, URAM_BUFFER>(
        mm2sStream, mm2sSizeStream, checksumInitStream, outStream, outEos, outSizeStream, checksumOutStream,
        checksumOutEos, checksumTypeStream);

    xf::compression::details::stream2MM<GMEM_DWIDTH, GMEM_BURST_SIZE, uint32_t>(
        out, checksumData, checksumOutStream, checksumOutEos, outStream, outEos, outSizeStream, compressd_size);
}
}
