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
#ifndef _XFCOMPRESSION_LZ_DECOMPRESS_HPP_
#define _XFCOMPRESSION_LZ_DECOMPRESS_HPP_

/**
 * @file lz_decompress.hpp
 * @brief Header for modules used in LZ4 and snappy decompression kernels.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

namespace xf {
namespace compression {

typedef ap_uint<32> compressd_dt;

/**
 * @brief This module writes the literals to the output stream as is
 * and when match length and offset are read, the literals will be read from
 * the local dictionary based on offset until match length.
 *
 * @tparam HISTORY_SIZE history size
 * @tparam READ_STATE read state
 * @tparam MATCH_STATE match state
 * @tparam LOW_OFFSET_STATE low offset state
 * @tparam LOW_OFFSET low offset
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param original_size original size
 */

template <int HISTORY_SIZE, int READ_STATE, int MATCH_STATE, int LOW_OFFSET_STATE, int LOW_OFFSET>
void lzDecompress(hls::stream<compressd_dt>& inStream, hls::stream<ap_uint<8> >& outStream, uint32_t original_size) {
    uint8_t local_buf[HISTORY_SIZE];
#pragma HLS dependence variable = local_buf inter false

    uint32_t match_len = 0;
    uint32_t out_len = 0;
    uint32_t match_loc = 0;
    uint32_t length_extract = 0;
    uint8_t next_states = READ_STATE;
    uint16_t offset = 0;
    compressd_dt nextValue;
    ap_uint<8> outValue = 0;
    ap_uint<8> prevValue[LOW_OFFSET];
#pragma HLS ARRAY PARTITION variable = prevValue dim = 0 complete
lz_decompress:
    for (uint32_t i = 0; i < original_size; i++) {
#pragma HLS PIPELINE II = 1
        if (next_states == READ_STATE) {
            nextValue = inStream.read();
            offset = nextValue.range(15, 0);
            length_extract = nextValue.range(31, 16);
            if (length_extract) {
                match_loc = i - offset - 1;
                match_len = length_extract + 1;
                // printf("HISTORY=%x\n",(uint8_t)outValue);
                out_len = 1;
                if (offset >= LOW_OFFSET) {
                    next_states = MATCH_STATE;
                    outValue = local_buf[match_loc % HISTORY_SIZE];
                } else {
                    next_states = LOW_OFFSET_STATE;
                    outValue = prevValue[offset];
                }
                match_loc++;
            } else {
                outValue = nextValue.range(7, 0);
                // printf("LITERAL=%x\n",(uint8_t)outValue);
            }
        } else if (next_states == LOW_OFFSET_STATE) {
            outValue = prevValue[offset];
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        } else {
            outValue = local_buf[match_loc % HISTORY_SIZE];
            // printf("HISTORY=%x\n",(uint8_t)outValue);
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        }
        local_buf[i % HISTORY_SIZE] = outValue;
        outStream << outValue;
        for (uint32_t pIdx = LOW_OFFSET - 1; pIdx > 0; pIdx--) {
#pragma HLS UNROLL
            prevValue[pIdx] = prevValue[pIdx - 1];
        }
        prevValue[0] = outValue;
    }
}

