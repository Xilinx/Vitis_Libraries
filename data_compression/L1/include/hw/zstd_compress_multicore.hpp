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

#ifndef _XFCOMPRESSION_ZSTD_COMPRESS_MULTICORE_HPP_
#define _XFCOMPRESSION_ZSTD_COMPRESS_MULTICORE_HPP_

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

#include "axi_stream_utils.hpp"
#include "zstd_specs.hpp"
#include "stream_upsizer.hpp"
#include "stream_downsizer.hpp"
#include "compress_utils.hpp"
#include "huffman_treegen.hpp"
#include "zstd_encoders.hpp"
#include "zstd_compress.hpp"

namespace xf {
namespace compression {
namespace details {

template <int BLOCK_SIZE, int MIN_BLK_SIZE = 128>
void inputBufferMinBlock(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                         hls::stream<bool>& rawBlockFlagStream,
                         hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    // write data and indicate if it should be raw block or not
    bool not_done = true;
    bool rawFlagNotSent = true;
    IntVectorStream_dt<8, 8> inVal;
stream_data:
    while (not_done) {
        // read data size in bytes
        uint32_t dataSize = 0;
        inVal.strobe = 8;
        rawFlagNotSent = true;
    send_in_block:
        while (inVal.strobe > 0 && dataSize < BLOCK_SIZE) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            if (inVal.strobe > 0) {
                outStream << inVal;
                dataSize += inVal.strobe;
                // indicate if more data than minimum block size
                if (dataSize > MIN_BLK_SIZE && rawFlagNotSent) {
                    rawBlockFlagStream << false;
                    rawFlagNotSent = false;
                }
            }
        }
        if (dataSize > 0 && dataSize < 1 + MIN_BLK_SIZE) rawBlockFlagStream << true;
        // end of block for last block with data
        if (dataSize > 0 && inVal.strobe == 0) outStream << inVal;
        // end of block/file
        inVal.strobe = 0;
        outStream << inVal;
        // terminate condition
        not_done = (dataSize == BLOCK_SIZE);
    }
    // for end of files, value must be false
    rawBlockFlagStream << false;
}

template <int BLOCK_SIZE, int CORE_COUNT>
void __inputDistributer(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                        hls::stream<bool>& rawBlockFlagStream,
                        hls::stream<ap_uint<68> > outStream[CORE_COUNT],
                        hls::stream<ap_uint<68> >& outStrdStream,
                        hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Send input blocks for compression or raw block packer and metadata to block packer.
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 8> inVal;
    ap_uint<68> rawVal;
    ap_uint<68> outVal;
    uint8_t coreIdx = 0;
stream_blocks:
    while (true) {
        uint32_t dataSize = 0;
        auto isRawBlock = rawBlockFlagStream.read();
    send_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            // assign values
            rawVal.range(3, 0) = inVal.strobe;
            outVal.range(3, 0) = inVal.strobe;
            for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS UNROLL
                rawVal.range((i * 8) + 11, (i * 8) + 4) = inVal.data[i];
                outVal.range((i * 8) + 11, (i * 8) + 4) = inVal.data[i];
            }
            // write to output streams
            if (!isRawBlock) outStream[coreIdx] << outVal;
            outStrdStream << rawVal;
            dataSize += inVal.strobe;
        }
        // End of block/file
        if (!isRawBlock) outStream[coreIdx] << 0;
        outStrdStream << 0;
        // write meta data
        if (dataSize > 0) {
            // send metadata to packer
            metaVal.strobe = 1;
            metaVal.data[0] = dataSize;
            blockMetaStream << metaVal;
            if (BLOCK_SIZE > (64 * 1024)) {
                metaVal.data[0] = dataSize >> 16;
            } else {
                metaVal.data[0] = 0;
            }
            blockMetaStream << metaVal;
        } else {
            // strobe 0 for end of data, exit condition
            metaVal.strobe = 0;
            blockMetaStream << metaVal;
            break;
        }
        ++coreIdx;
        coreIdx = ((coreIdx < CORE_COUNT) ? coreIdx : 0);
        // coreIdx = (coreIdx + 1) % CORE_COUNT; // increment within limits
    }
// terminate all lz77 blocks
terminate_blocks:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
        if (i != coreIdx) outStream[i] << 0; // "coreIdx" lz77 already terminated
    }
}

