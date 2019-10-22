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

#include "lz4_compress_stream.hpp"

const int kGMemBurstSize = 16;
const int c_gmemBurstSize = (2 * kGMemBurstSize);
const int c_lz4MaxLiteralCount = MAX_LIT_COUNT;
typedef ap_uint<32> compressd_dt;
typedef ap_uint<64> lz4_compressd_dt;
typedef ap_uint<8> streamDt;

extern "C" {
void xilLz4CompressStream(hls::stream<ap_axiu<8, 0, 0, 0> >& inaxistream,
                          hls::stream<ap_axiu<8, 0, 0, 0> >& outaxistream,
                          uint32_t inputSize) {
    uint32_t leftBytes = 64;

#pragma HLS interface axis port = inaxistream
#pragma HLS interface axis port = outaxistream
#pragma HLS interface s_axilite port = inputSize bundle = control
#pragma HLS interface s_axilite port = return bundle = control

    hls::stream<compressd_dt> compressdStream("compressdStream");
    hls::stream<compressd_dt> bestMatchStream("bestMatchStream");
    hls::stream<compressd_dt> boosterStream("boosterStream");
    hls::stream<uint8_t> litOut("litOut");
    hls::stream<uint32_t> compressedSize("compressedSize");
    hls::stream<streamDt> inStream("inStream");
    hls::stream<streamDt> outStream("outStream");
    hls::stream<lz4_compressd_dt> lenOffsetOut("lenOffsetOut");
    hls::stream<bool> lz4OutEos("lz4OutEos");

#pragma HLS STREAM variable = inStream depth = 2
#pragma HLS STREAM variable = outStream depth = 2
#pragma HLS STREAM variable = compressdStream depth = 8
#pragma HLS STREAM variable = bestMatchStream depth = 8
#pragma HLS STREAM variable = boosterStream depth = 8
#pragma HLS STREAM variable = litOut depth = c_lz4MaxLiteralCount
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmemBurstSize
#pragma HLS STREAM variable = lz4OutEos depth = 8

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream core = FIFO_SRL
#pragma HLS RESOURCE variable = compressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = boosterStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lenOffsetOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lz4OutEos core = FIFO_SRL

#pragma HLS dataflow
    uint32_t litLimit[1];
    litLimit[0] = 0; // max_lit_limit;

    xf::compression::kStreamRead<8>(inaxistream, inStream, inputSize);

    xf::compression::lzCompress<MATCH_LEN, MATCH_LEVEL, LZ_DICT_SIZE, BIT, MIN_OFFSET, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, inputSize, leftBytes);
    xf::compression::lzBestMatchFilter<MATCH_LEN, OFFSET_WINDOW>(compressdStream, bestMatchStream, inputSize,
                                                                 leftBytes);
    xf::compression::lzBooster<MAX_MATCH_LEN, OFFSET_WINDOW>(bestMatchStream, boosterStream, inputSize, leftBytes);
    xf::compression::lz4Divide<MAX_LIT_COUNT, PARALLEL_BLOCK>(boosterStream, litOut, lenOffsetOut, inputSize, litLimit,
                                                              0);
    xf::compression::lz4Compress(litOut, lenOffsetOut, outStream, lz4OutEos, compressedSize, inputSize);

    xf::compression::kStreamWrite<8>(outaxistream, outStream, lz4OutEos, compressedSize);
}
}
