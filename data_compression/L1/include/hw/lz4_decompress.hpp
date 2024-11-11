/*
 * (c) Copyright 2019-2022 Xilinx, Inc. All rights reserved.
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
#ifndef _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_
#define _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_

/**
 * @file lz4_decompress.hpp
 * @brief Header for modules used in LZ4 decompression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "hls_stream.h"
#include "inflate.hpp"
#include "lz4_specs.hpp"
#include "lz_decompress.hpp"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>

namespace xf {
namespace compression {

template <typename T>
T reg(T d) {
#pragma HLS PIPELINE II = 1
#pragma HLS INTERFACE ap_ctrl_none port = return
#pragma HLS INLINE off
    return d;
}

typedef struct lz4BlockInfo {
    uint32_t compressedSize;
    bool storedBlock;
} dt_lz4BlockInfo;

template <int PARALLEL_BYTES>
void lz4HeaderProcessing(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                         hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                         hls::stream<dt_lz4BlockInfo>& blockInfoStream,
                         const uint32_t inputSize) {
    if (inputSize == 0) return;

    const int c_parallelBit = PARALLEL_BYTES * 8;
    ap_uint<3 * c_parallelBit> inputWindow;
    ap_uint<c_parallelBit> outStreamValue = 0;

    uint32_t readBytes = 0, processedBytes = 0;
    uint32_t origCompLen = 0, compLen = 0, blockSizeinKB = 0;
    uint8_t inputIdx = 0;
    bool outFlag = false;

    // to send both compressSize and storedBlock data
    dt_lz4BlockInfo blockInfo;

    for (uint8_t i = 0; i < 3; i++) {
#pragma HLS PIPELINE II = 1
        inputWindow.range((i + 1) * c_parallelBit - 1, i * c_parallelBit) = inStream.read();
        readBytes += PARALLEL_BYTES;
    }

    // Read magic header 4 bytes
    char magic_hdr[] = {MAGIC_BYTE_1, MAGIC_BYTE_2, MAGIC_BYTE_3, MAGIC_BYTE_4};
    for (uint32_t i = 0; i < MAGIC_HEADER_SIZE; i++) {
#pragma HLS PIPELINE II = 1
        int magicByte = (int)inputWindow.range((i + 1) * 8 - 1, i * 8);
        if (magicByte == magic_hdr[i])
            continue;
        else {
            assert(0);
        }
    }

    char c = (char)inputWindow.range(47, 40);
    switch (c) {
        case BSIZE_STD_64KB:
            blockSizeinKB = 64;
            break;
        case BSIZE_STD_256KB:
            blockSizeinKB = 256;
            break;
        case BSIZE_STD_1024KB:
            blockSizeinKB = 1024;
            break;
        case BSIZE_STD_4096KB:
            blockSizeinKB = 4096;
            break;
        default:
            assert(0);
    }

    uint32_t blockSizeInBytes = blockSizeinKB * 1024;

    inputIdx += 7;
    processedBytes += 8;

    for (; (processedBytes + inputIdx) < inputSize;) {
        if ((inputIdx >= PARALLEL_BYTES)) {
            inputWindow >>= c_parallelBit;
            if (readBytes < inputSize) {
                ap_uint<c_parallelBit> input = inStream.read();
                readBytes += PARALLEL_BYTES;
                inputWindow.range(3 * c_parallelBit - 1, 2 * c_parallelBit) = input;
            }
            inputIdx = inputIdx - PARALLEL_BYTES;
            processedBytes += PARALLEL_BYTES;
        }

        uint32_t chunkSize = 0;

        ap_uint<32> compressedSize = inputWindow >> (inputIdx * 8);
        inputIdx += 4;
        bool storedBlkFlg = (compressedSize.range(31, 31) == 1) ? true : false;

        uint32_t tmp = compressedSize;
        tmp >>= 24;

        if (tmp == 128) {
            uint8_t b1 = compressedSize;
            uint8_t b2 = compressedSize >> 8;
            uint8_t b3 = compressedSize >> 16;

            if (b3 == 1) {
                compressedSize = blockSizeInBytes;
            } else {
                uint16_t size = 0;
                size = b2;
                size <<= 8;
                uint16_t temp = b1;
                size |= temp;
                compressedSize = size;
            }
        }

        compLen = compressedSize;

        blockInfo.compressedSize = compressedSize;

        if (storedBlkFlg) {
            blockInfo.storedBlock = 1;
        } else {
            blockInfo.storedBlock = 0;
        }
        // write compress length to outSizeStream
        blockInfoStream << blockInfo;

        uint32_t len = compLen;
        for (uint32_t blockData = 0; blockData < compLen; blockData += PARALLEL_BYTES) {
#pragma HLS PIPELINE II = 1
            if ((inputIdx >= PARALLEL_BYTES)) {
                inputWindow >>= c_parallelBit;
                if (readBytes < inputSize) {
                    ap_uint<c_parallelBit> input = inStream.read();
                    readBytes += PARALLEL_BYTES;
                    inputWindow.range(3 * c_parallelBit - 1, 2 * c_parallelBit) = input;
                }
                inputIdx = inputIdx - PARALLEL_BYTES;
                processedBytes += PARALLEL_BYTES;
            }

            outStreamValue = inputWindow >> (inputIdx * 8);
            outStream << outStreamValue;

            if (len >= PARALLEL_BYTES) {
                inputIdx += PARALLEL_BYTES;
                len -= PARALLEL_BYTES;
            } else {
                inputIdx += len;
                len = 0;
            }
        }
    }
    blockInfo.compressedSize = 0;
    // writing 0 to indicate end of data
    blockInfoStream << blockInfo;
}

/**
 * @brief This module reads the compressed data from input stream
 * and decodes the offset, match length and literals by processing
 * in various decompress states.
 *
 *
 * @param inStream Input stream 8bit
 * @param outStream Output stream 32bit
 * @param input_size Input size
 */
