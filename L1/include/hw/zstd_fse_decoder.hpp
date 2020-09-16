/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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

#ifndef _ZSTD_FSE_DECODER_HPP_
#define _ZSTD_FSE_DECODER_HPP_

/**
 * @file zstd_fse_decoder.hpp
 * @brief Header for modules used in ZSTD decompress kernel. This file contains
 * modules used for FSE and Huffman bitstream decoding and table generation.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include "hls_stream.h"
#include <ap_int.h>

#include "zstd_specs.hpp"

namespace xf {
namespace compression {
namespace details {

// uint8_t ccnt = 0;

template <uint8_t PARALLEL_BYTE>
int generateFSETable(uint32_t* table,
                     hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& inStream,
                     ap_uint<(8 * PARALLEL_BYTE * 2)>& fseAcc,
                     uint8_t& bytesInAcc,
                     uint8_t& tableLog,
                     uint16_t maxChar,
                     xfSymbolCompMode_t fseMode,
                     xfSymbolCompMode_t prevFseMode,
                     const int16_t* defDistTable,
                     int16_t* prevDistribution) {
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    uint32_t bytesAvailable = bytesInAcc;
    uint8_t bitsAvailable = bytesInAcc * 8;
    uint32_t bitsConsumed = 0;
    int totalBytesConsumed = 0;
    uint32_t totalBitsConsumed = 0;
    int remaining = 0;
    int threshold = 0;
    bool previous0 = false;
    uint16_t charnum = 0;
    int16_t normalizedCounter[64];
    // initialize normalized counter
    for (int i = 0; i < 64; ++i) normalizedCounter[i] = 0;

    if (fseMode == FSE_COMPRESSED_MODE) {
        // read PARALLEL_BYTE bytes
        if (bytesAvailable < PARALLEL_BYTE) {
            fseAcc.range((bitsAvailable + kStreamWidth - 1), bitsAvailable) = inStream.read();
            bytesAvailable += PARALLEL_BYTE;
            bitsAvailable += kStreamWidth;
        }
        uint8_t accuracyLog = (fseAcc & 0xF) + 5;
        tableLog = accuracyLog;
        fseAcc >>= 4;
        bitsAvailable -= 4;
        totalBitsConsumed = 4;

        remaining = (1 << accuracyLog) + 1;
        threshold = 1 << accuracyLog;
        accuracyLog += 1;
    genNormDistTable:
        while ((remaining > 1) & (charnum <= maxChar)) { /*TODO: II=1, merge loop*/
#pragma HLS PIPELINE II = 1
            bitsConsumed = 0;
            // read PARALLEL_BYTE bytes
            if (bytesAvailable < PARALLEL_BYTE - 1) {
                fseAcc.range((bitsAvailable + kStreamWidth - 1), bitsAvailable) = inStream.read();
                bytesAvailable += PARALLEL_BYTE;
                bitsAvailable += kStreamWidth;
            }
            if (previous0) {
                unsigned n0 = 0;
                if ((fseAcc & 0xFFFF) == 0xFFFF) {
                    n0 += 24;
                    bitsConsumed += 16;
                } else if ((fseAcc & 3) == 3) {
                    n0 += 3;
                    bitsConsumed += 2;
                } else {
                    n0 += fseAcc & 3;
                    bitsConsumed += 2;
                    previous0 = false;
                }
                charnum += n0;
            } else {
                int16_t max = (2 * threshold - 1) - remaining;
                int count;
                uint8_t c1 = 0;
                if ((fseAcc & (threshold - 1)) < max) {
                    c1 = 1;
                    count = fseAcc & (threshold - 1);
                    bitsConsumed += accuracyLog - 1;
                } else {
                    c1 = 2;
                    count = fseAcc & (2 * threshold - 1);
                    if (count >= threshold) count -= max;
                    bitsConsumed += accuracyLog;
                }

                --count;                                     /* extra accuracy */
                remaining -= ((count < 0) ? -count : count); /* -1 means +1 */
                normalizedCounter[charnum++] = count;
                previous0 = ((count == 0) ? 1 : 0);
            genDTableIntLoop:
                while (remaining < threshold) {
                    --accuracyLog;
                    threshold >>= 1;
                }
            }
            fseAcc >>= bitsConsumed;
            bitsAvailable -= bitsConsumed;
            totalBitsConsumed += bitsConsumed;

            bytesAvailable = bitsAvailable >> 3;
        }
        totalBytesConsumed += (totalBitsConsumed + 7) >> 3;
        bytesInAcc = bytesAvailable;
        // clear accumulator, so that it is byte aligned
        fseAcc >>= (bitsAvailable & 7);
        // copy to prevDistribution table
        for (int i = 0; i < 64; ++i) prevDistribution[i] = normalizedCounter[i];
    } else if (fseMode == PREDEFINED_MODE) { /*TODO: use fixed table directly*/
        for (int i = 0; i < maxChar + 1; ++i) {
            normalizedCounter[i] = defDistTable[i];
            prevDistribution[i] = defDistTable[i];
        }
        charnum = maxChar + 1;
    } else if (fseMode == RLE_MODE) {
        // next state for entire table is 0
        // accuracy log is 0
        // read PARALLEL_BYTE bytes
        if (bytesAvailable < 1) {
            fseAcc.range((bitsAvailable + kStreamWidth - 1), bitsAvailable) = inStream.read();
            bytesAvailable += PARALLEL_BYTE;
            bitsAvailable += kStreamWidth;
        }
        uint8_t symbol = (uint8_t)fseAcc;
        fseAcc >>= 8;
        if (bytesAvailable > 0) {
            bitsAvailable -= 8;
            --bytesAvailable;
        }
        table[0] = symbol;
        tableLog = 0;

        return 1;
    } else if (fseMode == REPEAT_MODE) { /*TODO: use previous table directly*/
        // check if previous mode was RLE
        if (prevFseMode == RLE_MODE) {
            return 0;
        }
        for (int i = 0; i < 64; ++i) normalizedCounter[i] = prevDistribution[i];
    } else {
        // else -> Error: invalid fse mode
        return 0;
    }

    // Table Generation
    const uint32_t tableSize = 1 << tableLog;
    uint32_t highThreshold = tableSize - 1;
    uint16_t symbolNext[64];
    // initialize table
    for (int i = 0; i < tableSize; ++i) table[i] = 0;

    for (uint16_t i = 0; i < charnum; i++) {
#pragma HLS PIPELINE II = 1
        if (normalizedCounter[i] == -1) {
            table[highThreshold] = i; // symbol, assign lower 8-bits
            --highThreshold;
            symbolNext[i] = 1;
        } else {
            symbolNext[i] = normalizedCounter[i];
        }
    }

    uint32_t mask = tableSize - 1;
    const uint32_t step = (tableSize >> 1) + (tableSize >> 3) + 3;
    uint32_t pos = 0;
    for (uint16_t i = 0; i < charnum; ++i) {
        for (int j = 0; j < normalizedCounter[i]; ++j) {
#pragma HLS PIPELINE II = 1
            table[pos] = i;
            pos = (pos + step) & mask;
            while (pos > highThreshold) pos = (pos + step) & mask;
        }
    }
    // FSE table
    for (uint32_t i = 0; i < tableSize; i++) {
#pragma HLS PIPELINE II = 1
        uint8_t symbol = (uint8_t)table[i];
        uint16_t nextState = symbolNext[symbol]++;
        uint16_t nBits = (uint8_t)(tableLog - (31 - __builtin_clz(nextState)));
        table[i] = (table[i] & 0xFFFF0000) + ((uint32_t)nBits << 8) + symbol;
        table[i] = (table[i] & 0x0000FFFF) | ((uint32_t)((nextState << nBits) - tableSize) << 16);
    }

    return totalBytesConsumed;
}

