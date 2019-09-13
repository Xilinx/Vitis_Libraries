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
#ifndef _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_
#define _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "ap_axi_sdata.h"
#include <ap_int.h>

#include "common.h"

#define DWIDTH 8
#define DWIDTH2 32

namespace xf {
namespace compression {

typedef qdma_axis<DWIDTH, 0, 0, 0> pkt1b;
typedef qdma_axis<DWIDTH2, 0, 0, 0> pkt4b;

/**
 * @brief axis2s Read data from axi stream and write to internal streams for compression modules.
 *
 * @param inputAxisStream   Incoming stream from qdma
 * @param inputStream       internal hls stream
 * @param inputSize         Size of the data coming from input axi stream
 *
 */
void axis2s(hls::stream<pkt1b>& inputAxiStream, hls::stream<streamDt>& inputStream, uint32_t inputSize) {
    for (int i = 0; i < inputSize; i++) {
#pragma HLS PIPELINE II = 1
        pkt1b t1 = inputAxiStream.read();
        ap_uint<DWIDTH> tmpOut;
        tmpOut = t1.get_data();
        inputStream << tmpOut;
    }
}

/**
 * @brief s2axis Read data from internal streams output by compression modules and write to output axi stream.
 *
 * @param outputStream      internal hls stream
 * @param outStreamEos      Stream to specify the end of stream
 * @param outputAxisStream  Output stream going to axi
 * @param outStreamSize     Size of the data coming to output from stream
 * @param outAxiStreamSize  Size of the data to go through output axi stream
 *
 */
void s2axis(hls::stream<streamDt>& outputStream,
            hls::stream<bool>& outStreamEos,
            hls::stream<pkt1b>& outputAxiStream,
            hls::stream<uint32_t>& outStreamSize,
            hls::stream<pkt4b>& outAxiStreamSize) {
    uint8_t temp;
    bool flag;
    do {
        temp = outputStream.read();
        flag = outStreamEos.read();
        pkt1b tOut;
        tOut.set_data(temp);
        tOut.set_last(flag);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    } while (!flag);
    uint32_t outSize = outStreamSize.read();
    pkt4b tOutSize;
    tOutSize.set_data(outSize);
    tOutSize.set_last(1);
    tOutSize.set_keep(-1);
    outAxiStreamSize.write(tOutSize);
}

/**
 * @brief s2axis2 Read data from internal streams output by compression modules and write to output axi stream.
 *
 * @param outputStream      Internal hls output stream
 * @param outputAxisStream  Output stream going to axi
 * @param originalSize      Output data size written to output stream
 *
 */
void s2axis2(hls::stream<streamDt>& outputStream, hls::stream<pkt1b>& outputAxiStream, uint32_t originalSize) {
    uint8_t temp;
    for (int i = 0; i < originalSize - 1; i++) {
        temp = outputStream.read();
        pkt1b tOut;
        tOut.set_data(temp);
        // tOut.set_last(0);
        tOut.set_keep(-1);
        outputAxiStream.write(tOut);
    }
    // last byte
    temp = outputStream.read();
    pkt1b tOut;
    tOut.set_data(temp);
    tOut.set_last(1);
    tOut.set_keep(-1);
    outputAxiStream.write(tOut);
}

} // end compression
} // end xf

#undef DWIDTH
#undef DWIDTH2

#endif // _XFCOMPRESSION_AXI_STREAM_UTILS_HPP_
