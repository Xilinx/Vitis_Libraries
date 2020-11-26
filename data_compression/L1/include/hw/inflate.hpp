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

#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "huffman_decoder.hpp"
#include "lz_decompress.hpp"
#include "stream_upsizer.hpp"
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

template <class SIZE_DT = uint8_t>
void lzProcessingUnit(hls::stream<ap_uint<32> >& inStream,
                      hls::stream<bool>& inEndOfStream,
                      hls::stream<SIZE_DT>& litLenStream,
                      hls::stream<uint8_t>& lenStream,
                      hls::stream<SIZE_DT>& matchLenStream,
                      hls::stream<ap_uint<16> >& offsetStream,
                      hls::stream<ap_uint<8> >& outStream) {
    ap_uint<32> inValue, nextValue;
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

template <int STREAM_WIDTH>
void kStreamReadZlibDecomp(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& inKStream,
                           hls::stream<ap_uint<STREAM_WIDTH> >& readStream,
                           uint32_t input_size) {
    /**
     * @brief kStreamReadZlibDecomp Read 16-bit wide data from internal streams output by compression modules
     *                              and write to output axi stream.
     *
     * @param inKStream     input kernel stream
     * @param readStream    internal stream to be read for processing
     * @param input_size    input data size
     *
     */
    int itrLim = 1 + (input_size - 1) / (STREAM_WIDTH / 8);
    for (int i = 0; i < itrLim; i++) {
#pragma HLS PIPELINE II = 1
        ap_axiu<STREAM_WIDTH, 0, 0, 0> tmp = inKStream.read();
        readStream << tmp.data;
    }
}

template <int STREAM_WIDTH>
void kStreamWriteZlibDecomp(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& outKStream,
                            hls::stream<ap_axiu<64, 0, 0, 0> >& sizestreamd,
                            hls::stream<ap_uint<STREAM_WIDTH> >& outDataStream,
                            hls::stream<bool>& byteEos,
                            hls::stream<uint64_t>& dataSize) {
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
    bool lastByte = false;
    ap_uint<STREAM_WIDTH> tmp;
    ap_axiu<STREAM_WIDTH, 0, 0, 0> t1;

    ap_axiu<64, 0, 0, 0> pcksize;

    bool flag = 0;
    for (lastByte = byteEos.read(); !lastByte; lastByte = byteEos.read()) {
#pragma HLS PIPELINE II = 1
        if (flag) {
            outKStream << t1;
        } else {
            flag = true;
        }
        tmp = outDataStream.read();
        t1.data = tmp;
        t1.last = 0;
    }
    // read extra packet
    outDataStream.read();
    // send the previously read packet
    t1.data = tmp;
    t1.last = 1;
    outKStream << t1;
    // write the total size of decompressed output
    pcksize.data = dataSize.read();
    // write total output size
    pcksize.last = 1;
    sizestreamd << pcksize;
}

template <int DECODER, int HISTORY_SIZE = (32 * 1024), int LOW_OFFSET = 8>
void inflateCoreStream(hls::stream<ap_uint<16> >& inStream,
                       hls::stream<ap_uint<8> >& outStream,
                       hls::stream<bool>& outStreamEoS,
                       hls::stream<uint32_t>& outStreamSize,
                       hls::stream<uint32_t>& inSize) {
    const int c_byteGenLoopII = 1;
    const eHuffmanType c_decoderType = (eHuffmanType)DECODER;

    hls::stream<ap_uint<32> > bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");
    hls::stream<uint32_t> sizeTrans("sizeTrans");

#pragma HLS STREAM variable = bitunpackstream depth = 32
#pragma HLS STREAM variable = bitendofstream depth = 32
#pragma HLS STREAM variable = sizeTrans depth = 3

#pragma HLS dataflow

    xf::compression::huffmanDecoderStream<c_decoderType, c_byteGenLoopII>(inStream, bitunpackstream, bitendofstream,
                                                                          inSize, sizeTrans);

    xf::compression::lzDecompressZlibEosStream<HISTORY_SIZE>(bitunpackstream, bitendofstream, outStream, outStreamEoS,
                                                             outStreamSize, sizeTrans);
}

template <int DECODER, int HISTORY_SIZE = (32 * 1024), int LOW_OFFSET = 8>
void inflateCore(hls::stream<ap_uint<16> >& inStream,
                 hls::stream<ap_uint<8> >& outStream,
                 hls::stream<bool>& outStreamEoS,
                 hls::stream<uint64_t>& outStreamSize,
                 uint32_t input_size) {
    const int c_byteGenLoopII = 1;
    const eHuffmanType c_decoderType = (eHuffmanType)DECODER;

    hls::stream<ap_uint<32> > bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");

#pragma HLS STREAM variable = bitunpackstream depth = 32
#pragma HLS STREAM variable = bitendofstream depth = 32

#pragma HLS dataflow

    xf::compression::huffmanDecoder<c_decoderType, c_byteGenLoopII>(inStream, bitunpackstream, bitendofstream,
                                                                    input_size);

    xf::compression::lzDecompressZlibEos<HISTORY_SIZE>(bitunpackstream, bitendofstream, outStream, outStreamEoS,
                                                       outStreamSize);
}

template <int DECODER, int PARALLEL_BYTES, int HUFF_LOOP_II = 1, int HISTORY_SIZE = (32 * 1024)>
void inflateMultiByteCore(hls::stream<ap_uint<16> >& inStream,
                          hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                          hls::stream<bool>& outStreamEoS,
                          hls::stream<uint64_t>& outStreamSize,
                          uint32_t inputSize) {
    const int c_parallelBit = PARALLEL_BYTES * 8;
    // HUFF_LOOP_II=1 gives better latency but FMax may drop
    // HUFF_LOOP_II=2 gives High FMax but latency will increase
    const int c_byteGenLoopII = HUFF_LOOP_II;
    const eHuffmanType c_decoderType = (eHuffmanType)DECODER;

    hls::stream<ap_uint<32> > bitunpackstream("bitUnPackStream");
    hls::stream<bool> bitendofstream("bitEndOfStream");
    hls::stream<ap_uint<c_parallelBit> > litStream("litStream");
    hls::stream<ap_uint<9> > litLenStream("litLenStream");
    hls::stream<ap_uint<9> > matchLenStream("matchLenStream");
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

#pragma HLS BIND_STORAGE variable = bitunpackstream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = bitunpackstream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = interLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lzProcOutStream type = RAM_2P impl = BRAM

#pragma HLS dataflow

    xf::compression::huffmanDecoder<c_decoderType, c_byteGenLoopII>(inStream, bitunpackstream, bitendofstream,
                                                                    inputSize);

    xf::compression::details::lzProcessingUnit<ap_uint<9> >(
        bitunpackstream, bitendofstream, litLenStream, interLenStream, matchLenStream, offsetStream, lzProcOutStream);

    xf::compression::details::lzLiteralUpsizer<PARALLEL_BYTES>(lzProcOutStream, interLenStream, litStream);

    xf::compression::lzMultiByteDecompress<PARALLEL_BYTES, HISTORY_SIZE, ap_uint<9> >(
        litLenStream, litStream, offsetStream, matchLenStream, outStream, outStreamEoS, outStreamSize);
}

} // namespace details

