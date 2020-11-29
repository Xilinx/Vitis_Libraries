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
#ifndef _XFCOMPRESSION_ZSTD_DECOMPRESS_HPP_
#define _XFCOMPRESSION_ZSTD_DECOMPRESS_HPP_

/**
 * @file zstd_decompress.hpp
 * @brief Header for modules used in ZSTD decompress kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include <ap_int.h>
#include <stdint.h>

#include "zstd_fse_decoder.hpp"
#include "zstd_specs.hpp"
#include "lz_decompress.hpp"

namespace xf {
namespace compression {
namespace details {

template <int PARALLEL_BYTE, int LMO_WIDTH>
void alignLiterals(hls::stream<bool>& litlenValidStream,
                   hls::stream<bool>& newBlockFlagStream,
                   hls::stream<ap_uint<8 * PARALLEL_BYTE> >& inLitStream,
                   hls::stream<ap_uint<LMO_WIDTH> >& inLitLenStream,
                   hls::stream<ap_uint<8 * PARALLEL_BYTE> >& litStream,
                   hls::stream<ap_uint<LMO_WIDTH> >& litLenStream) {
    // align literals to PARALLEL_BYTE boundary as per literal length values
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
align_block_lit_output:
    while (newBlockFlagStream.read()) {
        ap_uint<c_accRegWidth> litbuf = 0;
        uint8_t bytesInAcc = 0;
        uint8_t bitsInAcc = 0;
    align_literals:
        while (litlenValidStream.read()) {
            ap_uint<LMO_WIDTH> litLen = inLitLenStream.read();
            litLenStream << litLen;
            if (litLen > 0) {
                // if present, write literals aligned to PARALLEL_BYTE
                uint8_t bytesWritten = 0;
                uint8_t updBInAcc = bytesInAcc;
                uint8_t bitsInAcc = bytesInAcc * 8;
            aligned_literal_write:
                for (int i = 0; i < litLen; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                    if (i < (const int)(litLen - bytesInAcc)) {
                        litbuf.range(bitsInAcc + c_streamWidth - 1, bitsInAcc) = inLitStream.read();
                        updBInAcc += PARALLEL_BYTE;
                    }
                    litStream << litbuf.range(c_streamWidth - 1, 0);

                    if (i > (const int)(litLen - PARALLEL_BYTE)) {
                        bytesWritten = litLen - i;
                        break;
                    }
                    litbuf >>= c_streamWidth;
                    updBInAcc -= PARALLEL_BYTE;
                }
                litbuf >>= (bytesWritten * 8);
                bytesInAcc = updBInAcc - bytesWritten;
            }
        }
    }
    litLenStream << 0;
}

template <int PARALLEL_BYTE, int BLOCK_SIZE_KB, int LMO_WIDTH>
void decodeSequence(hls::stream<bool>& seqValidStream,
                    hls::stream<uint32_t>& seqMetaStream,
                    hls::stream<uint32_t>& fseTableStream,
                    hls::stream<ap_uint<8 * PARALLEL_BYTE> >& seqDecodeInStream,
                    hls::stream<ap_uint<LMO_WIDTH> >& litLenStream,
                    hls::stream<ap_uint<LMO_WIDTH> >& offsetStream,
                    hls::stream<ap_uint<LMO_WIDTH> >& matLenStream,
                    hls::stream<bool>& litlenValidStream,
                    hls::stream<bool>& newBlockFlagStream) {
    // decode sequences and output literal lengths, offsets and match lengths
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    uint32_t fseTableLL[512];
#pragma HLS BIND_STORAGE variable = fseTableLL type = ram_t2p impl = bram
    uint32_t fseTableOF[256];
#pragma HLS BIND_STORAGE variable = fseTableOF type = ram_t2p impl = bram
    uint32_t fseTableML[512];
#pragma HLS BIND_STORAGE variable = fseTableML type = ram_t2p impl = bram
    ap_uint<LMO_WIDTH> prevOffsets[3] = {1, 4, 8}; // list of previous 3 offsets
#pragma HLS ARRAY_PARTITION variable = prevOffsets complete

decodeSequence_main_loop:
    while (seqValidStream.read()) {
        uint32_t seqMeta = seqMetaStream.read();
        xfBlockType_t blockType = (xfBlockType_t)(seqMeta & 0x000000FF);
        uint32_t blockSize = seqMeta >> 8;
        newBlockFlagStream << 1;
        if (blockType == RLE_BLOCK) {
            litlenValidStream << 1;
            litLenStream << 1;
            offsetStream << 0;
            matLenStream << blockSize - 1;
        } else if (blockType == RAW_BLOCK) {
            litlenValidStream << 1;
            litLenStream << blockSize;
            offsetStream << 0;
            matLenStream << 0;
        } else {
            // write sequence meta data
            // Word 0 <blockMeta> ***already decoded
            // Word 1 <decode_flag> 1 if further decoding needed, else 0
            // Word 2 <literalCount>
            // Word 3 <symbolCompressionMode><remBlockSize>
            //              1 byte              3 bytes
            // Word 4 <seqCnt>
            // Word 5 <AccuracyLogs> --> <litlen><offset><matlen> 1 byte each lower-higher
            // decode fse bitstream
            uint8_t decode_flag = seqMetaStream.read();
            uint32_t litCount = seqMetaStream.read();
            if (decode_flag) {
            // read all fse tables
            seqd_read_fse_tables_outer:
                for (int fti = 0; fti < 3; ++fti) {
                    uint16_t fseVCnt = fseTableStream.read();
                    if (fseVCnt != 0xFFFFFFFF) {
                    seqd_read_fse_tables:
                        for (uint16_t i = 0; i < fseVCnt; ++i) {
#pragma HLS PIPELINE II = 1
                            auto tmpv = fseTableStream.read();
                            if (fti == 0) {
                                fseTableLL[i] = tmpv;
                            } else if (fti == 1) {
                                fseTableOF[i] = tmpv;
                            } else {
                                fseTableML[i] = tmpv;
                            }
                        }
                    }
                }

                uint32_t smbuf = seqMetaStream.read();
                uint32_t remBlockSize = smbuf >> 8;
                uint32_t seqCnt = seqMetaStream.read();
                uint32_t aclbuf = seqMetaStream.read(); // accuracy logs
                uint8_t accuracyLog[3] = {(uint8_t)aclbuf, (uint8_t)(aclbuf >> 8), (uint8_t)(aclbuf >> 16)};
                // Decode the FSE compressed bitstream
                ap_uint<64> acw = 0;
                fseDecode<PARALLEL_BYTE, BLOCK_SIZE_KB, LMO_WIDTH>(
                    acw, 0, seqDecodeInStream, fseTableLL, fseTableOF, fseTableML, seqCnt, litCount, remBlockSize,
                    accuracyLog, prevOffsets, litLenStream, offsetStream, matLenStream, litlenValidStream);
            } else {
                // zero sequence count condition
                litlenValidStream << 1;
                litLenStream << litCount;
                offsetStream << 0;
                matLenStream << 0;
                // update previous offsets
                prevOffsets[2] = prevOffsets[1];
                prevOffsets[1] = prevOffsets[0];
                prevOffsets[0] = 0;
            }
        }
        // clear LZ literal buffer
        litlenValidStream << 0;
    }
    // end of file
    newBlockFlagStream << 0;
    matLenStream << 0;
    offsetStream << 0;
}

template <int PARALLEL_BYTE, int BLOCK_SIZE_KB>
void decodeLiterals(hls::stream<bool>& litValidStream,
                    hls::stream<uint32_t>& litMetaStream,
                    hls::stream<uint32_t>& fseTableStream,
                    hls::stream<ap_uint<8 * PARALLEL_BYTE> >& litDecodeInStream,
                    hls::stream<ap_uint<8 * PARALLEL_BYTE> >& literalStream) {
    // decode literals and forward to literal stream
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    uint8_t hufWeights[256];
    uint32_t litFseTable[256];

    uint8_t huffDecoderTableLog = 6;
    uint16_t wCnt = 0;

decodeLiterals_main_loop:
    while (litValidStream.read()) {
        // Literal metadata format
        // Word 0 is block metadata Word 0
        uint32_t litMeta = litMetaStream.read();
        xfBlockType_t blockType = (xfBlockType_t)(litMeta & 0x000000FF);
        uint32_t blockSize = litMeta >> 8;
        // for raw and RLE Blocks forward as it is
        uint32_t lbtWrite = blockSize;
        if (blockType == RLE_BLOCK) lbtWrite = 1;

        if (blockType == RLE_BLOCK || blockType == RAW_BLOCK) {
        // write complete block data as it is
        decodelit_write_rawrle_block_data:
            for (uint32_t i = 0; i < lbtWrite; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                ap_uint<c_streamWidth> lit = litDecodeInStream.read();
                literalStream << lit;
            }
        } else {
            bool quadStream = false;
            xfLitBlockType_t litBlockType;
            uint32_t regeneratedSize;
            uint32_t compressedSize = 0;

            // literal metadata
            // Word 1 <litBlockType><quadStream><regeneratedSize>
            //           7-bits         1-bit       3 bytes
            litMeta = litMetaStream.read();
            litBlockType = (xfLitBlockType_t)((uint8_t)litMeta & 3);
            if (litMeta & 0x00000080) quadStream = true;
            regeneratedSize = litMeta >> 8;
            // write literals
            uint32_t litSize2Write = regeneratedSize;
            if (litBlockType == RLE_LBLOCK) {
                // pass the RLE literal
                ap_uint<c_streamWidth> tbuf = litDecodeInStream.read();
                uint8_t rleLit = tbuf;
            decodelit_prep_rlelit_buff:
                for (uint8_t i = 1; i < PARALLEL_BYTE; ++i) {
#pragma HLS UNROLL
                    tbuf.range(((i + 1) * 8) - 1, i * 8) = rleLit;
                }
            decodelit_write_rlelit_data:
                for (uint32_t i = 0; i < litSize2Write; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                    literalStream << tbuf;
                }
            } else if (litBlockType == RAW_LBLOCK) {
            // pass the raw block
            decodelit_write_rawlit_data:
                for (int i = 0; i < litSize2Write; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                    ap_uint<c_streamWidth> lit = litDecodeInStream.read();
                    literalStream << lit;
                }
            } else {
                // Literal metadata
                // Word 2 <huffmanHeader><compressedSize>
                //           1 byte         3 bytes
                litMeta = litMetaStream.read();
                uint8_t hufHeader = (uint8_t)litMeta;
                uint8_t remBytesInFse = 0;
                compressedSize = litMeta >> 8;
                litSize2Write = compressedSize;

                ap_uint<c_accRegWidth> accHuff = 0;
                uint8_t bytesInAcc = 0;
                uint32_t remBytes = 0;
                if (litBlockType == CMP_LBLOCK) {
                    uint8_t accuracyLog = 6;
                    uint16_t fwCnt = 0;
                    // init huffman weights
                    for (int i = 0; i < 256; i += 2) {
                        hufWeights[i] = 0;
                        hufWeights[i + 1] = 0;
                    }
                    if (hufHeader < 128) {
                        // read FSE table
                        uint32_t tblVCnt = fseTableStream.read();
                    lit_read_fse_table:
                        for (int i = 0; i < tblVCnt; ++i) {
#pragma HLS PIPELINE II = 1
                            litFseTable[i] = fseTableStream.read();
                        }
                        // Word 3 <accuracyLog><byteUsedByFSE>
                        //          1 byte          1 byte
                        litMeta = litMetaStream.read();
                        accuracyLog = (uint8_t)litMeta;
                        remBytesInFse = hufHeader - (litMeta >> 8);
                        // decode fse bitstream of huffman weights
                        fseDecodeHuffWeight<PARALLEL_BYTE>(litDecodeInStream, remBytesInFse, accHuff, bytesInAcc,
                                                           accuracyLog, litFseTable, hufWeights, fwCnt,
                                                           huffDecoderTableLog);
                        remBytes = compressedSize - hufHeader - 1;
                        wCnt = fwCnt;
                    } else {
                        // 4-bit huffman weights
                        uint16_t hwCnt = hufHeader - 127;
                        uint32_t totalWeights = 0;
                        if (bytesInAcc == 0) {
                            accHuff.range(c_streamWidth - 1, 0) = litDecodeInStream.read();
                            bytesInAcc = PARALLEL_BYTE;
                        }
                    huffman_decode_weights:
                        for (uint8_t i = 0; i < hwCnt; i += 2) {
#pragma HLS PIPELINE II = 1
                            if (bytesInAcc == 0) {
                                accHuff.range(c_streamWidth - 1, 0) = litDecodeInStream.read();
                                bytesInAcc = PARALLEL_BYTE;
                            }
                            uint8_t w8t = accHuff;
                            accHuff >>= 8;
                            --bytesInAcc;
                            // get huffman weights
                            uint8_t hfw0 = (w8t >> 4);
                            uint8_t hfw1 = (w8t & 0x0F);
                            hufWeights[i] = hfw0;
                            hufWeights[i + 1] = hfw1;
                            // sum weights
                            totalWeights += ((((uint16_t)1 << hfw0) >> 1) + (((uint16_t)1 << hfw1) >> 1));
                        }
                        remBytes = compressedSize - (1 + (hwCnt - 1) / 2) - 1;
                        // last weight calculation
                        huffDecoderTableLog = 1 + (31 - __builtin_clz(totalWeights));
                        // add last weight
                        uint16_t lw = (1 << huffDecoderTableLog) - totalWeights;
                        hufWeights[hwCnt++] = 1 + (31 - __builtin_clz(lw));
                        wCnt = hwCnt;
                    }
                } else {
                    remBytes = compressedSize;
                }
                huffDecodeLiterals<PARALLEL_BYTE>(litDecodeInStream, quadStream, accHuff, bytesInAcc, remBytes,
                                                  regeneratedSize, huffDecoderTableLog, wCnt, hufWeights,
                                                  literalStream);
            }
        }
    }
}

template <int PARALLEL_BYTE>
void parseBlockGenFSETable(hls::stream<bool>& blockValidStream,
                           hls::stream<uint32_t>& blockMetaStream,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& zstdInStream,
                           hls::stream<bool>& litValidStream,
                           hls::stream<uint32_t>& litMetaStream,
                           hls::stream<uint32_t>& fseTableLitStream,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& litDecodeInStream,
                           hls::stream<bool>& seqValidStream,
                           hls::stream<uint32_t>& seqMetaStream,
                           hls::stream<uint32_t>& fseTableSeqStream,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& seqDecodeInStream) {
    // parse blocks and decompress them
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_accRegWidth = c_streamWidth * 2;
    const uint16_t c_accRegWidthx3 = c_streamWidth * 3;

    uint8_t defDistOffsets[3] = {0, c_maxCharLit + 1, c_maxCharLit + c_maxCharDefOffset + 2};
    uint8_t maxCharLOM[3] = {c_maxCharLit, c_maxCharOffset, c_maxCharMatchlen};
    int16_t prevDistribution[3 * 64];
    uint8_t prevAccLog[3] = {6, 5, 6};
    xfSymbolCompMode_t prevFseMode[3] = {FSE_COMPRESSED_MODE, FSE_COMPRESSED_MODE, FSE_COMPRESSED_MODE};

parseBlock_main_loop:
    while (blockValidStream.read()) {
        // parse valid block and generate FSE tables for them
        xfBlockType_t blockType;
        uint32_t blockMeta = blockMetaStream.read(); // <blockType 8-bits><blockSize 24-bits>

        blockType = (xfBlockType_t)(blockMeta & 0x000000FF);
        uint32_t blockSize = blockMeta >> 8;
        // enable literal and decoding
        litValidStream << 1;
        seqValidStream << 1;

        // write common literal and sequence meta data
        litMetaStream << blockMeta;
        seqMetaStream << blockMeta;
        // write literal data
        if (blockType == RLE_BLOCK) {
            // write RLE byte
            ap_uint<c_streamWidth> tbuf = zstdInStream.read();
            tbuf &= (ap_uint<c_streamWidth>)(((uint64_t)1 << 8) - 1);
            litDecodeInStream << tbuf;
        } else if (blockType == RAW_BLOCK) {
        // write complete block data as it is
        block_write_rawblocklit_data:
            for (uint32_t i = 0; i < blockSize; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                ap_uint<c_streamWidth> tbuf = zstdInStream.read();
                litDecodeInStream << tbuf;
            }
        } else {
            // decompress zstd compressed blocks
            /*
             * Zstd compressed block contains Literals and Sequences sections
             *
             * Literals Section:
             *     <Literals_Section_Header><Huffman_Tree_Description><Jump_Table><Stream1><Stream2><Stream3><Stream4>
             * Sequences Section:
             *     <Sequences_Section_Header><Literals_Length_Table><Offset_Table><Match_Length_Table><bitStream>
             */
            uint32_t regeneratedSize = 0; // literal count
            uint32_t compressedSize = 0;
            bool quadStream;
            uint32_t remBlockSize = blockSize; // block header is not included in block size

            /*
             * Literal_Section_Header
             * <Literals_Block_Type><Size_Format><Regenerated_Size><Compressed_Size>
             *          2 bits              1-2 bits         5-20 bits            0-18 bits
             */
            // read a word
            ap_uint<c_accRegWidth> accRegister = zstdInStream.read();
            uint8_t bytesInAcc = PARALLEL_BYTE;
            uint8_t bitsInAcc = c_streamWidth;

            // Get Literals_Block_Type
            xfLitBlockType_t litBlockType = (xfLitBlockType_t)((uint8_t)accRegister & 3);
            accRegister >>= 2;
            bitsInAcc -= 2;

            // Read size format
            uint8_t sizeFormat = ((uint8_t)accRegister & 3);
            uint8_t regSizeBits = 0;
            bool getCompSize;
            uint8_t ebn = 0; // extra bytes needed
            // read regenerated and compressed size
            getCompSize = (litBlockType == CMP_LBLOCK || litBlockType == TREELESS_LBLOCK);
            if (getCompSize) {
                accRegister >>= 2;
                bitsInAcc -= 2;
                ebn = 2;
                switch (sizeFormat) {
                    case 0:
                        regSizeBits = 10;
                        quadStream = false;
                        break;
                    case 1:
                        regSizeBits = 10;
                        quadStream = true;
                        break;
                    case 2:
                        regSizeBits = 14;
                        quadStream = true;
                        ++ebn;
                        break;
                    case 3:
                        regSizeBits = 18;
                        quadStream = true;
                        ebn += 2;
                        break;
                }
            } else {
                // get bits used by Regenerated_Size
                if ((sizeFormat & 1) == 0) {
                    regSizeBits = 5;
                    accRegister >>= 1;
                    bitsInAcc -= 1;
                } else {
                    accRegister >>= 2;
                    bitsInAcc -= 2;
                    if (sizeFormat == 1) {
                        regSizeBits = 12;
                        ++ebn;
                    } else { // remaining option is 3
                        regSizeBits = 20;
                        ebn += 2;
                    }
                }
                quadStream = false;
            }

            // Read extra bytes
            if (bitsInAcc < ((1 + ebn) * 8)) { // if less than required bits
                accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                bitsInAcc += c_streamWidth;
            }

            // read regenerated size
            regeneratedSize = (accRegister & (((uint32_t)1 << regSizeBits) - 1));
            accRegister >>= regSizeBits;
            bitsInAcc -= regSizeBits;
            // read compressed size
            if (getCompSize) {
                compressedSize = (accRegister & (((uint32_t)1 << regSizeBits) - 1));
                accRegister >>= regSizeBits;
                bitsInAcc -= regSizeBits;
            }
            // will be byte aligned compulsorily
            bytesInAcc = bitsInAcc >> 3;
            remBlockSize -= (ebn + 1);

            // Literal metadata format
            // Word 0 is already written, which is default
            // Word 1 <litBlockType><quadStream><regeneratedSize>
            //           7-bits         1-bit       3 bytes
            // Word 2 <huffmanHeader><compressedSize>
            //           1 byte         3 bytes
            // FSETables if present
            uint32_t lmbuf = (uint8_t)litBlockType;
            if (quadStream) lmbuf += (1 << 7);
            lmbuf += (regeneratedSize << 8);
            litMetaStream << lmbuf; // write word 1

            // write literals
            uint32_t litSize2Write = regeneratedSize;
            bool noTree = false;
            switch (litBlockType) {
                case RLE_LBLOCK: {
                    uint8_t rleLit;
                    if (bytesInAcc == 0) {
                        accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                        bytesInAcc += PARALLEL_BYTE;
                    }
                    rleLit = ((uint8_t)accRegister & 0XFF);
                    accRegister >>= 8;
                    --bytesInAcc;
                    ap_uint<c_streamWidth> rlbuf = rleLit;
                    litDecodeInStream << rlbuf;

                    bitsInAcc = bytesInAcc * 8;
                    remBlockSize -= 1;
                    break;
                }
                case TREELESS_LBLOCK: {
                    noTree = true;
                }
                case CMP_LBLOCK: {
                    litSize2Write = compressedSize;
                    // compressed literal block, FSE decode + huffman tree generation
                    // read from input if needed
                    if (bytesInAcc == 0) {
                        accRegister = zstdInStream.read();
                        bytesInAcc += PARALLEL_BYTE;
                    }
                    uint8_t hufHeader = 0;
                    if (noTree) {
                        // Word 2 <huffmanHeader><compressedSize>
                        //           1 byte         3 bytes
                        litMetaStream << (compressedSize << 8);
                    } else {
                        hufHeader = accRegister;
                        accRegister >>= 8;
                        --bytesInAcc;
                        --litSize2Write;
                        --remBlockSize;
                        // Word 2 <huffmanHeader><compressedSize>
                        //           1 byte         3 bytes
                        litMetaStream << (compressedSize << 8) + hufHeader; // write word 2

                        // check if FSE decoding is required
                        if (hufHeader < 128) {
                            // FSE table generation and decoding before huffman decoding
                            uint8_t litAccuracyLog = 6; // max is 6 bits, will be modified as per input
                            // create FSE tables
                            int ret = generateFSETable<PARALLEL_BYTE>(fseTableLitStream, zstdInStream, accRegister,
                                                                      bytesInAcc, litAccuracyLog, c_maxCharHuffman,
                                                                      FSE_COMPRESSED_MODE, FSE_COMPRESSED_MODE,
                                                                      c_defaultDistribution, prevDistribution);
                            litSize2Write -= ret;
                            remBlockSize -= ret;
                            // send relevant metadata
                            // Word 3 <accuracyLog><byteUsedByFSE>
                            //          1 byte          1 byte
                            litMetaStream << litAccuracyLog + ((uint32_t)ret << 8); // write word 3
                        }
                    }
                    bitsInAcc = bytesInAcc * 8;
                }
                case RAW_LBLOCK: {
                    // Bigger register needed, because more bytes than PARALLEL_BYTE can be present in the accumulator
                    ap_uint<c_accRegWidthx3> wbuf = accRegister;
                    uint8_t bytesWritten = 0;
                    uint8_t updBInAcc = bytesInAcc;

                    bitsInAcc = bytesInAcc * 8;
                // write block data
                cmplit_write_block:
                    for (int i = 0; i < litSize2Write; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                        if (i < (const int)(litSize2Write - bytesInAcc)) {
                            wbuf.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                            updBInAcc += PARALLEL_BYTE;
                        }
                        ap_uint<c_streamWidth> tmpV = wbuf;
                        litDecodeInStream << tmpV;

                        if (i > (const int)(litSize2Write - PARALLEL_BYTE)) {
                            bytesWritten = litSize2Write - i;
                            break;
                        }
                        wbuf >>= c_streamWidth;
                        updBInAcc -= PARALLEL_BYTE;
                    }
                    accRegister = (wbuf >> (bytesWritten * 8));
                    bytesInAcc = updBInAcc - bytesWritten;
                    bitsInAcc = bytesInAcc * 8;
                    remBlockSize -= litSize2Write;
                } break;
                default:
                    // must not reach here
                    break;
            }
            // decode zstd sequence header
            /*
             * Sequences Section:
             *     <Sequences_Section_Header><Literals_Length_Table><Offset_Table><Match_Length_Table><bitStream>
             *
             * Sequences_Section_Header
             * <Number_of_Sequences><Symbol_Compression_Modes>
             *       1-3 bytes              1 byte
             */
            if (bytesInAcc < remBlockSize && bytesInAcc < 4) {
                accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                bytesInAcc += PARALLEL_BYTE;
            }
            uint8_t byteCnt = 0;
            uint32_t seqCnt = 0;
            xfSymbolCompMode_t literalsLengthMode;
            xfSymbolCompMode_t offsetsMode;
            xfSymbolCompMode_t matchLengthsMode;
            // read 1 byte and check number of sequences
            uint8_t byte_0 = (uint8_t)(accRegister & 0x000000FF);
            accRegister >>= 8;
            --bytesInAcc;
            ++byteCnt;

            if (byte_0 == 0) {
                // there are no sequences, sequence section stops here.
                // Decompressed content is defined as Literals Section content.
                // The FSE tables used in Repeat_Mode are not updated.
                remBlockSize -= byteCnt;
                // write sequence meta data
                // Word 0 <blockMeta> ***already written
                // Word 1 <1> if further decoding needed
                // Word 2 <literalCount>
                seqMetaStream << 0;               // Word 1
                seqMetaStream << regeneratedSize; // Word 2
            } else {
                // write sequence meta data
                // Word 0 <blockMeta> ***already written
                // Word 1 <decode_flag> 1 if further decoding needed, else 0
                // Word 2 <literalCount>
                seqMetaStream << 1;               // Word 1
                seqMetaStream << regeneratedSize; // Word 2

                if (byte_0 > 0 && byte_0 < 128) {
                    // uses one byte
                    seqCnt = byte_0;
                } else if (byte_0 > 127 && byte_0 < 255) {
                    // uses 2 bytes
                    ++byteCnt;
                    // calculate Number_of_Sequences = ((byte0-128) << 8) + byte1
                    seqCnt = ((((uint32_t)byte_0 - 128) << 8) + ((uint32_t)accRegister & 0xFF));
                    accRegister >>= 8;
                    --bytesInAcc;
                } else {
                    // uses 3 bytes, dump first byte, need next 2 bytes
                    // calculate Number_of_Sequences = byte1 + (byte2<<8) + 0x7F00
                    seqCnt = (((uint32_t)accRegister & 0x0000FFFF) + 32512);
                    byteCnt += 2;
                    accRegister >>= 16;
                    bytesInAcc -= 2;
                }
                // Get symbol compression mode
                byte_0 = (uint8_t)accRegister;
                accRegister >>= 8;
                --bytesInAcc;
                bitsInAcc = bytesInAcc * 8;
                ++byteCnt;

                matchLengthsMode = (xfSymbolCompMode_t)((byte_0 >> 2) & 3);
                offsetsMode = (xfSymbolCompMode_t)((byte_0 >> 4) & 3);
                literalsLengthMode = (xfSymbolCompMode_t)((byte_0 >> 6) & 3);

                remBlockSize -= byteCnt;

                // generate FSE tables
                ap_uint<c_accRegWidth> fseAcc = accRegister;
                xfSymbolCompMode_t modeLOM[3] = {literalsLengthMode, offsetsMode, matchLengthsMode};
                uint8_t accuracyLog[3] = {6, 5, 6}; // for default distribution, overwritten for fse compressed ones
#pragma HLS ARRAY_PARTITION variable = modeLOM complete
#pragma HLS ARRAY_PARTITION variable = accuracyLog complete

                if (offsetsMode == PREDEFINED_MODE)
                    maxCharLOM[1] = c_maxCharDefOffset;
                else if (offsetsMode == FSE_COMPRESSED_MODE)
                    maxCharLOM[1] = c_maxCharOffset;

            gen_lom_fse_tables:
                for (uint8_t i = 0; i < 3; ++i) {
                    // Generated FSE tables for literal lengths, offsets and match lengths
                    if (modeLOM[i] == REPEAT_MODE) accuracyLog[i] = prevAccLog[i];
                    int ret = generateFSETable<PARALLEL_BYTE>(
                        fseTableSeqStream, zstdInStream, fseAcc, bytesInAcc, accuracyLog[i], maxCharLOM[i], modeLOM[i],
                        prevFseMode[i], &(c_defaultDistribution[defDistOffsets[i]]), &(prevDistribution[i * 64]));
                    prevFseMode[i] = modeLOM[i];
                    prevAccLog[i] = accuracyLog[i];
                    remBlockSize -= ret;
                }
                accRegister = fseAcc;
                bitsInAcc = bytesInAcc * 8;
                // write sequence meta data
                // Word 0 <blockMeta> ***already written
                // Word 1 <decode_flag> 1 if further decoding needed, else 0
                // Word 2 <literalCount>
                // Word 3 <symbolCompressionMode><remBlockSize>
                //              1 byte              3 bytes
                // Word 4 <seqCnt>
                // Word 5 <AccuracyLogs> --> <litlen><offset><matlen> 1 byte each lower-higher
                uint32_t sqmbuf = (remBlockSize << 8) + byte_0;
                seqMetaStream << sqmbuf; // Word 3
                seqMetaStream << seqCnt; // Word 4
                sqmbuf = (((uint32_t)accuracyLog[2] << 16) + ((uint32_t)accuracyLog[1] << 8) + accuracyLog[0]);
                seqMetaStream << sqmbuf; // Word 5
                // send the remaining block data, sequence bitstream
                ap_uint<c_accRegWidthx3> wbuf = accRegister;
                uint8_t bytesWritten = 0;
                uint8_t updBInAcc = bytesInAcc;

                bitsInAcc = bytesInAcc * 8;
            // write block data
            cmpseq_write_block:
                for (int i = 0; i < remBlockSize; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                    if (i < (const int)(remBlockSize - bytesInAcc)) {
                        wbuf.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = zstdInStream.read();
                        updBInAcc += PARALLEL_BYTE;
                    }
                    ap_uint<c_streamWidth> tmpV = wbuf;
                    seqDecodeInStream << tmpV;

                    if (i > (const int)(remBlockSize - PARALLEL_BYTE)) {
                        bytesWritten = remBlockSize - i;
                        break;
                    }
                    wbuf >>= c_streamWidth;
                    updBInAcc -= PARALLEL_BYTE;
                }
            }
        }
    }
    litValidStream << 0;
    seqValidStream << 0;
}

