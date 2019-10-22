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
/**
 * @file xil_lz4_compress_kernel.cpp
 * @brief Source for LZ4 compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "lz4_compress_mm.hpp"

const int c_gmemBurstSize = (2 * GMEM_BURST_SIZE);
const int c_sizeStreamDepth = 8;
const int c_lz4MaxLiteralCount = MAX_LIT_COUNT;

// namespace hw_compress {

void lz4Core(hls::stream<xf::compression::uintMemWidth_t>& inStreamMemWidth,
             hls::stream<xf::compression::uintMemWidth_t>& outStreamMemWidth,
             hls::stream<bool>& outStreamMemWidthEos,
             hls::stream<uint32_t>& compressedSize,
             uint32_t max_lit_limit[PARALLEL_BLOCK],
             uint32_t input_size,
             uint32_t core_idx) {
    uint32_t left_bytes = 64;
    hls::stream<ap_uint<BIT> > inStream("inStream");
    hls::stream<xf::compression::compressd_dt> compressdStream("compressdStream");
    hls::stream<xf::compression::compressd_dt> bestMatchStream("bestMatchStream");
    hls::stream<xf::compression::compressd_dt> boosterStream("boosterStream");
    hls::stream<uint8_t> litOut("litOut");
    hls::stream<xf::compression::lz4_compressd_dt> lenOffsetOut("lenOffsetOut");
    hls::stream<ap_uint<8> > lz4Out("lz4Out");
    hls::stream<bool> lz4Out_eos("lz4Out_eos");
#pragma HLS STREAM variable = inStream depth = 8
#pragma HLS STREAM variable = compressdStream depth = 8
#pragma HLS STREAM variable = bestMatchStream depth = 8
#pragma HLS STREAM variable = boosterStream depth = 8
#pragma HLS STREAM variable = litOut depth = c_lz4MaxLiteralCount
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmemBurstSize
#pragma HLS STREAM variable = lz4Out depth = 8
#pragma HLS STREAM variable = lz4Out_eos depth = 8

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = compressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = boosterStream core = FIFO_SRL
#pragma HLS RESOURCE variable = lenOffsetOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lz4Out core = FIFO_SRL
#pragma HLS RESOURCE variable = lz4Out_eos core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::streamDownsizer<uint32_t, GMEM_DWIDTH, 8>(inStreamMemWidth, inStream, input_size);
    xf::compression::lzCompress<MATCH_LEN, MATCH_LEVEL, LZ_DICT_SIZE, BIT, MIN_OFFSET, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, input_size, left_bytes);
    xf::compression::lzBestMatchFilter<MATCH_LEN, OFFSET_WINDOW>(compressdStream, bestMatchStream, input_size,
                                                                 left_bytes);
    xf::compression::lzBooster<MAX_MATCH_LEN, BOOSTER_OFFSET_WINDOW>(bestMatchStream, boosterStream, input_size,
                                                                     left_bytes);
    xf::compression::lz4Divide<MAX_LIT_COUNT, PARALLEL_BLOCK>(boosterStream, litOut, lenOffsetOut, input_size,
                                                              max_lit_limit, core_idx);
    xf::compression::lz4Compress(litOut, lenOffsetOut, lz4Out, lz4Out_eos, compressedSize, input_size);
    xf::compression::upsizerEos<uint16_t, BIT, GMEM_DWIDTH>(lz4Out, lz4Out_eos, outStreamMemWidth,
                                                            outStreamMemWidthEos);
}

/**
 * @brief LZ4 compression kernel top.
 *
 * @param in input stream width
 * @param out output stream width
 * @param input_idx output size
 * @param output_idx intput size
 * @param input_size input size
 * @param max_lit_limit intput size
 */
