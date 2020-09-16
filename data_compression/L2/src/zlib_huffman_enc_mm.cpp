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
 * @file zlib_huffman_enc_mm.cpp
 * @brief Source for huffman kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_huffman_enc_mm.hpp"
#include "huffman_treegen.hpp"
// 64bits/8bit = 8 Bytes
typedef ap_uint<16> uintOutV_t;

// 4 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encoded_dt;

// 8 * 4 = 32 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encodedV_dt;
void huffmanCore(hls::stream<ap_uint<32> >& inStreamVec,
                 hls::stream<uintOutV_t>& outStreamVec,
                 hls::stream<uint32_t>& compressSize,
                 hls::stream<bool>& outStreamEos,
                 uint32_t input_size,
                 uint32_t core_idx,
                 hls::stream<uint16_t>& inStreamTree,
                 hls::stream<uint8_t>& inStreamSize) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
#pragma HLS STREAM variable = bitVals depth = c_gmemBSize
#pragma HLS STREAM variable = bitLen depth = c_gmemBSize

#pragma HLS BIND_STORAGE variable = bitVals type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = bitLen type = FIFO impl = SRL

#pragma HLS dataflow
    // Read data from kernel 1 in stream downsized manner
    // Write the data back to ddr and read it in host

    xf::compression::huffmanEncoder(inStreamVec, bitVals, bitLen, input_size, inStreamTree, inStreamSize);

    xf::compression::details::bitPacking(bitVals, bitLen, outStreamVec, outStreamEos, compressSize);
}

void huffman(const xf::compression::uintMemWidth_t* in,
             uint32_t* lit_freq,
             uint32_t* dist_freq,
             xf::compression::uintMemWidth_t* out,
             uint32_t input_idx[PARALLEL_BLOCK],
             uint32_t output_idx[PARALLEL_BLOCK],
             uint32_t input_size[PARALLEL_BLOCK],
             uint32_t output_size[PARALLEL_BLOCK],
             uint32_t n_blocks) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<ap_uint<32> > inStreamVec[PARALLEL_BLOCK];
    hls::stream<uint32_t> outStreamSize[PARALLEL_BLOCK];
    hls::stream<uintOutV_t> outStreamVec[PARALLEL_BLOCK];
    hls::stream<bool> outStreamEos[PARALLEL_BLOCK];

    hls::stream<uint16_t> codeStream[PARALLEL_BLOCK];
    hls::stream<uint8_t> codeSize[PARALLEL_BLOCK];
#pragma HLS STREAM variable = outStreamSize depth = c_gmemBSize
#pragma HLS STREAM variable = inStreamVec depth = c_gmemBSize
#pragma HLS STREAM variable = outStreamVec depth = c_gmemBSize
#pragma HLS STREAM variable = outStreamEos depth = c_gmemBSize
#pragma HLS STREAM variable = codeStream depth = 3
#pragma HLS STREAM variable = codeSize depth = 3
#pragma HLS BIND_STORAGE variable = outStreamSize type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = inStreamVec type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStreamVec type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStreamEos type = FIFO impl = SRL

#pragma HLS dataflow
    for (uint8_t i = 0; i < n_blocks; i++) {
        xf::compression::zlibTreegenInMMOutStream(&(lit_freq[i * 1024]), &(dist_freq[i * 64]), codeStream[i],
                                                  codeSize[i]);
    }
    xf::compression::details::mm2multStreamSize<32, PARALLEL_BLOCK, GMEM_DWIDTH, GMEM_BURST_SIZE>(
        in, input_idx, inStreamVec, input_size);

    for (uint8_t i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        huffmanCore(inStreamVec[i], outStreamVec[i], outStreamSize[i], outStreamEos[i], input_size[i], 0, codeStream[i],
                    codeSize[i]);
    }

    xf::compression::details::multStream2MM<16, PARALLEL_BLOCK, GMEM_DWIDTH, GMEM_BURST_SIZE>(
        outStreamVec, outStreamEos, outStreamSize, output_idx, out, output_size);
}

extern "C" {
void xilHuffmanKernel(xf::compression::uintMemWidth_t* in,
                      uint32_t* dyn_ltree_freq,
                      uint32_t* dyn_dtree_freq,
                      xf::compression::uintMemWidth_t* out,
                      uint32_t* in_block_size,
                      uint32_t* compressd_size,
                      uint32_t block_size_in_kb,
                      uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = dyn_ltree_freq offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_dtree_freq offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1

#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_ltree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
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

    // Figure out total blocks and block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int n_blocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) n_blocks = no_blocks - i;

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
            uint32_t idxcalc = (i + j) * max_block_size;
            if (j < n_blocks) {
                uint32_t inBlockSize = in_block_size[i + j];
                small_block[j] = 0;
                input_block_size[j] = inBlockSize;
                input_idx[j] = idxcalc * 4;
                output_idx[j] = idxcalc;
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
        }

        int lit_idx = i * 1024;
        int dist_idx = i * 64;

        huffman(in, &(dyn_ltree_freq[lit_idx]), &(dyn_dtree_freq[dist_idx]), out, input_idx, output_idx,
                input_block_size, output_block_size, n_blocks);

        for (int k = 0; k < n_blocks; k++) {
            in_block_size[block_idx] = output_block_size[k];
            block_idx++;
        }
    } // Main loop ends here
}
}