template <int PARALLEL_BYTE>
void parseFramesAndBlocks(hls::stream<ap_uint<8 * PARALLEL_BYTE> >& inStream,
                          hls::stream<ap_uint<4> >& inStrobe,
                          hls::stream<bool>& blockValidStream,
                          hls::stream<uint32_t>& blockMetaStream,
                          hls::stream<ap_uint<8 * PARALLEL_BYTE> >& zstdBlockStream) {
    // decompress all zstd frames
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    const uint16_t c_accRegWidth = c_streamWidth * 3;

    bool lastInputWord = false;
    ap_uint<c_accRegWidth> accRegister = 0;
    uint8_t bytesInAcc = 0;
    ap_uint<4> istb;
// parse all frames and exit when there is no data to read in inStream
parseFrames_main_loop:
    while (!lastInputWord) {
        uint32_t magicNumber;
        uint64_t windowSize = 0;
        uint32_t dictionaryId = 0;
        uint64_t frameContentSize = 0;
        uint32_t contentCheckSum = 0;
        uint32_t blockMaxSize = 0;
        uint8_t bitsInAcc = bytesInAcc << 3;

        /* Frame format
         * <Magic_Number><Frame_Header><Data_Block(s).....><Content_Checksum>
         * 	  4 bytes      2-14 bytes      n bytes....          0-4 bytes
         */
        // printf("Bytes in Acc: %d\n", bytesInAcc);
        // Read data to accumulator
        if (bytesInAcc < PARALLEL_BYTE) {
            istb = inStrobe.read();
            accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
            bitsInAcc += c_streamWidth;
        }
        if (istb == 0) { // 0 data here means data ends here and there is no more frames to decode
            lastInputWord = true;
            break;
        }
        magicNumber = accRegister;
        accRegister >>= 32;
        bitsInAcc -= 32;
        bytesInAcc = bitsInAcc >> 3;

        // verify magic number
        if (magicNumber != c_magicNumber) {
            if ((magicNumber & c_skippableFrameMask) != c_skipFrameMagicNumber) {
                // Error: Invalid Frame, magic number mismatch !!
                return;
            } else {
                // skippable frame
                if (bytesInAcc < 4) {
                    istb = inStrobe.read();
                    accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
                    bitsInAcc += c_streamWidth;
                }
                if (istb == 0) { // end of data stream, erroneous
                    lastInputWord = true;
                    break;
                }
                uint32_t frameSize = accRegister;
                accRegister >>= 32;
                bitsInAcc -= 32;
                bytesInAcc = bitsInAcc >> 3;

                if (bytesInAcc > frameSize) {
                    accRegister >>= (frameSize << 3);
                    bytesInAcc -= frameSize;
                    bitsInAcc = (bytesInAcc << 3);
                } else {
                    accRegister = 0; // needs to be skipped
                    uint32_t skfRemBytes = frameSize - bytesInAcc;
                    uint32_t iterLim = 1 + ((skfRemBytes - 1) / PARALLEL_BYTE);
                    // skip this frame
                    ap_uint<c_streamWidth> tmp = 0;
                skip_frame_loop:
                    for (uint32_t i = 0; i < iterLim; ++i) {
#pragma HLS PIPELINE II = 1
                        istb = inStrobe.read();
                        tmp = inStream.read();
                    }
                    // put residual in accumulator register
                    bytesInAcc = (PARALLEL_BYTE * iterLim) - skfRemBytes;
                    accRegister = (tmp >> (PARALLEL_BYTE - bytesInAcc));
                    bitsInAcc = (bytesInAcc << 3);
                }
                continue;
            }
        }
        /* Frame_Header format
         * <Frame_Header_Descriptor><Window_Descriptor><Dictionary_Id><Frame_Content_Size>
         * 		  1 byte				  0-1 byte		  0-4 bytes
         * 0-8
         * bytes
         */
        // Read data to accumulator
        if (bytesInAcc < PARALLEL_BYTE) {
            istb = inStrobe.read();
            accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
            bitsInAcc += c_streamWidth;
        }
        if (istb == 0) { // end of data stream, erroneous
            lastInputWord = true;
            break;
        }
        uint8_t headerDesc = (uint8_t)accRegister;
        accRegister >>= 8;
        bitsInAcc -= 8;

        // Parse frame header descriptor
        /*
         * Frame_Header_Descriptor
         * <Frame_Content_Size_flag><Single_Segment_flag><Not used><Content_Checksum_flag><Dictionary_ID_flag>
         *          bit 7-6                 bit 5         bit 4-3           bit 2               bit 1-0
         */
        bool checksumFlag = (headerDesc >> 2) & 1;
        bool singleSegmentFlag = (headerDesc >> 5) & 1;
        uint8_t didFieldSize = headerDesc & 3;
        uint8_t fcsFieldSize = headerDesc >> 6;

        didFieldSize += (uint8_t)(didFieldSize == 3);
        fcsFieldSize = fcsFieldSize ? ((uint8_t)1 << fcsFieldSize) : (fcsFieldSize | singleSegmentFlag);
        // Parse window descriptor
        /*
         * Calculate minimum buffer memory size (Window_Size) required for decompression for this frame.
         * Minimum window size = 1KB and maximum Window_Size is ( (1 << 41) + 7 * (1 << 38) ) bytes, which is 3.75 TB.
         *
         * <Exponent><Mantissa>
         *  bit 7-3	  bits 2-0
         */
        if (!singleSegmentFlag) {
            // read 1 byte
            uint8_t winDesc = (uint8_t)accRegister;
            accRegister >>= 8;
            bitsInAcc -= 8;
            // get window size
            uint8_t windowLog = (10 + (winDesc >> 3));
            uint64_t windowBase = (uint64_t)1 << windowLog;
            uint64_t windowAdd = (windowBase >> 3) * (winDesc & 7);
            windowSize = windowBase + windowAdd;
        }
        bytesInAcc = bitsInAcc >> 3;
        // read Dictionary_Id
        // read bytes if needed
        if (bytesInAcc < didFieldSize) {
            istb = inStrobe.read();
            accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
            bitsInAcc += c_streamWidth;
            bytesInAcc += PARALLEL_BYTE;
        }
        dictionaryId = (didFieldSize > 0) ? accRegister.range((8 * didFieldSize) - 1, 0) : 0;
        accRegister >>= (didFieldSize * 8);
        bytesInAcc -= didFieldSize;
        bitsInAcc = bytesInAcc << 3;

    // read Frame_Content_Size
    // read bytes if needed
    read_fss_bytes:
        while (bytesInAcc < fcsFieldSize) {
#pragma HLS PIPELINE II = 1
            istb = inStrobe.read();
            accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
            bitsInAcc += c_streamWidth;
            bytesInAcc += PARALLEL_BYTE;
        }
        frameContentSize = (fcsFieldSize > 0) ? accRegister.range((8 * fcsFieldSize) - 1, 0) : 0;
        frameContentSize += ((fcsFieldSize == 2) ? 256 : 0);
        accRegister >>= (fcsFieldSize * 8);
        bytesInAcc -= fcsFieldSize;
        bitsInAcc = bytesInAcc << 3;

        if (singleSegmentFlag) {
            windowSize = frameContentSize;
        }
        // Block_Max_Size is minimum of window size and 128K
        if (windowSize < (128 * 1024)) {
            blockMaxSize = windowSize;
        } else {
            blockMaxSize = 128 * 1024;
        }

        // parse Blocks in this Frame and pass block data to block decompression unit
        bool isLastBlock = false;
    // int blkcnt = 0;
    parse_block_in_frame:
        while (!isLastBlock) {
            if (bytesInAcc < PARALLEL_BYTE) {
                istb = inStrobe.read();
                accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
                bitsInAcc += c_streamWidth;
                bytesInAcc += PARALLEL_BYTE;
            }
            // Parse block header
            /*
             * Block_Header: 3 bytes
             * <Last_Block><Block_Type><Block_Size>
             *    bit 0		 bits 1-2    bits 3-23
             */
            ap_uint<24> blockHeader = accRegister;
            accRegister >>= 24;
            bitsInAcc -= 24;
            bytesInAcc -= 3;

            isLastBlock = (blockHeader & 1) ? true : false;
            xfBlockType_t blockType = (xfBlockType_t)(((uint8_t)blockHeader >> 1) & 3);
            uint32_t blockSize = (blockHeader >> 3);

            // write data to zstdStream
            // data transfer register
            ap_uint<c_accRegWidth> dtRegister = 0;
            uint8_t bytesUnused = 0;

            // enable processing for this block
            blockValidStream << 1;

            // write metadata to blockMetaStream
            uint32_t metabuf = (blockSize << 8) + ((uint8_t)blockType);
            blockMetaStream << metabuf;
            // copy blockSize data
            uint32_t bs2Write = blockSize;
            if (blockType == RLE_BLOCK) {
                bs2Write = 1;
            }
            /*
             * Use bytes from accumulator. Number of bytes in accumulator will always be less than PARALLEL_BYTE.
             * Number of bytes available in accumulator won't change during the iterations, but at last.
             */
            ap_uint<c_accRegWidth> wbuf = accRegister;
            uint8_t bytesWritten = 0;
            uint8_t updBInAcc = bytesInAcc;
        // write block data
        parser_write_block_data:
            for (int i = 0; i < bs2Write; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
                if (i < (const int)(bs2Write - bytesInAcc)) {
                    istb = inStrobe.read();
                    wbuf.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
                    updBInAcc += PARALLEL_BYTE;
                }
                ap_uint<c_streamWidth> tmpV = wbuf;
                zstdBlockStream << tmpV;

                if (i > (const int)(bs2Write - PARALLEL_BYTE)) {
                    bytesWritten = bs2Write - i;
                    break;
                }
                wbuf >>= c_streamWidth;
                updBInAcc -= PARALLEL_BYTE;
            }
            bytesInAcc = updBInAcc - bytesWritten;
            accRegister = wbuf >> (bytesWritten * 8);
            bitsInAcc = bytesInAcc * 8;
        }
        // check for checksum flag
        if (checksumFlag) {
            if (bytesInAcc < 4) {
                istb = inStrobe.read();
                accRegister.range((bitsInAcc + c_streamWidth - 1), bitsInAcc) = inStream.read();
                bytesInAcc += PARALLEL_BYTE;
            }
            bytesInAcc -= 4;
            accRegister >>= 32;
        }
    }
    blockValidStream << 0;
}

