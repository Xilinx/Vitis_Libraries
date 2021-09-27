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
 * @file block_stream_perf_dm.cpp
 * @brief Source file for data mover kernel which streams data to the streaming
 * kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "block_stream_perf_dm.hpp"

const int factor = GMEM_DWIDTH / 8;

void __xf_datamover(uintDataWidth* in,
                    uintDataWidth* out,
                    uint32_t input_size,
                    uint32_t* compressed_size,
                    uint32_t numItr,
                    uint32_t blckSize,
                    hls::stream<ap_axiu<GMEM_DWIDTH, 0, 0, 0> >& instream_orig,
                    hls::stream<ap_axiu<GMEM_DWIDTH, TUSER_DWIDTH, 0, 0> >& outstream_dest) {
    hls::stream<ap_uint<GMEM_DWIDTH> > mm2sStream;

    hls::stream<ap_uint<GMEM_DWIDTH> > outStream;
    hls::stream<bool> outEos;
    hls::stream<uint32_t> outSizeStream;

#pragma HLS STREAM variable = mm2sStream depth = 4
#pragma HLS STREAM variable = outStream depth = 4
#pragma HLS STREAM variable = outEos depth = 4
#pragma HLS STREAM variable = outSizeStream depth = 4

#pragma HLS BIND_STORAGE variable = mm2sStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outSizeStream type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::details::mm2sSimple<GMEM_DWIDTH, GMEM_BURST_SIZE>(in, mm2sStream, input_size, numItr);

    xf::compression::details::streamDm2k<GMEM_DWIDTH, uint32_t, 32>(mm2sStream, input_size, numItr, blckSize,
                                                                    instream_orig);
    xf::compression::details::streamK2Dm<factor, uint32_t, 32, TUSER_DWIDTH>(
        outStream, outEos, outSizeStream, outstream_dest, numItr, input_size, blckSize);

    // Segregate in two modules for GMEM Write
    xf::compression::details::s2mmEosSimple<GMEM_DWIDTH, GMEM_BURST_SIZE, uint32_t>(out, outStream, outEos, numItr,
                                                                                    input_size, blckSize);

    xf::compression::details::s2mmSizeWrite<uint32_t, TUSER_DWIDTH>(outSizeStream, compressed_size, numItr, input_size,
                                                                    blckSize);
}

// Top Function
extern "C" {
void xilDataMover(uintDataWidth* in,
                  uintDataWidth* out,
                  uint32_t input_size,
                  uint32_t numItr,
                  uint32_t blckSize,
                  uint32_t* compressed_size,
                  hls::stream<ap_axiu<GMEM_DWIDTH, 0, 0, 0> >& instream_orig,
                  hls::stream<ap_axiu<GMEM_DWIDTH, TUSER_DWIDTH, 0, 0> >& outstream_dest) {
    constexpr int c_gmem0_width = GMEM_DWIDTH;
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0 max_read_burst_length = 64 max_widen_bitwidth = \
    c_gmem0_width
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem1 max_write_burst_length = 64 max_widen_bitwidth = \
    c_gmem0_width
#pragma HLS INTERFACE m_axi port = compressed_size offset = slave bundle = gmem2
#pragma HLS interface axis port = instream_orig
#pragma HLS interface axis port = outstream_dest

    // Transfer Data to and from compression kernels
    __xf_datamover(in, out, input_size, compressed_size, numItr, blckSize, instream_orig, outstream_dest);
}
}
