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
#ifndef _XFCOMPRESSION_SNAPPY_DECOMPRESS_HPP_
#define _XFCOMPRESSION_SNAPPY_DECOMPRESS_HPP_

/**
 * @file snappy_decompress.hpp
 * @brief Header for modules used for snappy decompression kernle
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "lz_decompress.hpp"

typedef ap_uint<8> uintV_t;
typedef ap_uint<32> encoded_dt;

typedef struct BlockInfo {
    uint32_t compressSize;
    bool storedBlock;
} dt_blockInfo;

namespace xf {
namespace compression {

template <int PARALLEL_BYTES>
static void snappyHeaderProcessing(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                   hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                                   hls::stream<dt_blockInfo>& blockInfoStream,
                                   const uint32_t inputSize) {
    if (inputSize == 0) return;
    // Snappy Header states
    enum snappyDecompressHeaderStates { READ_COMP_LEN, WRITE_DATA };
    enum snappyDecompressHeaderStates nextState = READ_COMP_LEN;

    const int c_parallelBit = PARALLEL_BYTES * 8;
    ap_uint<3 * c_parallelBit> inputWindow;
    ap_uint<c_parallelBit> outStreamValue = 0;

    uint32_t readBytes = 0, processedBytes = 0;
    uint32_t origCompLen = 0, compLen = 0;
    uint8_t inputIdx = 0;
    bool outFlag = false;

    // process first 10 bytes
    ap_uint<c_parallelBit> temp1 = inStream.read();
    readBytes += PARALLEL_BYTES;
    processedBytes += PARALLEL_BYTES;

    if (PARALLEL_BYTES < 8) {
        temp1 = inStream.read();
        readBytes += PARALLEL_BYTES;
        processedBytes += PARALLEL_BYTES;
    }

    for (uint8_t i = 0; i < 3; i++) {
#pragma HLS PIPELINE II = 1
        inputWindow.range((i + 1) * c_parallelBit - 1, i * c_parallelBit) = inStream.read();
        readBytes += PARALLEL_BYTES;
    }

    inputWindow >>= 16;
    processedBytes += 2;

    uint8_t incrIdx = 2 * PARALLEL_BYTES - 2;
    dt_blockInfo blockInfo;

    for (; (processedBytes + inputIdx) < inputSize;) {
#pragma HLS PIPELINE II = 1

        if ((inputIdx >= PARALLEL_BYTES) & (readBytes < inputSize)) {
            inputWindow >>= c_parallelBit;

            ap_uint<c_parallelBit> input = inStream.read();
            readBytes += PARALLEL_BYTES;
            inputWindow.range((incrIdx + PARALLEL_BYTES) * 8 - 1, incrIdx * 8) = input;
            inputIdx = inputIdx - PARALLEL_BYTES;
            processedBytes += PARALLEL_BYTES;
        }

        if (nextState == READ_COMP_LEN) {
            uint32_t chunkSize = 0;

            ap_uint<32> inValue = inputWindow >> (inputIdx * 8);
            uint32_t temp = inValue.range(31, 24);
            temp <<= 16;
            chunkSize |= temp;
            temp = 0;
            temp = inValue.range(23, 16);
            temp <<= 8;
            chunkSize |= temp;
            temp = 0;
            chunkSize |= inValue.range(15, 8);

            origCompLen = chunkSize - 4;
            blockInfo.compressSize = origCompLen;
            compLen = origCompLen;

            uint8_t chunkIdx = inValue.range(7, 0);

            if (chunkIdx == 0x01) {
                blockInfo.storedBlock = 1;
            } else {
                blockInfo.storedBlock = 0;
            }
            // 4 bytes processed and remaining 4 bytes skipped
            inputIdx += 8;

            // write blockInfo to stream
            blockInfoStream << blockInfo;
            nextState = WRITE_DATA;
        } else if (nextState == WRITE_DATA) {
            outFlag = true;
            outStreamValue = inputWindow >> (inputIdx * 8);
            if (compLen >= PARALLEL_BYTES) {
                inputIdx += PARALLEL_BYTES;
                compLen -= PARALLEL_BYTES;
            } else {
                inputIdx += compLen;
                compLen = 0;
            }
            if (compLen == 0) {
                nextState = READ_COMP_LEN;
            } else {
                nextState = WRITE_DATA;
            }
        }

        if (outFlag) {
            outStream << outStreamValue;
            outFlag = false;
        }
    }
    blockInfo.compressSize = 0;
    // writing 0 to indicate end of data
    blockInfoStream << blockInfo;
}

/**
 * @brief This module decodes the compressed data based on the snappy decompression format
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param input_size input data size
 */