template <int PARALLEL_BYTE, int BLOCK_SIZE_KB, int LMO_WIDTH>
void decompressMultiFrames(hls::stream<ap_uint<8 * PARALLEL_BYTE> >& inStream,
                           hls::stream<ap_uint<4> >& inStrobe,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& literalStream,
                           hls::stream<ap_uint<LMO_WIDTH> >& litLenStream,
                           hls::stream<ap_uint<LMO_WIDTH> >& offsetStream,
                           hls::stream<ap_uint<LMO_WIDTH> >& matLenStream,
                           hls::stream<bool>& litlenValidStream,
                           hls::stream<bool>& newBlockFlagStream) {
    // Decompress all zstd frames in a file
    // const values
    const uint32_t c_litInStreamDepth = (BLOCK_SIZE_KB * 1024) / PARALLEL_BYTE;
    const uint32_t c_inStreamDepth = (4 * 1024) / PARALLEL_BYTE;
    const uint16_t c_metaStreamDepth = 1024;
    const uint16_t c_seqMetaStreamDepth = 2 * 1024;
    const uint8_t c_minStreamDepth = 8;
    // Internal data streams
    hls::stream<ap_uint<8 * PARALLEL_BYTE> > zstdInStream("zstdInStream");
    hls::stream<ap_uint<8 * PARALLEL_BYTE> > litDecodeInStream("litDecodeInStream");
    hls::stream<ap_uint<8 * PARALLEL_BYTE> > seqDecodeInStream("seqDecodeInStream");

    hls::stream<uint32_t> blockMetaStream("blockMetaStream");
    hls::stream<uint32_t> litMetaStream("litMetaStream");
    hls::stream<uint32_t> seqMetaStream("seqMetaStream");

    hls::stream<uint32_t> fseTableLitStream("fseTableLitStream");
    hls::stream<uint32_t> fseTableSeqStream("fseTableSeqStream");

    hls::stream<bool> blockValidStream("blockValidStream");
    hls::stream<bool> litValidStream("litValidStream");
    hls::stream<bool> seqValidStream("seqValidStream");

#pragma HLS STREAM variable = zstdInStream depth = c_inStreamDepth
#pragma HLS STREAM variable = litDecodeInStream depth = c_litInStreamDepth
#pragma HLS STREAM variable = seqDecodeInStream depth = c_inStreamDepth

#pragma HLS STREAM variable = blockMetaStream depth = c_minStreamDepth
#pragma HLS STREAM variable = litMetaStream depth = c_metaStreamDepth
#pragma HLS STREAM variable = seqMetaStream depth = c_metaStreamDepth

#pragma HLS STREAM variable = fseTableLitStream depth = c_metaStreamDepth
#pragma HLS STREAM variable = fseTableSeqStream depth = c_seqMetaStreamDepth

#pragma HLS STREAM variable = blockValidStream depth = c_minStreamDepth
#pragma HLS STREAM variable = litValidStream depth = c_minStreamDepth
#pragma HLS STREAM variable = seqValidStream depth = c_minStreamDepth

#pragma HLS dataflow

    // parse frames and block headers
    parseFramesAndBlocks<PARALLEL_BYTE>(inStream, inStrobe, blockValidStream, blockMetaStream, zstdInStream);

    // parse blocks and decompress them
    parseBlockGenFSETable<PARALLEL_BYTE>(blockValidStream, blockMetaStream, zstdInStream, litValidStream, litMetaStream,
                                         fseTableLitStream, litDecodeInStream, seqValidStream, seqMetaStream,
                                         fseTableSeqStream, seqDecodeInStream);

    // decode literals and forward to literal stream
    decodeLiterals<PARALLEL_BYTE, BLOCK_SIZE_KB>(litValidStream, litMetaStream, fseTableLitStream, litDecodeInStream,
                                                 literalStream);

    // decode sequences and output literal lengths, offsets and match lengths
    decodeSequence<PARALLEL_BYTE, BLOCK_SIZE_KB>(seqValidStream, seqMetaStream, fseTableSeqStream, seqDecodeInStream,
                                                 litLenStream, offsetStream, matLenStream, litlenValidStream,
                                                 newBlockFlagStream);
}

