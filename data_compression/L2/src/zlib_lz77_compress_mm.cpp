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
 * @file zlib_lz77_compress_mm.cpp
 * @brief Source for lz77 compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_lz77_compress_mm.hpp"

typedef ap_uint<64> lz77_compressd_dt;
typedef ap_uint<32> compressd_dt;

const int c_gmemBurstSize = 8;
const int kGMemDWidth = 512;
typedef ap_uint<kGMemDWidth> uintMemWidth_t;

void lz77Core(hls::stream<ap_uint<8> >& inStream,
              hls::stream<compressd_dt>& outStreamVec,
              hls::stream<bool>& outStreamVecEos,
              hls::stream<uint32_t>& outStreamTree,
              hls::stream<uint32_t>& compressedSize,
              uint32_t input_size) {
    hls::stream<compressd_dt> compressdStream("compressdStream");
    hls::stream<compressd_dt> compressdStream_1("compressdStream_1");
    hls::stream<compressd_dt> boosterStream("boosterStream");
    hls::stream<compressd_dt> boosterStream_freq("boosterStream");
    hls::stream<lz77_compressd_dt> lenOffsetOut("lenOffsetOut");
#pragma HLS STREAM variable = compressdStream depth = c_gmemBurstSize
#pragma HLS STREAM variable = compressdStream_1 depth = c_gmemBurstSize
#pragma HLS STREAM variable = boosterStream depth = c_gmemBurstSize
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmemBurstSize

#pragma HLS BIND_STORAGE variable = compressdStream core = FIFO_SRL
#pragma HLS BIND_STORAGE variable = compressdStream_1 core = FIFO_SRL
#pragma HLS BIND_STORAGE variable = boosterStream core = FIFO_SRL
#pragma HLS BIND_STORAGE variable = lenOffsetOut core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::lzCompress<MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(inStream, compressdStream, input_size);
    xf::compression::lzBestMatchFilter<MATCH_LEN, 0>(compressdStream, compressdStream_1, input_size);
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressdStream_1, boosterStream, input_size);
    xf::compression::lz77Divide(boosterStream, outStreamVec, outStreamVecEos, outStreamTree, compressedSize,
                                input_size);
}

void lz77(const uintMemWidth_t* in,
          uintMemWidth_t* out,
          const uint32_t input_idx[PARALLEL_BLOCK],
          const uint32_t output_idx[PARALLEL_BLOCK],
          const uint32_t input_size[PARALLEL_BLOCK],
          uint32_t output_size[PARALLEL_BLOCK],
          uint32_t* dyn_ltree_freq,
          uint32_t* dyn_dtree_freq) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<ap_uint<8> > inStream[PARALLEL_BLOCK];
    hls::stream<bool> outStreamVecEos[PARALLEL_BLOCK];
    hls::stream<compressd_dt> outStreamVec[PARALLEL_BLOCK];
    hls::stream<uint32_t> outStreamTreeData[PARALLEL_BLOCK];
#pragma HLS STREAM variable = outStreamVecEos depth = c_gmemBSize
#pragma HLS STREAM variable = inStream depth = c_gmemBSize
#pragma HLS STREAM variable = outStreamVec depth = c_gmemBSize
#pragma HLS STREAM variable = outStreamTreeData depth = c_gmemBSize

#pragma HLS BIND_STORAGE variable = outStreamVecEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStreamVec type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStreamTreeData type = FIFO impl = SRL

    hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK];
#pragma HLS STREAM variable = compressedSize depth = 2

#pragma HLS dataflow
    // MM2S Call
    xf::compression::details::mm2multStreamSize<8, PARALLEL_BLOCK, GMEM_DWIDTH, GMEM_BURST_SIZE>(in, input_idx,
                                                                                                 inStream, input_size);

    for (uint8_t i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        // lz77Core is instantiated based on the PARALLEL BLOCK
        lz77Core(inStream[i], outStreamVec[i], outStreamVecEos[i], outStreamTreeData[i], compressedSize[i],
                 input_size[i]);
    }

    // S2MM Call
    xf::compression::details::multStream2MMFreq<uint32_t, 32, PARALLEL_BLOCK, GMEM_DWIDTH, GMEM_BURST_SIZE>(
        outStreamVec, outStreamVecEos, compressedSize, outStreamTreeData, output_idx, out, output_size, dyn_ltree_freq,
        dyn_dtree_freq);
}

extern "C" {
void xilLz77Compress(const uintMemWidth_t* in,
                     uintMemWidth_t* out,
                     uint32_t* compressd_size,
                     uint32_t* in_block_size,
                     uint32_t* dyn_ltree_freq,
                     uint32_t* dyn_dtree_freq,
                     uint32_t block_size_in_kb,
                     uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_ltree_freq offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_dtree_freq offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_ltree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int block_idx = 0;
    int block_length = block_size_in_kb * 1024;
    int no_blocks = (input_size - 1) / block_length + 1;
    uint32_t max_block_size = block_size_in_kb * 1024;

    bool small_block[PARALLEL_BLOCK];
    uint32_t input_block_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t output_idx[PARALLEL_BLOCK];
    uint32_t output_block_size[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete

    // Figure out total blocks & block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int nblocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) {
            nblocks = no_blocks - i;
        }

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
            uint32_t idxcalc = (i + j) * max_block_size;

            if (j < nblocks) {
                uint32_t inBlockSize = in_block_size[i + j];
                input_block_size[j] = inBlockSize;
                input_idx[j] = idxcalc;
                output_idx[j] = idxcalc * 4;
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }

            output_block_size[j] = 0;
        }

        // Call for parallel compression
        lz77(in, out, input_idx, output_idx, input_block_size, output_block_size, dyn_ltree_freq, dyn_dtree_freq);

        for (int k = 0; k < nblocks; k++) {
            compressd_size[block_idx] = output_block_size[k];
            block_idx++;
        }
    }
}
}