static void snappyDecompress(hls::stream<uintV_t>& inStream, hls::stream<encoded_dt>& outStream, uint32_t input_size) {
    // Snappy Decoder states
    enum SnappyDecompressionStates {
        READ_STATE,
        MATCH_STATE,
        LOW_OFFSET_STATE,
        READ_TOKEN,
        READ_LITERAL,
        READ_LITLEN_60,
        READ_LITLEN_61,
        READ_OFFSET,
        READ_OFFSET_C01,
        READ_OFFSET_C10,
        READ_LITLEN_61_CONT,
        READ_OFFSET_C10_CONT
    };

    if (input_size == 0) return;

    enum SnappyDecompressionStates next_state = READ_TOKEN;
    ap_uint<8> nextValue;
    ap_uint<16> offset;
    encoded_dt decodedOut = 0;
    ap_uint<32> lit_len = 0;
    uint32_t match_len = 0;
    ap_uint<8> inValue = 0;
    bool read_instream = true;

    uint32_t inCntr_idx = 0;
    ap_uint<32> inBlkSize = 0;

    inValue = inStream.read();
    inCntr_idx++;

    if ((inValue >> 7) == 1) {
        inBlkSize.range(6, 0) = inValue.range(6, 0);
        inValue = inStream.read();
        inCntr_idx++;
        inBlkSize.range(13, 7) = inValue.range(6, 0);
        if ((inValue >> 7) == 1) {
            inValue = inStream.read();
            inCntr_idx++;
            inBlkSize.range(20, 14) = inValue.range(6, 0);
        }

    } else
        inBlkSize = inValue;

snappy_decompress:
    for (; inCntr_idx < input_size; inCntr_idx++) {
#pragma HLS PIPELINE II = 1
        if (read_instream)
            inValue = inStream.read();
        else
            inCntr_idx--;

        read_instream = true;
        if (next_state == READ_TOKEN) {
            if (inValue.range(1, 0) != 0) {
                next_state = READ_OFFSET;
                read_instream = false;
            } else {
                lit_len = inValue.range(7, 2);

                if (lit_len < 60) {
                    lit_len++;
                    next_state = READ_LITERAL;
                } else if (lit_len == 60) {
                    next_state = READ_LITLEN_60;
                } else if (lit_len == 61) {
                    next_state = READ_LITLEN_61;
                }
            }
        } else if (next_state == READ_LITERAL) {
            encoded_dt outValue = 0;
            outValue.range(7, 0) = inValue;
            outStream << outValue;
            lit_len--;
            if (lit_len == 0) next_state = READ_TOKEN;

        } else if (next_state == READ_OFFSET) {
            offset = 0;
            if (inValue.range(1, 0) == 1) {
                match_len = inValue.range(4, 2);
                offset.range(10, 8) = inValue.range(7, 5);
                next_state = READ_OFFSET_C01;
            } else if (inValue.range(1, 0) == 2) {
                match_len = inValue.range(7, 2);
                next_state = READ_OFFSET_C10;
            } else {
                next_state = READ_TOKEN;
                read_instream = false;
            }
        } else if (next_state == READ_OFFSET_C01) {
            offset.range(7, 0) = inValue;
            encoded_dt outValue = 0;
            outValue.range(31, 16) = match_len + 3;
            outValue.range(15, 0) = offset - 1;
            outStream << outValue;
            next_state = READ_TOKEN;
        } else if (next_state == READ_OFFSET_C10) {
            offset.range(7, 0) = inValue;
            next_state = READ_OFFSET_C10_CONT;
        } else if (next_state == READ_OFFSET_C10_CONT) {
            offset.range(15, 8) = inValue;
            encoded_dt outValue = 0;

            outValue.range(31, 16) = match_len;
            outValue.range(15, 0) = offset - 1;
            outStream << outValue;
            next_state = READ_TOKEN;

        } else if (next_state == READ_LITLEN_60) {
            lit_len = inValue + 1;
            next_state = READ_LITERAL;
        } else if (next_state == READ_LITLEN_61) {
            lit_len.range(7, 0) = inValue;
            next_state = READ_LITLEN_61_CONT;
        } else if (next_state == READ_LITLEN_61_CONT) {
            lit_len.range(15, 8) = inValue;
            lit_len = lit_len + 1;
            next_state = READ_LITERAL;
        }
    } // End of main snappy_decoder for-loop
}

