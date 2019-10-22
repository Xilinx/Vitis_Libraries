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
#include "hls_stream.h"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define PARALLEL_BLOCK 1
#include "lz4_compress.hpp"
#include "lz_compress.hpp"
#include "lz_optional.hpp"

#define BIT 8
#define MAX_LIT_COUNT 4096
const int c_lz4MaxLiteralCount = MAX_LIT_COUNT;

typedef ap_uint<32> compressd_dt;
typedef ap_uint<64> lz4_compressd_dt;
typedef ap_uint<BIT> uintV_t;

/**
 * @brief Lz4 compression engine.
 *
 * @tparam MIN_OFFSET lowest match distance
 * @tparam MIN_MATCH minimum match length
 * @tparam LZ_MAX_OFFSET_LIMIT maximum offset limit
 * @tparam OFFSET_WINDOW maximum possible distance of the match
 * @tparam BOOSTER_OFFSET_WINDOW maximum distance of match for booster
 * @tparam LZ_DICT_SIZE dictionary size
 * @tparam MAX_MATCH_LEN maximum match length supported
 * @tparam MATCH_LEN match length
 * @tparam MATCH_LEVEL number of levels to check for match
 *
 * @param inStream input hls stream
 * @param lz4Out output hls stream
 * @param lz4Out_eos output end of stream indicator
 * @param lz4OutSize output compressed size
 * @param max_lit_limit maximum literals before match
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
void lz4_compress_engine(hls::stream<uintV_t>& inStream,
                         hls::stream<uintV_t>& lz4Out,
                         hls::stream<bool>& lz4Out_eos,
                         hls::stream<uint32_t>& lz4OutSize,
                         uint32_t max_lit_limit[PARALLEL_BLOCK],
                         uint32_t input_size,
                         uint32_t core_idx) {
    uint32_t left_bytes = 64;
    hls::stream<compressd_dt> compressdStream("compressdStream");
    hls::stream<xf::compression::compressd_dt> bestMatchStream("bestMatchStream");
    hls::stream<compressd_dt> boosterStream("boosterStream");
    hls::stream<uint8_t> litOut("litOut");
    hls::stream<lz4_compressd_dt> lenOffsetOut("lenOffsetOut");

#pragma HLS STREAM variable = compressdStream depth = 8
#pragma HLS STREAM variable = bestMatchStream depth = 8
#pragma HLS STREAM variable = boosterStream depth = 8
#pragma HLS STREAM variable = litOut depth = c_lz4MaxLiteralCount
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmemBurstSize
#pragma HLS STREAM variable = lz4Out depth = 1024
#pragma HLS STREAM variable = lz4OutSize depth = c_gmemBurstSize
#pragma HLS STREAM variable = lz4Out_eos depth = 8

#pragma HLS RESOURCE variable = compressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = boosterStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lenOffsetOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lz4Out core = FIFO_SRL
#pragma HLS RESOURCE variable = lz4OutSize core = FIFO_SRL
#pragma HLS RESOURCE variable = lz4Out_eos core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::lzCompress<MATCH_LEN, MATCH_LEVEL, LZ_DICT_SIZE, BIT, MIN_OFFSET, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, input_size, left_bytes);
    xf::compression::lzBestMatchFilter<MATCH_LEN, OFFSET_WINDOW>(compressdStream, bestMatchStream, input_size,
                                                                 left_bytes);
    xf::compression::lzBooster<MAX_MATCH_LEN, BOOSTER_OFFSET_WINDOW>(bestMatchStream, boosterStream, input_size,
                                                                     left_bytes);
    xf::compression::lz4Divide<MAX_LIT_COUNT, PARALLEL_BLOCK>(boosterStream, litOut, lenOffsetOut, input_size,
                                                              max_lit_limit, core_idx);
    xf::compression::lz4Compress(litOut, lenOffsetOut, lz4Out, lz4Out_eos, lz4OutSize, input_size);
}