inline void lz4Decompress(hls::stream<ap_uint<8> >& inStream,
                          hls::stream<ap_uint<32> >& outStream,
                          uint32_t input_size) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET0, READ_OFFSET1, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    ap_uint<32> decompressdOut = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
lz4_decompressr:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> inValue = inStream.read();
        if (next_state == READ_TOKEN) {
            lit_len = inValue.range(7, 4);
            match_len = inValue.range(3, 0);
            if (lit_len == 0xF) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
            } else {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_LIT_LEN) {
            lit_len += inValue;
            if (inValue != 0xFF) {
                next_state = READ_LITERAL;
            }
        } else if (next_state == READ_LITERAL) {
            ap_uint<32> outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_OFFSET0) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET1;
        } else if (next_state == READ_OFFSET1) {
            offset.range(15, 8) = inValue;
            if (match_len == 0xF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                ap_uint<32> outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
            }
        } else if (next_state == READ_MATCH_LEN) {
            match_len += inValue;
            if (inValue != 0xFF) {
                ap_uint<32> outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
                next_state = READ_TOKEN;
            }
        }
    }
}

inline void lz4DecompressSimple(hls::stream<ap_uint<8> >& inStream,
                                hls::stream<ap_uint<32> >& outStream,
                                uint32_t input_size,
                                bool uncomp_flag) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET0, READ_OFFSET1, READ_MATCH_LEN };
    enum lz4DecompressStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    ap_uint<32> decompressdOut = 0;
    uint32_t lit_len = 0;
    uint32_t match_len = 0;
    if (uncomp_flag == 1) {
        next_state = READ_LITERAL;
        lit_len = input_size;
    }
lz4_decompressr:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> inValue = inStream.read();
        if (next_state == READ_TOKEN) {
            lit_len = inValue.range(7, 4);
            match_len = inValue.range(3, 0);
            if (lit_len == 0xF) {
                next_state = READ_LIT_LEN;
            } else if (lit_len) {
                next_state = READ_LITERAL;
            } else {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_LIT_LEN) {
            lit_len += inValue;
            if (inValue != 0xFF) {
                next_state = READ_LITERAL;
            }
        } else if (next_state == READ_LITERAL) {
            ap_uint<32> outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) {
                next_state = READ_OFFSET0;
            }
        } else if (next_state == READ_OFFSET0) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET1;
        } else if (next_state == READ_OFFSET1) {
            offset.range(15, 8) = inValue;
            if (match_len == 0xF) {
                next_state = READ_MATCH_LEN;
            } else {
                next_state = READ_TOKEN;
                ap_uint<32> outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
            }
        } else if (next_state == READ_MATCH_LEN) {
            match_len += inValue;
            if (inValue != 0xFF) {
                ap_uint<32> outValue = 0;
                outValue.range(31, 16) = (match_len + 3); //+3 for LZ4 standard
                outValue.range(15, 0) = (offset - 1);     //-1 for LZ4 standard
                outStream << outValue;
                next_state = READ_TOKEN;
            }
        }
    }
}

