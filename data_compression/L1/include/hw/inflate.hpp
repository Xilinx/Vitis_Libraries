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
#include "checksum_wrapper.hpp"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>

namespace xf {
namespace compression {

namespace details {

template <int PARALLEL_BYTES>
void lzLiteralUpsizer(hls::stream<ap_uint<10> >& inStream, hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream) {
    const uint8_t c_parallelBit = PARALLEL_BYTES * 8;
    const uint8_t c_maxLitLen = 128;
    ap_uint<c_parallelBit> outBuffer;
    ap_uint<4> idx = 0;

    ap_uint<2> status = 0;
    ap_uint<10> val;
    bool done = false;
lzliteralUpsizer:
    while (status != 2) {
#pragma HLS PIPELINE II = 1
        status = 0;
        val = inStream.read();
        status = val.range(1, 0);
        outBuffer.range((idx + 1) * 8 - 1, idx * 8) = val.range(9, 2);
        idx++;

        if ((status & 1) || (idx == 8)) {
            if (status != 3) {
                litStream << outBuffer;
            }
            idx = 0;
        }
    }
    if (idx > 1) {
        litStream << outBuffer;
        idx = 0;
    }
}

template <int PARALLEL_BYTES>
void lzLiteralUpsizerLL(hls::stream<ap_uint<10> >& inStream, hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream) {
    const uint8_t c_parallelBit = PARALLEL_BYTES * 8;
    const uint8_t c_maxLitLen = 128;
    ap_uint<c_parallelBit> outBuffer;
    ap_uint<4> idx = 0;

    ap_uint<2> status = 0;
    ap_uint<10> val;
    bool done = false;
lzliteralUpsizer:
    while (status != 2) {
#pragma HLS PIPELINE II = 1
        status = 0;
        val = inStream.read();
        status = val.range(1, 0);
        outBuffer.range((idx + 1) * 8 - 1, idx * 8) = val.range(9, 2);
        idx++;

        if ((status & 1) || (idx == PARALLEL_BYTES)) {
            if (idx > 1) {
                litStream << outBuffer;
            }
            idx = 0;
        }
    }
    if (idx > 1) {
        litStream << outBuffer;
        idx = 0;
    }
}

template <class SIZE_DT = uint8_t>
void lzProcessingUnit(hls::stream<ap_uint<17> >& inStream,
                      hls::stream<SIZE_DT>& litLenStream,
                      hls::stream<SIZE_DT>& matchLenStream,
                      hls::stream<ap_uint<16> >& offsetStream,
                      hls::stream<ap_uint<10> >& outStream) {
    ap_uint<17> inValue, nextValue;
    const int c_maxLitLen = 128;
    uint16_t offset = 0;
    uint16_t matchLen = 0;
    uint8_t litLen = 0;
    uint8_t outLitLen = 0;
    ap_uint<10> lit = 0;

    nextValue = inStream.read();
    bool eosFlag = nextValue.range(0, 0);
    bool lastLiteral = false;
    bool isLiteral = true;
lzProcessing:
    for (; eosFlag == false;) {
#pragma HLS PIPELINE II = 1
        inValue = nextValue;
        nextValue = inStream.read();
        eosFlag = nextValue.range(0, 0);

        bool outFlag, outStreamFlag;
        if (inValue.range(16, 9) == 0xFF && isLiteral) {
            outStreamFlag = true;
            outLitLen = litLen + 1;
            if (litLen == c_maxLitLen - 1) {
                outFlag = true;
                matchLen = 0;
                offset = 1; // dummy value
                litLen = 0;
            } else {
                outFlag = false;
                litLen++;
            }
        } else {
            if (isLiteral) {
                matchLen = inValue.range(16, 1);
                isLiteral = false;
                outFlag = false;
                outStreamFlag = false;
            } else {
                offset = inValue.range(16, 1);
                isLiteral = true;
                outFlag = true;
                outLitLen = litLen;
                litLen = 0;
                outStreamFlag = false;
            }
        }
        if (outStreamFlag) {
            lit.range(9, 2) = inValue.range(8, 1);
            if (nextValue.range(16, 9) == 0xFF) {
                lit.range(1, 0) = 0;
            } else {
                lit.range(1, 0) = 1;
            }
            lastLiteral = true;
            outStream << lit;
        } else if (lastLiteral) {
            outStream << 3;
            lastLiteral = false;
        }

        if (outFlag) {
            litLenStream << outLitLen;
            offsetStream << offset;
            matchLenStream << matchLen;
        }
    }

    if (litLen) {
        litLenStream << litLen;
        offsetStream << 0;
        matchLenStream << 0;
    }

    // Terminate condition
    outStream << 2;
    offsetStream << 0;
    matchLenStream << 0;
    litLenStream << 0;
}

template <class SIZE_DT = uint8_t, int PARALLEL_BYTES = 8, int OWIDTH = 16>
void lzPreProcessingUnitLL(hls::stream<SIZE_DT>& inLitLen,
                           hls::stream<SIZE_DT>& inMatchLen,
                           hls::stream<ap_uint<OWIDTH> >& inOffset,
                           hls::stream<ap_uint<11 + OWIDTH> >& outInfo) {
    SIZE_DT litlen = inLitLen.read();
    SIZE_DT matchlen = inMatchLen.read();
    ap_uint<OWIDTH> offset = inOffset.read();
    ap_uint<OWIDTH> litCount = litlen;

    ap_uint<4> l_litlen = 0;
    ap_uint<4> l_matchlen = 0;
    ap_uint<3> l_stateinfo = 0;
    ap_uint<OWIDTH> l_matchloc = litCount - offset;
    ap_uint<11 + OWIDTH> outVal = 0; // 0-15 Match Loc, 16-19 Match Len, 20-23 Lit length, 24-26 State Info
    bool done = false;
    bool read = false;
    bool fdone = false;

    if (litlen == 0) {
        outVal.range(OWIDTH - 1, 0) = 0;
        outVal.range(OWIDTH + 3, OWIDTH) = matchlen;
        outVal.range(OWIDTH + 7, OWIDTH + 4) = litlen;
        outVal.range(OWIDTH + 10, OWIDTH + 8) = 6;
        outInfo << outVal;
        done = true;
        fdone = false;
    }
    while (!done) {
#pragma HLS PIPELINE II = 1
        if (litlen) {
            SIZE_DT val = (litlen > PARALLEL_BYTES) ? (SIZE_DT)PARALLEL_BYTES : litlen;
            litlen -= val;
            l_litlen = val;
            l_matchlen = 0;
            l_stateinfo = 0;
            l_matchloc = 0;
            read = (matchlen || litlen) ? false : true;
        } else {
            l_matchlen = (offset > PARALLEL_BYTES)
                             ? ((matchlen > PARALLEL_BYTES) ? (SIZE_DT)PARALLEL_BYTES : (SIZE_DT)matchlen)
                             : (matchlen > offset) ? (SIZE_DT)offset : (SIZE_DT)matchlen;
            if (offset < 6 * PARALLEL_BYTES) {
                l_stateinfo.range(0, 0) = 1;
                l_stateinfo.range(2, 1) = 1;
            } else {
                l_stateinfo.range(0, 0) = 0;
                l_stateinfo.range(2, 1) = 1;
            }
            l_matchloc = litCount - offset;
            if (offset < PARALLEL_BYTES) {
                offset = offset << 1;
            }
            l_litlen = 0;
            litCount += l_matchlen;
            matchlen -= l_matchlen;
            litlen = 0;
            read = matchlen ? false : true;
        }
        outVal.range(OWIDTH - 1, 0) = l_matchloc;
        outVal.range(OWIDTH + 3, OWIDTH) = l_matchlen;
        outVal.range(OWIDTH + 7, OWIDTH + 4) = l_litlen;
        outVal.range(OWIDTH + 10, OWIDTH + 8) = l_stateinfo;
        outInfo << outVal;

        if (read) {
            litlen = inLitLen.read();
            matchlen = inMatchLen.read();
            offset = inOffset.read();
            litCount += litlen;
            if (litlen == 0 && matchlen == 0) {
                done = true;
                fdone = true;
            }
        }
    }
    if (fdone) {
        outVal.range(OWIDTH - 1, 0) = l_matchloc;
        outVal.range(OWIDTH + 3, OWIDTH) = matchlen;
        outVal.range(OWIDTH + 7, OWIDTH + 4) = litlen;
        outVal.range(OWIDTH + 10, OWIDTH + 8) = 6;
        outInfo << outVal;
    }
}

template <class SIZE_DT = uint8_t>
void lzProcessingUnitLL(hls::stream<ap_uint<16> >& inStream,
                        hls::stream<SIZE_DT>& litLenStream,
                        hls::stream<SIZE_DT>& matchLenStream,
                        hls::stream<ap_uint<16> >& offsetStream,
                        hls::stream<ap_uint<10> >& outStream) {
    ap_uint<16> inValue, nextValue;
    const int c_maxLitLen = 128;
    uint16_t offset = 0;
    uint16_t matchLen = 0;
    uint8_t litLen = 0;
    uint8_t outLitLen = 0;
    ap_uint<10> lit = 0;
    const uint16_t lbase[32] = {0,  3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27,
                                31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};

    const uint16_t dbase[32] = {1,    2,    3,    4,    5,    7,     9,     13,    17,  25,   33,
                                49,   65,   97,   129,  193,  257,   385,   513,   769, 1025, 1537,
                                2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0};
    nextValue = inStream.read();
    bool eosFlag = (nextValue == 0xFFFF) ? true : false;
    bool lastLiteral = false;
    bool isLitLength = true;
    bool isExtra = false;
    bool dummyValue = false;
lzProcessing:
    for (; eosFlag == false;) {
#pragma HLS PIPELINE II = 1
        inValue = nextValue;
        nextValue = inStream.read();
        eosFlag = (nextValue == 0xFFFF);

        bool outFlag, outStreamFlag;
        if ((inValue.range(15, 8) == 0xFE) || (inValue.range(15, 8) == 0xFD)) {
            // ignore invalid byte
            outFlag = false;
            outStreamFlag = false;
        } else if (inValue.range(15, 8) == 0xF0) {
            outStreamFlag = true;
            outLitLen = litLen + 1;
            if (litLen == c_maxLitLen - 1) {
                outFlag = true;
                matchLen = 0;
                offset = 1; // dummy value
                litLen = 0;
            } else {
                outFlag = false;
                matchLen = 0;
                offset = 1; // dummy value
                litLen++;
            }
        } else if (isExtra && isLitLength) { // matchLen Extra
            matchLen += inValue.range(15, 0);
            isExtra = false;
            isLitLength = false;
            outStreamFlag = true;

        } else if (isExtra && !isLitLength) { // offset Extra
            offset += inValue.range(15, 0);
            isExtra = false;
            isLitLength = true;
            outFlag = true;
            outStreamFlag = true;
        } else if (isLitLength) {
            auto val = inValue.range(4, 0);
            matchLen = lbase[val];
            if (val < 9) {
                isExtra = false;
                isLitLength = false;
            } else {
                isExtra = true;
                isLitLength = true;
            }
            outFlag = false;
            outStreamFlag = true;
            dummyValue = true;
        } else {
            auto val = inValue.range(4, 0);
            offset = dbase[val];
            if (val < 4) {
                isExtra = false;
                isLitLength = true;
                outFlag = true;

            } else {
                isExtra = true;
                isLitLength = false;
                outFlag = false;
            }

            outLitLen = litLen;
            litLen = 0;
            outStreamFlag = true;
        }

        if (outStreamFlag) {
            lit.range(9, 2) = inValue.range(7, 0);
            if ((inValue.range(15, 8) == 0xF0)) {
                lit.range(1, 0) = 0;
            } else {
                lit.range(1, 0) = 1;
            }
            outStream << lit;
        }

        if (outFlag) {
            litLenStream << outLitLen;
            offsetStream << offset;
            matchLenStream << matchLen;
        }
    }

    if (litLen) {
        litLenStream << litLen;
        offsetStream << 0;
        matchLenStream << 0;
        outStream << 3;
    }

    // Terminate condition
    outStream << 2;
    offsetStream << 0;
    matchLenStream << 0;
    litLenStream << 0;
}

template <int STREAM_WIDTH>
void kStreamReadZlibDecomp(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& in,
                           hls::stream<ap_uint<STREAM_WIDTH> >& out,
                           hls::stream<bool>& outEos) {
    /**
     * @brief kStreamReadZlibDecomp Read 16-bit wide data from internal streams output by compression modules
     *                              and write to output axi stream.
     *
     * @param inKStream     input kernel stream
     * @param readStream    internal stream to be read for processing
     * @param input_size    input data size
     *
     */
    bool last = false;
    while (last == false) {
#pragma HLS PIPELINE II = 1
        ap_axiu<STREAM_WIDTH, 0, 0, 0> tmp = in.read();
        out << tmp.data;
        last = tmp.last;
        outEos << 0;
    }
    out << 0;
    outEos << 1; // Terminate condition
}

template <int NUM_CORE>
void axi2HlsDistributor(hls::stream<ap_axiu<64, 0, 0, 0> >& in, hls::stream<ap_uint<72> > out[NUM_CORE]) {
    for (auto i = 0; i < NUM_CORE; i++) {
        bool last = false;
        ap_axiu<64, 0, 0, 0> tmp;
        ap_uint<8> kp = 0;
        ap_uint<8> cntr = 0;
        ap_uint<72> tmpVal = 0;
        while (last == false) {
#pragma HLS PIPELINE II = 1
            tmp = in.read();
            tmpVal.range(71, 8) = tmp.data;
            kp = tmp.keep;
            tmpVal.range(7, 0) = tmp.keep;
            if (kp == 0xFF)
                tmpVal.range(7, 0) = 8;
            else
                break;
            out[i] << tmpVal;
            last = tmp.last;
        }
        if (kp != 0xFF) {
            while (kp) {
                cntr += (kp & 0x1);
                kp >>= 1;
            }
            tmpVal.range(7, 0) = cntr;
            out[i] << tmpVal;
        }
        out[i] << 0; // Terminate condition
    }
}

template <int STREAM_WIDTH, int NUM_CORE>
void hls2AxiMerger(hls::stream<ap_axiu<64, 0, 0, 0> >& outKStream, hls::stream<ap_uint<72> > outDataStream[NUM_CORE]) {
    for (auto i = 0; i < NUM_CORE; i++) {
        ap_uint<8> strb = 0;
        ap_uint<64> data;
        ap_uint<72> tmp;
        ap_axiu<64, 0, 0, 0> t1;

        tmp = outDataStream[i].read();

        strb = tmp.range(7, 0);
        t1.data = tmp.range(71, 8);
        t1.strb = strb;
        t1.keep = strb;
        t1.last = 0;
        if (strb == 0) {
            t1.last = 1;
            outKStream << t1;
        }
        while (strb != 0) {
#pragma HLS PIPELINE II = 1
            tmp = outDataStream[i].read();

            strb = tmp.range(7, 0);
            if (strb == 0) {
                t1.last = 1;
            }
            outKStream << t1;

            t1.data = tmp.range(71, 8);
            t1.strb = strb;
            t1.keep = strb;
            t1.last = 0;
        }
    }
}

template <int STREAM_WIDTH>
void kStreamWriteZlibDecomp(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& outKStream,
                            hls::stream<ap_uint<STREAM_WIDTH + (STREAM_WIDTH / 8)> >& outDataStream) {
    /**
     * @brief kStreamWriteZlibDecomp Read 16-bit wide data from internal streams output by compression modules
     *                                and write to output axi stream.
     *
     * @param outKStream    output kernel stream
     * @param outDataStream output data stream from internal modules
     *
     */
    ap_uint<STREAM_WIDTH / 8> strb = 0;
    ap_uint<STREAM_WIDTH> data;
    ap_uint<STREAM_WIDTH + (STREAM_WIDTH / 8)> tmp;
    ap_axiu<STREAM_WIDTH, 0, 0, 0> t1;

    tmp = outDataStream.read();
    strb = tmp.range((STREAM_WIDTH / 8) - 1, 0);
    t1.data = tmp.range(STREAM_WIDTH + (STREAM_WIDTH / 8) - 1, STREAM_WIDTH / 8);
    t1.strb = strb;
    t1.keep = strb;
    t1.last = 0;
    if (strb == 0) {
        t1.last = 1;
        outKStream << t1;
    }
    while (strb != 0) {
#pragma HLS PIPELINE II = 1
        tmp = outDataStream.read();
        strb = tmp.range((STREAM_WIDTH / 8) - 1, 0);
        if (strb == 0) {
            t1.last = 1;
        }
        outKStream << t1;
        t1.data = tmp.range(STREAM_WIDTH + (STREAM_WIDTH / 8) - 1, STREAM_WIDTH / 8);
        t1.strb = strb;
        t1.keep = strb;
        t1.last = 0;
    }
}

template <int STREAM_WIDTH, int TUSER_WIDTH>
void hls2AXIWithTUSER(hls::stream<ap_axiu<STREAM_WIDTH, TUSER_WIDTH, 0, 0> >& outAxi,
                      hls::stream<ap_uint<STREAM_WIDTH + (STREAM_WIDTH / 8)> >& inData,
                      hls::stream<bool>& inError) {
    ap_uint<STREAM_WIDTH / 8> strb = 0;
    ap_uint<STREAM_WIDTH> data;
    ap_uint<STREAM_WIDTH + (STREAM_WIDTH / 8)> tmp;
    ap_axiu<STREAM_WIDTH, TUSER_WIDTH, 0, 0> t1;
    ap_uint<TUSER_WIDTH - 1> fileSize = 0;

    tmp = inData.read();
    strb = tmp.range((STREAM_WIDTH / 8) - 1, 0);
    t1.data = tmp.range(STREAM_WIDTH + (STREAM_WIDTH / 8) - 1, STREAM_WIDTH / 8);
    t1.strb = strb;
    t1.keep = strb;
    t1.last = 0;
    t1.user = 0;
    if (strb == 0) {
        t1.last = 1;
        outAxi << t1;
    }
AXI2HLS:
    while (strb != 0) {
#pragma HLS PIPELINE II = 1
    FILESIZE:
        for (ap_uint<4> i = 0; i < (STREAM_WIDTH / 8); i++) {
#pragma HLS UNROLL
            fileSize += (strb & 0x1);
            strb >>= 1;
        }
        tmp = inData.read();
        strb = tmp.range((STREAM_WIDTH / 8) - 1, 0);
        if (strb == 0) {
            t1.last = 1;
            t1.user.range(TUSER_WIDTH - 1, 1) = fileSize;
            t1.user.range(0, 0) = inError.read();
        }
        outAxi << t1;
        t1.data = tmp.range(STREAM_WIDTH + (STREAM_WIDTH / 8) - 1, STREAM_WIDTH / 8);
        t1.strb = strb;
        t1.keep = strb;
        t1.last = 0;
    }
}

/**
 * @brief Splits input into data and strb
 *
 * @tparam DATA_WIDTH data width of data stream
 * @param input combined input of data and strb
 * @param outData output data
 * @param outStrb output strb
 */
template <int DATA_WIDTH>
void dataStrbSplitter(hls::stream<ap_uint<(DATA_WIDTH * 8) + DATA_WIDTH> >& input,
                      hls::stream<ap_uint<DATA_WIDTH * 8> >& outData,
                      hls::stream<ap_uint<5> >& outStrb) {
    ap_uint<DATA_WIDTH> strb = 0;
splitter:
    while (1) {
#pragma HLS PIPELINE II = 1
        auto inVal = input.read();
        auto data = inVal.range((DATA_WIDTH * 8) + DATA_WIDTH - 1, DATA_WIDTH);
        strb = inVal.range(DATA_WIDTH - 1, 0);
        auto strbVal = strb;
        if (strb == 0) break;
        outData << data;

        ap_uint<5> size = 0;
        for (ap_uint<4> i = 0; i < DATA_WIDTH; i++) {
#pragma HLS UNROLL
            size += (strbVal & 0x1);
            strbVal >>= 1;
        }
        outStrb << size;
    }
    if (strb == 0) outStrb << 0;
}
/**
 * @brief Splits Data Stream into multiple
 *
 * @tparam DATA_WIDTH
 * @param input input stream
 * @param output1 output streams
 * @param output2 output streams
 */
template <int DATA_WIDTH>
void streamDistributor(hls::stream<ap_uint<(DATA_WIDTH * 8) + DATA_WIDTH> >& input,
                       hls::stream<ap_uint<(DATA_WIDTH * 8) + DATA_WIDTH> >& output1,
                       hls::stream<ap_uint<(DATA_WIDTH * 8) + DATA_WIDTH> >& output2) {
    ap_uint<DATA_WIDTH> strb = 0;
    do {
#pragma HLS PIPELINE II = 1
        auto inVal = input.read();
        strb = inVal.range(DATA_WIDTH - 1, 0);
        output1 << inVal;
        output2 << inVal;
    } while (strb != 0);
}

/**
 * @brief compare two checksums and generate 0 if match and 1 otherwise
 *
 * @param checkSum1 1st checksum input
 * @param checkSum2 2nd checksum input
 * @param output error output
 */
void chckSumComparator(hls::stream<ap_uint<32> >& checkSum1,
                       hls::stream<ap_uint<32> >& checkSum2,
                       hls::stream<bool>& output) {
    auto chk1 = checkSum1.read();
    auto chk2 = checkSum2.read();
    output << (chk1 != chk2);
}

template <int DECODER, int PARALLEL_BYTES, int FILE_FORMAT, bool LOW_LATENCY = false, int HISTORY_SIZE = (32 * 1024)>
void inflateMultiByteCore(hls::stream<ap_uint<16> >& inStream,
                          hls::stream<bool>& inEos,
                          hls::stream<ap_uint<(PARALLEL_BYTES * 8) + PARALLEL_BYTES> >& outStream) {
    const int c_parallelBit = PARALLEL_BYTES * 8;
    const eHuffmanType c_decoderType = (eHuffmanType)DECODER;
    const FileFormat c_fileformat = (FileFormat)FILE_FORMAT;

    hls::stream<ap_uint<17> > bitunpackstream("bitUnPackStream");
    hls::stream<ap_uint<16> > bitunpackstreamLL("bitUnPackStreamLL");
    hls::stream<ap_uint<c_parallelBit> > litStream("litStream");
    hls::stream<ap_uint<9> > matchLenStream("matchLenStream");
    hls::stream<ap_uint<9> > litLenStream("litLenStream");
    hls::stream<ap_uint<16> > offsetStream("offsetStream");
    hls::stream<ap_uint<10> > lzProcOutStream("lzProcOutStream");
    hls::stream<ap_uint<27> > infoStream("infoStream");

#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = lzProcOutStream depth = 16
#pragma HLS STREAM variable = bitunpackstream depth = 1024
#pragma HLS STREAM variable = bitunpackstreamLL depth = 256
#pragma HLS STREAM variable = litLenStream depth = 16
#pragma HLS STREAM variable = matchLenStream depth = 16
#pragma HLS STREAM variable = offsetStream depth = 16
#pragma HLS STREAM variable = infoStream depth = 4

#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = bitunpackstream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = bitunpackstreamLL type = fifo impl = lutram
#pragma HLS BIND_STORAGE variable = lzProcOutStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL

#pragma HLS dataflow

    if (LOW_LATENCY) {
        xf::compression::huffmanDecoderLL<c_decoderType, c_fileformat>(inStream, inEos, bitunpackstreamLL);

        xf::compression::details::lzProcessingUnitLL<ap_uint<9> >(bitunpackstreamLL, litLenStream, matchLenStream,
                                                                  offsetStream, lzProcOutStream);

        xf::compression::details::lzPreProcessingUnitLL<ap_uint<9>, PARALLEL_BYTES, 16>(litLenStream, matchLenStream,
                                                                                        offsetStream, infoStream);
        xf::compression::details::lzLiteralUpsizerLL<PARALLEL_BYTES>(lzProcOutStream, litStream);
        xf::compression::lzMultiByteDecompressLL<PARALLEL_BYTES, HISTORY_SIZE, 16, ap_uint<9>, ap_uint<16> >(
            litStream, infoStream, outStream);
    } else {
        xf::compression::huffmanDecoder<c_decoderType>(inStream, inEos, bitunpackstream);

        xf::compression::details::lzProcessingUnit<ap_uint<9> >(bitunpackstream, litLenStream, matchLenStream,
                                                                offsetStream, lzProcOutStream);

        xf::compression::details::lzLiteralUpsizer<PARALLEL_BYTES>(lzProcOutStream, litStream);
        xf::compression::lzMultiByteDecompress<PARALLEL_BYTES, HISTORY_SIZE, ap_uint<9> >(
            litLenStream, litStream, offsetStream, matchLenStream, outStream);
    }
}

template <int DECODER, int PARALLEL_BYTES, int FILE_FORMAT, int HISTORY_SIZE = (8 * 1024)>
void inflateWithChkSum(hls::stream<ap_uint<16> >& inStream,
                       hls::stream<bool>& inEos,
                       hls::stream<ap_uint<(PARALLEL_BYTES * 8) + PARALLEL_BYTES> >& outStream,
                       hls::stream<bool>& errorStrm) {
    const int c_parallelBit = PARALLEL_BYTES * 8;
    const eHuffmanType c_decoderType = (eHuffmanType)DECODER;
    const FileFormat c_fileformat = (FileFormat)FILE_FORMAT;

    hls::stream<ap_uint<16> > bitunpackstreamLL("bitUnPackStreamLL");
    hls::stream<ap_uint<c_parallelBit> > litStream("litStream");
    hls::stream<ap_uint<9> > matchLenStream("matchLenStream");
    hls::stream<ap_uint<9> > litLenStream("litLenStream");
    hls::stream<ap_uint<16> > offsetStream("offsetStream");
    hls::stream<ap_uint<10> > lzProcOutStream("lzProcOutStream");
    hls::stream<ap_uint<27> > infoStream("infoStream");
    hls::stream<ap_uint<32> > chckSum[2];
    hls::stream<ap_uint<(PARALLEL_BYTES * 8) + PARALLEL_BYTES> > lzOut("lzOut");
    hls::stream<ap_uint<(PARALLEL_BYTES * 8) + PARALLEL_BYTES> > lzDistOut("lzDistOut");
    hls::stream<ap_uint<(PARALLEL_BYTES * 8)> > lzData("lzData");
    hls::stream<ap_uint<5> > lzStrb("lzStrb");

#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = lzProcOutStream depth = 16
#pragma HLS STREAM variable = bitunpackstreamLL depth = 256
#pragma HLS STREAM variable = litLenStream depth = 64
#pragma HLS STREAM variable = matchLenStream depth = 64
#pragma HLS STREAM variable = offsetStream depth = 64
#pragma HLS STREAM variable = infoStream depth = 4
#pragma HLS STREAM variable = chckSum depth = 4
#pragma HLS STREAM variable = lzOut depth = 4
#pragma HLS STREAM variable = lzDistOut depth = 4
#pragma HLS STREAM variable = lzData depth = 4
#pragma HLS STREAM variable = lzStrb depth = 4
#pragma HLS STREAM variable = chckSum depth = 4

#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = bitunpackstreamLL type = fifo impl = lutram
#pragma HLS BIND_STORAGE variable = lzProcOutStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchLenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL

#pragma HLS dataflow

    xf::compression::huffmanDecoderLL<c_decoderType, c_fileformat>(inStream, inEos, bitunpackstreamLL, chckSum[0]);

    xf::compression::details::lzProcessingUnitLL<ap_uint<9> >(bitunpackstreamLL, litLenStream, matchLenStream,
                                                              offsetStream, lzProcOutStream);

    xf::compression::details::lzPreProcessingUnitLL<ap_uint<9>, PARALLEL_BYTES, 16>(litLenStream, matchLenStream,
                                                                                    offsetStream, infoStream);
    xf::compression::details::lzLiteralUpsizerLL<PARALLEL_BYTES>(lzProcOutStream, litStream);
    xf::compression::lzMultiByteDecompressLL<PARALLEL_BYTES, HISTORY_SIZE, 16, ap_uint<9>, ap_uint<16> >(
        litStream, infoStream, lzOut);
    xf::compression::details::streamDistributor<PARALLEL_BYTES>(lzOut, lzDistOut, outStream);
    xf::compression::details::dataStrbSplitter<PARALLEL_BYTES>(lzDistOut, lzData, lzStrb);
    xf::compression::details::adler32<PARALLEL_BYTES>(lzData, lzStrb, chckSum[1]);
    xf::compression::details::chckSumComparator(chckSum[0], chckSum[1], errorStrm);
}

} // namespace details

template <int NUM_CORE,
          int DECODER,
          int PARALLEL_BYTES,
          int FILE_FORMAT,
          bool LOW_LATENCY = false,
          int HISTORY_SIZE = (32 * 1024)>
void inflateMultiCores(hls::stream<ap_axiu<64, 0, 0, 0> >& inaxistream,
                       hls::stream<ap_axiu<64, 0, 0, 0> >& outaxistream) {
    constexpr int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<72> > axi2HlsStrm[NUM_CORE];
    hls::stream<ap_uint<72> > inflateOut[NUM_CORE];
    hls::stream<ap_uint<16> > hlsDownStrm[NUM_CORE];
    hls::stream<bool> hlsEos[NUM_CORE];

#pragma HLS STREAM variable = axi2HlsStrm depth = 4096
#pragma HLS STREAM variable = inflateOut depth = 4096
#pragma HLS STREAM variable = hlsDownStrm depth = 32
#pragma HLS STREAM variable = hlsEos depth = 32

#pragma HLS BIND_STORAGE variable = axi2HlsStrm type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = inflateOut type = fifo impl = URAM

#pragma HLS dataflow disable_start_propagation
    details::axi2HlsDistributor<NUM_CORE>(inaxistream, axi2HlsStrm);

    for (auto i = 0; i < NUM_CORE; i++) {
#pragma HLS UNROLL
        details::bufferDownsizer<NUM_CORE, PARALLEL_BYTES>(axi2HlsStrm[i], hlsDownStrm[i], hlsEos[i]);
        details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, FILE_FORMAT, LOW_LATENCY, HISTORY_SIZE>(
            hlsDownStrm[i], hlsEos[i], inflateOut[i]);
    }

    details::hls2AxiMerger<c_parallelBit, NUM_CORE>(outaxistream, inflateOut);
}

template <int DECODER, int PARALLEL_BYTES, int FILE_FORMAT, bool LOW_LATENCY = false, int HISTORY_SIZE = (32 * 1024)>
void inflateMultiByte(hls::stream<ap_axiu<16, 0, 0, 0> >& inaxistream,
                      hls::stream<ap_axiu<PARALLEL_BYTES * 8, 0, 0, 0> >& outaxistream) {
    const int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<16> > axi2HlsStrm("axi2HlsStrm");
    hls::stream<bool> axi2HlsEos("axi2HlsEos");
    hls::stream<ap_uint<c_parallelBit + PARALLEL_BYTES> > inflateOut("inflateOut");
    hls::stream<uint64_t> outSizeStream("outSizeStream");

#pragma HLS STREAM variable = axi2HlsStrm depth = 32
#pragma HLS STREAM variable = axi2HlsEos depth = 32
#pragma HLS STREAM variable = inflateOut depth = 32

#pragma HLS BIND_STORAGE variable = axi2HlsStrm type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = axi2HlsEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inflateOut type = fifo impl = SRL

#pragma HLS dataflow
    details::kStreamReadZlibDecomp<16>(inaxistream, axi2HlsStrm, axi2HlsEos);

    details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, FILE_FORMAT, LOW_LATENCY, HISTORY_SIZE>(
        axi2HlsStrm, axi2HlsEos, inflateOut);

    details::kStreamWriteZlibDecomp<c_parallelBit>(outaxistream, inflateOut);
}

template <int DECODER, int PARALLEL_BYTES, int FILE_FORMAT, int HISTORY_SIZE = (32 * 1024), int TUSER_WIDTH = 32>
void inflate(hls::stream<ap_axiu<16, 0, 0, 0> >& inaxistream,
             hls::stream<ap_axiu<PARALLEL_BYTES * 8, TUSER_WIDTH, 0, 0> >& outaxistream) {
    const int c_parallelBit = PARALLEL_BYTES * 8;

    hls::stream<ap_uint<16> > axi2HlsStrm("axi2HlsStrm");
    hls::stream<bool> axi2HlsEos("axi2HlsEos");
    hls::stream<ap_uint<c_parallelBit + PARALLEL_BYTES> > inflateOut("inflateOut");
    hls::stream<uint64_t> outSizeStream("outSizeStream");
    hls::stream<bool> error("error");

#pragma HLS STREAM variable = axi2HlsStrm depth = 32
#pragma HLS STREAM variable = axi2HlsEos depth = 32
#pragma HLS STREAM variable = inflateOut depth = 32
#pragma HLS STREAM variable = error depth = 4

#pragma HLS BIND_STORAGE variable = axi2HlsStrm type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = axi2HlsEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inflateOut type = fifo impl = SRL

#pragma HLS dataflow
    details::kStreamReadZlibDecomp<16>(inaxistream, axi2HlsStrm, axi2HlsEos);

    details::inflateWithChkSum<DECODER, PARALLEL_BYTES, FILE_FORMAT, HISTORY_SIZE>(axi2HlsStrm, axi2HlsEos, inflateOut,
                                                                                   error);

    details::hls2AXIWithTUSER<c_parallelBit, TUSER_WIDTH>(outaxistream, inflateOut, error);
}

template <int GMEM_DATAWIDTH,
          int GMEM_BRST_SIZE,
          int DECODER,
          int PARALLEL_BYTES,
          int FILE_FORMAT,
          bool LOW_LATENCY = false,
          int HISTORY_SIZE = (32 * 1024)>
void inflateMultiByteMM(const ap_uint<GMEM_DATAWIDTH>* in,
                        ap_uint<GMEM_DATAWIDTH>* out,
                        uint32_t* encodedSize,
                        uint32_t inputSize) {
#pragma HLS dataflow
    constexpr int c_inBitWidth = 16;
    constexpr int c_burstDepth = 2 * GMEM_BRST_SIZE;

    // Internal Streams
    hls::stream<ap_uint<GMEM_DATAWIDTH> > mm2sStream;
    hls::stream<ap_uint<c_inBitWidth> > inInfateStream;
    hls::stream<bool> inInfateStreamEos;
    hls::stream<uint32_t> sizeStream;
    hls::stream<uint32_t> sizeStreamV;
    hls::stream<ap_uint<GMEM_DATAWIDTH + PARALLEL_BYTES> > outStream;

    // Initialize Size Stream
    uint32_t tmp = inputSize;
    sizeStreamV.write(tmp);

#pragma HLS STREAM variable = mm2sStream depth = c_burstDepth
#pragma HLS STREAM variable = inInfateStream depth = c_burstDepth
#pragma HLS STREAM variable = inInfateStreamEos depth = c_burstDepth
#pragma HLS STREAM variable = sizeStream depth = 4
#pragma HLS STREAM variable = sizeStreamV depth = 4
#pragma HLS STREAM variable = outStream depth = c_burstDepth

    // MM2S
    xf::compression::details::mm2sSimple<GMEM_DATAWIDTH, GMEM_BRST_SIZE>(in, mm2sStream, sizeStream, sizeStreamV);

    // Downsizer
    xf::compression::details::streamDownsizerEos<GMEM_DATAWIDTH, c_inBitWidth>(mm2sStream, sizeStream, inInfateStream,
                                                                               inInfateStreamEos);

    // Decompression
    xf::compression::details::inflateMultiByteCore<DECODER, PARALLEL_BYTES, FILE_FORMAT, LOW_LATENCY, HISTORY_SIZE>(
        inInfateStream, inInfateStreamEos, outStream);

    // S2MM
    xf::compression::details::s2mmAxi<GMEM_DATAWIDTH, GMEM_BRST_SIZE, PARALLEL_BYTES>(out, outStream, encodedSize);
}

} // namespace compression
} // namespace xf
#endif