void lz4(const xf::compression::uintMemWidth_t* in,
         xf::compression::uintMemWidth_t* out,
         const uint32_t input_idx[PARALLEL_BLOCK],
         const uint32_t output_idx[PARALLEL_BLOCK],
         const uint32_t input_size[PARALLEL_BLOCK],
         uint32_t output_size[PARALLEL_BLOCK],
         uint32_t max_lit_limit[PARALLEL_BLOCK]) {
    hls::stream<xf::compression::uintMemWidth_t> inStreamMemWidth[PARALLEL_BLOCK];
    hls::stream<bool> outStreamMemWidthEos[PARALLEL_BLOCK];
    hls::stream<xf::compression::uintMemWidth_t> outStreamMemWidth[PARALLEL_BLOCK];
#pragma HLS STREAM variable = outStreamMemWidthEos depth = 2
#pragma HLS STREAM variable = inStreamMemWidth depth = c_gmemBurstSize
#pragma HLS STREAM variable = outStreamMemWidth depth = c_gmemBurstSize

#pragma HLS RESOURCE variable = outStreamMemWidthEos core = FIFO_SRL
#pragma HLS RESOURCE variable = inStreamMemWidth core = FIFO_SRL
#pragma HLS RESOURCE variable = outStreamMemWidth core = FIFO_SRL

    hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK];
    uint32_t left_bytes = 64;

#pragma HLS dataflow
    xf::compression::mm2sNb<GMEM_DWIDTH, GMEM_BURST_SIZE, PARALLEL_BLOCK>(in, input_idx, inStreamMemWidth, input_size);
    for (uint8_t i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        // lz4Core is instantiated based on the PARALLEL_BLOCK
        lz4Core(inStreamMemWidth[i], outStreamMemWidth[i], outStreamMemWidthEos[i], compressedSize[i], max_lit_limit,
                input_size[i], i);
    }

    xf::compression::s2mmEosNb<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH, PARALLEL_BLOCK>(
        out, output_idx, outStreamMemWidth, outStreamMemWidthEos, compressedSize, output_size);
}
//} // namespace end

extern "C" {
/**
 * @brief LZ4 compression kernel.
 *
 * @param in input stream width
 * @param out output stream width
 * @param compressd_size output size
 * @param in_block_size intput size
 * @param block_size_in_kb intput size
 * @param input_size input size
 */
void xilLz4Compress

    (const xf::compression::uintMemWidth_t* in,
     xf::compression::uintMemWidth_t* out,
     uint32_t* compressd_size,
     uint32_t* in_block_size,
     uint32_t block_size_in_kb,
     uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS data_pack variable = in
#pragma HLS data_pack variable = out

    uint32_t block_idx = 0;
    uint32_t block_length = block_size_in_kb * 1024;
    uint32_t no_blocks = (input_size - 1) / block_length + 1;
    uint32_t max_block_size = block_size_in_kb * 1024;

    bool small_block[PARALLEL_BLOCK];
    uint32_t input_block_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t output_idx[PARALLEL_BLOCK];
    uint32_t output_block_size[PARALLEL_BLOCK];
    uint32_t max_lit_limit[PARALLEL_BLOCK];
    uint32_t small_block_inSize[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = max_lit_limit dim = 0 complete

    // Figure out total blocks & block sizes
    for (uint32_t i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        uint32_t nblocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) {
            nblocks = no_blocks - i;
        }

        for (uint32_t j = 0; j < PARALLEL_BLOCK; j++) {
            if (j < nblocks) {
                uint32_t inBlockSize = in_block_size[i + j];
                if (inBlockSize < MIN_BLOCK_SIZE) {
                    small_block[j] = 1;
                    small_block_inSize[j] = inBlockSize;
                    input_block_size[j] = 0;
                    input_idx[j] = 0;
                } else {
                    small_block[j] = 0;
                    input_block_size[j] = inBlockSize;
                    input_idx[j] = (i + j) * max_block_size;
                    output_idx[j] = (i + j) * max_block_size;
                }
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
            max_lit_limit[j] = 0;
        }

        // Call for parallel compression
        lz4(in, out, input_idx, output_idx, input_block_size, output_block_size, max_lit_limit);

        for (uint32_t k = 0; k < nblocks; k++) {
            if (max_lit_limit[k]) {
                compressd_size[block_idx] = input_block_size[k];
            } else {
                compressd_size[block_idx] = output_block_size[k];
            }

            if (small_block[k] == 1) {
                compressd_size[block_idx] = small_block_inSize[k];
            }
            block_idx++;
        }
    }
}
}