template <int PARALLEL_BYTES, class SIZE_DT = uint32_t>
inline void lz4MultiByteDecompress(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                   hls::stream<SIZE_DT>& litlenStream,
                                   hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream,
                                   hls::stream<ap_uint<16> >& offsetStream,
                                   hls::stream<SIZE_DT>& matchlenStream,
                                   hls::stream<dt_lz4BlockInfo>& blockInfoStream) {
    enum lz4DecompressStates { READ_TOKEN, READ_LIT_LEN, READ_LITERAL, READ_OFFSET, READ_MATCH_LEN };
    for (dt_lz4BlockInfo bInfo = blockInfoStream.read(); bInfo.compressedSize != 0; bInfo = blockInfoStream.read()) {
        uint32_t input_size = bInfo.compressedSize;
        enum lz4DecompressStates next_state = READ_TOKEN;

        const int c_parallelBit = PARALLEL_BYTES * 8;
        uint8_t token_match_len = 0;
        uint8_t token_lit_len = 0;
        uint8_t input_index = 0;
        int8_t output_index = 0;
        SIZE_DT lit_len = 0;
        SIZE_DT match_len = 0;
        bool outFlag;

        ap_uint<16> offset;
        ap_uint<c_parallelBit> outStreamValue;
        ap_uint<c_parallelBit> inValue;

        ap_uint<2 * c_parallelBit> input_window;

        bool storedBlock = bInfo.storedBlock;
        if (storedBlock) {
            lit_len = input_size;
            litlenStream << lit_len;
            matchlenStream << 0;
            offsetStream << 0;
            next_state = READ_LITERAL;
        }

        ap_uint<2> numItr = (input_size <= PARALLEL_BYTES) ? 1 : 2;
        // Pre-read two data from the stream (two based on the READ_TOKEN)
        for (int i = 0; i < numItr; i++) {
#pragma HLS PIPELINE II = 1
            inValue = inStream.read();
            input_window.range(((i + 1) * c_parallelBit) - 1, i * c_parallelBit) = inValue;
        }

        // Initialize the loop readBytes variable to input_window buffer size as
        // that much data is already read from stream
        uint32_t readBytes = 2 * PARALLEL_BYTES;
        uint32_t processedBytes = 0;
    lz4_decompressr:
        for (; ((processedBytes + input_index) < input_size);) {
#pragma HLS PIPELINE II = 1
            uint8_t incrInputIdx = 0;
            outFlag = false;

            // READ TOKEN stage
            if (next_state == READ_TOKEN) {
                ap_uint<8> token_value = input_window >> (input_index * 8);
                token_lit_len = token_value.range(7, 4);
                token_match_len = token_value.range(3, 0);
                bool c0 = (token_lit_len == 0xF);
                incrInputIdx = 1;
                lit_len = token_lit_len;

                if (c0) {
                    next_state = READ_LIT_LEN;
                } else if (lit_len) {
                    next_state = READ_LITERAL;
                    litlenStream << lit_len;
                    matchlenStream << 0;
                    offsetStream << 0;
                } else {
                    next_state = READ_OFFSET;
                }
            } else if (next_state == READ_LIT_LEN) {
                ap_uint<8> token_value = input_window >> (input_index * 8);
                incrInputIdx = 1;
                lit_len += token_value;

                if (token_value == 0xFF) {
                    next_state = READ_LIT_LEN;
                } else {
                    next_state = READ_LITERAL;
                    litlenStream << lit_len;
                    matchlenStream << 0;
                    offsetStream << 0;
                }
            } else if (next_state == READ_LITERAL) {
                outFlag = true;
                outStreamValue = input_window >> (input_index * 8);
                uint32_t localLitLen = lit_len;
                if (localLitLen <= PARALLEL_BYTES) {
                    incrInputIdx = lit_len;
                    lit_len = 0;
                    next_state = READ_OFFSET;
                } else {
                    incrInputIdx = PARALLEL_BYTES;
                    lit_len -= PARALLEL_BYTES;
                    next_state = READ_LITERAL;
                }
            } else if (next_state == READ_OFFSET) {
                offset = input_window >> (input_index * 8);
                bool c0 = (token_match_len == 0xF);
                incrInputIdx = 2;
                match_len = token_match_len + 4; //+4 because of LZ4 standard

                if (c0) {
                    next_state = READ_MATCH_LEN;
                } else {
                    next_state = READ_TOKEN;
                    litlenStream << 0;
                    matchlenStream << match_len;
                    offsetStream << offset;
                }
            } else if (next_state == READ_MATCH_LEN) {
                ap_uint<8> token_value = input_window >> (input_index * 8);
                incrInputIdx = 1;
                match_len += token_value;

                if (token_value == 0xFF) {
                    next_state = READ_MATCH_LEN;
                } else {
                    next_state = READ_TOKEN;
                    litlenStream << 0;
                    matchlenStream << match_len;
                    offsetStream << offset;
                }
            } else {
                assert(0);
            }

            input_index += incrInputIdx;
            bool inputIdxFlag = reg<bool>((input_index >= PARALLEL_BYTES));
            // write to input stream based on PARALLEL BYTES
            if (inputIdxFlag) {
                input_window >>= c_parallelBit;
                input_index -= PARALLEL_BYTES;
                processedBytes += PARALLEL_BYTES;
                if (readBytes < input_size) {
                    ap_uint<c_parallelBit> input = inStream.read();
                    readBytes += PARALLEL_BYTES;
                    input_window.range(2 * c_parallelBit - 1, c_parallelBit) = input;
                }
            }

            if (outFlag) {
                litStream << outStreamValue;
                outFlag = false;
            }
        }
    }

    if (!blockInfoStream.empty()) dt_lz4BlockInfo bInfo = blockInfoStream.read();

    // signalling end of transaction
    litlenStream << 0;
    matchlenStream << 0;
    offsetStream << 0;
}

