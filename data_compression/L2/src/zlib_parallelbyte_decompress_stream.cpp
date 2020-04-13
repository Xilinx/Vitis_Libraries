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

#include "zlib_parallelbyte_decompress_stream.hpp"

const int c_parallelBit = MULTIPLE_BYTES * 8;
const int c_historySize = LZ_MAX_OFFSET_LIMIT;

typedef ap_uint<c_parallelBit> uintMemWidth_t;
typedef ap_uint<16> offset_dt;

/**
 * @brief kStreamReadZlibDecomp Read 16-bit wide data from internal streams output by compression modules
 *                              and write to output axi stream.
 *
 * @param inKStream     input kernel stream
 * @param readStream    internal stream to be read for processing
 * @param input_size    input data size
 *
 */
void kStreamReadZlibDecomp(hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& inKStream,
                           hls::stream<ap_uint<c_parallelBit> >& readStream,
                           uint32_t input_size) {
    int itrLim = 1 + (input_size - 1) / (c_parallelBit / 8);
    for (int i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_axiu<c_parallelBit, 0, 0, 0> tmp = inKStream.read();
        readStream << tmp.data;
    }
}

/**
 * @brief kStreamWriteZlibDecomp Read 16-bit wide data from internal streams output by compression modules
 *                                and write to output axi stream.
 *
 * @param outKStream    output kernel stream
 * @param sizestreamd   stream to indicate readable bytes in 512-bit wide stream
 * @param outDataStream output data stream from internal modules
 * @param byteEos       internal stream which indicates end of data stream
 * @param dataSize      size of data in streams
 *
 */
void kStreamWriteZlibDecomp(hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& outKStream,
                            hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& sizestreamd,
                            hls::stream<ap_uint<c_parallelBit> >& outDataStream,
                            hls::stream<bool>& byteEos,
                            hls::stream<uint32_t>& dataSize) {
    uint32_t outSize = 0;
    bool lastByte = false;
    uint32_t pckCount = 0;
    ap_uint<c_parallelBit> tmp;
    ap_axiu<c_parallelBit, 0, 0, 0> pcksize;
    ap_axiu<c_parallelBit, 0, 0, 0> t1;
    pcksize.data = c_parallelBit / 8;

    for (lastByte = byteEos.read(); !lastByte; pckCount++) {
#pragma HLS PIPELINE II = 1
        tmp = outDataStream.read();
        lastByte = byteEos.read();
        t1.data = tmp;
        t1.last = lastByte;

        if (lastByte) {
            outSize = dataSize.read();
            pcksize.data = outSize - (pckCount * (c_parallelBit / 8));
        }

        sizestreamd.write(pcksize);
        outKStream.write(t1);
    }
    outDataStream.read();
}

void xil_inflate(hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& inaxistream,
                 hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& outaxistream,
                 hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& sizestreamd,
                 uint32_t input_size) {
    hls::stream<ap_uint<c_parallelBit> > inStream("inputStream");
    hls::stream<ap_uint<c_parallelBit> > outStream("outStream");
    hls::stream<bool> outStreamEos("outStreamEos");
    hls::stream<uint32_t> outSizeStream("outSizeStream");

#pragma HLS STREAM variable = inStream depth = 32
#pragma HLS STREAM variable = outStream depth = 32
#pragma HLS STREAM variable = outStreamEos depth = 32

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream core = FIFO_SRL
#pragma HLS RESOURCE variable = outStreamEos core = FIFO_SRL

#pragma HLS dataflow
    kStreamReadZlibDecomp(inaxistream, inStream, input_size);
    xf::compression::zlibMultiByteDecompressEngine<MULTIPLE_BYTES, HIGH_FMAX_II, c_historySize>(
        inStream, outStream, outStreamEos, outSizeStream, input_size);
    kStreamWriteZlibDecomp(outaxistream, sizestreamd, outStream, outStreamEos, outSizeStream);
}

extern "C" {
void xilDecompressStream(uint32_t input_size,
                         hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& inaxistreamd,
                         hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& outaxistreamd,
                         hls::stream<ap_axiu<c_parallelBit, 0, 0, 0> >& sizestreamd) {
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS interface axis port = inaxistreamd
#pragma HLS interface axis port = outaxistreamd
#pragma HLS interface axis port = sizestreamd
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // Call for decompression
    xil_inflate(inaxistreamd, outaxistreamd, sizestreamd, input_size);
}
}