template <int PARALLEL_BYTE>
void kStreamReadZstdDecomp(hls::stream<ap_axiu<8 * PARALLEL_BYTE, 0, 0, 0> >& inKStream,
                           hls::stream<ap_uint<8 * PARALLEL_BYTE> >& zstdCoreInStream,
                           hls::stream<ap_uint<4> >& inStrobe,
                           uint64_t inputSize) {
    // write input data to core module from kernel axi stream
    const uint16_t c_streamWidth = 8 * PARALLEL_BYTE;
    uint8_t lbWidth = inputSize % PARALLEL_BYTE;
    ap_uint<4> strb = PARALLEL_BYTE;
    ap_axiu<c_streamWidth, 0, 0, 0> tmp;
ksReadZstIn:
    for (int i = 0; i < inputSize; i += PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1
        tmp = inKStream.read();
        zstdCoreInStream << tmp.data;
        if (inputSize < i + PARALLEL_BYTE) strb = lbWidth;
        inStrobe << strb;
    }
    // last strobe
    inStrobe << 0;
    zstdCoreInStream << 0;
}

template <int STREAM_WIDTH>
void kStreamWriteZstdDecomp(hls::stream<ap_axiu<STREAM_WIDTH, 0, 0, 0> >& outKStream,
                            hls::stream<ap_axiu<64, 0, 0, 0> >& outSizeStream,
                            hls::stream<ap_uint<STREAM_WIDTH> >& outDataStream,
                            hls::stream<bool>& byteEos,
                            hls::stream<uint64_t>& dataSize) {
    // write output data from core module to kernel axi stream
    bool lastByte = false;
    ap_uint<STREAM_WIDTH> tmp;
    ap_axiu<STREAM_WIDTH, 0, 0, 0> t1;

    ap_axiu<64, 0, 0, 0> pcksize;

    bool flag = false;
ksWriteOut:
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
    outSizeStream << pcksize;
}

} // details

