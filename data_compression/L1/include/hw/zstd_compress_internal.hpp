/*
 * (c) Copyright 2021 Xilinx, Inc. All rights reserved.
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
#ifndef _XFCOMPRESSION_ZSTD_COMPRESS_INTERNAL__HPP_
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

inline uint16_t getLLCode(uint16_t litlen) {
#pragma HLS INLINE
    return (litlen > 63) ? bitsUsed31(litlen) + c_litlenDeltaCode : c_litlenCode[litlen];
}

inline uint16_t getMLCode(uint16_t matlen) {
#pragma HLS INLINE
    return (matlen > 127) ? bitsUsed31(matlen) + c_matlenDeltaCode : c_matlenCode[matlen];
}

template <int MAX_FREQ_DWIDTH = 18, int MIN_MATCH = 3>
void zstdLz77DivideStream(hls::stream<IntVectorStream_dt<32, 1> >& inStream,
                          hls::stream<IntVectorStream_dt<8, 1> >& litStream,
                          hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& litFreqStream,
                          hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                          hls::stream<bool>& rleFlagStream,
                          hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& metaStream) {
    // lz77 encoder states
    enum LZ77EncoderStates { WRITE_LITERAL, WRITE_OFFSET0, WRITE_OFFSET1 };
    // offsets in frequency table for literals->litlen->matlen->offset
    // constexpr uint16_t offsets = {0, c_maxLitV + 1, c_maxLitV + c_maxCodeLL + 2, c_maxLitV + c_maxCodeLL +
    // c_maxCodeML + 3};
    IntVectorStream_dt<8, 1> outLitVal;
    DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> outSeqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outLitFreqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> outSeqFreqVal;
    ap_uint<MAX_FREQ_DWIDTH> metaVal = 0;
    bool last_block = false;
    bool just_started = true;
    int blk_n = 0;

    while (!last_block) {
        // iterate over multiple blocks in a file
        ++blk_n;
        enum LZ77EncoderStates next_state = WRITE_LITERAL;
        ap_uint<MAX_FREQ_DWIDTH> literal_freq[c_maxLitV + 1];
        ap_uint<MAX_FREQ_DWIDTH> litlen_freq[c_maxCodeLL + 1];
        ap_uint<MAX_FREQ_DWIDTH> matlen_freq[c_maxCodeML + 1];
        ap_uint<MAX_FREQ_DWIDTH> offset_freq[c_maxCodeOF + 1];

        ap_uint<MAX_FREQ_DWIDTH> seqCnt = 0;
        {
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
        ap_uint<MAX_FREQ_DWIDTH> litCount = 0;
        ap_uint<MAX_FREQ_DWIDTH> litTotal = 0;
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
                uint8_t llc = getLLCode(litCount);
                uint8_t mlc = getMLCode(tLen - MIN_MATCH);
                uint8_t ofc = bitsUsed31(tOffset);
                // reset code frequencies
                litlen_freq[llc]++;
                matlen_freq[mlc]++;
                offset_freq[ofc]++;
                // write sequence
                outSeqVal.data[0].litlen = litCount;
                outSeqVal.data[0].matlen = tLen - MIN_MATCH;
                outSeqVal.data[0].offset = tOffset;
                seqStream << outSeqVal;
                // if (blk_n == 2) printf("ll: %d, ml: %d, of: %d\n", litCount, tLen - MIN_MATCH, tOffset);
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
                // printf("%c", (char)tCh);
            }
        }
        if (!last_block) {
            isRLE = (isRLE && cLit == 0);
            if (isRLE) {
                // printf("RLE Case blk_n: %u\n", blk_n);
                literal_freq[3]++;
                outLitVal.data[0] = 3;
                litStream << outLitVal;
            }
            rleFlagStream << isRLE;
        }
        outLitVal.strobe = 0;
        outSeqVal.strobe = 0;
        litStream << outLitVal;
        seqStream << outSeqVal;
        // write literal and distance trees
        if (!last_block) {
            metaStream << (ap_uint<MAX_FREQ_DWIDTH>)litTotal;
            metaStream << (ap_uint<MAX_FREQ_DWIDTH>)seqCnt;
            outLitFreqVal.strobe = 1;
            outSeqFreqVal.strobe = 1;
        // if (blk_n == 1) printf("Literals freq\n");
        write_lit_freq:
            for (ap_uint<9> i = 0; i < c_maxLitV + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outLitFreqVal.data[0] = literal_freq[i];
                litFreqStream << outLitFreqVal;
                // if (blk_n == 1) printf("%d. freq: %u\n", (uint16_t)i, (uint16_t)literal_freq[i]);
            }
            // first write total number of sequences
            // printf("seqCnt: %u\n", (uint32_t)seqCnt);
            outSeqFreqVal.data[0] = seqCnt;
            seqFreqStream << outSeqFreqVal;
            // write last ll, ml, of codes
            outSeqFreqVal.data[0] = getLLCode(outSeqVal.data[0].litlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = getMLCode(outSeqVal.data[0].matlen);
            seqFreqStream << outSeqFreqVal;
            outSeqFreqVal.data[0] = bitsUsed31(outSeqVal.data[0].offset);
            seqFreqStream << outSeqFreqVal;
        // if (blk_n == 1) printf("Litlen freq\n");
        write_lln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeLL + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = litlen_freq[i];
                seqFreqStream << outSeqFreqVal;
                // if (blk_n == 1) printf("%d. freq: %u\n", (uint16_t)i, (uint16_t)litlen_freq[i]);
            }
        // if (blk_n == 12) printf("Offset freq\n");
        write_ofs_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeOF + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = offset_freq[i];
                seqFreqStream << outSeqFreqVal;
                // if (blk_n == 12) printf("%d. freq: %u\n", (uint16_t)i, (uint16_t)offset_freq[i]);
            }
        // if (blk_n == 12) printf("Matlen freq\n");
        write_mln_freq:
            for (ap_uint<9> i = 0; i < c_maxCodeML + 1; ++i) {
#pragma HLS PIPELINE II = 1
                outSeqFreqVal.data[0] = matlen_freq[i];
                seqFreqStream << outSeqFreqVal;
                // if (blk_n == 12) printf("%d. freq: %u\n", (uint16_t)i, (uint16_t)matlen_freq[i]);
            }
        }
    }
    // eos needed only to indicated end of block
    outLitFreqVal.strobe = 0;
    outSeqFreqVal.strobe = 0;
    litFreqStream << outLitFreqVal;
    seqFreqStream << outSeqFreqVal;
    metaStream << (ap_uint<MAX_FREQ_DWIDTH>)(-1);
}

// dummy processing for now, placeholder for block compression decision making
template <int MAX_FREQ_DWIDTH>
void zstdCompressionMeta(hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& metaStream,
                         hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outMetaStream) {
    auto inMetaVal = metaStream.read();
gen_meta_loop:
    while (inMetaVal != (ap_uint<MAX_FREQ_DWIDTH>)(-1)) {
        outMetaStream << inMetaVal;
        inMetaVal = metaStream.read();
    }
}

template <int MAX_BLOCK_SIZE = 128 * 1024,
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
                     hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outMetaStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
    hls::stream<ap_uint<MAX_FREQ_DWIDTH> > metaStream("metaStream");
#pragma HLS STREAM variable = compressdStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16
#pragma HLS STREAM variable = metaStream depth = 8

    // LZ77 compress
    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(inStream,
                                                                                                     compressedStream);
    // improve CR and generate clean sequences
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    // separate literals from sequences and generate literal frequencies
    xf::compression::details::zstdLz77DivideStream<MAX_FREQ_DWIDTH, MIN_MATCH>(
        boosterStream, litStream, seqStream, litFreqStream, seqFreqStream, rleFlagStream, metaStream);
    xf::compression::details::zstdCompressionMeta<MAX_FREQ_DWIDTH>(metaStream, outMetaStream);
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17, int PARALLEL_LIT_STREAMS = 4>
void preProcessLitStream(hls::stream<IntVectorStream_dt<8, 1> >& inLitStream,
                         hls::stream<IntVectorStream_dt<8, 1> >& outReversedLitStream) {
    /*
     *  Description: This module reads literals from input stream and divides them into 1 or 4 streams
     *               of equal size. Last stream can be upto 3 bytes less than other 3 streams. This module
     *               streams literals in reverse order of input.
     */
    // constexpr int c_litStrMaxSize = 1 + ((BLOCK_SIZE - 1) / PARALLEL_LIT_STREAMS);
    // literal buffer
    ap_uint<8> litBuffer[BLOCK_SIZE];
    IntVectorStream_dt<8, 1> outLit;

    while (true) {
        uint16_t litIdx = 0;
        uint16_t litCnt = 0;
        uint8_t streamCnt = 1;
        uint16_t streamSize[4] = {0, 0, 0, 0};
    // write to buffer and read from buffer
    read_literals_bf:
        for (auto inVal = inLitStream.read(); inVal.strobe > 0; inVal = inLitStream.read()) {
#pragma HLS PIPELINE II = 1
            litBuffer[litIdx++] = inVal.data[0];
            ++litCnt;
        }
        if (litCnt == 0) break;
        // get out stream count
        if (litCnt > 255) {
            streamCnt = 4;
            streamSize[0] = 1 + (litCnt - 1) / 4;
            streamSize[1] = streamSize[0];
            streamSize[2] = streamSize[0];
            streamSize[3] = litCnt - (streamSize[0] * 3);
        } else {
            streamSize[0] = litCnt;
        }
        outLit.strobe = 1;
        // first send the stream count
        outLit.data[0] = streamCnt;
        outReversedLitStream << outLit;

        int16_t idxL = -1;
        int16_t idxH = -1;
        int16_t litIdxRange = 0;
    write_rev_literals_outer:
        for (uint8_t k = 0; k < streamCnt; ++k) {
            idxL = idxH;
            idxH += streamSize[k];
            // First write sub-stream size, followed by the stream data
            outLit.data[0] = (uint8_t)(streamSize[k]);
            outReversedLitStream << outLit;
            outLit.data[0] = (uint8_t)(streamSize[k] >> 8);
            outReversedLitStream << outLit;
        // printf("LitStream %d, size: %d\nStream Reversed\n", k, streamSize[k]);

        write_rev_literals:
            for (int16_t idx = idxH; idx > idxL; --idx) {
#pragma HLS PIPELINE II = 1
                // write to output huffman encoder unit in reverse
                outLit.data[0] = litBuffer[idx];
                outReversedLitStream << outLit;
                // printf("%d, ", (uint8_t)litBuffer[idx]);
            }
            // printf("\n");
        }
        // end of block indicator not needed, since size is also sent
        // outLit.strobe = 0;
        // outReversedLitStream << outLit;
    }
    // end of data
    outLit.strobe = 0;
    outReversedLitStream << outLit;
}