template <int BLOCK_SIZE, int MIN_BLK_SIZE, int CORE_COUNT>
void inputDistributer(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                      hls::stream<ap_uint<68> > outStream[CORE_COUNT],
                      hls::stream<ap_uint<68> >& outStrdStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    constexpr uint16_t c_idataStreamDepth = 256 / CORE_COUNT;
    // Internal streams
    hls::stream<IntVectorStream_dt<8, 8> > intmDataStream("intmDataStream");
    hls::stream<bool> rawBlockFlagStream("rawBlockFlagStream");

#pragma HLS STREAM variable = intmDataStream depth = c_idataStreamDepth
#pragma HLS STREAM variable = rawBlockFlagStream depth = 32

#pragma HLS dataflow
    xf::compression::details::inputBufferMinBlock<BLOCK_SIZE, MIN_BLK_SIZE>(inStream, rawBlockFlagStream,
                                                                            intmDataStream);

    xf::compression::details::__inputDistributer<BLOCK_SIZE, CORE_COUNT>(intmDataStream, rawBlockFlagStream, outStream,
                                                                         outStrdStream, blockMetaStream);
}

template <int CORE_COUNT, int MAX_FREQ_DWIDTH>
void serializeLiterals(hls::stream<ap_uint<68> > litUpsizedStream[CORE_COUNT],
                       hls::stream<ap_uint<MAX_FREQ_DWIDTH> > litCntStream[CORE_COUNT],
                       hls::stream<ap_uint<68> >& outLitUpsizedStream,
                       hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& outLitCntStream) {
    bool allDone = false;
    ap_uint<68> litVal;
    uint8_t cIdx = 0;
srlz_lit_all_data:
    while (!allDone) {
    // get upsized literals from each core
    srlz_lit_all_cores:
        for (cIdx = 0; cIdx < CORE_COUNT; ++cIdx) {
            litVal = litUpsizedStream[cIdx].read();
            if (litVal.range(3, 0) == 0) { // break if first read is zero
                allDone = true;
                break;
            }
            // write first word (used for continuation or termination)
            outLitUpsizedStream << litVal;
            // get literal count
            auto litCnt = litCntStream[cIdx].read();
            outLitCntStream << litCnt;
        srlz_lit_loop:
            for (litVal = litUpsizedStream[cIdx].read(); litVal.range(3, 0) > 0;
                 litVal = litUpsizedStream[cIdx].read()) {
#pragma HLS PIPELINE II = 1
                outLitUpsizedStream << litVal;
            }
            // end of block
            outLitUpsizedStream << 0;
        }
    }
    // write strobe 0 to output
    outLitUpsizedStream << 0;
// dump strobe 0
srlz_lit_eos_dump:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
#pragma HLS PIPELINE II = 1
        if (i != cIdx) litUpsizedStream[i].read();
    }
}

