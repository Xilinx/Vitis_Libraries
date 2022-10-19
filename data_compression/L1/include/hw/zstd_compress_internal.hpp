/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
#ifndef _XFCOMPRESSION_ZSTD_COMPRESS_INTERNAL_HPP_
#define _XFCOMPRESSION_ZSTD_COMPRESS_INTERNAL_HPP_

/**
 * @file zstd_decompress.hpp
 * @brief Header for modules used in ZSTD compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include <ap_int.h>
#include <stdint.h>

#include "zstd_specs.hpp"
#include "compress_utils.hpp"
#include "lz_compress.hpp"
#include "lz_optional.hpp"
#include "stream_upsizer.hpp"

namespace xf {
namespace compression {
namespace details {

template <int SLAVES>
void streamEosDistributor(hls::stream<bool>& inStream, hls::stream<bool> outStream[SLAVES]) {
    do {
        bool i = inStream.read();
        for (int n = 0; n < SLAVES; n++) {
#pragma HLS UNROLL
            outStream[n] << i;
        }
        if (i == 1) break;
    } while (1);
}

template <int SLAVES>
void streamSizeDistributor(hls::stream<uint32_t>& inStream, hls::stream<uint32_t> outStream[SLAVES]) {
    do {
        uint32_t i = inStream.read();
        for (int n = 0; n < SLAVES; n++) {
#pragma HLS UNROLL
            outStream[n] << i;
        }
        if (i == 0) break;
    } while (1);
}

template <int MAX_FREQ_DWIDTH = 18, int MIN_MATCH = 3, class FREQ_DT = ap_uint<MAX_FREQ_DWIDTH> >
void zstdLz77DivideStream(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                          hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                          hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                          hls::stream<bool>& rleFlagStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                          hls::stream<FREQ_DT>& litCntStream) {
    // lz77 encoder states
    enum LZ77EncoderStates { WRITE_LITERAL, WRITE_OFFSET0, WRITE_OFFSET1 };
    IntVectorStream_dt<8, 1> outLitVal;
    DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> outSeqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outLitFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outSeqFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> metaVal;
    metaVal.strobe = 1;
    // frequency buffers
    FREQ_DT literal_freq[c_maxLitV + 1];
    FREQ_DT litlen_freq[c_maxCodeLL + 1];
    FREQ_DT matlen_freq[c_maxCodeML + 1];
    FREQ_DT offset_freq[c_maxCodeOF + 1];
#pragma HLS BIND_STORAGE variable = literal_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = litlen_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = matlen_freq type = ram_2p impl = lutram
#pragma HLS BIND_STORAGE variable = offset_freq type = ram_2p impl = lutram

    bool last_block = false;
    bool just_started = true;
    int blk_n = 0;

    while (!last_block) {
        // iterate over multiple blocks in a file
        ++blk_n;
        enum LZ77EncoderStates next_state = WRITE_LITERAL;
        FREQ_DT seqCnt = 0;

        { // Initialize frequencies memory
#pragma HLS LOOP_MERGE force
        lit_freq_init:
            for (uint16_t i = 0; i < c_maxLitV + 1; i++) {
                literal_freq[i] = 0;
            }
        ll_freq_init:
            for (uint16_t i = 0; i < c_maxCodeLL + 1; i++) {
                litlen_freq[i] = 0;
            }
        ml_freq_init:
            for (uint16_t i = 0; i < c_maxCodeML + 1; i++) {
                matlen_freq[i] = 0;
            }
        of_freq_init:
            for (uint16_t i = 0; i < c_maxCodeOF + 1; i++) {
                offset_freq[i] = 0;
            }
        }
        uint8_t tCh = 0;
        uint8_t tLen = 0;
        FREQ_DT litCount = 0;
        FREQ_DT litTotal = 0;
        // set output data to be of valid length
        outLitVal.strobe = 1;
        outSeqVal.strobe = 1;
        uint8_t cLit = 0;
        bool fv = true;
        bool isRLE = true;
    zstd_lz77_divide:
        while (true) {
#pragma HLS PIPELINE II = 1
#ifndef DISABLE_DEPENDENCE
#pragma HLS dependence variable = literal_freq inter false
#pragma HLS dependence variable = litlen_freq inter false
#pragma HLS dependence variable = matlen_freq inter false
#pragma HLS dependence variable = offset_freq inter false
#endif
            // read value from stream
            auto encodedValue = inStream.read();
            if (encodedValue.strobe == 0) {
                last_block = just_started;
                just_started = true;
                break;
            }
            just_started = false;
            tCh = encodedValue.data[0].range(7, 0);
            tLen = encodedValue.data[0].range(15, 8);
            uint16_t tOffset = encodedValue.data[0].range(31, 16) + 3 + 1;

            if (tLen) {
                // if match length present, get sequence codes
                uint8_t llc = getLLCode<16>((ap_uint<16>)litCount);
                uint8_t mlc = getMLCode<8>((ap_uint<8>)(tLen - MIN_MATCH));
                uint8_t ofc = bitsUsed31((uint32_t)tOffset);
                // reset code frequencies
                litlen_freq[llc]++;
                matlen_freq[mlc]++;
                offset_freq[ofc]++;
                // write sequence
                outSeqVal.data[0].litlen = litCount;
                outSeqVal.data[0].matlen = tLen; // - MIN_MATCH;
                outSeqVal.data[0].offset = tOffset;
                seqStream << outSeqVal;
                litCount = 0;
                ++seqCnt;
            } else {
                // store first literal to check for RLE block
                if (fv) {
                    cLit = tCh;
                } else {
                    if (cLit != tCh) isRLE = false;
                }
                fv = false;
                // increment literal count
                literal_freq[tCh]++;
                ++litCount;
                ++litTotal;
                // write literal
                outLitVal.data[0] = tCh;
                litStream << outLitVal;
            }
        }
        if (!last_block) {
            isRLE = (isRLE && cLit == 0);
            if (isRLE) {
                // printf("RLE literals\n");
                literal_freq[3]++;
                outLitVal.data[0] = 3;
                litStream << outLitVal;
            }
            rleFlagStream << isRLE;
        }
        // fix for zero sequences
        if (!last_block && seqCnt == 0) {
            outSeqVal.data[0].litlen = 0;
            outSeqVal.data[0].matlen = 0;
            outSeqVal.data[0].offset = 0;
            seqStream << outSeqVal;
        }

        // write strobe = 0
        outLitVal.strobe = 0;
        outSeqVal.strobe = 0;
        litStream << outLitVal;
        seqStream << outSeqVal;
        // write literal and distance trees
        if (!last_block) {
            metaVal.data[0] = litTotal;
            metaStream << metaVal;
            metaVal.data[0] = seqCnt;
            metaStream << metaVal;
            litCntStream << (isRLE ? (FREQ_DT)(litTotal + 1) : litTotal);
            // printf("litCount: %u, seqCnt: %u\n", (uint16_t)litTotal, (uint16_t)seqCnt);
            outLitFreqVal.strobe = 1;
            outSeqFreqVal.strobe = 1;
        write_lit_freq:
            for (ap_uint<9> i = 0; i < c_maxLitV + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outLitFreqVal.data[0] = literal_freq[i];
                litFreqStream << outLitFreqVal;
            }
            // first write total number of sequences
            outSeqFreqVal.data[0] = seqCnt;
            seqFreqStream << outSeqFreqVal;
            // write last ll, ml, of codes
            outSeqFreqVal.data[0] = getLLCode<16>(outSeqVal.data[0].litlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = getMLCode<8>(outSeqVal.data[0].matlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = bitsUsed31(outSeqVal.data[0].offset);
            seqFreqStream << outSeqFreqVal;
        write_lln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeLL + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = litlen_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        write_ofs_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeOF + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = offset_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        write_mln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeML + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = matlen_freq[i];
                seqFreqStream << outSeqFreqVal;
            }
        }
    }
    // eos needed only to indicated end of block
    outLitFreqVal.strobe = 0;
    outSeqFreqVal.strobe = 0;
    metaVal.strobe = 0;
    litFreqStream << outLitFreqVal;
    seqFreqStream << outSeqFreqVal;
    metaStream << metaVal;
}

// Block compression decision making
template <int BLOCK_SIZE, int MAX_FREQ_DWIDTH, class DT = ap_uint<MAX_FREQ_DWIDTH> >
void zstdCompressionMeta(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& metaStream,
                         hls::stream<ap_uint<2> >& strdBlockFlagStream,
                         hls::stream<DT>& outMetaStream) {
    uint8_t i = 0;
    DT litCnt = 0;
    ap_uint<2> stbFVal = 1; // <stb Flag 1-bit><strobe 1-bit>
gen_meta_loop:
    for (auto inMetaVal = metaStream.read(); inMetaVal.strobe > 0; inMetaVal = metaStream.read()) {
#pragma HLS PIPELINE off
        litCnt = inMetaVal.data[0];
        outMetaStream << litCnt;
        if (i == 0) {
            stbFVal.range(1, 1) = (ap_int<1>)(litCnt > BLOCK_SIZE - 2048);
            strdBlockFlagStream << stbFVal;
        }
        i = (i + 1) & 1;
    }
    // end of all data
    stbFVal = 0;
    strdBlockFlagStream << stbFVal;
}

template <int BLOCK_SIZE = 128 * 1024,
          int MAX_FREQ_DWIDTH = 17,
          int MATCH_LEN = 6,
          int MIN_MATCH = 3,
          int LZ_MAX_OFFSET_LIMIT = 32 * 1024,
          int MAX_MATCH_LEN = 255>
void getLitSequences(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                     hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                     hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                     hls::stream<bool>& rleFlagStream,
                     hls::stream<ap_uint<2> >& strdBlockFlagStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outMetaStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
    hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > metaStream("metaStream");
#pragma HLS STREAM variable = compressedStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16
#pragma HLS STREAM variable = metaStream depth = 8

    // LZ77 compress
    xf::compression::lzCompress<BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(inStream,
                                                                                                 compressedStream);
    // improve CR and generate clean sequences
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    // separate literals from sequences and generate literal frequencies
    xf::compression::details::zstdLz77DivideStream<MAX_FREQ_DWIDTH, MIN_MATCH>(
        boosterStream, litStream, seqStream, litFreqStream, seqFreqStream, rleFlagStream, metaStream, litCntStream);
    xf::compression::details::zstdCompressionMeta<BLOCK_SIZE, MAX_FREQ_DWIDTH>(metaStream, strdBlockFlagStream,
                                                                               outMetaStream);
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17, int PARALLEL_LIT_STREAMS = 4>
void preProcessLitStream(hls::stream<IntVectorStream_dt<8, 1> >& inLitStream,
                         hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream,
                         hls::stream<IntVectorStream_dt<8, 1> >& outReversedLitStream) {
    /*
     *  Description: This module reads literals from input stream and divides them into 1 or 4 streams
     *               of equal size. Last stream can be upto 3 bytes less than other 3 streams. This module
     *               streams literals in reverse order of input.
     */
    constexpr uint16_t c_litBufSize = (BLOCK_SIZE / 4);
    // literal buffer
    ap_uint<8> litBuffer[c_litBufSize];