template <int DECODER, int STREAM_WIDTH, int HISTORY_SIZE, int LOW_OFFSET>
void inflate(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& inaxistream,
             hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& outaxistream,
             hls::stream<ap_axiu<64, 0, 0, 0> >& sizestreamd,
             uint32_t input_size) {
    hls::stream<ap_uint<STREAM_WIDTH> > inhlsstream("inputStream");
    hls::stream<ap_uint<16> > outdownstream("outDownStream");
    hls::stream<ap_uint<8> > uncompoutstream("unCompOutStream");
    hls::stream<bool> byte_eos("byteEndOfStream");

    hls::stream<ap_uint<STREAM_WIDTH> > outhlsstream("outputStream");
    hls::stream<bool> outhlsstream_eos("outputStreamSize");

#pragma HLS STREAM variable = inhlsstream depth = 32
#pragma HLS STREAM variable = outdownstream depth = 256

#pragma HLS STREAM variable = uncompoutstream depth = 256
#pragma HLS STREAM variable = byte_eos depth = 32
#pragma HLS STREAM variable = outhlsstream depth = 32
#pragma HLS STREAM variable = outhlsstream_eos depth = 32
    hls::stream<uint64_t> outsize_val("outsize_val");

#pragma HLS dataflow

    details::kStreamReadZlibDecomp<STREAM_WIDTH>(inaxistream, inhlsstream, input_size);
    details::streamDownsizer<uint32_t, STREAM_WIDTH, 16>(inhlsstream, outdownstream, input_size);

    details::inflateCore<DECODER, HISTORY_SIZE, LOW_OFFSET>(outdownstream, uncompoutstream, byte_eos, outsize_val,
                                                            input_size);

    details::upsizerEos<8, STREAM_WIDTH>(uncompoutstream, byte_eos, outhlsstream, outhlsstream_eos);
    details::kStreamWriteZlibDecomp<STREAM_WIDTH>(outaxistream, sizestreamd, outhlsstream, outhlsstream_eos,
                                                  outsize_val);
}