template <int PARALLEL_BYTES>
static void snappyMultiByteDecompress(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                      hls::stream<uint32_t>& litlenStream,
                                      hls::stream<ap_uint<PARALLEL_BYTES * 8> >& litStream,
                                      hls::stream<ap_uint<16> >& offsetStream,
                                      hls::stream<uint32_t>& matchlenStream,
                                      hls::stream<dt_blockInfo>& blockInfoStream) {
    // Snappy Decoder states
    enum SnappyDecompressionStates { READ_TOKEN, READ_LITERAL };

    for (dt_blockInfo bInfo = blockInfoStream.read(); bInfo.compressSize != 0; bInfo = blockInfoStream.read()) {
        uint32_t input_size = bInfo.compressSize;
        enum SnappyDecompressionStates next_state = READ_TOKEN;
        ap_uint<16> offset;
        encoded_dt decodedOut = 0;
        ap_uint<32> lit_len = 0;
        uint32_t match_len = 0;
        ap_uint<8> inValue = 0, inValue1 = 0;

        uint32_t readBytes = 0, processedBytes = 0;
        uint8_t inputIdx = 0;
        ap_uint<32> inBlkSize = 0;

        const int c_parallelBit = PARALLEL_BYTES * 8;
        ap_uint<3 * c_parallelBit> input_window;
        ap_uint<c_parallelBit> outStreamValue;

        for (int i = 0; i < 3; i++) {
#pragma HLS PIPELINE II = 1
            input_window.range((i + 1) * c_parallelBit - 1, i * c_parallelBit) = inStream.read();
            readBytes += PARALLEL_BYTES;
        }

        uint8_t incrIdx = 2 * PARALLEL_BYTES;
        bool storedBlock = bInfo.storedBlock;

        if (storedBlock) {
            lit_len = input_size;
            litlenStream << lit_len;
            next_state = READ_LITERAL;
        } else {
            inValue = input_window.range((inputIdx + 1) * 8 - 1, inputIdx * 8);
            inputIdx++;

            inValue1 = input_window.range((inputIdx + 1) * 8 - 1, inputIdx * 8);
            inputIdx++;

            bool flag = false;
            bool c0 = ((inValue >> 7) == 1);
            bool c1 = ((inValue1 >> 7) == 1);
            if (c0 & c1) {
                inBlkSize.range(6, 0) = inValue.range(6, 0);
                inBlkSize.range(13, 7) = inValue1.range(6, 0);
                ap_uint<8> inValue2 = input_window.range((inputIdx + 1) * 8 - 1, inputIdx * 8);
                inputIdx++;
                inBlkSize.range(20, 14) = inValue2.range(6, 0);
                processedBytes += 3;
            } else if (c0) {
                inBlkSize.range(6, 0) = inValue.range(6, 0);
                inBlkSize.range(13, 7) = inValue1.range(6, 0);
                processedBytes += 2;
            } else {
                inBlkSize = inValue;
                inputIdx--;
                processedBytes += 1;
            }

            incrIdx -= inputIdx;
            input_window >>= inputIdx * 8;
            inputIdx = 0;
        }

    snappyMultiByteDecompress:
        for (; processedBytes + inputIdx < input_size;) {
#pragma HLS PIPELINE II = 1
            // write to input stream based on PARALLEL BYTES
            if (inputIdx >= PARALLEL_BYTES) {
                input_window >>= c_parallelBit;
                if (readBytes < input_size) {
                    ap_uint<c_parallelBit> input = inStream.read();
                    readBytes += PARALLEL_BYTES;
                    input_window.range((incrIdx + PARALLEL_BYTES) * 8 - 1, incrIdx * 8) = input;
                }
                inputIdx -= PARALLEL_BYTES;
                processedBytes += PARALLEL_BYTES;
            }

            if (next_state == READ_TOKEN) {
                ap_uint<24> inValue = input_window >> (inputIdx * 8);
                if (inValue.range(1, 0) != 0) {
                    offset = 0;
                    offset.range(7, 0) = inValue(15, 8);
                    if (inValue.range(1, 0) == 1) {
                        match_len = inValue.range(4, 2);
                        offset.range(10, 8) = inValue.range(7, 5);
                        inputIdx += 2;

                        match_len += 4;

                    } else if (inValue.range(1, 0) == 2) {
                        match_len = inValue.range(7, 2);
                        offset.range(15, 8) = inValue.range(23, 16);
                        inputIdx += 3;

                        match_len += 1;
                    }
                    next_state = READ_TOKEN;
                    litlenStream << 0;
                    offsetStream << offset;
                    matchlenStream << match_len;
                } else {
                    lit_len = inValue.range(7, 2);
                    inputIdx += 1;

                    if (lit_len == 60) {
                        inputIdx += 1;
                        lit_len = inValue.range(15, 8);
                    } else if (lit_len == 61) {
                        lit_len = inValue.range(23, 8);
                        inputIdx += 2;
                    }
                    lit_len = lit_len + 1;
                    litlenStream << lit_len;
                    next_state = READ_LITERAL;
                }
            } else if (next_state == READ_LITERAL) {
                litStream << (input_window >> (inputIdx * 8));
                if (lit_len >= PARALLEL_BYTES) {
                    inputIdx += PARALLEL_BYTES;
                    lit_len -= PARALLEL_BYTES;
                } else {
                    inputIdx += lit_len;
                    lit_len = 0;
                }
                if (lit_len == 0) {
                    next_state = READ_TOKEN;
                    matchlenStream << 0;
                    offsetStream << 0;
                } else {
                    next_state = READ_LITERAL;
                }
            } else {
                assert(0);
            }

        } // End of main snappy_decoder for-loop
    }
    // signalling end of transaction
    litlenStream << 0;
    matchlenStream << 0;
    offsetStream << 0;
}