template <uint8_t PARALLEL_BYTE, uint8_t BLOCK_SIZE_KB, uint8_t BSWIDTH>
void fseStreamStates(uint32_t* litFSETable,
                     uint32_t* oftFSETable,
                     uint32_t* mlnFSETable,
                     ap_uint<BSWIDTH>* bitStream,
                     int byteIndx,
                     uint8_t lastByteValidBits,
                     uint32_t seqCnt,
                     uint8_t* accuracyLog,
                     hls::stream<uint8_t>& symbolStream,
                     hls::stream<ap_uint<32> >& bsWordStream,
                     hls::stream<ap_uint<5> >& extraBitStream) {
    // fetch fse states from fse sequence bitstream and stream out for further processing
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    const uint8_t kbsBytes = BSWIDTH / 8;

    uint16_t fseStateLL, fseStateOF, fseStateML; // literal_length, offset, match_length states
    FseBSState bsStateLL, bsStateOF, bsStateML;  // offset, match_length and literal_length
    ap_uint<kAccRegWidth> acchbs;
    uint8_t bitsInAcc = lastByteValidBits;
    uint8_t bitsToRead = 0;
    uint8_t bytesToRead = 0;

    acchbs.range(BSWIDTH - 1, 0) = bitStream[byteIndx--];
    uint8_t byte_0 = acchbs.range(bitsInAcc - 1, bitsInAcc - 8);
// find valid last bit, bitstream read in reverse order
fsedseq_skip_zero:
    for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
        if ((byte_0 & (1 << i)) > 0) {
            --bitsInAcc;
            break;
        }
        --bitsInAcc; // discount higher bits which are zero
    }

fsedseq_fill_acc:
    while ((bitsInAcc + BSWIDTH < kAccRegWidth) && (byteIndx > -1)) {
#pragma HLS PIPELINE II = 1
        acchbs <<= BSWIDTH;
        acchbs.range(BSWIDTH - 1, 0) = bitStream[byteIndx--];
        bitsInAcc += BSWIDTH;
    }
    // Read literal_length, offset and match_length states from input stream
    // get *accuracyLog bits from higher position in accuracyLog, mask out higher scrap bits
    uint64_t mskLL = ((1 << accuracyLog[0]) - 1);
    fseStateLL = ((acchbs >> (bitsInAcc - accuracyLog[0])) & mskLL);
    uint64_t mskOF = ((1 << accuracyLog[1]) - 1);
    fseStateOF = ((acchbs >> (bitsInAcc - (accuracyLog[0] + accuracyLog[1]))) & mskOF);
    uint64_t mskML = ((1 << accuracyLog[2]) - 1);
    fseStateML = ((acchbs >> (bitsInAcc - (accuracyLog[0] + accuracyLog[1] + accuracyLog[2]))) & mskML);

    bitsInAcc -= (accuracyLog[0] + accuracyLog[1] + accuracyLog[2]);

    enum FSEDState_t { LITLEN = 0, MATLEN, OFFSET, NEXTSTATE };
    FSEDState_t smState = OFFSET;
    uint8_t bitCntLML, bitCntLMO;