template <int CORE_COUNT, int MAX_FREQ_DWIDTH>
void serializeLZData(hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> > seqStream[CORE_COUNT],
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > litFreqStream[CORE_COUNT],
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > seqFreqStream[CORE_COUNT],
                     hls::stream<bool> rleFlagStream[CORE_COUNT],
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> > lzMetaStream[CORE_COUNT],
                     hls::stream<DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> >& outSeqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outLitFreqStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outSeqFreqStream,
                     hls::stream<bool>& outRleFlagStream,
                     hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outLzMetaStream) {
    constexpr uint8_t c_seqFreqCnt = (details::c_maxCodeLL + details::c_maxCodeOF + details::c_maxCodeML) + 7;
    bool allDone = false;
    uint8_t cIdx = 0;
    DSVectorStream_dt<SequencePack<MAX_FREQ_DWIDTH, 8>, 1> seqVal;
    IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> litFreqVal, seqFreqVal, lzMetaVal;
    bool rleFlag;

srlz_lz_all_data:
    while (!allDone) {
    // get lz data from all cores and write to individual output streams
    srlz_lz_data_all_cores:
        for (cIdx = 0; cIdx < CORE_COUNT; ++cIdx) {
            seqVal = seqStream[cIdx].read();
            if (seqVal.strobe == 0) { // break if first read is zero
                allDone = true;
                break;
            } else {
#pragma HLS DATAFLOW
                // write sequences
                {
                srlz_lz_seq_loop:
                    for (; seqVal.strobe > 0; seqVal = seqStream[cIdx].read()) {
#pragma HLS PIPELINE II = 1
                        outSeqStream << seqVal;
                    }
                    // end of block, strobe 0
                    outSeqStream << seqVal;
                }
                // write rle flag
                {
                    rleFlag = rleFlagStream[cIdx].read();
                    outRleFlagStream << rleFlag;
                }
            // write lz meta values
            srlz_lz_meta_loop:
                for (uint8_t m = 0; m < 2; ++m) {
#pragma HLS PIPELINE II = 1
                    lzMetaVal = lzMetaStream[cIdx].read();
                    outLzMetaStream << lzMetaVal;
                }
            // write literal frequencies
            srlz_lz_lit_freq_loop:
                for (uint16_t k = 0; k < details::c_maxLitV + 1; ++k) {
#pragma HLS PIPELINE II = 1
                    litFreqVal = litFreqStream[cIdx].read();
                    outLitFreqStream << litFreqVal;
                }
            // write sequence frequencies
            srlz_lz_seq_freq_loop:
                for (uint8_t s = 0; s < c_seqFreqCnt; ++s) {
#pragma HLS PIPELINE II = 1
                    seqFreqVal = seqFreqStream[cIdx].read();
                    outSeqFreqStream << seqFreqVal;
                }
            }
        }
    }
    // write strobe 0 to output
    seqVal.strobe = 0;
    litFreqVal.strobe = 0;
    seqFreqVal.strobe = 0;
    lzMetaVal.strobe = 0;
    outSeqStream << seqVal;
    outLitFreqStream << litFreqVal;
    outSeqFreqStream << seqFreqVal;
    outLzMetaStream << lzMetaVal;
// dump strobe 0
srlz_lz_eos_dump:
    for (uint8_t i = 0; i < CORE_COUNT; ++i) {
#pragma HLS PIPELINE II = 1
        if (i != cIdx) {
            seqStream[i].read();
        }
        // read eos from all other streams
        litFreqStream[i].read();
        seqFreqStream[i].read();
        lzMetaStream[i].read();
    }
}

void streamCmpStrdFrame(hls::stream<ap_uint<68> >& inRawBStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& inCmpBStream,
                        hls::stream<ap_uint<2> >& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    IntVectorStream_dt<8, 8> outVal;
    ap_uint<24> strdBlockHeader = 1; // bit-0 = 1, indicating last block, bits 1-2 = 0, indicating raw block
stream_cmp_file:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
        // read frame content size
        outVal = inCmpBStream.read();
        strdBlockHeader.range(10, 3) = (uint8_t)outVal.data[0];
        strdBlockHeader.range(18, 11) = (uint8_t)outVal.data[1];
        strdBlockHeader.range(23, 19) = (uint8_t)outVal.data[2];
    // write the frame header, written for each block of input data as stated in its meta data
    // unsigned t = 0;
    write_frame_header:
        for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
        }
        if (isRawBlk) {
            // Write stored block header
            outVal.data[0] = strdBlockHeader.range(7, 0);
            outVal.data[1] = strdBlockHeader.range(15, 8);
            outVal.data[2] = strdBlockHeader.range(23, 16);
            outVal.strobe = 3;
            outStream << outVal;
        write_raw_blk_data:
            for (auto rbVal = inRawBStream.read(); rbVal.range(3, 0) > 0; rbVal = inRawBStream.read()) {
#pragma HLS PIPELINE II = 1
                for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS UNROLL
                    outVal.data[i] = rbVal.range((i * 8) + 11, (i * 8) + 4);
                }
                outVal.strobe = rbVal.range(3, 0);
                outStream << outVal;
            }
        } else {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
                outStream << outVal;
            }
        }
    }
    // dump last strobe 0
    inRawBStream.read();
    // end of file
    outVal.strobe = 0;
    outStream << outVal;
}

} // details