template <int PARALLEL_BYTES, int HISTORY_SIZE>
void lz4CoreDecompressEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                             hls::stream<ap_uint<(PARALLEL_BYTES * 8) + PARALLEL_BYTES> >& outStream,
                             hls::stream<uint32_t>& blockSizeStream) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t;
    typedef ap_uint<16> offset_dt;

    hls::stream<uint32_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint32_t> matchlenStream("matchlenStream");
    hls::stream<dt_lz4BlockInfo> blockInfoStream("blockInfoStream");
    hls::stream<ap_uint<27> > lzInfoStream("lzInfoStream");
#pragma HLS STREAM variable = litlenStream depth = 16
#pragma HLS STREAM variable = litStream depth = 256
#pragma HLS STREAM variable = offsetStream depth = 16
#pragma HLS STREAM variable = matchlenStream depth = 16
#pragma HLS STREAM variable = blockInfoStream depth = 4
#pragma HLS STREAM variable = lzInfoStream depth = 4

#pragma HLS BIND_STORAGE variable = litlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = blockInfoStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lzInfoStream type = FIFO impl = SRL
    dt_lz4BlockInfo blockInfo;
    for (int i = 0; i < 2; i++) {
        blockInfo.compressedSize = blockSizeStream.read();
        blockInfo.storedBlock = 0;
        blockInfoStream << blockInfo;
    }
#pragma HLS dataflow
    lz4MultiByteDecompress<PARALLEL_BYTES>(inStream, litlenStream, litStream, offsetStream, matchlenStream,
                                           blockInfoStream);
    details::lzPreProcessingUnitLL<uint32_t, PARALLEL_BYTES>(litlenStream, matchlenStream, offsetStream, lzInfoStream);
    lzMultiByteDecompressLL<PARALLEL_BYTES, HISTORY_SIZE, 16, ap_uint<17> >(litStream, lzInfoStream, outStream);
}

/**
 * @brief This module reads the compressed data from input stream
 * @tparam PARALLEL_BYTES It is fixed with value 8 bytes
 * @tparam HISTORY_SIZE It is fixed with value 4K bytes
 *
 *
 * @param inStream Input stream 64bit
 * @param outStream Output stream 74bit
 * @param _input_size Input size
 */