decode_sequence_bitStream:
    while (seqCnt) {
#pragma HLS PIPELINE II = 1
        // read data to bitstream if necessary
        if (bitsInAcc < kStreamWidth && byteIndx > -1) {
            auto tmp = bitStream[byteIndx--];
            acchbs = (acchbs << BSWIDTH) + tmp;
            bitsInAcc += BSWIDTH;
        }
        uint32_t stateVal;
        uint8_t extraBits;
        // get state values and stream
        // stream fse metadata to decoding unit
        if (smState == LITLEN) {
            stateVal = litFSETable[fseStateLL]; // literal_length
            bsStateLL.symbol = stateVal & 0x000000FF;
            bsStateLL.nextState = (stateVal >> 16) & 0x0000FFFF;
            bsStateLL.bitCount = (stateVal >> 8) & 0x000000FF;

            extraBits = kLLExtraBits[bsStateLL.symbol]; // max 16-bits
            // if(maxExtBits < extraBits) maxExtBits = extraBits;
            bitsInAcc -= extraBits;
            symbolStream << bsStateLL.symbol;
            bsWordStream << (uint32_t)(acchbs >> bitsInAcc);
            extraBitStream << extraBits;

            bitCntLML += bsStateLL.bitCount;
            bitCntLMO += bsStateLL.bitCount;
            smState = NEXTSTATE;
        } else if (smState == MATLEN) {
            stateVal = mlnFSETable[fseStateML]; // match_length
            bsStateML.symbol = stateVal & 0x000000FF;
            bsStateML.nextState = (stateVal >> 16) & 0x0000FFFF;
            bsStateML.bitCount = (stateVal >> 8) & 0x000000FF;

            extraBits = kMLExtraBits[bsStateML.symbol]; // max 16-bits
            // if(maxExtBits < extraBits) maxExtBits = extraBits;
            bitsInAcc -= extraBits;
            symbolStream << bsStateML.symbol;
            bsWordStream << (uint32_t)(acchbs >> bitsInAcc);
            extraBitStream << extraBits;

            bitCntLML = bsStateML.bitCount;
            bitCntLMO += bsStateML.bitCount;
            smState = LITLEN;
        } else if (smState == OFFSET) {
            // get the state codes for offset, match_length and literal_length
            stateVal = oftFSETable[fseStateOF]; // offset
            bsStateOF.symbol = stateVal & 0x000000FF;
            bsStateOF.nextState = (stateVal >> 16) & 0x0000FFFF;
            bsStateOF.bitCount = (stateVal >> 8) & 0x000000FF;

            extraBits = bsStateOF.symbol; // also represents extra bits to be read, max 31-bits
            // if(maxExtBits < extraBits) maxExtBits = extraBits;
            bitsInAcc -= extraBits;
            symbolStream << bsStateOF.symbol;
            bsWordStream << (uint32_t)(acchbs >> bitsInAcc);

            bitCntLMO = bsStateOF.bitCount;
            smState = MATLEN;
        } else {
            // update state for next sequence
            // read bits to get states for literal_length, match_length, offset
            // accumulator must contain these many bits can be max 26-bits
            mskLL = (((uint64_t)1 << bsStateLL.bitCount) - 1);
            fseStateLL = ((acchbs >> (bitsInAcc - bsStateLL.bitCount)) & mskLL);
            fseStateLL += bsStateLL.nextState;

            mskML = (((uint64_t)1 << bsStateML.bitCount) - 1);
            fseStateML = ((acchbs >> (bitsInAcc - bitCntLML)) & mskML);
            fseStateML += bsStateML.nextState;

            mskOF = (((uint64_t)1 << bsStateOF.bitCount) - 1);
            fseStateOF = ((acchbs >> (bitsInAcc - bitCntLMO)) & mskOF);
            fseStateOF += bsStateOF.nextState;

            bitsInAcc -= bitCntLMO;

            --seqCnt;
            smState = OFFSET;
        }
    }
}

void fseDecodeStates(hls::stream<uint8_t>& symbolStream,
                     hls::stream<ap_uint<32> >& bsWordStream,
                     hls::stream<ap_uint<5> >& extraBitStream,
                     uint32_t seqCnt,
                     uint32_t litCnt,
                     uint16_t* prevOffsets,
                     hls::stream<uint16_t>& litLenStream,
                     hls::stream<ap_uint<16> >& offsetStream,
                     hls::stream<uint16_t>& matLenStream,
                     hls::stream<bool>& litlenValidStream) {
    // calculate literal length, match length and offset values, stream them for sequence execution
    enum FSEDecode_t { LITLEN = 0, MATLEN, OFFSET_CALC, OFFSET_WNU };
    FSEDecode_t sqdState = OFFSET_CALC;
    uint16_t offsetVal;
    uint16_t litLenCode;
    uint8_t ofi;
    bool checkLL = false;
decode_sequence_codes:
    while (seqCnt) {
#pragma HLS PIPELINE II = 1
        if (sqdState == OFFSET_CALC) {
            // calculate offset and set prev offsets
            auto symbol = symbolStream.read();
            auto bsWord = bsWordStream.read();
            auto extBit = symbol;
            uint32_t extVal = bsWord & (((uint64_t)1 << extBit) - 1);
            offsetVal = (1 << symbol) + extVal;

            ofi = 3;
            if (offsetVal > 3) {
                offsetVal -= 3;
                checkLL = false;
            } else {
                checkLL = true;
            }
            sqdState = MATLEN;
        } else if (sqdState == MATLEN) {
            // calculate match length
            auto symbol = symbolStream.read();
            auto bsWord = bsWordStream.read();
            auto extBit = extraBitStream.read();
            uint16_t extVal = (bsWord & (((uint64_t)1 << extBit) - 1));
            uint16_t matchLenCode = kMLBase[symbol] + extVal;
            matLenStream << matchLenCode;

            sqdState = LITLEN;
        } else if (sqdState == LITLEN) {
            // calculate literal length
            auto symbol = symbolStream.read();
            auto bsWord = bsWordStream.read();
            auto extBit = extraBitStream.read();
            uint16_t extVal = (bsWord & (((uint64_t)1 << extBit) - 1));
            litLenCode = kLLBase[symbol] + extVal;
            litlenValidStream << 1;
            litLenStream << litLenCode;
            litCnt -= litLenCode;

            // update offset as per literal length
            if (checkLL) {
                // repeat offsets 1 - 3
                if (litLenCode == 0) {
                    if (offsetVal == 3) {
                        offsetVal = prevOffsets[0] - 1;
                        ofi = 2;
                    } else {
                        ofi = offsetVal;
                        offsetVal = prevOffsets[offsetVal];
                    }
                } else {
                    ofi = offsetVal - 1;
                    offsetVal = prevOffsets[offsetVal - 1];
                }
            }
            checkLL = false;
            sqdState = OFFSET_WNU;
        } else {
            // OFFSET_WNU: write offset and update previous offsets
            offsetStream << offsetVal;
            // shift previous offsets
            auto prev1 = prevOffsets[1];
            if (ofi > 1) {
                prevOffsets[2] = prev1;
            }
            if (ofi > 0) {
                prevOffsets[1] = prevOffsets[0];
                prevOffsets[0] = offsetVal;
            }

            sqdState = OFFSET_CALC;
            --seqCnt;
        }
    }
    if (litCnt > 0) {
        litlenValidStream << 1;
        litLenStream << litCnt;
        matLenStream << 0;
        offsetStream << 0;
    }
}

