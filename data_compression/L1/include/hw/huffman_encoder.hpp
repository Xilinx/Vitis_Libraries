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

#include <ap_int.h>
#include "zlib_tables.hpp"
// 64bits/8bit = 8 Bytes
typedef ap_uint<16> uintOutV_t;

// 4 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encoded_dt;

// 8 * 4 = 32 Bytes containing LL (1), ML (1), OFset (2)
typedef ap_uint<32> encodedV_dt;

#define WRITE_TOKEN 0
#define ML_DIST_REP 1
#define LIT_REP 2
#define SEND_OUTPUT 3
#define ML_EXTRA 4
#define DIST_REP 5
#define DIST_EXTRA 6

// LZ specific Defines
#define BIT 8

#define d_code(dist, dist_code) ((dist) < 256 ? dist_code[dist] : dist_code[256 + ((dist) >> 7)])

namespace xf {
namespace compression {

void bitPacking(hls::stream<uint16_t>& inStream,
                hls::stream<uint8_t>& inStreamSize,
                hls::stream<uintOutV_t>& outStream,
                hls::stream<bool>& outStreamEos,
                hls::stream<uint32_t>& compressedSize) {
    ap_uint<64> localBits = 0;
    uint32_t localBits_idx = 0;
    bool flag_run = false;
    uint32_t cSize_cntr = 0;
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

    if (leftBits) {
        // 3 bit header
        localBits.range(localBits_idx + 3 - 1, localBits_idx) = 0;
        localBits_idx += 3;

        leftBits = localBits_idx % 8;

        if (leftBits != 0) {
            val = BIT - leftBits;
            localBits.range(localBits_idx + val - 1, localBits_idx) = 0;
            localBits_idx += val;
        }

        if (localBits_idx >= 16) {
            ap_uint<16> pack_byte = 0;
            pack_byte = localBits.range(15, 0);
            localBits >>= 16;
            localBits_idx -= 16;
            outStream << pack_byte;
            outStreamEos << 0;
            cSize_cntr += 2;
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
    for (uint32_t i = 0; i < localBits_idx; i += 16) {
        uint16_t pack_byte = 0;
        pack_byte = localBits.range(15, 0);
        localBits >>= 16;
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

/**
 * @brief This module does zlib/gzip dynamic huffman encoding
 * @param inStream input packet of 32bit size which contains either literal or match length and distance
 * information.
 *          Example: [Literal (1 Byte) | ML (1 Byte) | DIST (2 Bytes)]
 * @param outStream output bit encoded LZ77 compressed data
 * @param outStreamSize output Stream Size
 * @param input_size uncompressed data input size
 * @param inStreamLCodes Huffman Literal Codes
 * @param inStreamLBLen  Huffman Literal Code Bit Lengths
 * @param inStreamDCodes Huffman Distance Codes
 * @param inStreamDBLen Huffman Distance Code Bit Lengths
 * @param inStreamBLCodes Huffman BitLength Codes
 * @param inStreamBLCodes Huffman BitLength Code Bit Lengths
 * @param inStreamMaxCode Maximum codes for literal, distance and bitlengths
 */
void huffmanEncoder(hls::stream<encodedV_dt>& inStream,
                    hls::stream<uint16_t>& outStream,
                    hls::stream<uint8_t>& outStreamSize,
                    uint32_t input_size,
                    hls::stream<uint32_t>& inStreamLCodes,
                    hls::stream<uint32_t>& inStreamLBlen,
                    hls::stream<uint32_t>& inStreamDCodes,
                    hls::stream<uint32_t>& inStreamDBlen,
                    hls::stream<uint32_t>& inStreamBLCodes,
                    hls::stream<uint32_t>& inStreamBLBlen,
                    hls::stream<uint32_t>& inStreamMaxCode) {
    if (input_size == 0) {
        outStreamSize << 0;
        return;
    }

    uint8_t extra_lbits[LENGTH_CODES] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                         2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

    uint8_t extra_dbits[DISTANCE_CODES] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                           6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    uint16_t litmtree_codes[LTREE_SIZE];
    uint8_t litmtree_blen[LTREE_SIZE];

    uint16_t distree_codes[DTREE_SIZE];
    uint8_t dtree_blen[DTREE_SIZE];

    uint16_t bitlentree_codes[BLTREE_SIZE];
    uint8_t bitlentree_blen[BLTREE_SIZE];

    for (uint16_t i = 0; i < LTREE_SIZE; i++) {
        litmtree_codes[i] = inStreamLCodes.read();
        litmtree_blen[i] = inStreamLBlen.read();
    }

    for (uint16_t i = 0; i < DTREE_SIZE; i++) {
        distree_codes[i] = inStreamDCodes.read();
        dtree_blen[i] = inStreamDBlen.read();
    }

    for (uint16_t i = 0; i < BLTREE_SIZE; i++) {
        bitlentree_codes[i] = inStreamBLCodes.read();
        bitlentree_blen[i] = inStreamBLBlen.read();
    }

    uint16_t lit_max_code = inStreamMaxCode.read();
    uint16_t dst_max_code = inStreamMaxCode.read();
    uint16_t bl_max_code = inStreamMaxCode.read();

    uint8_t bitlen_vals[BL_CODES] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    // This holds output stream data
    uintOutV_t tmpOut;

    uint32_t out_cntr = 0;
    uint32_t total_outCntr = 0;

    uint32_t size = (input_size - 1) / 4 + 1;
    uint32_t outByteCnt = 0;
    uint8_t length = 0;
    uint8_t value1, value2;

    ap_uint<64> localBits = 0;
    uint32_t localBits_idx = 0;

    // Remove this 5 to 4 for final version
    uint8_t start_of_block = 4;
    uint8_t start_of_block_len = 3;

    // Send Dynamic Huffman Block flag ------ Step 1

    // All the blocks run in kernel are no end of blocks
    // Add a special end of block in host with block rep = 3.
    outStream << start_of_block;
    outStreamSize << start_of_block_len;

    // Send Dynamic Huffman Trees Literas/Distances --------- Step 2

    // lcodes
    outStream << ((lit_max_code + 1) - 257);
    outStreamSize << 5;

    // dcodes
    outStream << ((dst_max_code + 1) - 1);
    outStreamSize << 5;

    // blcodes
    outStream << ((bl_max_code + 1) - 4);
    outStreamSize << 4;

// Send BL length data
send_bltree:
    for (int rank = 0; rank < bl_max_code + 1; rank++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
#pragma HLS PIPELINE II = 1
        outStream << bitlentree_blen[bitlen_vals[rank]];
        outStreamSize << 3;
    } // BL data copy loop

    int prevlen = -1;               // Last emitted Length
    int curlen = 0;                 // Length of Current Code
    int nextlen = litmtree_blen[0]; // Length of next code
    int count = 0;
    int max_count = 7; // Max repeat count
    int min_count = 4; // Min repeat count

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

send_ltree:
    for (int n = 0; n <= lit_max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 286 max = 286
        curlen = nextlen;
        nextlen = litmtree_blen[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
        lit_cnt:
            for (uint8_t cnt = count; cnt != 0; --cnt) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
#pragma HLS PIPELINE II = 1
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
            }
            count = 0;

        } else if (curlen != 0) {
            if (curlen != prevlen) {
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
                count--;
            }
            uint16_t blen = bitlentree_blen[REUSE_PREV_BLEN];
            uint16_t code = bitlentree_codes[REUSE_PREV_BLEN];
            outStream << code;
            outStreamSize << blen;

            outStream << count - 3;
            outStreamSize << 2;

        } else if (count <= 10) {
            uint16_t blen = bitlentree_blen[REUSE_ZERO_BLEN];
            uint16_t code = bitlentree_codes[REUSE_ZERO_BLEN];
            outStream << code;
            outStreamSize << blen;

            outStream << count - 3;
            outStreamSize << 3;

        } else {
            outStream << bitlentree_codes[REUSE_ZERO_BLEN_7];
            outStreamSize << bitlentree_blen[REUSE_ZERO_BLEN_7];

            outStream << count - 11;
            outStreamSize << 7;
        }

        count = 0;
        prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }

    } // send_ltree for-loop ends here

    prevlen = -1;            // Last emitted Length
    curlen = 0;              // Length of Current Code
    nextlen = dtree_blen[0]; // Length of next code
    count = 0;
    max_count = 7; // Max repeat count
    min_count = 4; // Min repeat count

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

send_dtree:
    for (int n = 0; n <= dst_max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
        curlen = nextlen;
        nextlen = dtree_blen[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
        dist_cnt:
            for (uint8_t cnt = count; cnt != 0; --cnt) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
#pragma HLS PIPELINE II = 1
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
            }
            count = 0;
        } else if (curlen != 0) {
            if (curlen != prevlen) {
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
                count--;
            }
            uint16_t blen = bitlentree_blen[REUSE_PREV_BLEN];
            uint16_t code = bitlentree_codes[REUSE_PREV_BLEN];
            outStream << code;
            outStreamSize << blen;
            outStream << count - 3;
            outStreamSize << 2;

        } else if (count <= 10) {
            uint16_t blen = bitlentree_blen[REUSE_ZERO_BLEN];
            uint16_t code = bitlentree_codes[REUSE_ZERO_BLEN];
            outStream << code;
            outStreamSize << blen;

            outStream << count - 3;
            outStreamSize << 3;
        } else {
            uint16_t blen = bitlentree_blen[REUSE_ZERO_BLEN_7];
            uint16_t code = bitlentree_codes[REUSE_ZERO_BLEN_7];
            outStream << code;
            outStreamSize << blen;
            outStream << count - 11;
            outStreamSize << 7;
        }
        count = 0;
        prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    } // send_dtree for-loop ends here

    uint8_t next_state = WRITE_TOKEN;
    uint8_t tCh = 0;
    uint8_t tLen = 0;
    uint16_t tOffset = 0;

    bool flag_lit_rep = false;
    bool flag_ml_extra = false;
    bool flag_ml_dist_rep = false;

    uint16_t lcode = 0;
    uint16_t dcode = 0;
    uint32_t lextra = 0;
    uint32_t dextra = 0;

    bool flag_out = false;
huffman_loop:
    for (uint32_t inIdx = 0; (inIdx < size) || (next_state != WRITE_TOKEN);) {
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
        ap_uint<16> outValue;
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
                next_state = LIT_REP;
            }
        } else if (next_state == ML_DIST_REP) {
            uint16_t code_s = litmtree_codes[lcode + LITERALS + 1];
            uint16_t bitlen = litmtree_blen[lcode + LITERALS + 1];
            lextra = extra_lbits[lcode];

            outStream << code_s;
            outStreamSize << bitlen;

            if (lextra)
                next_state = ML_EXTRA;
            else
                next_state = DIST_REP;

        } else if (next_state == LIT_REP) {
            uint16_t bitlen = litmtree_blen[tCh];
            uint16_t code = litmtree_codes[tCh];

            outStream << code;
            outStreamSize << bitlen;

            next_state = WRITE_TOKEN;

        } else if (next_state == DIST_REP) {
            uint16_t code_s = distree_codes[dcode];
            uint16_t bitlen = dtree_blen[dcode];
            dextra = extra_dbits[dcode];

            outStream << code_s;
            outStreamSize << bitlen;

            if (dextra)
                next_state = DIST_EXTRA;
            else
                next_state = WRITE_TOKEN;

        } else if (next_state == ML_EXTRA) {
            if (lextra) {
                tLen -= base_length[lcode];
                outStream << tLen;
                outStreamSize << lextra;

                next_state = DIST_REP;
            } else
                next_state = DIST_REP;

        } else if (next_state == DIST_EXTRA) {
            if (dextra) {
                tOffset -= base_dist[dcode];
                outStream << tOffset;
                outStreamSize << dextra;
                next_state = WRITE_TOKEN;
            } else
                next_state = WRITE_TOKEN;
        }
    }
    uint32_t first_push = 0;

    // End of block as per GZip standard
    outStream << litmtree_codes[256];
    outStreamSize << litmtree_blen[256];

    outStreamSize << 0;

} // Dynamic Huffman Funciton Ends here
}
}
#endif