template <int PARALLEL_BYTES, int HISTORY_SIZE>
void lz4DecompressEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                         hls::stream<ap_uint<(PARALLEL_BYTES * 8) + PARALLEL_BYTES> >& outStream,
                         const uint32_t _input_size) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t;
    typedef ap_uint<16> offset_dt;

    uint32_t input_size1 = _input_size;
    hls::stream<uint32_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<uintV_t> headerStream("headerStream");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint32_t> matchlenStream("matchlenStream");
    hls::stream<dt_lz4BlockInfo> blockInfoStream("blockInfoStream");
    hls::stream<ap_uint<27> > lzInfoStream("lzInfoStream");
#pragma HLS STREAM variable = litlenStream depth = 16
#pragma HLS STREAM variable = headerStream depth = 16
#pragma HLS STREAM variable = litStream depth = 256
#pragma HLS STREAM variable = offsetStream depth = 16
#pragma HLS STREAM variable = matchlenStream depth = 16
#pragma HLS STREAM variable = blockInfoStream depth = 4
#pragma HLS STREAM variable = lzInfoStream depth = 4

#pragma HLS BIND_STORAGE variable = litlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = headerStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = blockInfoStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lzInfoStream type = FIFO impl = SRL

#pragma HLS dataflow
    lz4HeaderProcessing<PARALLEL_BYTES>(inStream, headerStream, blockInfoStream, input_size1);
    lz4MultiByteDecompress<PARALLEL_BYTES, uint32_t>(headerStream, litlenStream, litStream, offsetStream,
                                                     matchlenStream, blockInfoStream);
    details::lzPreProcessingUnitLL<uint32_t, PARALLEL_BYTES>(litlenStream, matchlenStream, offsetStream, lzInfoStream);
    lzMultiByteDecompressLL<PARALLEL_BYTES, HISTORY_SIZE, 16, ap_uint<17> >(litStream, lzInfoStream, outStream);
}

