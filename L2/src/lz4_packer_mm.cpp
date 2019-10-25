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
 * @file lz4_packer_mm.cpp
 * @brief Source for LZ4 P2P compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "lz4_packer_mm.hpp"

void lz4(const uint512_t* in,
         uint512_t* out,
         uint512_t* head_prev_blk,
         uint32_t* compressd_size,
         uint32_t* in_block_size,
         uint32_t* encoded_size,
         uint512_t* orig_input_data,
         uint32_t no_blocks,
         uint32_t head_res_size,
         uint32_t offset,
         uint32_t block_size_in_kb,
         uint32_t tail_bytes) {
    hls::stream<uint512_t> inStream512("inStream512_mm2s");
    hls::stream<uintV_t> inStreamV("inStreamV_dsizer");
    hls::stream<uintV_t> packStreamV("packerStreamOut");
    hls::stream<uint512_t> outStream512("UpsizeStreamOut");
#pragma HLS STREAM variable = inStream512 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStreamV depth = c_gmem_burst_size
#pragma HLS STREAM variable = packStreamV depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = inStream512 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStreamV core = FIFO_SRL
#pragma HLS RESOURCE variable = packStreamV core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512 core = FIFO_SRL

    hls::stream<uint32_t> mm2sStreamSize("mm2sOutSize");
    hls::stream<uint32_t> downStreamSize("dstreamOutSize");
    hls::stream<uint32_t> packStreamSize("packOutSize");
    hls::stream<uint32_t> upStreamSize("upStreamSize");
#pragma HLS STREAM variable = mm2sStreamSize depth = c_gmem_burst_size
#pragma HLS STREAM variable = downStreamSize depth = c_gmem_burst_size
#pragma HLS STREAM variable = packStreamSize depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = mm2sStreamSize core = FIFO_SRL
#pragma HLS RESOURCE variable = downStreamSize core = FIFO_SRL
#pragma HLS RESOURCE variable = packStreamSize core = FIFO_SRL
#pragma HLS RESOURCE variable = upStreamSize core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::mm2s<GMEM_DWIDTH, GMEM_BURST_SIZE>(in, head_prev_blk, orig_input_data, inStream512, mm2sStreamSize,
                                                        compressd_size, in_block_size, no_blocks, block_size_in_kb,
                                                        head_res_size, offset);
    xf::compression::streamDownSizerP2PComp<GMEM_DWIDTH, PACK_WIDTH>(inStream512, inStreamV, mm2sStreamSize,
                                                                     downStreamSize, no_blocks);
    encoded_size[0] = xf::compression::lz4Packer<PACK_WIDTH, PARLLEL_BYTE>(
        inStreamV, packStreamV, downStreamSize, packStreamSize, block_size_in_kb, no_blocks, head_res_size, tail_bytes);
    xf::compression::streamUpsizerP2P<GMEM_DWIDTH, PACK_WIDTH>(packStreamV, outStream512, packStreamSize, upStreamSize);
    xf::compression::s2mm<GMEM_DWIDTH>(outStream512, out, upStreamSize);
}

extern "C" {
void xilLz4Packer(const uint512_t* in,
                  uint512_t* out,
                  uint512_t* head_prev_blk,
                  uint32_t* compressd_size,
                  uint32_t* in_block_size,
                  uint32_t* encoded_size,
                  uint512_t* orig_input_data,
                  uint32_t head_res_size,
                  uint32_t offset,
                  uint32_t block_size_in_kb,
                  uint32_t no_blocks,
                  uint32_t tail_bytes) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = head_prev_blk offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = encoded_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = orig_input_data offset = slave bundle = gmem0
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = head_prev_blk bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = encoded_size bundle = control
#pragma HLS INTERFACE s_axilite port = orig_input_data bundle = control
#pragma HLS INTERFACE s_axilite port = head_res_size bundle = control
#pragma HLS INTERFACE s_axilite port = offset bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = no_blocks bundle = control
#pragma HLS INTERFACE s_axilite port = tail_bytes bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    lz4(in, out, head_prev_blk, compressd_size, in_block_size, encoded_size, orig_input_data, no_blocks, head_res_size,
        offset, block_size_in_kb, tail_bytes);

    return;
}
}