template <int BLOCK_SIZE = 32 * 1024, int MAX_FREQ_DWIDTH = 17, int MIN_MATCH = 3>
void reverseSeq(hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& seqStream,
                hls::stream<DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> >& outReversedSeqStream,
                hls::stream<DSVectorStream_dt<Sequence_dt<8>, 1> >& outseqCodeStream) {
    constexpr int c_seqMemSize = 1 + ((BLOCK_SIZE - 1) / MIN_MATCH);
    // sequence buffer
    Sequence_dt<MAX_FREQ_DWIDTH> seqBuffer[c_seqMemSize];
    bool done = false;
    while (!done) {
        // sequence reversing
        int sqIdx = 0;
    // read sequence into buffer
    read_sq_bf:
        for (auto inSeq = seqStream.read(); inSeq.strobe > 0; inSeq = seqStream.read()) {
#pragma HLS PIPELINE II = 1
            // assign sequence and sequence code
            seqBuffer[sqIdx] = inSeq.data[0];
            ++sqIdx;
        }

        done = (sqIdx == 0);
        DSVectorStream_dt<Sequence_dt<MAX_FREQ_DWIDTH>, 1> rsqReg;
        DSVectorStream_dt<Sequence_dt<8>, 1> rsqCodeReg;
        rsqReg.strobe = 1;
        rsqCodeReg.strobe = 1;
    // pop data from buffer to output stream in reverse
    // printf("Reversed sequences and codes\n");
    write_rev_sq:
        for (int i = sqIdx - 1; i > -1; --i) {
#pragma HLS PIPELINE II = 1
            // fetch sequence and calculate codes
            auto seq = seqBuffer[i];
            Sequence_dt<8> seqCode;
            seqCode.litlen = getLLCode(seq.litlen);
            seqCode.matlen = getMLCode(seq.matlen);
            seqCode.offset = bitsUsed31(seq.offset);
            // write to output
            rsqReg.data[0] = seq;
            rsqCodeReg.data[0] = seqCode;
            outReversedSeqStream << rsqReg;
            outseqCodeStream << rsqCodeReg;

            // printf("ll: %u, ml: %u, of: %u -- ",
            //		(uint32_t)seqBuffer[i].litlen, (uint32_t)seqBuffer[i].matlen, (uint32_t)seqBuffer[i].offset);
            // printf("llc: %u, mlc: %u, ofc: %u\n",
            //		(uint8_t)seqCodeBuffer[i].litlen, (uint8_t)seqCodeBuffer[i].matlen,
            //(uint8_t)seqCodeBuffer[i].offset);
        }
        // block/file end indication
        rsqReg.strobe = 0;
        rsqCodeReg.strobe = 0;
        outReversedSeqStream << rsqReg;
        outseqCodeStream << rsqCodeReg;
    }
}

template <int MAX_FREQ_DWIDTH = 17>
void frequencySequencer(hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& wghtFreqStream,
                        hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& seqFreqStream,
                        hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& freqStream) {
    // sequence input frequencies into single output stream
    constexpr uint16_t c_limits[4] = {c_maxCodeLL + 1, c_maxCodeOF + 1, c_maxZstdHfBits + 1, c_maxCodeML + 1};
    const uint8_t c_hfIdx = 2;
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