template <uint8_t PARALLEL_BYTE, uint8_t BLOCK_SIZE_KB, uint8_t BSWIDTH>
void decodeSeqCore(uint32_t* litFSETable,
                   uint32_t* oftFSETable,
                   uint32_t* mlnFSETable,
                   ap_uint<BSWIDTH>* bitStream,
                   int bsIndx,
                   uint8_t lastByteValidBits,
                   uint32_t seqCnt,
                   uint32_t litCnt,
                   uint8_t* accuracyLog,
                   uint16_t* prevOffsets,
                   hls::stream<uint16_t>& litLenStream,
                   hls::stream<ap_uint<16> >& offsetStream,
                   hls::stream<uint16_t>& matLenStream,
                   hls::stream<bool>& litlenValidStream) {
    // core module for decoding fse sequences
    // Internal streams
    hls::stream<uint8_t> symbolStream("symbolStream");
    hls::stream<ap_uint<32> > bsWordStream("bsWordStream");
    hls::stream<ap_uint<5> > extraBitStream("extraBitStream");

#pragma HLS STREAM variable = symbolStream depth = 16
#pragma HLS STREAM variable = bsWordStream depth = 16
#pragma HLS STREAM variable = extraBitStream depth = 16

#pragma HLS dataflow

    fseStreamStates<PARALLEL_BYTE, BLOCK_SIZE_KB, BSWIDTH>(litFSETable, oftFSETable, mlnFSETable, bitStream, bsIndx,
                                                           lastByteValidBits, seqCnt, accuracyLog, symbolStream,
                                                           bsWordStream, extraBitStream);

    fseDecodeStates(symbolStream, bsWordStream, extraBitStream, seqCnt, litCnt, prevOffsets, litLenStream, offsetStream,
                    matLenStream, litlenValidStream);
}

template <uint8_t PARALLEL_BYTE, uint8_t BLOCK_SIZE_KB>
inline void fseDecode(ap_uint<64> accword,
                      uint8_t bytesInAcc,
                      hls::stream<ap_uint<8 * PARALLEL_BYTE> >& inStream,
                      uint32_t* litFSETable,
                      uint32_t* oftFSETable,
                      uint32_t* mlnFSETable,
                      uint32_t seqCnt,
                      uint32_t litCount,
                      uint32_t remBlockSize,
                      uint8_t* accuracyLog,
                      uint16_t* prevOffsets,
                      hls::stream<uint16_t>& litLenStream,
                      hls::stream<ap_uint<16> >& offsetStream,
                      hls::stream<uint16_t>& matLenStream,
                      hls::stream<bool>& litlenValidStream) {
    // decode fse encoded bitstream using prebuilt fse tables
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    const uint16_t kBSWidth = kStreamWidth;
    const uint8_t kbsBytes = kBSWidth / 8;

    ap_uint<kBSWidth> bitStream[(BLOCK_SIZE_KB * 1024) / kbsBytes];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_2p impl = bram

    uint8_t bitsInAcc = bytesInAcc * 8;

    // copy data from bitstream to buffer
    ap_uint<kAccRegWidth> bsbuff = accword;
    int bsIdx = 0;
    uint8_t bitsWritten = kBSWidth;
// write block data
fsedseq_fill_bitstream:
    for (int i = 0; i < remBlockSize; i += kbsBytes) {
#pragma HLS pipeline II = 1
        if (i + bytesInAcc < remBlockSize) {
            if (bytesInAcc < kbsBytes) {
                bsbuff.range(bitsInAcc + kStreamWidth - 1, bitsInAcc) = inStream.read();
                bitsInAcc += kStreamWidth;
            }
        }

        bitStream[bsIdx++] = bsbuff.range(kBSWidth - 1, 0);

        if (i + kbsBytes > remBlockSize) bitsWritten = 8 * (remBlockSize - i);
        bsbuff >>= bitsWritten;
        bitsInAcc -= bitsWritten;
        bytesInAcc = bitsInAcc >> 3;
    }
    decodeSeqCore<PARALLEL_BYTE, BLOCK_SIZE_KB, kBSWidth>(litFSETable, oftFSETable, mlnFSETable, bitStream, bsIdx - 1,
                                                          bitsWritten, seqCnt, litCount, accuracyLog, prevOffsets,
                                                          litLenStream, offsetStream, matLenStream, litlenValidStream);
}

