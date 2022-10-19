/*
 * (c) Copyright 2019-2022 Xilinx, Inc. All rights reserved.
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
 * @file zlib_dm_wr.cpp
 * @brief Source for data writer kernel for streaming data to zlib decompression
 * streaming kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "zlibd_mc_mm2s_tester.hpp"

template <int STREAMDWIDTH>
void streamDataDm2kSync(hls::stream<ap_uint<STREAMDWIDTH> >& in,
                        hls::stream<ap_axiu<STREAMDWIDTH, 0, 0, 0> >& inStream_dm,
                        uint32_t inputSize,
                        uint32_t numItr,
                        uint32_t last) {
    // read data from input hls to input stream for decompression kernel
    auto swidth = STREAMDWIDTH / 8;
    uint32_t itrLim = 1 + (inputSize - 1) / swidth;
    uint8_t strb = (1 << (inputSize % swidth)) - 1;

    for (auto z = 0; z < numItr; z++) {
    streamDataDm2kSync:
        for (uint32_t i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
            ap_uint<STREAMDWIDTH> temp = in.read();
            ap_axiu<STREAMDWIDTH, 0, 0, 0> dataIn;
            dataIn.data = temp; // kernel to kernel data transfer
            // for (int i = 0; i < STREAMDWIDTH/8; i++) {
            //  printf("%c",(char)temp.range((i+1)*8 -1, i*8));
            //}
            dataIn.last = 0;
            dataIn.strb = -1;
            if (i == itrLim - 1) {
                dataIn.last = (ap_uint<1>)last;
                dataIn.strb = strb;
            }
            inStream_dm.write(dataIn);
        }
    }
}

extern "C" {
void xilGzipMM2S(uintMemWidth_t* in,
                 uint32_t inputSize,
                 uint32_t last,
                 uint32_t numItr,
                 hls::stream<ap_axiu<c_inStreamDwidth, 0, 0, 0> >& outStream) {
    const int c_gmem0_width = INPUT_BYTES * 8;
#pragma HLS INTERFACE m_axi port = in max_widen_bitwidth = c_gmem0_width offset = slave bundle = \
    gmem0 max_read_burst_length = 64 max_write_burst_length = 2 num_read_outstanding = 8 num_write_outstanding = 1
#pragma HLS interface axis port = outStream
#pragma HLS_INTERFACE ap_ctrl_chain port = return bundle = control

    hls::stream<uintMemWidth_t> inHlsStream("inputStream");
#pragma HLS STREAM variable = inHlsStream depth = 512

#pragma HLS dataflow
    xf::compression::details::mm2sSimple<INPUT_BYTES * 8, 128>(in, inHlsStream, inputSize, numItr);
    streamDataDm2kSync<c_inStreamDwidth>(inHlsStream, outStream, inputSize, numItr, last);
}
}