/**
 * @brief This module decompresses the ZStd compressed file read from input stream. It reads the
 * input stream till valid strobe input is provided. It produces the decompressed data at the
 * output stream.
 *
 * @tparam PARALLEL_BYTE Data stream width in bytes
 * @tparam BLOCK_SIZE_KB ZStd block size
 * @tparam LZ_MAX_OFFSET LZ history size or Window size
 * @tparam LMO_WIDTH data width for offset data
 *
 * @param inStream input stream
 * @param inStrobe valid input strobe stream
 * @param outStream output stream
 * @param endOfStream end of output stream flag stream
 * @param sizeOutStream output data size
 */
template <int PARALLEL_BYTE, int BLOCK_SIZE_KB, int LZ_MAX_OFFSET, int LMO_WIDTH>
void zstdDecompressStream(hls::stream<ap_uint<8 * PARALLEL_BYTE> >& inStream,
                          hls::stream<ap_uint<4> >& inStrobe,
                          hls::stream<ap_uint<8 * PARALLEL_BYTE> >& outStream,
                          hls::stream<bool>& endOfStream,
                          hls::stream<uint64_t>& sizeOutStream) {
    // take zstd compressed bitstream as input and generate a zstd decompressed output stream
    // const values
    const uint32_t c_intlLiteralStreamDepth = (BLOCK_SIZE_KB * 1024) / PARALLEL_BYTE;
    const uint32_t c_literalStreamDepth = (36 * 1024) / (8 * PARALLEL_BYTE);
    const uint16_t c_lmoStreamDepth = 1024;
    const uint16_t c_litlenStreamDepth = 8 * 1024;
    const uint8_t c_minStreamDepth = 4;
    // internal streams
    hls::stream<ap_uint<8 * PARALLEL_BYTE> > intlLiteralStream("intlLiteralStream");
    hls::stream<ap_uint<8 * PARALLEL_BYTE> > literalStream("literalStream");
    hls::stream<ap_uint<LMO_WIDTH> > intlLitLenStream("intlLitLenStream");
    hls::stream<ap_uint<LMO_WIDTH> > litLenStream("litLenStream");
    hls::stream<ap_uint<LMO_WIDTH> > offsetStream("offsetStream");
    hls::stream<ap_uint<LMO_WIDTH> > matLenStream("matLenStream");
    hls::stream<bool> litlenValidStream("litlenValidStream");
    hls::stream<bool> newBlockFlagStream("newBlockFlagStream");

#pragma HLS STREAM variable = intlLiteralStream depth = c_intlLiteralStreamDepth
#pragma HLS STREAM variable = intlLitLenStream depth = c_litlenStreamDepth
#pragma HLS STREAM variable = litlenValidStream depth = c_litlenStreamDepth
#pragma HLS STREAM variable = literalStream depth = c_literalStreamDepth
#pragma HLS STREAM variable = litLenStream depth = c_lmoStreamDepth
#pragma HLS STREAM variable = offsetStream depth = c_lmoStreamDepth
#pragma HLS STREAM variable = matLenStream depth = c_lmoStreamDepth
#pragma HLS STREAM variable = newBlockFlagStream depth = c_minStreamDepth

#pragma HLS dataflow

    // decode blocks in frames and generate steady stream of literals, literal lengths, offsets and match lengths
    details::decompressMultiFrames<PARALLEL_BYTE, BLOCK_SIZE_KB, LMO_WIDTH>(
        inStream, inStrobe, intlLiteralStream, intlLitLenStream, offsetStream, matLenStream, litlenValidStream,
        newBlockFlagStream);

    details::alignLiterals<PARALLEL_BYTE, LMO_WIDTH>(litlenValidStream, newBlockFlagStream, intlLiteralStream,
                                                     intlLitLenStream, literalStream, litLenStream);

    // lz decompress for the aligned (to PARALLEL_BYTE bytes) stream of input
    lzMultiByteDecompress<PARALLEL_BYTE, LZ_MAX_OFFSET, ap_uint<LMO_WIDTH>, ap_uint<LMO_WIDTH> >(
        litLenStream, literalStream, offsetStream, matLenStream, outStream, endOfStream, sizeOutStream);
}