template <uint8_t PARALLEL_BYTE>
void fseDecodeHuffWeight(hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& inStream,
                         uint32_t remSize,
                         ap_uint<(8 * PARALLEL_BYTE * 2)>& accHuff,
                         uint8_t& bytesInAcc,
                         uint8_t accuracyLog,
                         uint32_t* fseTable,
                         uint8_t* weights,
                         uint16_t& weightCnt,
                         uint8_t& huffDecoderTableLog) {
    //#pragma HLS INLINE
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    uint8_t bitsInAcc = bytesInAcc * 8;

    uint8_t bitStream[128];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_2p impl = bram
    int bsByteIndx = remSize - 1;

    // copy data from bitstream to buffer
    uint32_t itrCnt = 1 + ((remSize - 1) / PARALLEL_BYTE);
    uint32_t k = 0;
fseDecHF_read_input:
    for (uint32_t i = 0; i < itrCnt; ++i) {
        accHuff.range(bitsInAcc + kStreamWidth - 1, bitsInAcc) = inStream.read();
        bitsInAcc += kStreamWidth;
        for (uint8_t j = 0; j < PARALLEL_BYTE && k < remSize; ++j, ++k) {
#pragma HLS PIPELINE II = 1
            bitStream[k] = accHuff.range(7, 0);
            accHuff >>= 8;
            bitsInAcc -= 8;
        }
    }
    bytesInAcc = bitsInAcc >> 3;

    // decode FSE bitStream using fse table to get huffman weights
    // skip initial 0 bits and single 1 bit
    uint32_t accState;
    uint32_t codeIdx = 0;
    uint8_t bitsToRead = 0;
    bitsInAcc = 0;
    int32_t rembits = remSize * 8;

    uint8_t fseState[2];
    FseBSState bsState[2];
    // find beginning of the stream
    accState = bitStream[bsByteIndx--];
    bitsInAcc = 8;
    for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
        if ((accState & (1 << i)) > 0) {
            --bitsInAcc;
            break;
        }
        --bitsInAcc; // discount higher bits which are zero
    }
    rembits -= (8 - bitsInAcc);

    // Read bits needed for first two states
    bitsToRead = accuracyLog * 2;
    if (bitsToRead > bitsInAcc) {
        uint8_t bytesToRead = 1 + ((bitsToRead - bitsInAcc - 1) / 8);
        for (uint8_t i = 0; i < bytesToRead; ++i) {
            uint8_t tmp = bitStream[bsByteIndx--];
            accState <<= 8;
            accState += tmp;
            bitsInAcc += 8;
        }
    }
    // Read initial state1 and state2
    // get readBits bits from higher position in accuracyLog, mask out higher scrap bits
    // read state 1
    bitsInAcc -= accuracyLog;
    uint64_t msk = ((1 << accuracyLog) - 1);
    fseState[0] = ((accState >> bitsInAcc) & msk);
    // read state 2
    bitsInAcc -= accuracyLog;
    msk = ((1 << accuracyLog) - 1);
    fseState[1] = ((accState >> bitsInAcc) & msk);
    rembits -= (accuracyLog * 2);

    bool stateIdx = 0; // 0 for even, 1 for odd
    bool overflow = false;
    uint32_t totalWeights = 0;
fse_decode_huff_weights:
    while (1) {
#pragma HLS PIPELINE II = 1
        // get the weight, bitCount and nextState
        uint32_t stateVal = fseTable[fseState[stateIdx]];
        uint8_t cw = (uint8_t)(stateVal & 0xFF);
        weights[codeIdx++] = cw;
        totalWeights += (1 << cw) >> 1;

        // overflow
        if (rembits < 0) break;

        // get other values
        bsState[stateIdx].nextState = (stateVal >> 16) & 0x0000FFFF;
        bsState[stateIdx].bitCount = (stateVal >> 8) & 0x000000FF;
        uint8_t bitsToRead = bsState[stateIdx].bitCount;
        if (bitsToRead > bitsInAcc) {
            uint8_t tmp = 0;
            if (bsByteIndx > -1) {
                // max 1 read is required, since accuracy log <= 6
                tmp = bitStream[bsByteIndx--];
            }
            accState <<= 8;
            accState += tmp;
            bitsInAcc += 8;
        }
        // get next fse state
        bitsInAcc -= bitsToRead;
        uint8_t msk = ((1 << bitsToRead) - 1);
        fseState[stateIdx] = ((accState >> bitsInAcc) & msk);
        fseState[stateIdx] += bsState[stateIdx].nextState;
        rembits -= bitsToRead;

        // switch state flow
        stateIdx = (stateIdx + 1) & 1; // 0 if 1, 1 if 0
    }
    huffDecoderTableLog = 1 + (31 - __builtin_clz(totalWeights));
    // add last weight
    uint16_t lw = (1 << huffDecoderTableLog) - totalWeights;
    weights[codeIdx++] = 1 + (31 - __builtin_clz(lw));
    weightCnt = codeIdx;
}

template <int MAX_CODELEN>
void huffGenLookupTable(uint8_t* weights, HuffmanTable* huffTable, uint8_t accuracyLog, uint16_t weightCnt) {
    // create huffman lookup table
    // regenerate huffman tree using literal bit-lengths
    typedef ap_uint<MAX_CODELEN + 1> LCL_Code_t;
    LCL_Code_t first_codeword[MAX_CODELEN + 1];
    ap_uint<32> bitlen_histogram[MAX_CODELEN + 1];
    ap_uint<4> bitlens[256];
#pragma HLS ARRAY_PARTITION variable = first_codeword complete
#pragma HLS ARRAY_PARTITION variable = bitlen_histogram complete

    uint16_t codes[256];
// initialize first_codeword and bitlength histogram
hflkpt_init_blen_hist:
    for (uint8_t i = 0; i < MAX_CODELEN + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = MAX_CODELEN max = MAX_CODELEN
        bitlen_histogram[i] = 0;
    }
// read bit-lengths
hflkpt_fill_blen_hist:
    for (uint16_t i = 0; i < weightCnt; ++i) {
#pragma HLS PIPELINE II = 1
        // convert weight to bitlen
        uint8_t cblen = weights[i];
        bitlen_histogram[cblen]++;
        if (cblen > 0) cblen = (accuracyLog + 1 - cblen);
        bitlens[i] = cblen;
    }

    // generate first codes
    first_codeword[0] = bitlen_histogram[0];

    uint16_t nextCode = 0;
hflkpt_initial_codegen:
    for (uint8_t i = 1; i < accuracyLog + 1; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 8
        uint16_t cur = nextCode;
        nextCode += (bitlen_histogram[i] << (i - 1));
        first_codeword[i] = cur;
    }

hflkpt_codegen_outer:
    for (int i = 0; i < weightCnt; ++i) {
        uint32_t hfw = weights[i];
        const uint32_t len = (1 << hfw) >> 1;
        const auto fcw = first_codeword[hfw];
    hflkpt_codegen:
        for (uint16_t u = fcw; u < fcw + len; ++u) {
#pragma HLS PIPELINE II = 1
            huffTable[u].symbol = i;
            huffTable[u].bitlen = bitlens[i];
        }
        first_codeword[hfw] = fcw + len;
    }
}

