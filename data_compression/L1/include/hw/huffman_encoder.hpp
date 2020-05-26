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
#ifndef _XFCOMPRESSION_HUFFMAN_ENCODER_HPP_
#define _XFCOMPRESSION_HUFFMAN_ENCODER_HPP_

/**
 * @file huffman_encoder.hpp
 * @brief Header for module used in ZLIB huffman kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <ap_int.h>
#include "zlib_tables.hpp"
// 64bits/8bit = 8 Bytes
typedef ap_uint<16> uintOutV_t;

// 4 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encoded_dt;

// 8 * 4 = 32 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encodedV_dt;

// LZ specific Defines
#define d_code(dist, dist_code) ((dist) < 256 ? dist_code[dist] : dist_code[256 + ((dist) >> 7)])

namespace xf {
namespace compression {
namespace details {

void bitPackingSize(hls::stream<uint16_t>& inStream,
                    hls::stream<uint8_t>& inStreamSize,
                    hls::stream<uintOutV_t>& outStream,
                    hls::stream<uint16_t>& outStreamSize) {
    ap_uint<64> localBits = 0;
    uint32_t localBits_idx = 0;
    bool flag_run = false;

bitpack:
    for (uint8_t size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
#pragma HLS PIPELINE II = 1
        localBits.range(size + localBits_idx - 1, localBits_idx) = inStream.read();
        localBits_idx += size;

        if (localBits_idx >= 16) {
            ap_uint<16> pack_byte = 0;
            pack_byte = localBits.range(15, 0);
            localBits >>= 16;
            localBits_idx -= 16;
            outStreamSize << 2;
            outStream << pack_byte;
        }
        flag_run = true;
    }

    if (flag_run == false) {
        outStreamSize << 0;
        return;
    }

    int leftBits = localBits_idx % 8;
    int val = 0;

    if (leftBits) {
        // 3 bit header
        localBits.range(localBits_idx + 3 - 1, localBits_idx) = 0;
        localBits_idx += 3;

        leftBits = localBits_idx % 8;

        if (leftBits != 0) {
            val = 8 - leftBits;
            localBits.range(localBits_idx + val - 1, localBits_idx) = 0;
            localBits_idx += val;
        }

        if (localBits_idx >= 16) {
            ap_uint<16> pack_byte = 0;
            pack_byte = localBits.range(15, 0);
            localBits >>= 16;
            localBits_idx -= 16;
            outStreamSize << 2;
            outStream << pack_byte;
        }

        // Zero bytes and complement as per type0 z_sync_flush
        localBits.range(localBits_idx + 8 - 1, localBits_idx) = 0x00;
        localBits.range(localBits_idx + 16 - 1, localBits_idx + 8) = 0x00;
        localBits.range(localBits_idx + 24 - 1, localBits_idx + 16) = 0xff;
        localBits.range(localBits_idx + 32 - 1, localBits_idx + 24) = 0xff;
        localBits_idx += 32;

    } else {
        // Zero bytes and complement as per type0 z_sync_flush
        localBits.range(localBits_idx + 8 - 1, localBits_idx) = 0x00;
        localBits.range(localBits_idx + 16 - 1, localBits_idx + 8) = 0x00;
        localBits.range(localBits_idx + 24 - 1, localBits_idx + 16) = 0x00;
        localBits.range(localBits_idx + 32 - 1, localBits_idx + 24) = 0xff;
        localBits.range(localBits_idx + 40 - 1, localBits_idx + 32) = 0xff;
        localBits_idx += 40;
    }

    for (uint32_t i = 0; i < localBits_idx; i += 8) {
        uint16_t pack_byte = 0;
        pack_byte = localBits.range(7, 0);
        localBits >>= 8;
        outStreamSize << 1;
        outStream << pack_byte;
    }

    outStreamSize << 0;
}

void bitPacking(hls::stream<uint16_t>& inStream,
                hls::stream<uint8_t>& inStreamSize,
                hls::stream<uintOutV_t>& outStream,
                hls::stream<bool>& outStreamEos,
                hls::stream<uint32_t>& compressedSize) {
    ap_uint<32> localBits = 0;
    uint8_t localBits_idx = 0;
    bool flag_run = false;
    uint32_t cSize_cntr = 0;
bitpack:
    for (uint8_t size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
#pragma HLS PIPELINE II = 1
        localBits.range(size + localBits_idx - 1, localBits_idx) = inStream.read();
        localBits_idx += size;

        if (localBits_idx >= 16) {
            uint16_t pack_byte = localBits;
            localBits >>= 16;
            localBits_idx -= 16;
            outStream << pack_byte;
            outStreamEos << 0;
            cSize_cntr += 2;
        }
        flag_run = true;
    }

    if (flag_run == false) {
        outStream << 0;
        outStreamEos << 1;
        compressedSize << cSize_cntr;
        return;
    }

    int leftBits = localBits_idx % 8;
    int val = 0;
    ap_uint<64> packedBits = 0;

    if (leftBits) {
        // 3 bit header
        localBits_idx += 3;

        leftBits = localBits_idx % 8;

        if (leftBits != 0) {
            val = 8 - leftBits;
            localBits_idx += val;
        }

        // Zero bytes and complement as per type0 z_sync_flush
        uint64_t val = 0xffff0000;
        packedBits = localBits | (val << localBits_idx);
        localBits_idx += 32;

    } else {
        // Zero bytes and complement as per type0 z_sync_flush
        uint64_t val = 0xffff000000;
        packedBits = localBits | (val << localBits_idx);
        localBits_idx += 40;
    }
    for (uint32_t i = 0; i < localBits_idx; i += 16) {
        uint16_t pack_byte = packedBits;
        packedBits >>= 16;
        outStream << pack_byte;
        outStreamEos << 0;
        cSize_cntr += 2;
    }
    if (localBits_idx % 16 == 0)
        compressedSize << cSize_cntr;
    else
        compressedSize << cSize_cntr - 1;

    outStream << 0;
    outStreamEos << 1;
}
} // end details

/**
 * @brief This module does zlib/gzip dynamic huffman encoding
 * @param inStream input packet of 32bit size which contains either literal or match length and distance
 * information.
 *          Example: [Literal (1 Byte) | ML (1 Byte) | DIST (2 Bytes)]
 * @param outStream output bit encoded LZ77 compressed data
 * @param outStreamSize output Stream Size
 * @param input_size uncompressed data input size
 * @param inStreamCodes Huffman Codes
 * @param inStreamCodeSize HuffmanCode Lengths
 */
