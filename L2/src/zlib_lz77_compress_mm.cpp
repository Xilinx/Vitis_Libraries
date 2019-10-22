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
 * @file xil_lz77_compress_kernel.cpp
 * @brief Source for lz77 compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */
#include "zlib_lz77_compress_mm.hpp"

// Dynamic Huffman Frequency counts
// Based on GZip spec
#include "zlib_tables.hpp"

typedef ap_uint<OUT_BYTES * BIT> uintOut_t;
typedef ap_uint<DICT_ELE_WIDTH> uintDict_t;
typedef ap_uint<MATCH_LEVEL * DICT_ELE_WIDTH> uintDictV_t;
typedef ap_uint<MATCH_LEN * BIT> uintMatchV_t;
typedef ap_uint<2 * OUT_BYTES * BIT> uintDoubleOut_t;
typedef ap_uint<BIT * 32> compressdV_dt;
typedef ap_uint<64> lz77_compressd_dt;
typedef ap_uint<32> compressd_dt;
const int kGMemBurstSize = 16;
const int c_gmemBurstSize = (2 * kGMemBurstSize);
const int kGMemDWidth = 512;
typedef ap_uint<kGMemDWidth> uintMemWidth_t;

void lz77Divide(hls::stream<compressd_dt>& inStream,
                hls::stream<ap_uint<32> >& outStream,
                hls::stream<bool>& endOfStream,
                hls::stream<uint32_t>& outStreamTree,
                hls::stream<uint32_t>& compressedSize,
                uint32_t input_size,
                uint32_t core_idx) {
    if (input_size == 0) {
        compressedSize << 0;
        outStream << 0;
        endOfStream << 1;
        outStreamTree << 9999;
        return;
    }

    uint32_t lcl_dyn_ltree[LTREE_SIZE];
    uint32_t lcl_dyn_dtree[DTREE_SIZE];

ltree_init:
    for (uint32_t i = 0; i < LTREE_SIZE; i++) lcl_dyn_ltree[i] = 0;

dtree_init:
    for (uint32_t i = 0; i < DTREE_SIZE; i++) lcl_dyn_dtree[i] = 0;

    int length = 0;
    int code = 0;
    int n = 0;

    uint32_t out_cntr = 0;
    uint8_t match_len = 0;
    uint32_t loc_idx = 0;

    compressd_dt nextEncodedValue = inStream.read();
    uint32_t cSizeCntr = 0;
lz77_divide:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = lcl_dyn_ltree inter false
#pragma HLS dependence variable = lcl_dyn_dtree inter false
        compressd_dt tmpEncodedValue = nextEncodedValue;
        if (i < (input_size - 1)) nextEncodedValue = inStream.read();

        uint8_t tCh = tmpEncodedValue.range(7, 0);
        uint8_t tLen = tmpEncodedValue.range(15, 8);
        uint16_t tOffset = tmpEncodedValue.range(31, 16);
        uint32_t ltreeIdx, dtreeIdx;
        if (tLen > 0) {
            ltreeIdx = length_code[tLen - 3] + LITERALS + 1;
            dtreeIdx = d_code(tOffset, dist_code);

            i += (tLen - 1);
            tmpEncodedValue.range(15, 8) = tLen - 3;
        } else {
            ltreeIdx = tCh;
            dtreeIdx = 63;
        }
        lcl_dyn_ltree[ltreeIdx]++;
        lcl_dyn_dtree[dtreeIdx]++;

        outStream << tmpEncodedValue;
        endOfStream << 0;
        cSizeCntr++;
    }

    compressedSize << (cSizeCntr * 4);
    outStream << 0;
    endOfStream << 1;

    for (uint32_t i = 0; i < LTREE_SIZE; i++) outStreamTree << lcl_dyn_ltree[i];

    for (uint32_t j = 0; j < DTREE_SIZE; j++) outStreamTree << lcl_dyn_dtree[j];
}

