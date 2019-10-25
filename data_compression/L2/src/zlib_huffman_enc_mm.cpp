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
 * @file xil_huffman_kernel.cpp
 * @brief Source for huffman kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_huffman_enc_mm.hpp"

// 64bits/8bit = 8 Bytes
typedef ap_uint<16> uintOutV_t;

// 4 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encoded_dt;

// 8 * 4 = 32 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encodedV_dt;

void huffmanCore(hls::stream<xf::compression::uintMemWidth_t>& inStream512,
                 hls::stream<xf::compression::uintMemWidth_t>& outStream512,
                 hls::stream<bool>& outStream512Eos,
                 uint32_t input_size,
                 uint32_t core_idx,
                 hls::stream<uint32_t>& inStreamLTreeCode,
                 hls::stream<uint32_t>& inStreamLTreeBlen,
                 hls::stream<uint32_t>& inStreamDTreeCode,
                 hls::stream<uint32_t>& inStreamDTreeBlen,
                 hls::stream<uint32_t>& inStreamBLTreeCode,
                 hls::stream<uint32_t>& inStreamBLTreeBlen,
                 hls::stream<uint32_t>& inStreamMaxCode,
                 hls::stream<uint32_t>& compressedSize) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<encodedV_dt> inStream("inStream");
    hls::stream<uintOutV_t> huffOut("huffOut");
    hls::stream<bool> huffOutEos("huffOutEos");
    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
#pragma HLS STREAM variable = inStream depth = c_gmemBSize
#pragma HLS STREAM variable = huffOut depth = 2048
#pragma HLS STREAM variable = huffOutEos depth = c_gmemBSize
#pragma HLS STREAM variable = bitVals depth = c_gmemBSize
#pragma HLS STREAM variable = bitLen depth = c_gmemBSize

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = huffOut core = FIFO_SRL
#pragma HLS RESOURCE variable = huffOutEos core = FIFO_SRL
#pragma HLS RESOURCE variable = bitVals core = FIFO_SRL
#pragma HLS RESOURCE variable = bitLen core = FIFO_SRL

#pragma HLS dataflow
    // Read data from kernel 1 in stream downsized manner
    // Write the data back to ddr and read it in host
    xf::compression::streamDownsizer<uint32_t, GMEM_DWIDTH, 32>(inStream512, inStream, input_size);

    xf::compression::huffmanEncoder(inStream, bitVals, bitLen, input_size, inStreamLTreeCode, inStreamLTreeBlen,
                                    inStreamDTreeCode, inStreamDTreeBlen, inStreamBLTreeCode, inStreamBLTreeBlen,
                                    inStreamMaxCode);

    xf::compression::bitPacking(bitVals, bitLen, huffOut, huffOutEos, compressedSize);

    xf::compression::upsizerEos<uint16_t, 16, GMEM_DWIDTH>(huffOut, huffOutEos, outStream512, outStream512Eos);
}

void huffman(const xf::compression::uintMemWidth_t* in,
             xf::compression::uintMemWidth_t* out,
             uint32_t input_idx[PARALLEL_BLOCK],
             uint32_t output_idx[PARALLEL_BLOCK],
             uint32_t input_size[PARALLEL_BLOCK],
             uint32_t output_size[PARALLEL_BLOCK],
             uint32_t* dyn_litmtree_codes,
             uint32_t* dyn_distree_codes,
             uint32_t* dyn_bitlentree_codes,
             uint32_t* dyn_litmtree_blen,
             uint32_t* dyn_dtree_blen,
             uint32_t* dyn_bitlentree_blen,
             uint32_t* dyn_max_codes,
             uint32_t n_blocks) {
    const uint32_t c_gmemBSize = 1024;

    hls::stream<xf::compression::uintMemWidth_t> inStream512[PARALLEL_BLOCK];
    hls::stream<bool> outStream512Eos[PARALLEL_BLOCK];
    hls::stream<xf::compression::uintMemWidth_t> outStream512[PARALLEL_BLOCK];

    hls::stream<uint32_t> strlitmtree_codes[PARALLEL_BLOCK];
    hls::stream<uint32_t> strlitmtree_blen[PARALLEL_BLOCK];

    hls::stream<uint32_t> strdistree_codes[PARALLEL_BLOCK];
    hls::stream<uint32_t> strdtree_blen[PARALLEL_BLOCK];

    hls::stream<uint32_t> strbitlentree_codes[PARALLEL_BLOCK];
    hls::stream<uint32_t> strbitlentree_blen[PARALLEL_BLOCK];
    hls::stream<uint32_t> strmax_code[PARALLEL_BLOCK];
#pragma HLS STREAM variable = outStream512Eos depth = 32
#pragma HLS STREAM variable = inStream512 depth = 32
#pragma HLS STREAM variable = outStream512 depth = 32
#pragma HLS STREAM variable = strlitmtree_codes depth = c_gmemBSize
#pragma HLS STREAM variable = strlitmtree_blen depth = c_gmemBSize
#pragma HLS STREAM variable = strdistree_codes depth = c_gmemBSize
#pragma HLS STREAM variable = strdtree_blen depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_codes depth = c_gmemBSize
#pragma HLS STREAM variable = strbitlentree_blen depth = c_gmemBSize
#pragma HLS STREAM variable = strmax_code depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStream512Eos core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512 core = FIFO_SRL

    hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK];