#pragma HLS BIND_STORAGE variable = litBuffer type = ram_t2p impl = bram
    IntVectorStream_dt<8, 1> outLit;

pre_proc_lit_main:
    while (true) {
        bool rdLitFlag = false;
        auto inVal = inLitStream.read();
        if (inVal.strobe == 0) break;
        uint8_t streamCnt = 1;
        uint16_t streamSize[4] = {0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = streamSize complete
        ap_uint<MAX_FREQ_DWIDTH> litCnt = litCntStream.read();
        // get out stream count and sizes
        if (litCnt > 255) {
            streamCnt = 4;
            streamSize[0] = 1 + (litCnt - 1) / 4;
            streamSize[1] = streamSize[0];
            streamSize[2] = streamSize[0];
            streamSize[3] = litCnt - (streamSize[0] * 3);
        } else {
            streamSize[0] = litCnt;
            streamSize[3] = litCnt;
        }
        outLit.strobe = 1;
        // first send the stream count
        outLit.data[0] = streamCnt;
        outReversedLitStream << outLit;
    write_stream_sizes:
        for (uint8_t k = 0; k < streamCnt; ++k) {
#pragma HLS PIPELINE II = 2
            outLit.data[0] = (uint8_t)(streamSize[k]);
            outReversedLitStream << outLit;
            outLit.data[0] = (uint8_t)(streamSize[k] >> 8);
            outReversedLitStream << outLit;
        }
        // write already read value into buffer
        uint16_t litIdx = 0;
        uint16_t wIdx = 1;
        uint16_t rIdx = 0;
        litBuffer[0] = inVal.data[0];
    rev_lit_loop_1:
        for (litIdx = 1; litIdx < streamSize[0]; ++litIdx) {
#pragma HLS PIPELINE II = 1
            inVal = inLitStream.read();
            litBuffer[wIdx++] = inVal.data[0];
        }
        // init read-write indices
        rIdx = wIdx - 1;
        wIdx = c_litBufSize - 1;

        int8_t rwInc = -1;
    rev_lit_loop_234:
        for (uint8_t i = 0; i < 3 && streamCnt > 1; ++i) {
        rev_lit_loop_overlap:
            for (litIdx = 0; litIdx < streamSize[0]; ++litIdx) {
#pragma HLS PIPELINE II = 1
                // stream out previously read data
                outLit.data[0] = litBuffer[rIdx];
                outReversedLitStream << outLit;
                rIdx += rwInc;
                // buffer next sub-stream
                if (litIdx < streamSize[i + 1]) {
                    inVal = inLitStream.read();
                    litBuffer[wIdx] = inVal.data[0];
                    wIdx += rwInc;
                }
            }
            // manage direction in memory access
            // both indices move in same direction
            rwInc = (~rwInc) + 1; // flip +1/-1
            // manage memory access indices
            if ((uint8_t)(i & 1) == 0) { // even, works only after type-casting to int types
                rIdx = wIdx + 1;
                wIdx = 0;
            } else { // odd
                rIdx = wIdx - 1;
                wIdx = c_litBufSize - 1;
            }
        }

    rev_lit_loop_5:
        for (litIdx = 0; litIdx < streamSize[3]; ++litIdx) {
#pragma HLS PIPELINE II = 1
            // stream out previously read data
            outLit.data[0] = litBuffer[rIdx];
            outReversedLitStream << outLit;
            rIdx += rwInc;
        }
        // dump strobe 0 from input
        inVal = inLitStream.read();
        // end of block indicator not needed, since size is also sent
    }
    // end of data
    outLit.strobe = 0;
    outReversedLitStream << outLit;
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17>
void reverseLitQuadStreams(hls::stream<ap_uint<68> >& inLitStream,
                           hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream,
                           hls::stream<ap_uint<68> >& outReversedLitStream) {
    /*
     *  Description: This module reads literals from input stream and divides them into 1 or 4 streams
     *               of equal size. Last stream can be upto 3 bytes less than other 3 streams. This module
     *               streams literals in reverse order of input.
     */
    constexpr uint16_t c_litBufSize = (BLOCK_SIZE / 32);
    // storing only 1/4th of the block size, 8-bytes a time
    // literal buffer
    ap_uint<64> litBuffer[c_litBufSize];
#pragma HLS BIND_STORAGE variable = litBuffer type = ram_t2p impl = bram
    ap_uint<64> outLit;
    ap_uint<68> outVal;

pre_proc_lit_main:
    while (true) {
        bool rdLitFlag = false;
        auto inVal = inLitStream.read();
        if (inVal.range(3, 0) == 0) break;
        uint8_t streamCnt = 1;
        uint16_t streamSize[5] = {0, 0, 0, 0, 0}; // 1 dummy entry
#pragma HLS ARRAY_PARTITION variable = streamSize complete
        ap_uint<MAX_FREQ_DWIDTH> litCnt = litCntStream.read();
        // get out stream count and sizes
        if (litCnt > 255) {
            streamCnt = 4;
            streamSize[0] = 1 + (litCnt - 1) / 4;
            streamSize[1] = streamSize[0];
            streamSize[2] = streamSize[0];
            streamSize[3] = litCnt - (streamSize[0] * 3);
        } else {
            streamSize[0] = litCnt;
            streamSize[3] = litCnt;
        }
        outVal.range(3, 0) = 1;
        // first send the stream count
        outVal.range(11, 4) = streamCnt;
        outReversedLitStream << outVal;
    write_stream_sizes:
        for (uint8_t k = 0; k < streamCnt; ++k) {
#pragma HLS UNROLL
            outLit.range(15 + (k * 16), k * 16) = streamSize[k];
        }
        outVal.range(67, 4) = outLit;
        outReversedLitStream << outVal;
        // indexes
        uint16_t litRcnt = 0, litWcnt = 0;
        uint16_t wIdx = 1;
        uint16_t rIdx = 0;
        // write already read value into buffer
        litBuffer[0] = inVal.range(67, 4);
        ap_uint<64> prevWord = 0;
        uint8_t extBytesRead = 0;
        int8_t rwInc = 1;
        litWcnt = inVal.range(3, 0);
    rev_lit_loop_1to5:
        for (uint8_t si = 0; si < streamCnt + 1; ++si) {
            if (extBytesRead) {
                // write first byte to buffer
                litBuffer[wIdx] = prevWord;
                wIdx += rwInc;
                litWcnt = extBytesRead;
                // write first output for previous sub-stream
                outVal.range(3, 0) = 8 - extBytesRead;
                outLit = litBuffer[rIdx];
            // assign value in reverse byte order
            assign_outVal_1:
                for (uint8_t k = 0; k < 8; ++k) {
#pragma HLS UNROLL
                    uint8_t rK = 7 - k;
                    outVal.range((k * 8) + 11, (k * 8) + 4) = outLit.range((rK * 8) + 7, (rK * 8));
                }
                outReversedLitStream << outVal;
                rIdx += rwInc;
                litRcnt = outVal.range(3, 0);
            }
        rev_lit_loop_overlap:
            while ((si < streamCnt && litWcnt < streamSize[si]) || (si > 0 && litRcnt < streamSize[si - 1])) {
#pragma HLS PIPELINE II = 1
                // run till one of the conditions is true
                // buffer sub-stream
                if (si < streamCnt && litWcnt < streamSize[si]) {
                    inVal = inLitStream.read();
                    litBuffer[wIdx] = inVal.range(67, 4);
                    prevWord = inVal.range(67, 4);
                    litWcnt += 8; // to handle last stream size not aligned to 8 bytes case
                    wIdx += rwInc;
                }
                // read-out buffer data in reverse after first sub-stream is buffered
                if (si > 0 && litRcnt < streamSize[si - 1]) {
                    outLit = litBuffer[rIdx];
                    rIdx += rwInc;
                // assign value in reverse byte order
                assign_outVal_2:
                    for (uint8_t k = 0; k < 8; ++k) {
#pragma HLS UNROLL
                        uint8_t rK = 7 - k;
                        outVal.range((k * 8) + 11, (k * 8) + 4) = outLit.range((rK * 8) + 7, (rK * 8));
                    }
                    outVal.range(3, 0) = 8;
                    outReversedLitStream << outVal;
                    litRcnt += 8;
                }
            }
            // printf("Written %u bytes\n", wb);
            // get extra bytes for currently buffered stream
            extBytesRead = litWcnt - streamSize[si];
            // re-initialize literal index in sub-stream
            litWcnt = 0;
            litRcnt = 0;
            // manage direction in memory access
            // both indices move in same direction
            rwInc = (~rwInc) + 1; // flip +1/-1
            // manage memory access indices
            if ((uint8_t)(si & 1) != 0) { // odd, works only after type-casting to int types
                rIdx = wIdx + 1;
                wIdx = 0;
            } else { // even
                rIdx = wIdx - 1;
                wIdx = c_litBufSize - 1;
            }
        }
        // dump strobe 0 from input
        inVal = inLitStream.read();
        // end of block indicator not needed, since size is also sent
    }
    // end of data
    outVal.range(3, 0) = 0;
    outReversedLitStream << outVal;
}

template <int MAX_FREQ_DWIDTH>
void downSizeLitlen(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& inSeqStream,
                    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& outSeqStream) {
    // Downsize Literal length to 8-bits
    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> outSeqVal;
    bool done = false;
dsz_ll_outer:
    while (!done) {
        auto inSeqVal = inSeqStream.read();
        bool dszDone = (inSeqVal.strobe == 0);
        done = dszDone;
        auto litLen = inSeqVal.data[0].litlen;
        outSeqVal.strobe = 1;
    dsz_litlen:
        while (!dszDone) {
#pragma HLS PIPELINE II = 1
            if (litLen > 255) {
                outSeqVal.data[0].setLitlen(255);
                outSeqVal.data[0].setMatlen(0);
                outSeqVal.data[0].setOffset(0);
                litLen -= 255;
            } else {
                outSeqVal.data[0].setLitlen(litLen.range(7, 0));
                outSeqVal.data[0].setMatlen(inSeqVal.data[0].matlen);
                outSeqVal.data[0].setOffset(inSeqVal.data[0].offset);

                // read next input sequence
                inSeqVal = inSeqStream.read();
                litLen = inSeqVal.data[0].litlen;
                dszDone = (inSeqVal.strobe == 0);
            }
            // write output sequence
            outSeqStream << outSeqVal;
        }
        // End of block/file
        outSeqVal.strobe = 0;
        outSeqStream << outSeqVal;
    }
}

template <int MAX_FREQ_DWIDTH, int MIN_MATCH>
void upSizeLitlen(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& inSeqStream,
                  hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outSeqStream) {
    // Upsize litlen to MAX_FREQ_DWIDTH-bits from 8-bits in reversed sequences stream
    DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> outSeqVal;
    bool done = false;
upsz_ll_outer:
    while (!done) {
        auto inSeqVal = inSeqStream.read();
        bool uszDone = (inSeqVal.strobe == 0);
        done = uszDone;
        // store current sequence
        outSeqVal.data[0].litlen = inSeqVal.data[0].getLitlen();
        outSeqVal.data[0].offset = inSeqVal.data[0].getOffset();
        // check for noSeq condition
        if (inSeqVal.data[0].getLitlen() == 0 && inSeqVal.data[0].getMatlen() == 0 &&
            inSeqVal.data[0].getOffset() == 0) {
            outSeqVal.data[0].matlen = 0;
        } else {
            outSeqVal.data[0].matlen = inSeqVal.data[0].getMatlen() - MIN_MATCH;
        }
        outSeqVal.strobe = 1;
    upsz_litlen:
        while (!uszDone) {
#pragma HLS PIPELINE II = 1
            inSeqVal = inSeqStream.read();
            uszDone = (inSeqVal.strobe == 0);
            if (inSeqVal.data[0].getMatlen() == 0 && inSeqVal.data[0].getOffset() == 0 && !uszDone) {
                outSeqVal.data[0].litlen = outSeqVal.data[0].litlen + inSeqVal.data[0].getLitlen();
            } else { // if regular or last sequence
                outSeqStream << outSeqVal;
                // update out sequence values with current values
                outSeqVal.data[0].litlen = inSeqVal.data[0].getLitlen();
                outSeqVal.data[0].matlen = inSeqVal.data[0].getMatlen() - MIN_MATCH;
                outSeqVal.data[0].offset = inSeqVal.data[0].getOffset();
            }
        }
        // end of block/file
        outSeqVal.strobe = 0;
        outSeqStream << outSeqVal;
    }
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17>
void reverseSeqIntl(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& seqStream,
                    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& outReversedSeqStream) {
    constexpr uint32_t c_seqMemSize = BLOCK_SIZE / 4;
    // sequence buffer
    SequencePack<MAX_FREQ_DWIDTH, 8> seqBuffer[c_seqMemSize];
#pragma HLS BIND_STORAGE variable = seqBuffer type = ram_t2p impl = bram

    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> outSeqV;
    bool done = false;
    bool blockDone = true;
    // operation modes
    bool streamMode = 0;
    bool bufferMode = 1;

    uint16_t memReadBegin[2];                               // starting index for BRAM read
    constexpr int16_t memReadLimit[2] = {-1, c_seqMemSize}; // last index for buffer read
#pragma HLS ARRAY_PARTITION variable = memReadBegin complete
#pragma HLS ARRAY_PARTITION variable = memReadLimit complete
    uint16_t wIdx = 0;
    int16_t rIdx = 0; // may become negative for writing strobe 0
    int8_t wInc = 1, rInc = 1;
    uint8_t rsi = 0, wsi = 0;
    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> inSeq;
    outSeqV.strobe = 1;
// sequence read and reverse
reverse_seq_stream:
    while (bufferMode || streamMode) {
#pragma HLS PIPELINE II = 1 rewind
        if (bufferMode) {
            // Write to internal memory
            inSeq = seqStream.read();
            seqBuffer[wIdx].setData(inSeq.data[0].getData());
            // check mode
            if (inSeq.strobe == 0) {
                done = blockDone;
                blockDone = true;
                // Enable memory read/stream mode
                if (streamMode == 0 && !done) {
                    streamMode = 1;
                    rIdx = wIdx - wInc;
                }
                // set mem read begin index
                memReadBegin[wsi] = wIdx - wInc; // since an extra increment has been done here
                // change increment direction
                wInc = (~wInc) + 1;  // flip 1 and -1
                wsi = (wsi + 1) & 1; // flip 1 and 0
                // post increment check if bufferMode to be continued/paused/stopped
                if (done || (wsi == rsi)) bufferMode = 0;
                // set stream mode's last mem index for even and odd stream indices
                wIdx = (uint16_t)(memReadLimit[wsi] + wInc);
                continue;
            } else {
                blockDone = false;
                // directional increment
                wIdx += wInc;
            }
        }
        if (streamMode) {
            // update stream mode state
            if (rIdx == memReadLimit[rsi]) {
                // write end of block indication as strobe 0
                outSeqV.strobe = 0;
                // in case bufferMode pauses due to finishing earlier
                if (bufferMode == 0) bufferMode = (wsi == rsi && !done);
                rsi = (rsi + 1) & 1;
                rIdx = memReadBegin[rsi];
                rInc = (~rInc) + 1; // flip 1 and -1
                // either previous streamMode ended quicker than next bufferMode or streamCnt reached
                if (done || (wsi == rsi)) streamMode = 0;
            } else {
                // Read from internal memory to output stream
                outSeqV.data[0].setData(seqBuffer[rIdx].getData());
                // directional decrement
                rIdx -= rInc;
                outSeqV.strobe = 1;
            }
            // write data or strobe 0
            outReversedSeqStream << outSeqV;
        }
    }
    // file end indication
    outSeqV.strobe = 0;
    outReversedSeqStream << outSeqV;
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17, int MIN_MATCH = 3>
void reverseSeq(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& inSeqStream,
                hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outReversedSeqStream) {
    // reverse sequences
    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > dszSeqStream("dszSeqStream");
    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > dszReversedSeqStream("dszReversedSeqStream");
#pragma HLS STREAM variable = dszSeqStream depth = 16
#pragma HLS STREAM variable = dszReversedSeqStream depth = 16

#pragma HLS DATAFLOW
    // MAX_FREQ_DWIDTH-bit literal length to 8-bit
    details::downSizeLitlen<MAX_FREQ_DWIDTH>(inSeqStream, dszSeqStream);
    // reverse sequences
    details::reverseSeqIntl<BLOCK_SIZE, MAX_FREQ_DWIDTH>(dszSeqStream, dszReversedSeqStream);
    // upsize reversed sequences to MAX_FREQ_DWIDTH-bit literal length
    details::upSizeLitlen<MAX_FREQ_DWIDTH, MIN_MATCH>(dszReversedSeqStream, outReversedSeqStream);
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17, int MIN_MATCH = 3>
void reverseSeq(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& inSeqStream,
                hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outReversedSeqStream) {
    // reverse sequences
    hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > dszReversedSeqStream("dszReversedSeqStream");
#pragma HLS STREAM variable = dszReversedSeqStream depth = 16

#pragma HLS DATAFLOW
    // reverse sequences
    details::reverseSeqIntl<BLOCK_SIZE, MAX_FREQ_DWIDTH>(inSeqStream, dszReversedSeqStream);
    // upsize reversed sequences to MAX_FREQ_DWIDTH-bit literal length
    details::upSizeLitlen<MAX_FREQ_DWIDTH, MIN_MATCH>(dszReversedSeqStream, outReversedSeqStream);
}

template <int CORE_IDX,
          int BLOCK_SIZE = 128 * 1024,
          int MAX_FREQ_DWIDTH = 17,
          int MATCH_LEN = 6,
          int MIN_MATCH = 3,
          int LZ_MAX_OFFSET_LIMIT = 32 * 1024,
          int MAX_MATCH_LEN = 255>
void getLitSequences(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                     hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                     hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& seqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                     hls::stream<bool>& rleFlagStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outMetaStream,
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& litCntStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
    hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> > seqFszStream("seqFszStream");
#pragma HLS STREAM variable = compressedStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16
#pragma HLS STREAM variable = seqFszStream depth = 16

    // LZ77 compress
    xf::compression::lzCompress<BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT, CORE_IDX>(
        inStream, compressedStream);
    // improve CR and generate clean sequences
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    // separate literals from sequences and generate literal frequencies
    xf::compression::details::zstdLz77DivideStream<MAX_FREQ_DWIDTH, MIN_MATCH>(
        boosterStream, litStream, seqFszStream, litFreqStream, seqFreqStream, rleFlagStream, outMetaStream,
        litCntStream);
    // Downsize literal lengths
    xf::compression::details::downSizeLitlen<MAX_FREQ_DWIDTH>(seqFszStream, seqStream);
}

template <int MAX_FREQ_DWIDTH = 17>
void frequencySequencer(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& wghtFreqStream,
                        hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                        hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& freqStream) {
    // sequence input frequencies into single output stream
    constexpr uint16_t c_limits[4] = {c_maxCodeLL + 1, c_maxCodeOF + 1, c_maxCodeML + 1, c_maxZstdHfBits + 1};
    const uint8_t c_hfIdx = 3;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outfreq;
    while (true) {
        outfreq = seqFreqStream.read();
        if (outfreq.strobe == 0) break; // will come only at the end of all data
        // first value is sequences count
        freqStream << outfreq;
    // next three values are last ll, ml, of codes
    write_last_seq:
        for (uint8_t i = 0; i < 3; ++i) {
#pragma HLS PIPELINE II = 1
            outfreq = seqFreqStream.read();
            freqStream << outfreq;
        }
    write_bl_ll_ml_of:
        for (uint8_t fIdx = 0; fIdx < 4; ++fIdx) {
            auto llim = c_limits[fIdx];
            bool isBlen = (c_hfIdx == fIdx);
            if (isBlen) {
                // maxVal from literals
                outfreq = wghtFreqStream.read();
                freqStream << outfreq;
            }
            // first send the size, followed by data
            outfreq.data[0] = llim;
            freqStream << outfreq;
        write_inp_freq:
            for (uint16_t i = 0; i < llim; ++i) {
#pragma HLS PIPELINE II = 1
                if (isBlen) {
                    outfreq = wghtFreqStream.read();
                } else {
                    outfreq = seqFreqStream.read();
                }
                freqStream << outfreq;
            }
            // dump strobe 0
            if (isBlen) wghtFreqStream.read();
        }
    }
    // dump strobe 0
    wghtFreqStream.read();
    outfreq.data[0] = 0;
    outfreq.strobe = 0;
    freqStream << outfreq;
}

} // details
} // compression
} // xf
#endif