template <int PARALLEL_BYTES, class SIZE_DT = uint32_t>
inline void lz4MultiByteDecompress_opt1(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                        hls::stream<SIZE_DT>& litlenStream,
                                        hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream,
                                        hls::stream<ap_uint<16> >& offsetStream,
                                        hls::stream<SIZE_DT>& matchlenStream,
                                        hls::stream<dt_lz4BlockInfo>& blockInfoStream) {
    dt_lz4BlockInfo bInfo;
    const int c_parallelBit = PARALLEL_BYTES * 8;

    enum lz4DecompressStates {
        READ_TOKEN,
        READ_LIT_LEN,
        READ_LITERAL,
        READ_OFFSET,
        READ_MATCH_LEN,
        READ_LITLEN_LIT_OFFSET
    };
    for (bInfo = blockInfoStream.read(); bInfo.compressedSize != 0; bInfo = blockInfoStream.read()) {
        uint32_t input_size = bInfo.compressedSize;

        enum lz4DecompressStates next_state = READ_LITLEN_LIT_OFFSET;

        const int c_parallelBit = PARALLEL_BYTES * 8;
        uint8_t token_match_len = 0;
        uint8_t token_match_len_p4 = 0;
        uint8_t token_lit_len = 0;
        uint8_t token_lit_len_p1 = 0;
        uint8_t token_lit_len_p3 = 0;
        uint8_t input_index = 0;
        uint8_t input_index_offset = 0;
        int8_t output_index = 0;
        SIZE_DT lit_len = 0;
        SIZE_DT match_len = 0;
        bool outFlag;

        ap_uint<16> offset;
        ap_uint<c_parallelBit> outStreamValue;
        ap_uint<c_parallelBit> inValue;

        ap_uint<2 * c_parallelBit> input_window;

        bool storedBlock = bInfo.storedBlock;
        if (storedBlock) {
            lit_len = input_size;
            litlenStream << lit_len;
            matchlenStream << 0;
            offsetStream << 0;
            next_state = READ_LITERAL;
        }

        ap_uint<2> numItr = (input_size <= PARALLEL_BYTES) ? 1 : 2;
        // Pre-read two data from the stream (two based on the READ_TOKEN)
        for (int i = 0; i < numItr; i++) {
#pragma HLS PIPELINE II = 1
            inValue = inStream.read();
            // DEBUG
            // std::cout << "<ByteDecompress 2> inValue : " << std::hex <<  inValue <<
            // std::endl;

            input_window.range(((i + 1) * c_parallelBit) - 1, i * c_parallelBit) = inValue;
        }

        // Initialize the loop readBytes variable to input_window buffer size as
        // that much data is already read from stream
        uint32_t readBytes = 2 * PARALLEL_BYTES;
        uint32_t processedBytes = 0;

    LZ4_decompressr2:
        for (; ((processedBytes + input_index) < input_size);) {
#pragma HLS PIPELINE II = 1
            uint8_t incrInputIdx = 0;
            outFlag = false;
            // READ TOKEN stage
            if (next_state == READ_LITLEN_LIT_OFFSET) {
                // if works will be 1 cycle, or is 2 cycles : offset shift mv to next
                // state
                ap_uint<8> token_value = input_window >> (input_index * 8);
                token_lit_len = token_value.range(7, 4);
                token_match_len = token_value.range(3, 0);
                token_match_len_p4 = token_match_len + 4;
                token_lit_len_p3 = token_lit_len + 3;

                bool c0 = (token_lit_len == 0xF);

                lit_len = token_lit_len;
                outStreamValue = input_window >> ((input_index + 1) * 8);
                offset = input_window >> ((input_index + token_lit_len + 1) * 8);

                if (c0) {
                    next_state = READ_LIT_LEN;
                    incrInputIdx = 1;
                    outFlag = false;
                } else if (lit_len == 0) {
                    next_state = READ_OFFSET;
                    incrInputIdx = 1;
                    outFlag = false;
                } else if (lit_len <= 0x4 && ((token_match_len != 0xF))) { //(token_match_len !=0xF)
                    next_state = READ_LITLEN_LIT_OFFSET;
                    incrInputIdx = token_lit_len_p3;
                    outFlag = true;

                    litlenStream << lit_len;
                    matchlenStream << token_match_len_p4;
                    offsetStream << offset;
                } else { // if (lit_len>0x4) or (token_match_len ==0xF)
                    next_state = READ_LITERAL;
                    incrInputIdx = 1;
                    outFlag = false;
                    litlenStream << lit_len;
                    matchlenStream << 0;
                    offsetStream << 0;
                }

            } else if (next_state == READ_LIT_LEN) { // 1
                ap_uint<8> token_value = input_window >> (input_index * 8);
                incrInputIdx = 1;
                lit_len += token_value;

                if (token_value == 0xFF) {
                    next_state = READ_LIT_LEN;
                } else {
                    next_state = READ_LITERAL;
                    litlenStream << lit_len;
                    matchlenStream << 0;
                    offsetStream << 0;
                }
            } else if (next_state == READ_LITERAL) { // 2
                outFlag = true;
                outStreamValue = input_window >> (input_index * 8);
                uint32_t localLitLen = lit_len;
                if (localLitLen <= PARALLEL_BYTES) {
                    incrInputIdx = lit_len;
                    lit_len = 0;
                    next_state = READ_OFFSET;
                } else {
                    incrInputIdx = PARALLEL_BYTES;
                    lit_len -= PARALLEL_BYTES;
                    next_state = READ_LITERAL;
                }
            } else if (next_state == READ_OFFSET) { // 3
                offset = input_window >> (input_index * 8);
                bool c0 = (token_match_len == 0xF);
                incrInputIdx = 2;
                match_len = token_match_len + 4; //+4 because of LZ4 standard

                if (c0) {
                    next_state = READ_MATCH_LEN;
                } else {
                    next_state = READ_LITLEN_LIT_OFFSET;
                    litlenStream << 0;
                    matchlenStream << match_len;
                    offsetStream << offset;
                }
            } else if (next_state == READ_MATCH_LEN) { // 4
                ap_uint<8> token_value = input_window >> (input_index * 8);
                incrInputIdx = 1;
                match_len += token_value;

                if (token_value == 0xFF) {
                    next_state = READ_MATCH_LEN;
                } else {
                    next_state = READ_LITLEN_LIT_OFFSET;
                    litlenStream << 0;
                    matchlenStream << match_len;
                    offsetStream << offset;
                }
            } else {
                assert(0);
            }

            input_index += incrInputIdx;
            bool inputIdxFlag = reg<bool>((input_index >= PARALLEL_BYTES));
            // write to input stream based on PARALLEL BYTES
            if (inputIdxFlag) {
                input_window >>= c_parallelBit;
                input_index -= PARALLEL_BYTES;
                processedBytes += PARALLEL_BYTES;

                if (readBytes < input_size) {
                    ap_uint<c_parallelBit> input;
                    inStream.read(input);
                    readBytes += PARALLEL_BYTES;
                    input_window.range(2 * c_parallelBit - 1, c_parallelBit) = input;
                }
            }

            if (outFlag) {
                litStream << outStreamValue;
                outFlag = false;
            }
        } // end one block
    }     // end block

    if (!blockInfoStream.empty()) dt_lz4BlockInfo bInfo = blockInfoStream.read();

    // signalling end of transaction
    litlenStream << 0;
    matchlenStream << 0;
    offsetStream << 0;
}