/**
 * @brief This module decompresses the ZStd compressed file read from input stream. It reads the
 * input stream till the given input size. It produces the decompressed data at the output stream.
 *
 * @tparam PARALLEL_BYTE Data stream width in bytes
 * @tparam BLOCK_SIZE_KB ZStd block size
 * @tparam LZ_MAX_OFFSET LZ history size or Window size
 * @tparam LMO_WIDTH data width for offset data
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param outSizeStream output data size
 * @param inputSize size of input data
 */
template <int PARALLEL_BYTE, int BLOCK_SIZE_KB, int LZ_MAX_OFFSET, int LMO_WIDTH = 15>
void zstdDecompressCore(hls::stream<ap_axiu<8 * PARALLEL_BYTE, 0, 0, 0> >& inStream,
                        hls::stream<ap_axiu<8 * PARALLEL_BYTE, 0, 0, 0> >& outStream,
                        hls::stream<ap_axiu<64, 0, 0, 0> >& outSizeStream,
                        uint64_t inputSize) {
    // Core function to use zstdDecompressStream module with zlib data movers
    // const values
    const uint16_t c_inStreamDepth = 128 / PARALLEL_BYTE; // 32 for PARALLEL_BYTE=4 and 16 for PARALLEL_BYTE=8
    const uint8_t c_minStreamDepth = 4;
    // internal streams
    hls::stream<ap_uint<(8 * PARALLEL_BYTE)> > zstdCoreInStream("zstdCoreInStream");
    hls::stream<ap_uint<(8 * PARALLEL_BYTE)> > outputStream("outputStream");
    hls::stream<ap_uint<4> > inStrobe("inStrobe");
    hls::stream<bool> endOfStream("endOfStream");
    hls::stream<uint64_t> sizeOutStream("sizeOutStream");

#pragma HLS STREAM variable = zstdCoreInStream depth = c_inStreamDepth
#pragma HLS STREAM variable = outputStream depth = c_inStreamDepth
#pragma HLS STREAM variable = inStrobe depth = c_inStreamDepth
#pragma HLS STREAM variable = sizeOutStream depth = c_minStreamDepth
#pragma HLS STREAM variable = endOfStream depth = c_inStreamDepth

#pragma HLS dataflow

    details::kStreamReadZstdDecomp<PARALLEL_BYTE>(inStream, zstdCoreInStream, inStrobe, inputSize);

    zstdDecompressStream<PARALLEL_BYTE, BLOCK_SIZE_KB, LZ_MAX_OFFSET, LMO_WIDTH>(
        zstdCoreInStream, inStrobe, outputStream, endOfStream, sizeOutStream);

    details::kStreamWriteZstdDecomp<8 * PARALLEL_BYTE>(outStream, outSizeStream, outputStream, endOfStream,
                                                       sizeOutStream);
}

} // compression
} // xf
#endif // _XFCOMPRESSION_ZSTD_DECOMPRESS_HPP_