template <int PARALLEL_BYTES, int HISTORY_SIZE>
void snappyDecompressEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                            hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                            hls::stream<bool>& outStreamEoS,
                            hls::stream<uint32_t>& outSizeStream,
                            const uint32_t _input_size) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t;
    typedef ap_uint<16> offset_dt;

    uint32_t input_size1 = _input_size;
    hls::stream<uintV_t> headerStream("headerStream");
    hls::stream<uint32_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint32_t> matchlenStream("matchlenStream");
    hls::stream<dt_blockInfo> blockInfoStream("blockInfoStream");
#pragma HLS STREAM variable = headerStream depth = 32
#pragma HLS STREAM variable = blockInfoStream depth = 32
#pragma HLS STREAM variable = litlenStream depth = 32
#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = offsetStream depth = 32
#pragma HLS STREAM variable = matchlenStream depth = 32

#pragma HLS RESOURCE variable = headerStream core = FIFO_SRL
#pragma HLS RESOURCE variable = blockInfoStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litlenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litStream core = FIFO_SRL
#pragma HLS RESOURCE variable = offsetStream core = FIFO_SRL
#pragma HLS RESOURCE variable = matchlenStream core = FIFO_SRL

#pragma HLS dataflow
    snappyHeaderProcessing<PARALLEL_BYTES>(inStream, headerStream, blockInfoStream, input_size1);
    snappyMultiByteDecompress<PARALLEL_BYTES>(headerStream, litlenStream, litStream, offsetStream, matchlenStream,
                                              blockInfoStream);
    lzMultiByteDecoder<PARALLEL_BYTES, HISTORY_SIZE, uint32_t>(litlenStream, litStream, offsetStream, matchlenStream,
                                                               outStream, outStreamEoS, outSizeStream);
}

template <int PARALLEL_BYTES, int HISTORY_SIZE>
void snappyDecompressCoreEngine(hls::stream<ap_uint<PARALLEL_BYTES * 8> >& inStream,
                                hls::stream<ap_uint<PARALLEL_BYTES * 8> >& outStream,
                                hls::stream<bool>& outStreamEoS,
                                hls::stream<uint32_t>& outSizeStream,
                                hls::stream<uint32_t>& blockSizeStream) {
    typedef ap_uint<PARALLEL_BYTES * 8> uintV_t;
    typedef ap_uint<16> offset_dt;

    hls::stream<uint32_t> litlenStream("litlenStream");
    hls::stream<uintV_t> litStream("litStream");
    hls::stream<offset_dt> offsetStream("offsetStream");
    hls::stream<uint32_t> matchlenStream("matchlenStream");
    hls::stream<dt_blockInfo> blockInfoStream("blockInfoStream");
#pragma HLS STREAM variable = litlenStream depth = 32
#pragma HLS STREAM variable = litStream depth = 32
#pragma HLS STREAM variable = offsetStream depth = 32
#pragma HLS STREAM variable = matchlenStream depth = 32
#pragma HLS STREAM variable = blockInfoStream depth = 32

#pragma HLS RESOURCE variable = litlenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litStream core = FIFO_SRL
#pragma HLS RESOURCE variable = offsetStream core = FIFO_SRL
#pragma HLS RESOURCE variable = matchlenStream core = FIFO_SRL
#pragma HLS RESOURCE variable = blockInfoStream core = FIFO_SRL

    dt_blockInfo blockInfo;
    for (int i = 0; i < 2; i++) {
        blockInfo.compressSize = blockSizeStream.read();
        blockInfo.storedBlock = 0;

        blockInfoStream << blockInfo;
    }
#pragma HLS dataflow
    snappyMultiByteDecompress<PARALLEL_BYTES>(inStream, litlenStream, litStream, offsetStream, matchlenStream,
                                              blockInfoStream);
    lzMultiByteDecoder<PARALLEL_BYTES, HISTORY_SIZE, uint32_t>(litlenStream, litStream, offsetStream, matchlenStream,
                                                               outStream, outStreamEoS, outSizeStream);
}

} // namespace compression
} // namespace xf

#endif // _XFCOMPRESSION_SNAPPY_DECOMPRESS_HPP_