/**
 * @brief This module compresses the input file read from input stream using multiple lz77 modules.
 *        It produces the ZSTD compressed data at the output stream.
 *
 * @tparam CORE_COUNT Total number of lz77 cores
 * @tparam BLOCK_SIZE ZStd block size
 * @tparam LZWINDOW_SIZE LZ77 history size or Window size
 * @tparam MIN_BLCK_SIZE Minimum block size, less than that will be considered stored block
 * @tparam PARALLEL_HUFFMAN Number of Huffman encoding units used
 * @tparam MIN_MATCH Minimum match in LZ77
 *
 * @param inStream input stream
 * @param outStream output stream
 */
template <int CORE_COUNT, int BLOCK_SIZE, int LZWINDOW_SIZE, int MIN_BLCK_SIZE, int MIN_MATCH = 3>
void zstdCompressQuadCore(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                          hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
    // zstd compression main module
    constexpr uint8_t c_parallel_huffman = (CORE_COUNT < 4) ? 2 : (CORE_COUNT < 6 ? 4 : 8);
    constexpr uint32_t c_freq_dwidth = maxBitsUsed(BLOCK_SIZE);
    constexpr uint32_t c_dataUpSDepth = BLOCK_SIZE / 8;
    constexpr uint32_t c_seqInSDepth = BLOCK_SIZE / 4;
    constexpr uint32_t c_rawBlkSDepth = CORE_COUNT * BLOCK_SIZE / 8;
    constexpr uint32_t c_hfLitStreamDepth = BLOCK_SIZE / c_parallel_huffman;
    constexpr uint32_t c_seqBlockDepth = BLOCK_SIZE / 8;
    constexpr uint8_t c_wfreqSDepth = 24 * CORE_COUNT;

    // Internal streams
    hls::stream<ap_uint<68> > inBlockStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<8, 1> > inBlockDszStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<16, 1> > packerMetaStream("packerMetaStream");
    hls::stream<ap_uint<68> > rawBlockStream("rawBlockStream");
    hls::stream<ap_uint<68> > rawBlkFinalStream("rawBlkFinalStream");
#pragma HLS STREAM variable = inBlockStream depth = c_dataUpSDepth
#pragma HLS STREAM variable = inBlockDszStream depth = 16
#pragma HLS STREAM variable = packerMetaStream depth = 16
#pragma HLS STREAM variable = rawBlkFinalStream depth = 16
#pragma HLS STREAM variable = rawBlockStream depth = c_rawBlkSDepth
#pragma HLS BIND_STORAGE variable = rawBlockStream type = FIFO impl = URAM

    hls::stream<IntVectorStream_dt<8, 1> > litStream[CORE_COUNT];
    hls::stream<ap_uint<68> > litUpsizedStream[CORE_COUNT];
    hls::stream<ap_uint<68> > serialLitUpszStream("serialLitUpszStream");
    hls::stream<ap_uint<68> > reverseLitStream("reverseLitStream");
#pragma HLS STREAM variable = litStream depth = 64
#pragma HLS STREAM variable = reverseLitStream depth = 64
#pragma HLS STREAM variable = serialLitUpszStream depth = 8
#pragma HLS STREAM variable = litUpsizedStream depth = c_dataUpSDepth

    hls::stream<DSVectorStream_dt<details::SequencePack<c_freq_dwidth, 8>, 1> > seqStream[CORE_COUNT];
    hls::stream<DSVectorStream_dt<details::SequencePack<c_freq_dwidth, 8>, 1> > serialSeqStream("serialSeqStream");
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > reverseSeqStream("reverseSeqStream");
#pragma HLS STREAM variable = seqStream depth = c_seqInSDepth
#pragma HLS AGGREGATE variable = seqStream
#pragma HLS STREAM variable = serialSeqStream depth = 8
#pragma HLS STREAM variable = reverseSeqStream depth = 2048 // 4096
// 2-4K depth needed to keep reading input sequences even if previous block decoding waits for fse table generation
#pragma HLS BIND_STORAGE variable = reverseSeqStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = seqStream type = FIFO impl = BRAM

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > litFreqStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > seqFreqStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > serialLitFreqStream("serialLitFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > serialSeqFreqStream("serialSeqFreqStream");
#pragma HLS STREAM variable = litFreqStream depth = 16
#pragma HLS STREAM variable = seqFreqStream depth = 128
#pragma HLS STREAM variable = serialLitFreqStream depth = 8
#pragma HLS STREAM variable = serialSeqFreqStream depth = 8

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > wghtFreqStream("wghtFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > freqStream("freqStream");
#pragma HLS STREAM variable = wghtFreqStream depth = c_wfreqSDepth
#pragma HLS STREAM variable = freqStream depth = 128

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > lzMetaStream[CORE_COUNT];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > serialLzMetaStream("serialLzMetaStream");
    hls::stream<ap_uint<c_freq_dwidth> > bscLzMetaStream("bscLzMetaStream");
    hls::stream<bool> rleFlagStream[CORE_COUNT];
    hls::stream<bool> serialRleFlagStream("serialRleFlagStream");
    hls::stream<ap_uint<2> > rawBlockFlagStream("rawBlockFlagStream");
    hls::stream<ap_uint<2> > rwbFinalFlagStream1("rwbFinalFlagStream1");
    hls::stream<ap_uint<2> > rwbFinalFlagStream2("rwbFinalFlagStream2");
    hls::stream<ap_uint<c_freq_dwidth> > litCntStream[CORE_COUNT];
    hls::stream<ap_uint<c_freq_dwidth> > serialLitCntStream("serialLitCntStream");
#pragma HLS STREAM variable = lzMetaStream depth = 16
#pragma HLS STREAM variable = serialLzMetaStream depth = 16
#pragma HLS STREAM variable = bscLzMetaStream depth = 16
#pragma HLS STREAM variable = rleFlagStream depth = 4
#pragma HLS STREAM variable = serialRleFlagStream depth = 4
#pragma HLS STREAM variable = rawBlockFlagStream depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream1 depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream2 depth = 4
#pragma HLS STREAM variable = litCntStream depth = 8
#pragma HLS STREAM variable = serialLitCntStream depth = 8

    hls::stream<IntVectorStream_dt<8, 2> > fseHeaderStream("fseHeaderStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseLitTableStream("fseLitTableStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseSeqTableStream("fseSeqTableStream");
#pragma HLS STREAM variable = fseHeaderStream depth = 128
#pragma HLS STREAM variable = fseLitTableStream depth = 8
#pragma HLS STREAM variable = fseSeqTableStream depth = 8

    hls::stream<ap_uint<16> > hufLitMetaStream("hufLitMetaStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hufCodeStream;
    hls::stream<IntVectorStream_dt<4, 1> > hufWeightStream("hufWeightStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, c_parallel_huffman> > hfEncodedLitStream;
    hls::stream<ap_uint<c_parallel_huffman * details::c_maxZstdHfBits> > hfLitBitstream("hfLitBitstream");
    hls::stream<ap_uint<8> > hfMultiBlenStream("hfMultiBlenStream");
    hls::stream<IntVectorStream_dt<8, c_parallel_huffman> > hfEncBitStream("hfEncBitStream");
#pragma HLS STREAM variable = hufLitMetaStream depth = 256
#pragma HLS STREAM variable = hufCodeStream depth = 16
#pragma HLS STREAM variable = hufWeightStream depth = 16
#pragma HLS STREAM variable = hfEncodedLitStream depth = 16
#pragma HLS STREAM variable = hfLitBitstream depth = 16
#pragma HLS STREAM variable = hfMultiBlenStream depth = 16
#pragma HLS STREAM variable = hfEncBitStream depth = c_hfLitStreamDepth
#pragma HLS BIND_STORAGE variable = hfEncBitStream type = FIFO impl = BRAM

    hls::stream<IntVectorStream_dt<8, 2> > litEncodedStream("litEncodedStream");
    hls::stream<ap_uint<c_freq_dwidth> > seqEncSizeStream("seqEncSizeStream");
    hls::stream<IntVectorStream_dt<8, 8> > seqEncodedStream("seqEncodedStream");
#pragma HLS STREAM variable = litEncodedStream depth = 128
#pragma HLS STREAM variable = seqEncSizeStream depth = 4
#pragma HLS STREAM variable = seqEncodedStream depth = c_seqBlockDepth
    hls::stream<ap_uint<c_freq_dwidth> > bscMetaStream("bscMetaStream");
    hls::stream<IntVectorStream_dt<8, 8> > bscBitstream("bscBitstream");
    hls::stream<IntVectorStream_dt<8, 8> > cmpFrameStream("cmpFrameStream");
    hls::stream<IntVectorStream_dt<8, 8> > cmpFrameFinalStream("cmpFrameFinalStream");
#pragma HLS STREAM variable = bscMetaStream depth = 128
#pragma HLS STREAM variable = bscBitstream depth = 128
#pragma HLS STREAM variable = cmpFrameStream depth = 64
#pragma HLS STREAM variable = cmpFrameFinalStream depth = 16

    // select URAM vs BRAMs for streams
    if (BLOCK_SIZE < 32768) {
#pragma HLS BIND_STORAGE variable = inBlockStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = BRAM
#pragma HLS BIND_STORAGE variable = seqEncodedStream type = FIFO impl = BRAM
    } else {
#pragma HLS BIND_STORAGE variable = inBlockStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = seqEncodedStream type = FIFO impl = URAM
    }

#pragma HLS dataflow

    // Module-1: Input reading and LZ77 compression
    {
        details::inputDistributer<BLOCK_SIZE, MIN_BLCK_SIZE, CORE_COUNT>(inStream, inBlockStream, rawBlockStream,
                                                                         packerMetaStream);
        for (uint8_t coreIdx = 0; coreIdx < CORE_COUNT; ++coreIdx) {
#pragma HLS UNROLL
            details::bufferDownsizerVec<64, 8, 4>(inBlockStream[coreIdx], inBlockDszStream[coreIdx]);
        }
        // LZ77 compression of input blocks to get separate streams
        // for literals, sequences (litlen, metlen, offset), literal frequencies and sequences frequencies
        details::getLitSequences<0, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[0], litStream[0], seqStream[0],
                                                               litFreqStream[0], seqFreqStream[0], rleFlagStream[0],
                                                               lzMetaStream[0], litCntStream[0]);
        if (CORE_COUNT > 1) {
            details::getLitSequences<1, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[1], litStream[1], seqStream[1],
                                                                   litFreqStream[1], seqFreqStream[1], rleFlagStream[1],
                                                                   lzMetaStream[1], litCntStream[1]);
        }
        // Quad Core
        if (CORE_COUNT > 2) {
            details::getLitSequences<2, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[2], litStream[2], seqStream[2],
                                                                   litFreqStream[2], seqFreqStream[2], rleFlagStream[2],
                                                                   lzMetaStream[2], litCntStream[2]);
            details::getLitSequences<3, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[3], litStream[3], seqStream[3],
                                                                   litFreqStream[3], seqFreqStream[3], rleFlagStream[3],
                                                                   lzMetaStream[3], litCntStream[3]);
        }
        // Hexa Core
        if (CORE_COUNT > 4) {
            details::getLitSequences<4, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[4], litStream[4], seqStream[4],
                                                                   litFreqStream[4], seqFreqStream[4], rleFlagStream[4],
                                                                   lzMetaStream[4], litCntStream[4]);
            details::getLitSequences<5, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[5], litStream[5], seqStream[5],
                                                                   litFreqStream[5], seqFreqStream[5], rleFlagStream[5],
                                                                   lzMetaStream[5], litCntStream[5]);
        }

        // Octa Core
        if (CORE_COUNT > 6) {
            details::getLitSequences<6, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[6], litStream[6], seqStream[6],
                                                                   litFreqStream[6], seqFreqStream[6], rleFlagStream[6],
                                                                   lzMetaStream[6], litCntStream[6]);
            details::getLitSequences<7, BLOCK_SIZE, c_freq_dwidth>(inBlockDszStream[7], litStream[7], seqStream[7],
                                                                   litFreqStream[7], seqFreqStream[7], rleFlagStream[7],
                                                                   lzMetaStream[7], litCntStream[7]);
        }

        for (uint8_t coreIdx = 0; coreIdx < CORE_COUNT; ++coreIdx) {
#pragma HLS UNROLL
            // Upsize literals
            details::simpleStreamUpsizer<8, 64, 4>(litStream[coreIdx], litUpsizedStream[coreIdx]);
        }
        // serialize output literals, sequences and frequencies
        details::serializeLiterals<CORE_COUNT, c_freq_dwidth>(litUpsizedStream, litCntStream, serialLitUpszStream,
                                                              serialLitCntStream);
        details::serializeLZData<CORE_COUNT, c_freq_dwidth>(
            seqStream, litFreqStream, seqFreqStream, rleFlagStream, lzMetaStream, serialSeqStream, serialLitFreqStream,
            serialSeqFreqStream, serialRleFlagStream, serialLzMetaStream);
        // compress type detection
        details::zstdCompressionMeta<BLOCK_SIZE, c_freq_dwidth>(serialLzMetaStream, rawBlockFlagStream,
                                                                bscLzMetaStream);
        // skip raw block based on flags
        details::skipPassRawBlock<64, 4>(rawBlockStream, rawBlockFlagStream, rawBlkFinalStream, rwbFinalFlagStream1,
                                         rwbFinalFlagStream2);
    }
    // Module-2: Encoding table generation and data preparation
    {
        // Buffer, reverse and break input literal stream into 4 streams of 1/4th size
        details::reverseLitQuadStreams<BLOCK_SIZE, c_freq_dwidth>(serialLitUpszStream, serialLitCntStream,
                                                                  reverseLitStream);
        // Reverse sequences stream
        details::reverseSeq<BLOCK_SIZE, c_freq_dwidth, MIN_MATCH>(serialSeqStream, reverseSeqStream);
        // generate hufffman tree and get codes-bitlens
        zstdTreegenStream<c_freq_dwidth, details::c_maxZstdHfBits>(serialLitFreqStream, hufCodeStream, hufWeightStream,
                                                                   wghtFreqStream);
        // feed frequency data to fse table gen from literals and sequences
        details::frequencySequencer<c_freq_dwidth>(wghtFreqStream, serialSeqFreqStream, freqStream);
        // generate FSE Tables for litlen, matlen, offset and literal-bitlen
        details::fseTableGen(freqStream, fseHeaderStream, fseLitTableStream, fseSeqTableStream);
    }
    // Module-3: Encoding literal and sequences
    {
        // Huffman encoding of literal stream
        details::zstdHuffmanMultiEncoder<details::c_maxZstdHfBits, c_parallel_huffman>(
            reverseLitStream, serialRleFlagStream, hufCodeStream, hfEncodedLitStream, hufLitMetaStream);
        // Huffman bitstream packer
        details::zstdHuffMultiBitPacker<details::c_maxZstdHfBits, c_parallel_huffman>(
            hfEncodedLitStream, hfLitBitstream, hfMultiBlenStream);
        // packed bitstream alignment downsizer
        details::bitDownSizeByte<c_parallel_huffman * details::c_maxZstdHfBits, 8 * c_parallel_huffman, 8>(
            hfLitBitstream, hfMultiBlenStream, hfEncBitStream);
        // FSE encoding of literals
        details::fseEncodeLitHeader(hufWeightStream, fseLitTableStream, litEncodedStream);
        // FSE encode sequences generated by lz77 compression
        details::fseEncodeSequences(reverseSeqStream, fseSeqTableStream, seqEncodedStream, seqEncSizeStream);
    }
    // Module-4: Output block and frame packing
    {
        // collect data from different input byte streams and output 2 continuous streams
        details::bytestreamCollector<c_freq_dwidth, 8, c_parallel_huffman>(
            bscLzMetaStream, hufLitMetaStream, hfEncBitStream, fseHeaderStream, litEncodedStream, seqEncodedStream,
            seqEncSizeStream, bscMetaStream, bscBitstream);
        // pack compressed data into single sequential block stream
        details::packCompressedFrame<BLOCK_SIZE, MIN_BLCK_SIZE, c_freq_dwidth, 8>(packerMetaStream, bscMetaStream,
                                                                                  bscBitstream, cmpFrameStream);
        details::skipPassCmpBlock<8>(cmpFrameStream, rwbFinalFlagStream1, cmpFrameFinalStream);
        // Output compressed or raw block based on input flag stream
        details::streamCmpStrdFrame(rawBlkFinalStream, cmpFrameFinalStream, rwbFinalFlagStream2, outStream);
    }
}

