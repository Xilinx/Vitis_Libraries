/*
 * Copyright 2019 Xilinx, Inc.
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
 */

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "hls_stream.h"
#include "lz_compress.hpp"
#include "lz_optional.hpp"

#define PARALLEL_BLOCK 1
#include "snappy_compress.hpp"
#ifdef LARGE_LIT_RANGE
#define MAX_LIT_COUNT 4090
#define MAX_LIT_STREAM_SIZE 4096
#else
#define MAX_LIT_COUNT 60
#define MAX_LIT_STREAM_SIZE 64
#endif

const int c_snappyMaxLiteralStream = MAX_LIT_STREAM_SIZE;
#define BIT 8

typedef ap_uint<BIT> uintV_t;

/**
 * @brief Snappy compress engine which runs over the snappy core modules.
 *
 * @tparam MIN_OFFSET lowest match distance supported
 * @tparam MIN_MATCH  minimum match length
 * @tparam LZ_MAX_OFFSET_LIMIT maximum offset limit
 * @tparam OFFSET_WINDOW maximum distance of the match that can go
 * @tparam BOOSTER_OFFSET_WINDOW maximum distance of the match that can go
 * @tparam LZ_DICT_SIZE dictionary size
 * @tparam MAX_MATCH_LEN max match length supported
 * @tparam MATCH_LEN match length
 * @tparam MATCH_LEVEL number of levels for a match
 *
 * @param inStream input data from Stream
 * @param snappyOut snappy output data
 * @param snappyOut_eos end of stream for output
 * @param compressedSize compressed size stream
 * @param max_lit_limit maximum literals can go before match
 * @param input_size input data size
 * @param core_idx engine index
 */

template <int MIN_OFFSET,
          int MIN_MATCH,
          int LZ_MAX_OFFSET_LIMIT,
          int OFFSET_WINDOW,
          int BOOSTER_OFFSET_WINDOW,
          int LZ_DICT_SIZE,
          int MAX_MATCH_LEN,
          int MATCH_LEN,
          int MATCH_LEVEL>
void snappy_compress_engine(hls::stream<ap_uint<8> >& inStream,
                            hls::stream<ap_uint<8> >& snappyOut,
                            hls::stream<bool>& snappyOut_eos,
                            hls::stream<uint32_t>& compressedSize,
                            uint32_t max_lit_limit[PARALLEL_BLOCK],
                            uint32_t input_size,
                            uint32_t core_idx) {
    uint32_t left_bytes = 64;
    hls::stream<xf::compression::compressd_dt> compressdStream("compressdStream");
    hls::stream<xf::compression::compressd_dt> bestMatchStream("bestMatchStream");
    hls::stream<xf::compression::compressd_dt> boosterStream("boosterStream");
    hls::stream<uint8_t> litOut("litOut");
    hls::stream<xf::compression::snappy_compressd_dt> lenOffsetOut("lenOffsetOut");

#pragma HLS STREAM variable = compressdStream depth = 8
#pragma HLS STREAM variable = bestMatchStream depth = 8
#pragma HLS STREAM variable = boosterStream depth = 8
#pragma HLS STREAM variable = litOut depth = c_snappyMaxLiteralStream
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmemBurstSize
#pragma HLS STREAM variable = snappyOut depth = 8

#pragma HLS RESOURCE variable = compressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = boosterStream core = FIFO_SRL
#pragma HLS RESOURCE variable = lenOffsetOut core = FIFO_SRL
#pragma HLS RESOURCE variable = snappyOut core = FIFO_SRL
#pragma HLS RESOURCE variable = snappyOut_eos core = FIFO_SRL

#pragma HLS dataflow

    xf::compression::lzCompress<MATCH_LEN, MATCH_LEVEL, LZ_DICT_SIZE, BIT, MIN_OFFSET, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, input_size, left_bytes);
    xf::compression::lzBestMatchFilter<MATCH_LEN, OFFSET_WINDOW>(compressdStream, bestMatchStream, input_size,
                                                                 left_bytes);
    xf::compression::lzBooster<MAX_MATCH_LEN, OFFSET_WINDOW>(bestMatchStream, boosterStream, input_size, left_bytes);
    xf::compression::snappyDivide<MAX_LIT_COUNT, MAX_LIT_STREAM_SIZE, PARALLEL_BLOCK>(
        boosterStream, litOut, lenOffsetOut, input_size, max_lit_limit, core_idx);
    xf::compression::snappyCompress(litOut, lenOffsetOut, snappyOut, snappyOut_eos, compressedSize, input_size);
}
