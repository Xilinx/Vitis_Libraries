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
 * @file xil_zlib_dm_kernel.cpp
 * @brief Source for data mover kernel for streaming data to zlib decompression streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "zlib_dm.hpp"

const int kGMemBurstSize = 16;

// typedef ap_uint<kGMemDWidth> uintMemWidth_t;

void zlib_dm(uintMemWidth_t* in,
             uintMemWidth_t* out,
             uint32_t* encoded_size,
             uint32_t input_size,
             hls::stream<ap_axiu<16, 0, 0, 0> >& instream,
             hls::stream<ap_axiu<8, 0, 0, 0> >& outstream) {
    hls::stream<uintMemWidth_t> instream512("inputStream");
    hls::stream<ap_uint<16> > outdownstream("outDownStream");
    hls::stream<ap_uint<8> > uncompoutstream("unCompOutStream");
    hls::stream<bool> byte_eos("byteEndOfStream");
    hls::stream<uintMemWidth_t> outstream512("outputStream");
    hls::stream<bool> outstream512_eos("outputStreamSize");

#pragma HLS STREAM variable = outdownstream depth = 32

#pragma HLS STREAM variable = instream512 depth = 32
#pragma HLS STREAM variable = outstream512 depth = 32
#pragma HLS STREAM variable = outstream512_eos depth = 32
#pragma HLS STREAM variable = byte_eos depth = 32

    hls::stream<uint32_t> outsize_val;

#pragma HLS dataflow
    xf::compression::mm2sSimple<kGMemDWidth, kGMemBurstSize>(in, instream512, input_size);

    xf::compression::streamDownsizer<uint32_t, kGMemDWidth, 16>(instream512, outdownstream, input_size);

    xf::compression::streamDataDm2k<16>(outdownstream, instream, input_size);
    xf::compression::streamDataK2dm(uncompoutstream, byte_eos, outsize_val, outstream);

    xf::compression::upsizerEos<uint16_t, 8, kGMemDWidth>(uncompoutstream, byte_eos, outstream512, outstream512_eos);
    xf::compression::s2mmEosSimple<uint32_t, kGMemBurstSize, kGMemDWidth, 1>(out, outstream512, outstream512_eos,
                                                                             outsize_val, encoded_size);
}

extern "C" {
void xilZlibDm(uintMemWidth_t* in,
               uintMemWidth_t* out,
               uint32_t* encoded_size,
               uint32_t input_size,
               hls::stream<ap_axiu<16, 0, 0, 0> >& instreamk,
               hls::stream<ap_axiu<8, 0, 0, 0> >& outstreamk) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = encoded_size offset = slave bundle = gmem
#pragma HLS interface axis port = instreamk
#pragma HLS interface axis port = outstreamk
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = encoded_size bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    ////printme("In decompress kernel \n");
    // Call for compression
    zlib_dm(in, out, encoded_size, input_size, instreamk, outstreamk);
}
}