template <int HISTORY_SIZE, int READ_STATE, int MATCH_STATE, int LOW_OFFSET_STATE, int LOW_OFFSET>
uint32_t lzDecompressZlibEos(hls::stream<compressd_dt>& inStream,
                             hls::stream<bool>& inStream_eos,
                             hls::stream<ap_uint<8> >& outStream,
                             hls::stream<bool>& outStream_eos) {
    uint8_t local_buf[HISTORY_SIZE];
#pragma HLS dependence variable = local_buf inter false

    uint32_t match_len = 0;
    uint32_t out_len = 0;
    uint32_t match_loc = 0;
    uint32_t length_extract = 0;
    uint8_t next_states = READ_STATE;
    uint16_t offset = 0;
    compressd_dt nextValue;
    ap_uint<8> outValue = 0;
    ap_uint<8> prevValue[LOW_OFFSET];
#pragma HLS ARRAY PARTITION variable = prevValue dim = 0 complete
    uint32_t i = 0;
    uint32_t out_cntr = 0;

lz_decompress:
    for (bool eos_flag = inStream_eos.read(); eos_flag == false; i++, eos_flag = inStream_eos.read()) {
#pragma HLS PIPELINE II = 1
        ////printme("Start of the loop %d state %d \n", i, next_states);
        if (next_states == READ_STATE) {
            nextValue = inStream.read();
            offset = nextValue.range(15, 0);
            length_extract = nextValue.range(31, 16);
            // printme("offset %d length_extract %d \n", offset, length_extract);
            if (length_extract) {
                match_loc = i - offset;
                match_len = length_extract;
                ////printme("HISTORY=%x\n",(uint8_t)outValue);
                out_len = 1;
                if (offset >= LOW_OFFSET) {
                    next_states = MATCH_STATE;
                    outValue = local_buf[match_loc % HISTORY_SIZE];
                } else {
                    next_states = LOW_OFFSET_STATE;
                    outValue = prevValue[offset];
                }
                match_loc++;
            } else {
                outValue = nextValue.range(7, 0);
            }
        } else if (next_states == LOW_OFFSET_STATE) {
            outValue = prevValue[offset];
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        } else {
            outValue = local_buf[match_loc % HISTORY_SIZE];
            ////printme("HISTORY=%x\n",(uint8_t)outValue);
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        }
        local_buf[i % HISTORY_SIZE] = outValue;

        outStream << outValue;
        out_cntr++;
        outStream_eos << 0;

        // printme("%c", (uint8_t)outValue);
        for (uint32_t pIdx = LOW_OFFSET - 1; pIdx > 0; pIdx--) {
#pragma HLS UNROLL
            prevValue[pIdx] = prevValue[pIdx - 1];
        }
        prevValue[0] = outValue;
    }
    // printme("Exited main for-llop \n");

    outStream << 0;
    outStream_eos << 1;

    return out_cntr;
}

template <int HISTORY_SIZE, int LOW_OFFSET>
uint32_t lzDecompressZlibEos_new(hls::stream<compressd_dt>& inStream,
                                 hls::stream<bool>& inStream_eos,
                                 hls::stream<ap_uint<8> >& outStream,
                                 hls::stream<bool>& outStream_eos,
                                 hls::stream<uint32_t>& outSize_val) {
    enum lz_d_states { READ_STATE, MATCH_STATE, LOW_OFFSET_STATE };
    uint8_t local_buf[HISTORY_SIZE];
#pragma HLS dependence variable = local_buf inter false

    uint32_t match_len = 0;
    uint32_t out_len = 0;
    uint32_t match_loc = 0;
    uint32_t length_extract = 0;
    lz_d_states next_states = READ_STATE;
    uint16_t offset = 0;
    compressd_dt nextValue, currValue;
    ap_uint<8> outValue = 0;
    ap_uint<8> prevValue[LOW_OFFSET];
#pragma HLS ARRAY PARTITION variable = prevValue dim = 0 complete
    uint32_t out_cntr = 0;

    bool eos_flag = inStream_eos.read();
    nextValue = inStream.read();
lz_decompress:
    for (uint32_t i = 0; (eos_flag == false) || (next_states != READ_STATE); i++) {
#pragma HLS PIPELINE II = 1
        ////printme("Start of the loop %d state %d \n", i, next_states);
        if (next_states == READ_STATE) {
            currValue = nextValue;
            eos_flag = inStream_eos.read();
            nextValue = inStream.read();
            offset = currValue.range(15, 0);
            length_extract = currValue.range(31, 16);
            // printme("offset %d length_extract %d \n", offset, length_extract);
            if (length_extract) {
                match_loc = i - offset;
                match_len = length_extract;
                ////printme("HISTORY=%x\n",(uint8_t)outValue);
                out_len = match_len - 1;
                if (offset >= LOW_OFFSET) {
                    next_states = MATCH_STATE;
                    outValue = local_buf[match_loc % HISTORY_SIZE];
                } else {
                    next_states = LOW_OFFSET_STATE;
                    outValue = prevValue[offset - 1];
                }
                match_loc++;
            } else {
                outValue = currValue.range(7, 0);
            }
        } else if (next_states == LOW_OFFSET_STATE) {
            outValue = prevValue[offset - 1];
            match_loc++;
            if (out_len == 1) {
                next_states = READ_STATE;
            }
            if (out_len) {
                out_len--;
            }
        } else {
            outValue = local_buf[match_loc % HISTORY_SIZE];
            ////printme("HISTORY=%x\n",(uint8_t)outValue);
            match_loc++;
            if (out_len == 1) {
                next_states = READ_STATE;
            }
            if (out_len) {
                out_len--;
            }
        }
        local_buf[i % HISTORY_SIZE] = outValue;

        outStream << outValue;
        out_cntr++;
        outStream_eos << 0;

        // printme("%c", (uint8_t)outValue);
        for (uint32_t pIdx = LOW_OFFSET - 1; pIdx > 0; pIdx--) {
#pragma HLS UNROLL
            prevValue[pIdx] = prevValue[pIdx - 1];
        }
        prevValue[0] = outValue;
    }
    // printme("Exited main for-llop \n");

    outStream << 0;
    outStream_eos << 1;

    outSize_val << out_cntr;
}

