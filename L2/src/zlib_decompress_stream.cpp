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

#include "zlib_decompress_stream.hpp"

/**
 * @brief kStreamReadZlibDecomp Read 16-bit wide data from internal streams output by compression modules
 *                              and write to output axi stream.
 *
 * @param inKStream     input kernel stream
 * @param readStream    internal stream to be read for processing
 * @param input_size    input data size
 *
 */
void kStreamReadZlibDecomp(hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& inKStream,
                           hls::stream<ap_uint<kGMemDWidth> >& readStream,
                           uint32_t input_size) {
    int itrLim = 1 + (input_size - 1) / (kGMemDWidth / 8);
    for (int i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_axiu<kGMemDWidth, 0, 0, 0> tmp = inKStream.read();
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
void kStreamWriteZlibDecomp(hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& outKStream,
                            hls::stream<ap_axiu<32, 0, 0, 0> >& sizestreamd,
                            hls::stream<ap_uint<kGMemDWidth> >& outDataStream,
                            hls::stream<bool>& byteEos,
                            hls::stream<uint32_t>& dataSize) {
    uint32_t outSize = 0;
    bool lastByte = false;
    uint32_t pckCount = 0;
    ap_uint<kGMemDWidth> tmp;
    ap_axiu<32, 0, 0, 0> pcksize;
    ap_axiu<kGMemDWidth, 0, 0, 0> t1;
    pcksize.data = kGMemDWidth / 8;

    for (lastByte = byteEos.read(); !lastByte; pckCount++) {
#pragma HLS PIPELINE II = 1
        tmp = outDataStream.read();
        lastByte = byteEos.read();
        t1.data = tmp;
        t1.last = lastByte;

        if (lastByte) {
            outSize = dataSize.read();
            pcksize.data = outSize - (pckCount * (kGMemDWidth / 8));
        }

        sizestreamd.write(pcksize);
        outKStream.write(t1);
    }
    outDataStream.read();
}

void xil_inflate(hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& inaxistream,
                 hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& outaxistream,
                 hls::stream<ap_axiu<32, 0, 0, 0> >& sizestreamd,
                 uint32_t input_size) {
    hls::stream<uintMemWidth_t> inhlsstream("inputStream");
    hls::stream<ap_uint<16> > outdownstream("outDownStream");
    hls::stream<ap_uint<8> > uncompoutstream("unCompOutStream");
    hls::stream<bool> byte_eos("byteEndOfStream");

    hls::stream<xf::compression::compressd_dt> bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");
    hls::stream<uintMemWidth_t> outhlsstream("outputStream");
    hls::stream<bool> outhlsstream_eos("outputStreamSize");

#pragma HLS STREAM variable = inhlsstream depth = 32
#pragma HLS STREAM variable = bitunpackstream depth = 32
#pragma HLS STREAM variable = bitendofstream depth = 32
#pragma HLS STREAM variable = outdownstream depth = 256

#pragma HLS STREAM variable = uncompoutstream depth = 256
#pragma HLS STREAM variable = byte_eos depth = 32
#pragma HLS STREAM variable = outhlsstream depth = 32
#pragma HLS STREAM variable = outhlsstream_eos depth = 32
    hls::stream<uint32_t> outsize_val("outsize_val");

#pragma HLS dataflow

    kStreamReadZlibDecomp(inaxistream, inhlsstream, input_size);
    xf::compression::details::streamDownsizer<uint32_t, kGMemDWidth, 16>(inhlsstream, outdownstream, input_size);
    xf::compression::huffmanDecoderDynamic(outdownstream, bitunpackstream, bitendofstream, input_size);
    xf::compression::lzDecompressZlibEos<HISTORY_SIZE, LOW_OFFSET>(bitunpackstream, bitendofstream, uncompoutstream,
                                                                   byte_eos, outsize_val);
    xf::compression::details::upsizerEos<8, kGMemDWidth>(uncompoutstream, byte_eos, outhlsstream, outhlsstream_eos);
    kStreamWriteZlibDecomp(outaxistream, sizestreamd, outhlsstream, outhlsstream_eos, outsize_val);
}

extern "C" {
void xilDecompressStream(uint32_t input_size,
                         hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& inaxistreamd,
                         hls::stream<ap_axiu<kGMemDWidth, 0, 0, 0> >& outaxistreamd,
                         hls::stream<ap_axiu<32, 0, 0, 0> >& sizestreamd) {
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS interface axis port = inaxistreamd
#pragma HLS interface axis port = outaxistreamd
#pragma HLS interface axis port = sizestreamd
#pragma HLS INTERFACE s_axilite port = return bundle = control
    // Call for decompression
    xil_inflate(inaxistreamd, outaxistreamd, sizestreamd, input_size);
}
}