/**
 * @brief This module is top level wrapper for zstd compression core module
 *        It compresses the input file read from input axi stream.
 *        It produces the ZSTD compressed data at the output axi stream.
 *
 * @tparam IN_DWIDTH Input stream data bit-width
 * @tparam OUT_DWIDTH Output stream data bit-width
 * @tparam BLOCK_SIZE ZStd block size
 * @tparam LZWINDOW_SIZE LZ77 history size or Window size
 * @tparam MIN_BLCK_SIZE Minimum block size, less than that will be considered stored block
 *
 * @param inStream input stream
 * @param outStream output stream
 */
template <int IN_DWIDTH, int OUT_DWIDTH, int BLOCK_SIZE, int LZWINDOW_SIZE, int MIN_BLCK_SIZE, int CORE_COUNT>
void zstdCompressMultiCoreStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                                    hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
    // Internal streams
    hls::stream<IntVectorStream_dt<8, IN_DWIDTH / 8> > inZstdStream("inZstdStream");
    hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> > outCompressedStream("outCompressedStream");
#pragma HLS STREAM variable = inZstdStream depth = 8
#pragma HLS STREAM variable = outCompressedStream depth = 8

#pragma HLS DATAFLOW
    // AXI 2 HLS Stream
    xf::compression::details::zstdAxiu2hlsStream<IN_DWIDTH>(inStream, inZstdStream);

    // Zstd Compress Stream IO Engine
    xf::compression::zstdCompressQuadCore<CORE_COUNT, BLOCK_SIZE, LZWINDOW_SIZE, MIN_BLCK_SIZE>(inZstdStream,
                                                                                                outCompressedStream);
    // HLS 2 AXI Stream
    xf::compression::details::zstdHlsVectorStream2axiu<OUT_DWIDTH>(outCompressedStream, outStream);
}

} // compression
} // xf
#endif