template <int HISTORY_SIZE, int READ_STATE, int MATCH_STATE, int LOW_OFFSET_STATE, int LOW_OFFSET>
void lzDecompressZlib(hls::stream<compressd_dt>& inStream,
                      hls::stream<ap_uint<8> >& outStream,
                      hls::stream<uint32_t>& outStreamSize,
                      uint32_t original_size) {
    uint8_t local_buf[HISTORY_SIZE];
#pragma HLS dependence variable = local_buf inter false
    uint32_t out_cntr = 0;
    uint32_t match_len = 0;
    uint32_t out_len = 0;
    uint32_t match_loc = 0;
    uint32_t length_extract = 0;
    uint8_t next_states = READ_STATE;
    uint16_t offset = 0;
    compressd_dt nextValue;
    ap_uint<8> outValue = 0;
    ap_uint<8> prevValue[LOW_OFFSET];
#pragma HLS ARRAY PARTITION variable = prevValue dim = 0 complete
lz_decompress:
    for (uint32_t i = 0; i < original_size; i++) {
#pragma HLS PIPELINE II = 1
        if (next_states == READ_STATE) {
            nextValue = inStream.read();
            offset = nextValue.range(15, 0);
            length_extract = nextValue.range(31, 16);
            // printf("offset %d length_extract %d \n", offset, length_extract);
            if (length_extract) {
                match_loc = i - offset;
                match_len = length_extract;
                // printf("HISTORY=%x\n",(uint8_t)outValue);
                out_len = 1;
                if (offset >= LOW_OFFSET) {
                    next_states = MATCH_STATE;
                    outValue = local_buf[match_loc % HISTORY_SIZE];
                } else {
                    next_states = LOW_OFFSET_STATE;
                    outValue = prevValue[offset];
                }
                match_loc++;
            } else {
                outValue = nextValue.range(7, 0);
            }
        } else if (next_states == LOW_OFFSET_STATE) {
            outValue = prevValue[offset];
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        } else {
            outValue = local_buf[match_loc % HISTORY_SIZE];
            // printf("HISTORY=%x\n",(uint8_t)outValue);
            match_loc++;
            out_len++;
            if (out_len == match_len) next_states = READ_STATE;
        }
        local_buf[i % HISTORY_SIZE] = outValue;
        outStream << outValue;
        out_cntr++;

        if (out_cntr >= 512) {
            outStreamSize << out_cntr;
            out_cntr = 0;
        }

        // printf("%c", (uint8_t)outValue);
        for (uint32_t pIdx = LOW_OFFSET - 1; pIdx > 0; pIdx--) {
#pragma HLS UNROLL
            prevValue[pIdx] = prevValue[pIdx - 1];
        }
        prevValue[0] = outValue;
    }

    if (out_cntr) outStreamSize << out_cntr;

    outStreamSize << 0;

    // printf("\n");
}

} // namespace compression
} // namespace xf
#endif
