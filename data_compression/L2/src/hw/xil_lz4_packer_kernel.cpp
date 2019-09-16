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
 * @file xil_lz4_packer_kernel.cpp
 * @brief Source for LZ4 compression kernel.
 *
 * This file is part of XF Compression Library.
 */

#include "lz4_packer_kernel.hpp"

#define GMEM_DWIDTH 512
#define GMEM_BURST_SIZE 16

// Byte length
#define PACKER_PARALLEL_BYTE 8

//#define MAGIC_BYTE_1 4
//#define MAGIC_BYTE_2 34
//#define MAGIC_BYTE_3 77
//#define MAGIC_BYTE_4 24
//#define FLG_BYTE 104

// Below are the codes as per LZ4 standard for
// various maximum block sizes supported.
//#define BSIZE_STD_64KB 64
//#define BSIZE_STD_256KB 80
//#define BSIZE_STD_1024KB 96
//#define BSIZE_STD_4096KB 112

uint32_t packer(hls::stream<ap_uint<PACK_WIDTH> >& inStream,
                hls::stream<ap_uint<PACK_WIDTH> >& outStream,
                hls::stream<uint32_t>& inStreamSize,
                hls::stream<uint32_t>& outStreamSize,
                uint32_t block_size_in_kb,
                uint32_t no_blocks,
                uint32_t head_res_size,
                uint32_t tail_bytes) {
    // 16 bytes can be held in on shot
    ap_uint<2 * PACK_WIDTH> lcl_buffer;
    uint32_t lbuf_idx = 0;

    uint32_t cBlen = 4;

    uint32_t endSizeCnt = 0;
    uint32_t sizeOutput = 0;
    uint32_t word_head = 0;
    uint32_t over_size = 0;
    uint32_t flag = 0;
    uint32_t size_cntr = 0;
// Core packer logic
packer:
    for (int blkIdx = 0; blkIdx < no_blocks + 1; blkIdx++) {
        uint32_t size = inStreamSize.read();
        // printf("lbuf %d \n", lbuf_idx);
        // Find out the compressed size header
        // This value is sent by mm2s module
        // by using compressed_size[] buffer picked from
        // LZ4 compression kernel
        if (blkIdx == 0) {
            uint32_t word_head;
            if (head_res_size < 8)
                word_head = 1;
            else
                word_head = head_res_size / 8;
            sizeOutput = word_head;
            endSizeCnt += head_res_size;
        } else {
            // Size is nothing but 8bytes * 8 gives original input
            over_size = lbuf_idx + cBlen + size;
            endSizeCnt += cBlen + size;
            // 64bit size value including headers etc
            sizeOutput = over_size / 8;
        }
        // printf("%s - trailSize %d size %d lbuf %d word_64size %d over_size_pblick %d \n", __FUNCTION__, trailSize,
        // size, lbuf_idx, sizeOutput, over_size);
        // Send the size of output to next block
        outStreamSize << sizeOutput;

        if (blkIdx != 0) {
            // Update local buffer with compress size of current block - 4Bytes
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size;
            lbuf_idx++;
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size >> 8;
            lbuf_idx++;
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size >> 16;
            lbuf_idx++;
            lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = size >> 24;
            lbuf_idx++;
        }

        if (lbuf_idx >= 8) {
            outStream << lcl_buffer.range(63, 0);
            lcl_buffer >>= PACK_WIDTH;
            lbuf_idx -= 8;
        }
        // printf("%s size %d sizeOutput %d \n", __FUNCTION__, size, sizeOutput);

        uint32_t chunk_size = 0;
    // Packer logic
    // Read compressed data of a block
    // Stream it to upsizer
    pack_post:
        for (int i = 0; i < size; i += PACKER_PARALLEL_BYTE) {
#pragma HLS PIPELINE II = 1

            if (i + chunk_size > size)
                chunk_size = size - i;
            else
                chunk_size = PACKER_PARALLEL_BYTE;

            // Update local buffer with new set of data
            // Update local buffer index
            lcl_buffer.range((lbuf_idx * 8) + PACK_WIDTH - 1, lbuf_idx * 8) = inStream.read();
            lbuf_idx += chunk_size;

            if (lbuf_idx >= 8) {
                outStream << lcl_buffer.range(63, 0);
                lcl_buffer >>= PACK_WIDTH;
                lbuf_idx -= 8;
            }
        } // End of main packer loop
    }

    // printf("End of packer \n");
    if (tail_bytes) {
        // Trailing bytes based on LZ4 standard
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
        lcl_buffer.range((lbuf_idx * 8) + 8 - 1, lbuf_idx * 8) = 0;
        lbuf_idx++;
    }
    // printf("flag %d lbuf_idx %d\n", flag, lbuf_idx);

    uint32_t extra_size = (lbuf_idx - 1) / 8 + 1;
    if (lbuf_idx) outStreamSize << extra_size;

    while (lbuf_idx) {
        // printf("Ssent the data \n");
        outStream << lcl_buffer.range(63, 0);

        if (lbuf_idx >= 8) {
            lcl_buffer >>= PACK_WIDTH;
            lbuf_idx -= 8;
        } else {
            lbuf_idx = 0;
        }
    }

    // printf("%s Done \n", __FUNCTION__);
    // Termination condition of
    // next block
    outStreamSize << 0;
    // printf("endsizeCnt %d \n", endSizeCnt);
    return endSizeCnt;
}