#pragma HLS STREAM variable = compressedSize depth = c_gmemBSize
#pragma HLS RESOURCE variable = compressedSize core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::mm2sNbFreq<GMEM_DWIDTH, GMEM_BURST_SIZE, PARALLEL_BLOCK>(
        in, dyn_litmtree_codes, dyn_litmtree_blen, dyn_distree_codes, dyn_dtree_blen, dyn_bitlentree_codes,
        dyn_bitlentree_blen, dyn_max_codes, input_idx, inStream512, strlitmtree_codes, strlitmtree_blen,
        strdistree_codes, strdtree_blen, strbitlentree_codes, strbitlentree_blen, strmax_code, n_blocks, input_size);

    for (int i = 0; i < PARALLEL_BLOCK; i++) {
        huffmanCore(inStream512[i], outStream512[i], outStream512Eos[i], input_size[i], i, strlitmtree_codes[i],
                    strlitmtree_blen[i], strdistree_codes[i], strdtree_blen[i], strbitlentree_codes[i],
                    strbitlentree_blen[i], strmax_code[i], compressedSize[i]);
    }

    xf::compression::s2mmEosNb<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH, PARALLEL_BLOCK>(
        out, output_idx, outStream512, outStream512Eos, compressedSize, output_size);
}

extern "C" {
void xilHuffmanKernel(xf::compression::uintMemWidth_t* in,
                      xf::compression::uintMemWidth_t* out,
                      uint32_t* in_block_size,
                      uint32_t* compressd_size,
                      uint32_t* dyn_litmtree_codes,
                      uint32_t* dyn_distree_codes,
                      uint32_t* dyn_bitlentree_codes,
                      uint32_t* dyn_litmtree_blen,
                      uint32_t* dyn_dtree_blen,
                      uint32_t* dyn_bitlentree_blen,
                      uint32_t* dyn_max_codes,
                      uint32_t block_size_in_kb,
                      uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1

#pragma HLS INTERFACE m_axi port = dyn_litmtree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_distree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_bitlentree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_litmtree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_dtree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_bitlentree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_max_codes offset = slave bundle = gmem1

#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_litmtree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_distree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bitlentree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_litmtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bitlentree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_max_codes bundle = control
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
    uint32_t small_block_inSize[PARALLEL_BLOCK];
    uint32_t max_lit_limit[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = max_lit_limit dim = 0 complete

    // Figure out total blocks and block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int n_blocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) n_blocks = no_blocks - i;

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
            if (j < n_blocks) {
                uint32_t inBlockSize = in_block_size[i + j];

                if (inBlockSize < MIN_BLOCK_SIZE) {
                    small_block[j] = 1;
                    small_block_inSize[j] = inBlockSize;
                    input_block_size[j] = 0;
                    input_idx[j] = 0;
                } else {
                    small_block[j] = 0;
                    input_block_size[j] = inBlockSize;
                    input_idx[j] = (i + j) * max_block_size * 4;
                    output_idx[j] = (i + j) * max_block_size;
                }
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
            max_lit_limit[j] = 0;
        }

        huffman(in, out, input_idx, output_idx, input_block_size, output_block_size, dyn_litmtree_codes,
                dyn_distree_codes, dyn_bitlentree_codes, dyn_litmtree_blen, dyn_dtree_blen, dyn_bitlentree_blen,
                dyn_max_codes, n_blocks);

        for (int k = 0; k < n_blocks; k++) {
            if (max_lit_limit[k]) {
                in_block_size[block_idx] = input_block_size[k];
            } else {
                in_block_size[block_idx] = output_block_size[k];
            }

            if (small_block[k] == 1) in_block_size[block_idx] = small_block_inSize[k];

            block_idx++;
        }
    } // Main loop ends here
}
}