void lz77Core(hls::stream<uintMemWidth_t>& inStream512,
              hls::stream<uintMemWidth_t>& outStream512,
              hls::stream<bool>& outStream512Eos,
              hls::stream<uint32_t>& outStreamTree,
              hls::stream<uint32_t>& compressedSize,
              uint32_t max_lit_limit[PARALLEL_BLOCK],
              uint32_t input_size,
              uint32_t core_idx) {
    uint32_t left_bytes = 64;
    hls::stream<ap_uint<BIT> > inStream("inStream");
    hls::stream<compressd_dt> compressdStream("compressdStream");
    hls::stream<compressd_dt> boosterStream("boosterStream");
    hls::stream<compressd_dt> boosterStream_freq("boosterStream");
    hls::stream<uint8_t> litOut("litOut");
    hls::stream<lz77_compressd_dt> lenOffsetOut("lenOffsetOut");
    hls::stream<ap_uint<32> > lz77Out("lz77Out");
    hls::stream<bool> lz77Out_eos("lz77Out_eos");
#pragma HLS STREAM variable = inStream depth = c_gmemBurstSize
#pragma HLS STREAM variable = compressdStream depth = c_gmemBurstSize
#pragma HLS STREAM variable = boosterStream depth = c_gmemBurstSize
#pragma HLS STREAM variable = litOut depth = max_literal_count
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmemBurstSize
#pragma HLS STREAM variable = lz77Out depth = 1024
#pragma HLS STREAM variable = lz77Out_eos depth = c_gmemBurstSize

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = compressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = boosterStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lenOffsetOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lz77Out core = FIFO_SRL
#pragma HLS RESOURCE variable = lz77Out_eos core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::streamDownsizer<uint32_t, GMEM_DWIDTH, 8>(inStream512, inStream, input_size);
    xf::compression::lzCompress<MATCH_LEN, MATCH_LEVEL, LZ_DICT_SIZE, BIT, MIN_OFFSET, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, input_size, left_bytes);
    xf::compression::lzBooster<MAX_MATCH_LEN, OFFSET_WINDOW>(compressdStream, boosterStream, input_size, left_bytes);
    lz77Divide(boosterStream, lz77Out, lz77Out_eos, outStreamTree, compressedSize, input_size, core_idx);
    xf::compression::upsizerEos<uint16_t, 32, GMEM_DWIDTH>(lz77Out, lz77Out_eos, outStream512, outStream512Eos);
}

void lz77(const uintMemWidth_t* in,
          uintMemWidth_t* out,
          const uint32_t input_idx[PARALLEL_BLOCK],
          const uint32_t output_idx[PARALLEL_BLOCK],
          const uint32_t input_size[PARALLEL_BLOCK],
          uint32_t output_size[PARALLEL_BLOCK],
          uint32_t max_lit_limit[PARALLEL_BLOCK],
          uint32_t* dyn_ltree_freq,
          uint32_t* dyn_dtree_freq) {
    const uint32_t c_gmemBSize = 32;

    hls::stream<uintMemWidth_t> inStreamMemWidth[PARALLEL_BLOCK];
    hls::stream<bool> outStreamMemWidthEos[PARALLEL_BLOCK];
    hls::stream<uintMemWidth_t> outStreamMemWidth[PARALLEL_BLOCK];
    hls::stream<uint32_t> outStreamTreeData[PARALLEL_BLOCK];
#pragma HLS STREAM variable = outStreamMemWidthEos depth = c_gmemBSize
#pragma HLS STREAM variable = inStreamMemWidth depth = c_gmemBSize
#pragma HLS STREAM variable = outStreamMemWidth depth = c_gmemBSize
#pragma HLS STREAM variable = outStreamTreeData depth = c_gmemBSize

#pragma HLS RESOURCE variable = outStreamMemWidthEos core = FIFO_SRL
#pragma HLS RESOURCE variable = inStreamMemWidth core = FIFO_SRL
#pragma HLS RESOURCE variable = outStreamMemWidth core = FIFO_SRL
#pragma HLS RESOURCE variable = outStreamTreeData core = FIFO_SRL

    hls::stream<uint32_t> compressedSize[PARALLEL_BLOCK];

#pragma HLS dataflow
    // MM2S Call
    xf::compression::mm2sNb<GMEM_DWIDTH, GMEM_BURST_SIZE, PARALLEL_BLOCK>(in, input_idx, inStreamMemWidth, input_size);

    for (uint8_t i = 0; i < PARALLEL_BLOCK; i++) {
#pragma HLS UNROLL
        // lz77Core is instantiated based on the PARALLEL BLOCK
        lz77Core(inStreamMemWidth[i], outStreamMemWidth[i], outStreamMemWidthEos[i], outStreamTreeData[i],
                 compressedSize[i], max_lit_limit, input_size[i], i);
    }

    // S2MM Call
    xf::compression::s2mmEosNbFreq<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH, PARALLEL_BLOCK>(
        out, output_idx, outStreamMemWidth, outStreamMemWidthEos, outStreamTreeData, compressedSize, output_size,
        input_size, dyn_ltree_freq, dyn_dtree_freq);
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
    uint32_t max_lit_limit[PARALLEL_BLOCK];
    uint32_t small_block_inSize[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = max_lit_limit dim = 0 complete

    // Figure out total blocks & block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int nblocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) {
            nblocks = no_blocks - i;
        }

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
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
                    output_idx[j] = (i + j) * max_block_size * 4;
                }
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
            max_lit_limit[j] = 0;
        }

        // Call for parallel compression
        lz77(in, out, input_idx, output_idx, input_block_size, output_block_size, max_lit_limit, dyn_ltree_freq,
             dyn_dtree_freq);

        for (int k = 0; k < nblocks; k++) {
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