void packerCore(const xf::compression::uintMemWidth_t* in,
                xf::compression::uintMemWidth_t* out,
                xf::compression::uintMemWidth_t* head_prev_blk,
                uint32_t* compressd_size,
                uint32_t* in_block_size,
                uint32_t* encoded_size,
                xf::compression::uintMemWidth_t* orig_input_data,
                uint32_t no_blocks,
                uint32_t head_res_size,
                uint32_t offset,
                uint32_t block_size_in_kb,
                uint32_t tail_bytes) {
    hls::stream<xf::compression::uintMemWidth_t> inStream512("inStream512_mm2s");
    hls::stream<uintV_t> inStreamV("inStreamV_dsizer");
    hls::stream<uintV_t> packStreamV("packerStreamOut");
    hls::stream<xf::compression::uintMemWidth_t> outStream512("UpsizeStreamOut");
#pragma HLS STREAM variable = inStream512 depth = c_gmemBurstSize
#pragma HLS STREAM variable = inStreamV depth = c_gmemBurstSize
#pragma HLS STREAM variable = packStreamV depth = c_gmemBurstSize
#pragma HLS STREAM variable = outStream512 depth = c_gmemBurstSize

#pragma HLS RESOURCE variable = inStream512 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStreamV core = FIFO_SRL
#pragma HLS RESOURCE variable = packStreamV core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512 core = FIFO_SRL

    hls::stream<uint32_t> mm2sStreamSize("mm2sOutSize");
    hls::stream<uint32_t> downStreamSize("dstreamOutSize");
    hls::stream<uint32_t> packStreamSize("packOutSize");
    hls::stream<uint32_t> upStreamSize("upStreamSize");
#pragma HLS STREAM variable = mm2sStreamSize depth = c_gmemBurstSize
#pragma HLS STREAM variable = downStreamSize depth = c_gmemBurstSize
#pragma HLS STREAM variable = packStreamSize depth = c_gmemBurstSize

#pragma HLS RESOURCE variable = mm2sStreamSize core = FIFO_SRL
#pragma HLS RESOURCE variable = downStreamSize core = FIFO_SRL
#pragma HLS RESOURCE variable = packStreamSize core = FIFO_SRL
#pragma HLS RESOURCE variable = upStreamSize core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::packerMm2s<GMEM_DWIDTH, GMEM_BURST_SIZE>(in, head_prev_blk, orig_input_data, inStream512,
                                                              mm2sStreamSize, compressd_size, in_block_size, no_blocks,
                                                              block_size_in_kb, head_res_size, offset);
    xf::compression::packerStreamDownSizer<GMEM_DWIDTH, PACK_WIDTH>(inStream512, inStreamV, mm2sStreamSize,
                                                                    downStreamSize, no_blocks);
    encoded_size[0] = packer(inStreamV, packStreamV, downStreamSize, packStreamSize, block_size_in_kb, no_blocks,
                             head_res_size, tail_bytes);
    xf::compression::packerStreamUpSizer(packStreamV, outStream512, packStreamSize, upStreamSize);
    xf::compression::packerS2mm(outStream512, out, upStreamSize);
}

extern "C" {
/**
 * @brief LZ4 compression kernel.
 *
 * @param in input stream width
 * @param out output stream width
 * @param head_prev_blk output size
 * @param compressd_size output size
 * @param in_block_size intput size
 * @param encoded_size intput size
 * @param orig_input_data intput size
 * @param head_res_size intput size
 * @param offset intput size
 * @param block_size_in_kb intput size
 * @param no_blocks input size
 * @param tail_bytes intput size
 */
void xilLz4Packer(const xf::compression::uintMemWidth_t* in,
                  xf::compression::uintMemWidth_t* out,
                  xf::compression::uintMemWidth_t* head_prev_blk,
                  uint32_t* compressd_size,
                  uint32_t* in_block_size,
                  uint32_t* encoded_size,
                  xf::compression::uintMemWidth_t* orig_input_data,
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

    packerCore(in, out, head_prev_blk, compressd_size, in_block_size, encoded_size, orig_input_data, no_blocks,
               head_res_size, offset, block_size_in_kb, tail_bytes);

    return;
}
}
