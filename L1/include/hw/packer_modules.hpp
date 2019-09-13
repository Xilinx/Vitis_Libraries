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
#ifndef _XFCOMPRESSION_PACKER_MODULES_HPP_
#define _XFCOMPRESSION_PACKER_MODULES_HPP_

/**
 * @file packer_modules.hpp
 * @brief Header for modules used for streaming to memory mapped interface conversion.
 *
 * This file is part of XF Compression Library.
 */

#include "common.h"

#define PACK_WIDTH 64
#define GMEM_DWIDTH 512

namespace xf {
namespace compression {
/**
 * @brief Streaming to memory mapped interface conversion
 *
 * @param inStream output memory address
 * @param out output index
 * @param inStreamSize input stream size
 */
void packerS2mm(hls::stream<uintMemWidth_t>& inStream, uintMemWidth_t* out, hls::stream<uint32_t>& inStreamSize) {
    const int c_byteSize = 8;
    const int c_factor = GMEM_DWIDTH / c_byteSize;

    uint32_t outIdx = 0;
    uint32_t size = 1;
    uint32_t sizeIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
    mwr:
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
            out[outIdx + i] = inStream.read();
        }
        outIdx += size;
    }
}

/**
 * @brief Memory mapped to streaming interface conversion used with packer kernel
 *
 * @tparam DATAWIDTH width of data bus
 * @tparam BURST_SIZE burst size of the data transfers
 *
 * @param in intput memory address
 * @param head_prev_blk intput index
 * @param orig_input_data intput index
 * @param outStream output stream
 * @param outStreamSize intput stream size
 * @param compressd_size intput stream size
 * @param in_block_size intput stream size
 * @param no_blocks intput stream size
 * @param block_size_in_kb intput stream size
 * @param head_res_size intput stream size
 * @param offset intput stream size
 */
template <int DATAWIDTH, int BURST_SIZE>
void packerMm2s(const uintMemWidth_t* in,
                uintMemWidth_t* head_prev_blk,
                uintMemWidth_t* orig_input_data,
                hls::stream<ap_uint<DATAWIDTH> >& outStream,
                hls::stream<uint32_t>& outStreamSize,
                uint32_t* compressd_size,
                uint32_t* in_block_size,
                uint32_t no_blocks,
                uint32_t block_size_in_kb,
                uint32_t head_res_size,
                uint32_t offset) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    ap_uint<DATAWIDTH> buffer[BURST_SIZE];
#pragma HLS RESOURCE variable = buffer core = RAM_2P_LUTRAM

    uint32_t offset_gmem = offset ? offset / 64 : 0;

    // Handle header or residue here
    uint32_t block_stride = block_size_in_kb * 1024 / 64;

    uint32_t blkCompSize = 0;
    uint32_t origSize = 0;
    uint32_t sizeInWord = 0;
    uint32_t byteSize = 0;
    // Run over number of blocks
    for (int bIdx = 0; bIdx < no_blocks + 1; bIdx++) {
        if (bIdx == 0) {
            sizeInWord = head_res_size ? ((head_res_size - 1) / c_wordSize + 1) : 0;
            byteSize = head_res_size;
        } else {
            blkCompSize = compressd_size[bIdx - 1];
            origSize = in_block_size[bIdx - 1];
            // Put compress block & input block
            // into streams for next block
            sizeInWord = (blkCompSize - 1) / c_wordSize + 1;
            byteSize = blkCompSize;
        }

        // Send size in bytes
        outStreamSize << byteSize;

        // printf("[ %s ]blkCompSize %d origSize %d sizeInWord_512 %d offset %d head_res_size %d\n", __FUNCTION__,
        // blkCompSize, origSize, sizeInWord, offset, head_res_size);

        // Copy data from global memory to local
        // Put it into stream
        for (uint32_t i = 0; i < sizeInWord; i += BURST_SIZE) {
            uint32_t chunk_size = BURST_SIZE;

            if (i + BURST_SIZE > sizeInWord) chunk_size = sizeInWord - i;

        memrd1:
            for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                if (bIdx == 0)
                    buffer[j] = head_prev_blk[(offset_gmem + i) + j];
                else if (blkCompSize == origSize)
                    buffer[j] = orig_input_data[(block_stride * (bIdx - 1) + i) + j];
                else
                    buffer[j] = in[(block_stride * (bIdx - 1) + i) + j];
            }

        memrd2:
            for (uint32_t j = 0; j < chunk_size; j++) {
#pragma HLS PIPELINE II = 1
                outStream << buffer[j];
            }
        }
    }
    // printf("%s Done \n", __FUNCTION__);
    outStreamSize << 0;
}