template <int DECODER, int PARALLEL_BYTES, int HUFF_LOOP_II = 1, int HISTORY_SIZE = (32 * 1024)>
void inflateMultiByte(hls::stream<ap_axiu<16, 0, 0, 0> >& inaxistream,
                      hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& outaxistream,
                      hls::stream<ap_axiu<64, 0, 0, 0> >& sizestreamd,
                      uint32_t inputSize) {
    // HUFF_LOOP_II=1 gives better latency but FMax may drop
    // HUFF_LOOP_II=2 gives High FMax but latency will increase
    const int c_byteGenLoopII = HUFF_LOOP_II;
    const int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<16> > inStream("inStream");
    hls::stream<ap_uint<c_parallelBit> > outStream("outStream");
    hls::stream<bool> outStreamEos("outStreamEos");
    hls::stream<uint64_t> outSizeStream("outSizeStream");

#pragma HLS STREAM variable = inStream depth = 32
#pragma HLS STREAM variable = outStream depth = 32
#pragma HLS STREAM variable = outStreamEos depth = 32
#pragma HLS STREAM variable = outSizeStream depth = 3

#pragma HLS BIND_STORAGE variable = inStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStreamEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outSizeStream type = FIFO impl = SRL

#pragma HLS dataflow
    details::kStreamReadZlibDecomp<16>(inaxistream, inStream, inputSize);

    details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, c_byteGenLoopII, HISTORY_SIZE>(
        inStream, outStream, outStreamEos, outSizeStream, inputSize);

    details::kStreamWriteZlibDecomp<c_parallelBit>(outaxistream, sizestreamd, outStream, outStreamEos, outSizeStream);
}

template <int DECODER,
          int PARALLEL_BYTES,
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
    hls::stream<uint64_t> outSizeStream("outSizeStream");

#pragma HLS STREAM variable = inStream
#pragma HLS STREAM variable = outStream
#pragma HLS STREAM variable = outStreamEos
#pragma HLS DATAFLOW
    details::mm2Stream<c_krnlInWidth>(in, inStream, inputSize);
    details::inflateMultiByteCore<DECODER, PARALLEL_BYTES>(inStream, outStream, outStreamEos, outSizeStream, inputSize);
    details::stream2MM<c_krnlOutWidth>(outStream, outStreamEos, outSizeStream, out, outSize);
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_INFLATE_HPP_