/**
 * @brief This module can read 8-byte input and generate
 * 16-byte output.
 * @tparam PARALLEL_BYTES It is fixed with value 8 bytes
 * @tparam PARALLEL_OUT_BYTES It can be 8 bytes or 16 bytes
 * @tparam HISTORY_SIZE It is fixed with value 4K bytes
 *
 *
 * @param inStream Input stream 64bit
 * @param outStream Output stream 72bit or 144bit
 * @param _input_size Input size
 */
template <int PARALLEL_BYTES, int PARALLEL_OUT_BYTES, int HISTORY_SIZE>
void lz4DecompressEngine_NinMout(
    hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
    hls::stream<ap_uint<(PARALLEL_OUT_BYTES * 8) + PARALLEL_OUT_BYTES> >& outStream, // opt2
    const uint32_t _input_size) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t; // 64b
    typedef ap_uint<16> offset_dt;

    uint32_t input_size1 = _input_size;

    hls::stream<uint32_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<uintV_t> headerStream("headerStream");
    hls::stream<uintV_t> headerStream1("headerStream1");
    hls::stream<uintV_t> headerStream2("headerStream2");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint32_t> matchlenStream("matchlenStream");
    hls::stream<dt_lz4BlockInfo> blockInfoStream("blockInfoStream");
    hls::stream<dt_lz4BlockInfo> blockInfoStream1("blockInfoStream1");
    hls::stream<dt_lz4BlockInfo> blockInfoStream2("blockInfoStream2");
    hls::stream<ap_uint<27 + 1> > lzInfoStream("lzInfoStream"); // opt2

#pragma HLS STREAM variable = litlenStream depth = 16
#pragma HLS STREAM variable = headerStream depth = 16
#pragma HLS STREAM variable = headerStream1 depth = 16
#pragma HLS STREAM variable = headerStream2 depth = 16

#pragma HLS STREAM variable = litStream depth = 256
#pragma HLS STREAM variable = offsetStream depth = 16
#pragma HLS STREAM variable = matchlenStream depth = 16
#pragma HLS STREAM variable = blockInfoStream depth = 4
#pragma HLS STREAM variable = blockInfoStream1 depth = 4
#pragma HLS STREAM variable = blockInfoStream2 depth = 4

#pragma HLS STREAM variable = lzInfoStream depth = 4

#pragma HLS BIND_STORAGE variable = litlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = litStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = headerStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = offsetStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = matchlenStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = blockInfoStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lzInfoStream type = FIFO impl = SRL

#pragma HLS dataflow

    lz4HeaderProcessing<PARALLEL_BYTES>(inStream, headerStream, blockInfoStream, input_size1);

    lz4MultiByteDecompress_opt1<PARALLEL_BYTES, uint32_t>(headerStream, litlenStream, litStream, offsetStream,
                                                          matchlenStream, blockInfoStream);

    details::lzPreProcessingUnitLL_opt2<uint32_t, PARALLEL_BYTES, PARALLEL_OUT_BYTES>(litlenStream, matchlenStream,
                                                                                      offsetStream, lzInfoStream);

    lzMultiByteDecompressLL_opt2<PARALLEL_BYTES, PARALLEL_OUT_BYTES, HISTORY_SIZE, 16, ap_uint<17> >(
        litStream, lzInfoStream, outStream);
}

} // namespace compression
} // namespace xf
#endif // _XFCOMPRESSION_LZ4_DECOMPRESS_HPP_