/**
 * @brief Upsize the data stream from packer kernel
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param inStreamSize input stream size
 * @param outStreamSize output stream size
 */
void packerStreamUpSizer(hls::stream<ap_uint<64> >& inStream,
                         hls::stream<ap_uint<512> >& outStream,
                         hls::stream<uint32_t>& inStreamSize,
                         hls::stream<uint32_t>& outStreamSize) {
    const int c_byteWidth = 8;
    const int c_upsizeFactor = GMEM_DWIDTH / c_byteWidth;
    const int c_inSize = PACK_WIDTH / c_byteWidth;

    // Declaring double buffers
    ap_uint<2 * GMEM_DWIDTH> outBuffer = 0;
    uint32_t byteIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // printf("Size %d \n", size);
        // Rounding off the output size
        uint32_t outSize = (size * c_byteWidth + byteIdx) / PACK_WIDTH;

        if (outSize) outStreamSize << outSize;
    streamUpsizer:
        for (int i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
            // printf("val/size %d/%d \n", i, size);
            ap_uint<PACK_WIDTH> tmpValue = inStream.read();
            outBuffer.range((byteIdx + c_inSize) * c_byteWidth - 1, byteIdx * c_byteWidth) = tmpValue;
            byteIdx += c_byteWidth;

            if (byteIdx >= c_upsizeFactor) {
                outStream << outBuffer.range(GMEM_DWIDTH - 1, 0);
                outBuffer >>= GMEM_DWIDTH;
                byteIdx -= c_upsizeFactor;
            }
        }
    }

    if (byteIdx) {
        outStreamSize << 1;
        outStream << outBuffer.range(GMEM_DWIDTH - 1, 0);
    }
    // printf("%s Done \n", __FUNCTION__);
    // end of block
    outStreamSize << 0;
}

/**
 * @brief Downsize the data stream for use by packer kernel.
 *
 * @tparam IN_WIDTH input width
 * @tparam OUT_WIDTH output width
 *
 * @param inStream input stream
 * @param outStream output stream
 * @param inStreamSize input stream size
 * @param outStreamSize output stream size
 * @param no_blocks number of blocks
 */
template <int IN_WIDTH, int OUT_WIDTH>
void packerStreamDownSizer(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                           hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                           hls::stream<uint32_t>& inStreamSize,
                           hls::stream<uint32_t>& outStreamSize,
                           uint32_t no_blocks) {
    const int c_byteWidth = 8;
    const int c_inputWord = IN_WIDTH / c_byteWidth;
    const int c_outWord = OUT_WIDTH / c_byteWidth;

    int factor = c_inputWord / c_outWord;
    ap_uint<IN_WIDTH> inBuffer = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        // input size interms of 512width * 64 bytes after downsizing
        uint32_t sizeOutputV = (size - 1) / c_outWord + 1;

        // Send ouputSize of the module
        outStreamSize << size;

    //       printf("[ %s ] sizeOutputV %d input_size %d size_4m_mm2s %d \n", __FUNCTION__, sizeOutputV, input_size,
    //       size);

    conv512toV:
        for (int i = 0; i < sizeOutputV; i++) {
#pragma HLS PIPELINE II = 1
            int idx = i % factor;
            if (idx == 0) inBuffer = inStream.read();
            ap_uint<OUT_WIDTH> tmpValue = inBuffer.range((idx + 1) * PACK_WIDTH - 1, idx * PACK_WIDTH);
            outStream << tmpValue;
        }
    }
    // printf("%s Done \n", __FUNCTION__);
    // outStreamSize << 0;
}

} // namespace compression
} // namespace xf

#undef PACK_WIDTH
#undef GMEM_DWIDTH

#endif // _XFCOMPRESSION_PACKER_MODULES_HPP_