void huffmanEncoder(hls::stream<encodedV_dt>& inStream,
                    hls::stream<uint16_t>& outStream,
                    hls::stream<uint8_t>& outStreamSize,
                    uint32_t input_size,
                    hls::stream<uint16_t>& inStreamCodes,
                    hls::stream<uint8_t>& inStreamCodeSize) {
    enum huffmanEncoderState { WRITE_TOKEN, ML_DIST_REP, LIT_REP, SEND_OUTPUT, ML_EXTRA, DIST_REP, DIST_EXTRA };

    if (input_size == 0) {
        outStreamSize << 0;
        return;
    }
    const uint8_t c_length_codes = 29;
    const uint8_t c_distance_codes = 30;
    const uint16_t c_ltree_size = 286;
    const uint8_t c_dtree_size = 30;
    const uint8_t c_bltree_size = 19;
    const uint8_t c_bl_codes = 19;

    uint8_t extra_lbits[c_length_codes] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                           2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

    uint8_t extra_dbits[c_distance_codes] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                             6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    uint16_t litmtree_code[c_ltree_size];
    ap_uint<4> litmtree_blen[c_ltree_size];

    uint16_t distree_codes[c_dtree_size];
    ap_uint<4> dtree_blen[c_dtree_size];

    // Passthrough the Preamble Data coming from Treegen to BitPacker
    int inSize = inStreamCodeSize.read();
    for (; inSize != 0;) {
#pragma HLS PIPELINE II = 1
        outStream << inStreamCodes.read();
        outStreamSize << inSize;
        inSize = inStreamCodeSize.read();
    }
    for (uint16_t i = 0; i < c_ltree_size; i++) {
#pragma HLS PIPELINE II = 1
        litmtree_code[i] = inStreamCodes.read();
        litmtree_blen[i] = inStreamCodeSize.read();
    }

    for (uint8_t i = 0; i < c_dtree_size; i++) {
#pragma HLS PIPELINE II = 1
        distree_codes[i] = inStreamCodes.read();
        dtree_blen[i] = inStreamCodeSize.read();
    }

    uint32_t size = (input_size - 1) / 4 + 1;

    enum huffmanEncoderState next_state = WRITE_TOKEN;
    uint8_t tCh = 0;
    uint8_t tLen = 0;
    uint16_t tOffset = 0;

    uint16_t lcode = 0;
    uint16_t dcode = 0;
    uint8_t lextra = 0;
    uint8_t dextra = 0;

    bool flag_out = false;
huffman_loop:
    for (uint32_t inIdx = 0; (inIdx < size) || (next_state != WRITE_TOKEN);) {
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
        if (next_state == WRITE_TOKEN) {
            encoded_dt tmpEncodedValue = inStream.read();
            inIdx++;
            tCh = tmpEncodedValue.range(7, 0);
            tLen = tmpEncodedValue.range(15, 8);
            tOffset = tmpEncodedValue.range(31, 16);
            dcode = d_code(tOffset, dist_code);
            lcode = length_code[tLen];

            if (tLen > 0) {
                next_state = ML_DIST_REP;
            } else {
                outStream << litmtree_code[tCh];
                outStreamSize << litmtree_blen[tCh];

                next_state = WRITE_TOKEN;
            }
        } else if (next_state == ML_DIST_REP) {
            uint16_t code_s = litmtree_code[lcode + c_literals + 1];
            uint8_t bitlen = litmtree_blen[lcode + c_literals + 1];
            lextra = extra_lbits[lcode];

            outStream << code_s;
            outStreamSize << bitlen;

            if (lextra)
                next_state = ML_EXTRA;
            else
                next_state = DIST_REP;

        } else if (next_state == DIST_REP) {
            uint16_t code_s = distree_codes[dcode];
            uint8_t bitlen = dtree_blen[dcode];
            dextra = extra_dbits[dcode];

            outStream << code_s;
            outStreamSize << bitlen;

            if (dextra)
                next_state = DIST_EXTRA;
            else
                next_state = WRITE_TOKEN;

        } else if (next_state == ML_EXTRA) {
            tLen -= base_length[lcode];
            outStream << tLen;
            outStreamSize << lextra;

            next_state = DIST_REP;

        } else if (next_state == DIST_EXTRA) {
            tOffset -= base_dist[dcode];
            outStream << tOffset;
            outStreamSize << dextra;
            next_state = WRITE_TOKEN;
        }
    }
    // End of block as per GZip standard
    outStream << litmtree_code[256];
    outStreamSize << litmtree_blen[256];

    outStreamSize << 0;

} // Dynamic Huffman Funciton Ends here
}
}
#endif // _XFCOMPRESSION_HUFFMAN_ENCODER_HPP_
