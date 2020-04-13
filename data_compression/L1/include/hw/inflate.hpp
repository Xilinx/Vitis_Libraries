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
#ifndef _XFCOMPRESSION_INFLATE_HPP_
#define _XFCOMPRESSION_INFLATE_HPP_

#include "hls_stream.h"
#include "huffman_decoder.hpp"
#include "lz_decompress.hpp"
#include "stream_downsizer.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

namespace xf {
namespace compression {

namespace details {

template <int PARALLEL_BYTES>
void lzLiteralUpsizer(hls::stream<ap_uint<8> >& inStream,
                      hls::stream<uint8_t>& inStreamSize,
                      hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream) {
    const uint8_t c_parallelBit = PARALLEL_BYTES * 8;
    const uint8_t c_maxLitLen = 128;
    ap_uint<c_parallelBit> outBuffer;
    uint8_t idx = 0;

    uint8_t i = 0;
    bool outFlag = false;
lzLiteralUpsizer:
    for (uint8_t size = inStreamSize.read(); size != (c_maxLitLen + 1);) {
#pragma HLS PIPELINE II = 1
        if (size) {
            outBuffer.range((idx + 1) * 8 - 1, idx * 8) = inStream.read();
            if (i == (size - 1)) {
                size = inStreamSize.read();
                outFlag = true;
                idx = 0;
                i = 0;
            } else {
                i++;
                if (idx == (PARALLEL_BYTES - 1)) {
                    idx = 0;
                    outFlag = true;
                } else {
                    idx++;
                    outFlag = false;
                }
            }
            if (outFlag) litStream << outBuffer;
        } else {
            size = inStreamSize.read();
        }
    }
}

void lzProcessingUnit(hls::stream<xf::compression::compressd_dt>& inStream,
                      hls::stream<bool>& inEndOfStream,
                      hls::stream<uint8_t>& litLenStream,
                      hls::stream<uint8_t>& lenStream,
                      hls::stream<uint8_t>& matchLenStream,
                      hls::stream<ap_uint<16> >& offsetStream,
                      hls::stream<ap_uint<8> >& outStream) {
    xf::compression::compressd_dt inValue, nextValue;
    const int c_maxLitLen = 128;
    uint16_t offset = 0;
    uint16_t matchLen = 0;
    uint8_t litLen = 0;
    uint8_t outLitLen = 0;

    bool eosFlag = inEndOfStream.read();

    nextValue = inStream.read();
    for (; eosFlag == false;) {
#pragma HLS PIPELINE II = 1
        inValue = nextValue;
        nextValue = inStream.read();
        eosFlag = inEndOfStream.read();

        offset = inValue.range(15, 0);
        matchLen = inValue.range(31, 16);

        bool outFlag, outStreamFlag;
        if (matchLen) {
            outFlag = true;
            outLitLen = litLen;
            litLen = 0;
            outStreamFlag = false;
        } else {
            outStreamFlag = true;
            outLitLen = litLen + 1;
            if (litLen == c_maxLitLen - 1) {
                outFlag = true;
                litLen = 0;
            } else {
                outFlag = false;
                litLen++;
            }
        }
        if (outStreamFlag) {
            outStream << inValue.range(7, 0);
        }
        if (outFlag) {
            litLenStream << outLitLen;
            offsetStream << offset;
            matchLenStream << matchLen;
            lenStream << outLitLen;
        }
    }

    if (litLen) {
        litLenStream << litLen;
        lenStream << litLen;
        offsetStream << 0;
        matchLenStream << 0;
    }

    lenStream << c_maxLitLen + 1;
    litLenStream << 0;
    offsetStream << 0;
    matchLenStream << 0;
}

} // namespace details

template <int HISTORY_SIZE = (32 * 1024)>
void inflate(hls::stream<ap_uint<16> >& inStream,
             hls::stream<ap_uint<8> >& outStream,
             hls::stream<bool>& outStreamEoS,
             hls::stream<uint32_t>& outStreamSize,
             uint32_t input_size) {
    hls::stream<xf::compression::compressd_dt> bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");

#pragma HLS STREAM variable = bitunpackstream depth = 32
#pragma HLS STREAM variable = bitendofstream depth = 32

#pragma HLS dataflow
    xf::compression::huffmanDecoderDynamic(inStream, bitunpackstream, bitendofstream, input_size);
    xf::compression::lzDecompressZlibEos<HISTORY_SIZE>(bitunpackstream, bitendofstream, outStream, outStreamEoS,
                                                       outStreamSize);
}

template <int PARALLEL_BYTES, int HUFF_LOOP_II = 1, int HISTORY_SIZE = (32 * 1024)>
void inflateMultiByte(hls::stream<ap_uint<16> >& inStream,
                      hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                      hls::stream<bool>& outStreamEoS,
                      hls::stream<uint32_t>& outStreamSize,
                      uint32_t inputSize) {
    const int c_parallelBit = PARALLEL_BYTES * 8;
    // HUFF_LOOP_II=1 gives better latency but FMax may drop
    // HUFF_LOOP_II=2 gives High FMax but latency will increase
    const int c_byteGenLoopII = HUFF_LOOP_II;

    hls::stream<xf::compression::compressd_dt> bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");
    hls::stream<ap_uint<c_parallelBit> > litStream("litStream");
    hls::stream<uint8_t> litLenStream("litLenStream");
    hls::stream<uint8_t> matchLenStream("matchLenStream");
    hls::stream<ap_uint<16> > offsetStream("offsetStream");
    hls::stream<ap_uint<8> > lzProcOutStream("lzProcOutStream");
    hls::stream<uint8_t> interLenStream("lenStream");

#pragma HLS STREAM variable = bitunpackstream depth = 16
#pragma HLS STREAM variable = bitendofstream depth = 16
#pragma HLS STREAM variable = lzProcOutStream depth = 256
#pragma HLS STREAM variable = interLenStream depth = 16
#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = litLenStream depth = 16
#pragma HLS STREAM variable = matchLenStream depth = 16
#pragma HLS STREAM variable = offsetStream depth = 16

#pragma HLS RESOURCE variable = bitunpackstream core = FIFO_SRL
#pragma HLS RESOURCE variable = bitunpackstream core = FIFO_SRL
#pragma HLS RESOURCE variable = interLenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litLenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = matchLenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = offsetStream core = FIFO_SRL
#pragma HLS RESOURCE variable = lzProcOutStream core = RAM_2P_BRAM

#pragma HLS dataflow
    xf::compression::huffmanDecoderDynamic<c_byteGenLoopII>(inStream, bitunpackstream, bitendofstream, inputSize);
    xf::compression::details::lzProcessingUnit(bitunpackstream, bitendofstream, litLenStream, interLenStream,
                                               matchLenStream, offsetStream, lzProcOutStream);
    xf::compression::details::lzLiteralUpsizer<PARALLEL_BYTES>(lzProcOutStream, interLenStream, litStream);

    xf::compression::lzMultiByteDecompress<PARALLEL_BYTES, HISTORY_SIZE, uint8_t>(
        litLenStream, litStream, offsetStream, matchLenStream, outStream, outStreamEoS, outStreamSize);
}

template <int PARALLEL_BYTES, int HUFF_LOOP_II = 1, int HISTORY_SIZE = (32 * 1024)>
void zlibMultiByteDecompressEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                   hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                                   hls::stream<bool>& outStreamEoS,
                                   hls::stream<uint32_t>& outStreamSize,
                                   uint32_t inputSize) {
    const int c_parallelBit = PARALLEL_BYTES * 8;
    // HUFF_LOOP_II=1 gives better latency but FMax may drop
    // HUFF_LOOP_II=2 gives High FMax but latency will increase
    const int c_byteGenLoopII = HUFF_LOOP_II;
    hls::stream<ap_uint<16> > outdownstream("outDownStream");
#pragma HLS STREAM variable = outdownstream depth = 16
#pragma HLS RESOURCE variable = outdownstream core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::details::streamDownsizer<uint32_t, c_parallelBit, 16>(inStream, outdownstream, inputSize);
    xf::compression::inflateMultiByte<PARALLEL_BYTES, c_byteGenLoopII>(outdownstream, outStream, outStreamEoS,
                                                                       outStreamSize, inputSize);
}

template <int PARALLEL_BYTES,
          int IN_GMEM_DATAWIDTH = 512,
          int OUT_GMEM_DATAWIDTH = 512,
          int IN_BURST_SIZE = 16,
          int OUT_BURST_SIZE = 16>
void inflateMultiByteMM(const ap_uint<IN_GMEM_DATAWIDTH>* in,
                        ap_uint<OUT_GMEM_DATAWIDTH>* out,
                        ap_uint<OUT_GMEM_DATAWIDTH>* outSize,
                        const uint32_t inputSize) {
    const uint8_t c_krnlInWidth = 16;
    const uint8_t c_krnlOutWidth = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<c_krnlInWidth> > inStream("inStream");
    hls::stream<ap_uint<c_krnlOutWidth> > outStream("outStream");
    hls::stream<bool> outStreamEos("outStreamEos");
    hls::stream<uint32_t> outSizeStream("outSizeStream");

#pragma HLS STREAM variable = inStream
#pragma HLS STREAM variable = outStream
#pragma HLS STREAM variable = outStreamEos
#pragma HLS DATAFLOW
    xf::compression::details::mm2Stream<c_krnlInWidth>(in, inStream, inputSize);
    xf::compression::inflateMultiByte<PARALLEL_BYTES>(inStream, outStream, outStreamEos, outSizeStream, inputSize);
    xf::compression::details::stream2MM<c_krnlOutWidth>(outStream, outStreamEos, outSizeStream, out, outSize);
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_INFLATE_HPP_