template <uint8_t PARALLEL_BYTE>
void huffDecodeLiterals(hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& inStream,
                        bool quadStream,
                        ap_uint<(8 * PARALLEL_BYTE * 2)> accHuff,
                        uint8_t bytesInAcc,
                        uint32_t remSize,
                        uint32_t regeneratedSize,
                        uint8_t accuracyLog,
                        uint16_t weightCnt,
                        uint8_t* weights,
                        hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& literalStream) {
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kBSWidth = 16;
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    const uint16_t kAccRegWidthx3 = kStreamWidth * 3;
    const uint16_t kMaxCodeLen = 11;
    // huffman lookup table
    HuffmanTable huffTable[2048];
#pragma HLS BIND_STORAGE variable = huffTable type = ram_2p impl = bram

    ap_uint<kBSWidth> bitStream[16 * 1024];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_2p impl = bram
    uint16_t decSize[4];
    uint16_t cmpSize[4];
#pragma HLS ARRAY_PARTITION variable = cmpSize complete
#pragma HLS ARRAY_PARTITION variable = decSize complete
    uint8_t streamCnt = 1;

    // get stream sizes if 4 streams are present
    if (quadStream) {
        streamCnt = 4;
        // Jump table is 6 bytes long
        // read from input if needed
        if (bytesInAcc < PARALLEL_BYTE) {
            accHuff.range(((PARALLEL_BYTE + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += PARALLEL_BYTE;
        }
        // use 4 bytes
        // get decompressed size
        uint32_t dcmpSize = (regeneratedSize + 3) / 4;
        decSize[0] = decSize[1] = decSize[2] = dcmpSize;
        decSize[3] = regeneratedSize - (dcmpSize * 3);

        // get compressed size
        cmpSize[0] = accHuff;
        accHuff >>= 16;
        cmpSize[1] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 4;
        // read from input if needed
        if (bytesInAcc < 2) {
            accHuff.range(((PARALLEL_BYTE + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += PARALLEL_BYTE;
        }
        cmpSize[2] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 2;

        cmpSize[3] = remSize - (6 + cmpSize[0] + cmpSize[1] + cmpSize[2]);
    } else {
        decSize[0] = regeneratedSize;
        cmpSize[0] = remSize;
    }
    // generate huffman lookup table
    huffGenLookupTable<kMaxCodeLen>(weights, huffTable, accuracyLog, weightCnt);

    // decode bitstreams
    ap_uint<(8 * PARALLEL_BYTE)> outBuffer;
    uint8_t obbytes = 0;
    ap_uint<kAccRegWidth> bsbuff = accHuff;
    uint8_t bitsInAcc = bytesInAcc * 8;

    ap_uint<kStreamWidth> bsacc[kMaxCodeLen + 1];
#pragma HLS ARRAY_PARTITION variable = bsacc complete

decode_huff_bitstream_outer:
    for (uint8_t si = 0; si < streamCnt; ++si) {
        // copy data from bitstream to buffer
        uint32_t bsIdx = 0;
        uint8_t bitsWritten = kBSWidth;
        const int bsPB = kBSWidth / 8;
        int sIdx = 0;
    // write block data
    hufdlit_fill_bitstream:
        for (int i = 0; i < cmpSize[si]; i += bsPB) {
#pragma HLS PIPELINE II = 1
            if (i + bytesInAcc < cmpSize[si] && bytesInAcc < bsPB) {
                bsbuff.range(((bytesInAcc + PARALLEL_BYTE) * 8) - 1, bytesInAcc * 8) = inStream.read();
                bitsInAcc += kStreamWidth;
            }

            bitStream[bsIdx++] = bsbuff.range(kBSWidth - 1, 0);

            if (i + bsPB > cmpSize[si]) bitsWritten = 8 * (cmpSize[si] - i);
            bsbuff >>= bitsWritten;
            bitsInAcc -= bitsWritten;
            bytesInAcc = bitsInAcc >> 3;
        }

        // generate decompressed bytes from huffman encoded stream
        ap_uint<kStreamWidth> acchbs = 0;
        uint8_t bitcnt = 0;
        int byteIndx = bsIdx - 1;
        uint32_t outBytes = 0;

        acchbs.range(kBSWidth - 1, 0) = bitStream[byteIndx--];
        bitcnt = bitsWritten;
        uint8_t byte_0 = acchbs.range(bitcnt - 1, bitcnt - 8);
    // find valid last bit, bitstream read in reverse order
    hufdlit_skip_zero:
        for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
            if ((byte_0 & (1 << i)) > 0) {
                --bitcnt;
                break;
            }
            --bitcnt; // discount higher bits which are zero
        }
        // shift to higher end
        acchbs <<= (kStreamWidth - bitcnt);
        const int msbBitCnt = kStreamWidth - kBSWidth;
        uint8_t shiftCnt = kStreamWidth - accuracyLog;
        uint8_t sym, blen = 0;
    // decode huffman bitstream
    huf_dec_bitstream:
        while (outBytes < decSize[si]) {
#pragma HLS PIPELINE II = 1
            // fill the acchbs in reverse
            if (bitcnt < 16 && byteIndx > -1) {
                uint32_t tmp = bitStream[byteIndx--];
                acchbs += tmp << (msbBitCnt - bitcnt);
                bitcnt += kBSWidth;
            }

            uint16_t code = acchbs >> shiftCnt;

            sym = huffTable[code].symbol;
            blen = huffTable[code].bitlen;

        hfdbs_shift_acc:
            for (int s = 1; s < kMaxCodeLen + 1; ++s) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = kMaxCodeLen max = kMaxCodeLen
                bsacc[s] = acchbs << s;
            }
            bitcnt -= blen;
            acchbs = bsacc[blen];

            // write the literal to output stream
            outBuffer.range(((obbytes + 1) * 8) - 1, obbytes * 8) = sym;
            ++obbytes;
            if (obbytes == PARALLEL_BYTE) {
                literalStream << outBuffer;
                obbytes = 0;
                outBuffer = 0;
            }
            ++outBytes;
        }
    }
    if (obbytes > 0) {
        literalStream << outBuffer;
    }
}

template <uint8_t PARALLEL_BYTE, int BS_WIDTH>
void hfdDataFeader(hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& inStream,
                   uint8_t streamCnt,
                   uint16_t* cmpSize,
                   ap_uint<(8 * PARALLEL_BYTE * 2)> accHuff,
                   uint8_t bytesInAcc,
                   hls::stream<ap_uint<BS_WIDTH> >& huffBitStream,
                   hls::stream<ap_uint<8> >& validBitCntStream) {
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kBSWidth = BS_WIDTH;
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    const int kbsPB = kBSWidth / 8;
    const int bsUpperLim = (32 / kbsPB) * 1024;

    ap_uint<kBSWidth> bitStream[bsUpperLim];
#pragma HLS BIND_STORAGE variable = bitStream type = ram_2p impl = bram

    // internal registers
    ap_uint<kAccRegWidth> bsbuff = accHuff; // must not contain more than 3 bytes
    uint8_t bitsInAcc = bytesInAcc * 8;

    int wIdx = 0;
    int rIdx = 0;
    int fmInc = 1; // can be +1 or -1
    int smInc = 1;
    uint8_t bitsWritten = kBSWidth;
    int streamRBgn[4];     // starting index for BRAM read
    int streamRLim[4 + 1]; // point/index till which the BRAM can be read, 1 extra buffer entry
#pragma HLS ARRAY_PARTITION variable = streamRBgn complete
#pragma HLS ARRAY_PARTITION variable = streamRLim complete
    uint8_t wsi = 0, rsi = 0;
    int inIdx = 0;
    // modes
    bool fetchMode = 1;
    bool streamMode = 0;
    bool done = 0;
    // initialize
    streamRLim[wsi] = 0; // initial stream will stream from higher to lower address
hfdl_dataStreamer:
    while (!done) {
#pragma HLS PIPELINE II = 1
        // stream data, bitStream buffer width is equal to inStream width for simplicity
        if (fetchMode) {
            // fill bitstream in direction specified by increment variable
            if (inIdx + bytesInAcc < cmpSize[wsi] && bytesInAcc < kbsPB) {
                bsbuff.range(bitsInAcc + kStreamWidth - 1, bitsInAcc) = inStream.read();
                bitsInAcc += kStreamWidth;
            }
            bitStream[wIdx] = bsbuff.range(kBSWidth - 1, 0);
            if (inIdx + kbsPB >= cmpSize[wsi]) {
                auto bw = 8 * (cmpSize[wsi] - inIdx);
                bitsWritten = (bw == 0) ? bitsWritten : bw;
                validBitCntStream << bitsWritten;
                bsbuff >>= bitsWritten;
                bitsInAcc -= bitsWritten;
                bytesInAcc = bitsInAcc >> 3;

                // update fetch mode state
                if (!streamMode) {
                    streamMode = 1;
                    rIdx = wIdx;
                }
                inIdx = 0;            // just an index, not directional
                fmInc = (~fmInc) + 1; // flip 1 and -1
                streamRBgn[wsi] = wIdx;
                ++wsi;
                if (wsi & 1) {
                    streamRLim[wsi] = bsUpperLim - 1;
                    wIdx = bsUpperLim - 1;
                } else {
                    streamRLim[wsi] = 0;
                    wIdx = 0;
                }
                // post increment checks
                if ((wsi == streamCnt) || (wsi - rsi > 1)) fetchMode = 0;
                // reset default value
                bitsWritten = kBSWidth;
                continue;
            } else {
                bsbuff >>= bitsWritten;
                bitsInAcc -= bitsWritten;
                bytesInAcc = bitsInAcc >> 3;

                inIdx += kbsPB;
                wIdx += fmInc;
            }
        }
        if (streamMode) {
            // write data to output stream
            uint32_t tmp = bitStream[rIdx];
            huffBitStream << tmp;
            // update stream mode state
            if (rIdx == streamRLim[rsi]) {
                ++rsi;
                rIdx = streamRBgn[rsi];
                smInc = (~smInc) + 1; // flip 1 and -1
                // no need to check if fetchMode == 0
                if (wsi < streamCnt) fetchMode = 1;
                // either previous streamMode ended quicker than next fetchMode or streamCnt reached
                if (wsi == rsi) streamMode = 0;
            } else {
                rIdx -= smInc;
            }
        }
        // end condition
        if (!(fetchMode | streamMode)) done = 1;
    }
}

template <uint8_t PARALLEL_BYTE, int MAX_CODELEN, int BS_WIDTH>
void hfdGetCodesStreamLiterals(uint16_t* cmpSize,
                               uint16_t* decSize,
                               uint8_t accuracyLog,
                               uint8_t streamCnt,
                               uint16_t weightCnt,
                               uint8_t* weights,
                               hls::stream<ap_uint<BS_WIDTH> >& huffBitStream,
                               hls::stream<ap_uint<8> >& validBitCntStream,
                               hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& literalStream) {
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    const uint16_t kBSWidth = BS_WIDTH;
    const int kbsPB = kBSWidth / 8;
    // huffman lookup tables
    HuffmanTable huffTable0[2048];
#pragma HLS BIND_STORAGE variable = huffTable0 type = ram_2p impl = bram

    ap_uint<kStreamWidth> bsacc[MAX_CODELEN + 1];
#pragma HLS ARRAY_PARTITION variable = bsacc complete

    // generate huffman lookup table
    huffGenLookupTable<MAX_CODELEN>(weights, huffTable0, accuracyLog, weightCnt);

    ap_uint<kStreamWidth> outBuffer;
    uint8_t obbytes = 0;
decode_huff_bitstream_outer:
    for (uint8_t si = 0; si < streamCnt; ++si) {
        // generate decompressed bytes from huffman encoded stream
        ap_uint<kStreamWidth> acchbs = 0;
        uint8_t bitcnt = 0;
        uint32_t outBytes = 0;

        bitcnt = validBitCntStream.read();
        acchbs.range(kBSWidth - 1, 0) = huffBitStream.read();
        uint8_t byte_0 = acchbs.range(bitcnt - 1, bitcnt - 8);
    // find valid last bit, bitstream read in reverse order
    hufdlit_skip_zero:
        for (uint8_t i = 7; i >= 0; --i) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 7
            if ((byte_0 & (1 << i)) > 0) {
                --bitcnt;
                break;
            }
            --bitcnt; // discount higher bits which are zero
        }
        // shift to higher end
        acchbs <<= (kStreamWidth - bitcnt);
        uint16_t byteIndx = kbsPB; // just to verify with the compressed size
        const int msbBitCnt = kStreamWidth - kBSWidth;
        const uint8_t shiftCnt = kStreamWidth - accuracyLog;
        uint8_t shiftCnt_mn = 16 - accuracyLog;
        uint8_t sym, blen = 0;
    // decode huffman bitstreams
    huf_dec_bitstream:
        while (outBytes < decSize[si]) {
#pragma HLS PIPELINE II = 1
            if (bitcnt < 16 && byteIndx < cmpSize[si]) {
                uint32_t tmp = huffBitStream.read();
                acchbs += tmp << (msbBitCnt - bitcnt);
                bitcnt += kBSWidth;
                byteIndx += kbsPB;
            }
            uint16_t code = acchbs >> shiftCnt;
            auto lkVal = huffTable0[code];

        hfdbs_shift_acc:
            for (int s = 1; s < MAX_CODELEN + 1; ++s) {
#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min = MAX_CODELEN max = MAX_CODELEN
                bsacc[s] = acchbs << s;
            }
            bitcnt -= lkVal.bitlen;
            acchbs = bsacc[lkVal.bitlen];

            // write the literal to output stream
            outBuffer.range(((obbytes + 1) * 8) - 1, obbytes * 8) = lkVal.symbol;
            ++obbytes;
            if (obbytes == PARALLEL_BYTE) {
                literalStream << outBuffer;
                obbytes = 0;
                outBuffer = 0;
            }
            ++outBytes;
        }
    }
    if (obbytes > 0) {
        literalStream << outBuffer;
    }
}

template <uint8_t PARALLEL_BYTE, int MAX_CODELEN>
void huffDecodeLitInternal(hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& inStream,
                           ap_uint<(8 * PARALLEL_BYTE * 2)> accHuff,
                           uint8_t bytesInAcc,
                           uint16_t* cmpSize,
                           uint16_t* decSize,
                           uint8_t accuracyLog,
                           uint8_t streamCnt,
                           uint16_t weightCnt,
                           uint8_t* weights,
                           hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& literalStream) {
    const uint16_t kBSWidth = 16;
    // internal streams
    hls::stream<ap_uint<kBSWidth> > huffBitStream("huffBitStream");
    hls::stream<ap_uint<8> > validBitCntStream("validBitCntStream");
#pragma HLS STREAM variable = huffBitStream depth = 16
#pragma HLS STREAM variable = validByteCntStream depth = 8

#pragma HLS DATAFLOW

    hfdDataFeader<PARALLEL_BYTE, kBSWidth>(inStream, streamCnt, cmpSize, accHuff, bytesInAcc, huffBitStream,
                                           validBitCntStream);

    hfdGetCodesStreamLiterals<PARALLEL_BYTE, MAX_CODELEN, kBSWidth>(
        cmpSize, decSize, accuracyLog, streamCnt, weightCnt, weights, huffBitStream, validBitCntStream, literalStream);
}

template <uint8_t PARALLEL_BYTE>
void huffDecodeLiteralsStreaming(hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& inStream,
                                 bool quadStream,
                                 ap_uint<(8 * PARALLEL_BYTE * 2)> accHuff,
                                 uint8_t bytesInAcc,
                                 uint32_t remSize,
                                 uint32_t regeneratedSize,
                                 uint8_t accuracyLog,
                                 uint16_t weightCnt,
                                 uint8_t* weights,
                                 hls::stream<ap_uint<(8 * PARALLEL_BYTE)> >& literalStream) {
    const uint16_t kStreamWidth = (8 * PARALLEL_BYTE);
    const uint16_t kAccRegWidth = kStreamWidth * 2;
    const uint16_t kAccRegWidthx3 = kStreamWidth * 3;
    const uint16_t kMaxCodeLen = 11;
    uint8_t streamCnt = 1;
    uint16_t decSize[4];
    uint16_t cmpSize[4];
#pragma HLS ARRAY_PARTITION variable = decSize complete
#pragma HLS ARRAY_PARTITION variable = cmpSize complete
    // get stream sizes if 4 streams are present
    if (quadStream) {
        streamCnt = 4;
        // Jump table is 6 bytes long
        // read from input if needed
        if (bytesInAcc < PARALLEL_BYTE) {
            accHuff.range(((PARALLEL_BYTE + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += PARALLEL_BYTE;
        }
        // use 4 bytes
        // get decompressed size
        uint32_t dcmpSize = (regeneratedSize + 3) / 4;
        decSize[0] = decSize[1] = decSize[2] = dcmpSize;
        decSize[3] = regeneratedSize - (dcmpSize * 3);

        // get compressed size
        cmpSize[0] = accHuff;
        accHuff >>= 16;
        cmpSize[1] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 4;
        // read from input if needed
        if (bytesInAcc < 2) {
            accHuff.range(((PARALLEL_BYTE + bytesInAcc) * 8) - 1, bytesInAcc * 8) = inStream.read();
            bytesInAcc += PARALLEL_BYTE;
        }
        cmpSize[2] = accHuff;
        accHuff >>= 16;
        bytesInAcc -= 2;

        remSize -= 6;
        cmpSize[3] = remSize - (cmpSize[0] + cmpSize[1] + cmpSize[2]);
    } else {
        decSize[0] = regeneratedSize;
        cmpSize[0] = remSize;
    }
    // parallel huffman decoding
    huffDecodeLitInternal<PARALLEL_BYTE, kMaxCodeLen>(inStream, accHuff, bytesInAcc, cmpSize, decSize, accuracyLog,
                                                      streamCnt, weightCnt, weights, literalStream);
}

} // details
} // compression
} // xf

#endif // _ZSTD_FSE_DECODER_HPP_
