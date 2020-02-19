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
#ifndef _XFCOMPRESSION_HUFFMAN_DECODER_HPP_
#define _XFCOMPRESSION_HUFFMAN_DECODER_HPP_

/**
 * @file huffman_decoder.hpp
 * @brief Header for module used in ZLIB decompress kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

typedef ap_uint<32> compressd_dt;

namespace xf {
namespace compression {
namespace details {

template <int ByteGenLoopII = 2>
void bytegen(uint64_t& bitbuffer,
             uint8_t& bits_cntr,
             hls::stream<compressd_dt>& outStream,
             hls::stream<bool>& endOfStream,
             uint32_t lit_mask,
             uint32_t dist_mask,
             uint8_t& next_state,
             uint32_t& in_cntr,
             hls::stream<ap_uint<16> >& inStream,
             const uint8_t* array_codes_op,
             const uint8_t* array_codes_bits,
             const uint16_t* array_codes_val,
             uint32_t& used) {
#pragma HLS INLINE
    const int c_byteGenLoopII = ByteGenLoopII;
    bool done = false;
    uint16_t lidx = bitbuffer & lit_mask;
    uint8_t current_op = array_codes_op[lidx];
    uint8_t current_bits = array_codes_bits[lidx];
    uint16_t current_val = array_codes_val[lidx];
    bool is_length = true;
    compressd_dt tmpVal;

ByteGen:
    for (; !done;) {
#pragma HLS PIPELINE II = c_byteGenLoopII
        bitbuffer >>= current_bits;
        bits_cntr -= current_bits;

        if (current_op == 0) {
            tmpVal.range(7, 0) = (uint8_t)(current_val);
            tmpVal.range(31, 8) = 0;
            outStream << tmpVal;
            endOfStream << 0;
            lidx = (bitbuffer & 0XFFFF) & lit_mask;
            is_length = true;
        } else if (current_op & 16) {
            uint16_t ml_op = current_op;
            uint16_t len = (uint16_t)(current_val);
            ml_op &= 15;
            len += (uint16_t)bitbuffer & ((1 << ml_op) - 1);
            bitbuffer >>= ml_op;
            bits_cntr -= ml_op;
            if (is_length) {
                tmpVal.range(31, 16) = len;
            } else {
                tmpVal.range(15, 0) = len;
                outStream << tmpVal;
                endOfStream << 0;
            }
            uint16_t array_offset = (is_length) ? used : 0;
            uint32_t mask = (is_length) ? dist_mask : lit_mask;
            lidx = array_offset + (bitbuffer & mask);
            is_length = !(is_length);
        } else if ((current_op & 64) == 0) {
            uint16_t array_offset = (is_length) ? 0 : used;
            lidx = array_offset + current_val + (bitbuffer & ((1 << current_op) - 1));
        } else if (current_op & 32) {
            next_state = (is_length) ? 2 : 77;
            done = true;
        }
        if (bits_cntr < 32) {
            uint16_t temp = inStream.read();
            in_cntr += 2;
            bitbuffer |= (uint64_t)(temp) << bits_cntr;
            bits_cntr += (uint8_t)16;
        }
        current_op = array_codes_op[lidx];
        current_bits = array_codes_bits[lidx];
        current_val = array_codes_val[lidx];
    }
}

template <int ByteGenLoopII = 2>
uint8_t huffmanBytegen(uint64_t& _bitbuffer,
                       uint8_t& bits_cntr,
                       hls::stream<compressd_dt>& outStream,
                       hls::stream<bool>& endOfStream,
                       uint32_t lit_mask,
                       uint32_t dist_mask,
                       uint32_t& in_cntr,
                       hls::stream<ap_uint<16> >& inStream,
                       const uint8_t* array_codes_op,
                       const uint8_t* array_codes_bits,
                       const uint16_t* array_codes_val,
                       uint32_t& used) {
#pragma HLS INLINE
    const int c_byteGenLoopII = ByteGenLoopII;
    uint64_t bitbuffer = _bitbuffer;
    uint16_t lidx = bitbuffer & lit_mask;
    uint8_t current_op = array_codes_op[lidx];
    uint8_t current_bits = array_codes_bits[lidx];
    uint16_t current_val = array_codes_val[lidx];
    bool is_length = true;
    compressd_dt tmpVal;
    uint8_t ret = 0;

    bool done = false;
ByteGen:
    for (; !done;) {
#pragma HLS PIPELINE II = c_byteGenLoopII
        ap_uint<4> len1 = current_bits;
        ap_uint<4> len2 = 0;
        uint16_t ml_op = current_op & 0xF;
        uint64_t bitbuffer1 = bitbuffer >> current_bits;
        uint64_t bitbuffer2 = bitbuffer >> (current_bits + ml_op);
        bits_cntr -= current_bits;

        if (current_op == 0) {
            tmpVal.range(7, 0) = (uint8_t)(current_val);
            tmpVal.range(31, 8) = 0;
            outStream << tmpVal;
            endOfStream << 0;
            lidx = (bitbuffer1 & 0XFFFF) & lit_mask;
            is_length = true;
        } else if (current_op & 16) {
            uint16_t len = (uint16_t)(current_val);
            len += (uint16_t)bitbuffer1 & ((1 << ml_op) - 1);
            len2 = ml_op;
            bits_cntr -= ml_op;
            if (is_length) {
                tmpVal.range(31, 16) = len;
            } else {
                tmpVal.range(15, 0) = len;
                outStream << tmpVal;
                endOfStream << 0;
            }
            uint16_t array_offset = (is_length) ? used : 0;
            uint32_t mask = (is_length) ? dist_mask : lit_mask;
            lidx = array_offset + (bitbuffer2 & mask);
            is_length = !(is_length);
        } else if ((current_op & 64) == 0) {
            uint16_t array_offset = (is_length) ? 0 : used;
            lidx = array_offset + current_val + (bitbuffer1 & ((1 << current_op) - 1));
        } else if (current_op & 32) {
            if (is_length) {
                ret = 2;
            } else {
                ret = 77;
            }
            done = true;
        }
        if (bits_cntr < 32) {
            uint16_t inValue = inStream.read();
            in_cntr += 2;
            bitbuffer = (bitbuffer >> (len1 + len2)) | (uint64_t)(inValue) << bits_cntr;
            bits_cntr += (uint8_t)16;
        } else {
            bitbuffer >>= (len1 + len2);
        }
        current_op = array_codes_op[lidx];
        current_bits = array_codes_bits[lidx];
        current_val = array_codes_val[lidx];
    }
    _bitbuffer = bitbuffer;
    return ret;
}

void code_generator_array(uint8_t curr_table,
                          uint16_t* lens,
                          uint32_t codes,
                          uint8_t* table_op,
                          uint8_t* table_bits,
                          uint16_t* table_val,
                          uint32_t* bits,
                          uint32_t* used) {
/**
 * @brief This module regenerates the code values based on bit length
 * information present in block preamble. Output generated by this module
 * presents operation, bits and value for each literal, match length and
 * distance.
 *
 * @param curr_table input current module to process i.e., literal or
 * distance table etc
 * @param lens input bit length information
 * @param codes input number of codes
 * @param table_op output operation per active symbol (literal or distance)
 * @param table_bits output bits to process per symbol (literal or distance)
 * @param table_val output value per symbol (literal or distance)
 * @param bits represents the start of the table
 * @param used presents next valid entry in table
 */
#pragma HLS INLINE REGION
    uint16_t sym = 0;
    uint16_t min, max;
    uint32_t root = *bits;
    uint16_t curr;
    uint16_t drop;
    uint16_t huff = 0;
    uint16_t incr;
    uint16_t fill;
    uint16_t low;
    uint16_t mask;

    const uint16_t c_maxbits = 15;
    uint8_t code_data_op = 0;
    uint8_t code_data_bits = 0;
    uint16_t code_data_val = 0;

    uint8_t* nptr_op;
    uint8_t* nptr_bits;
    uint16_t* nptr_val;

    const uint16_t* base;
    const uint16_t* extra;
    uint16_t match;
    uint16_t count[c_maxbits + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = count

    uint16_t offs[c_maxbits + 1];
#pragma HLS ARRAY_PARTITION variable = offs

    uint16_t codeBuffer[512];

    const uint16_t lbase[32] = {3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
                                35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0,  0};
    const uint16_t lext[32] = {16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18,  18,
                               19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 77, 202, 0};
    const uint16_t dbase[32] = {1,    2,    3,    4,    5,    7,     9,     13,    17,  25,   33,
                                49,   65,   97,   129,  193,  257,   385,   513,   769, 1025, 1537,
                                2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0};
    const uint16_t dext[32] = {16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
                               23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 64, 64};

cnt_lens:
    for (uint16_t i = 0; i < codes; i++)
#pragma HLS PIPELINE II = 1
        count[lens[i]]++;

max_loop:
    for (max = c_maxbits; max >= 1; max--)
#pragma HLS PIPELINE II = 1
        if (count[max] != 0) break;

    if (root > max) root = max;

min_loop:
    for (min = 1; min < max; min++) {
#pragma HLS PIPELINE II = 1
        if (count[min] != 0) break;
    }

    if (root < min) root = min;

    int left = 1;
left_loop:
    for (uint16_t i = 1; i <= c_maxbits; i++) {
#pragma HLS PIPELINE II = 1
        left <<= 1;
        left -= count[i];
    }

    offs[1] = 0;
offs_loop:
    for (uint16_t i = 1; i < c_maxbits; i++)
#pragma HLS PIPELINE II = 1
        offs[i + 1] = offs[i] + count[i];

codes_loop:
    for (uint16_t i = 0; i < codes; i++) {
#pragma HLS PIPELINE II = 1
        if (lens[i] != 0) codeBuffer[offs[lens[i]]++] = (uint16_t)i;
    }

    switch (curr_table) {
        case 1:
            base = extra = codeBuffer;
            match = 20;
            break;
        case 2:
            base = lbase;
            extra = lext;
            match = 257;
            break;
        case 3:
            base = dbase;
            extra = dext;
            match = 0;
    }

    uint16_t len = min;

    nptr_op = table_op;
    nptr_bits = table_bits;
    nptr_val = table_val;

    curr = root;
    drop = 0;
    low = (uint32_t)(-1);
    *used = 1 << root;
    mask = *used - 1;

code_gen:
    for (;;) {
#pragma HLS PIPELINE II = 1
        code_data_bits = (uint8_t)(len - drop);

        if (codeBuffer[sym] + 1 < match) {
            code_data_op = (uint8_t)0;
            code_data_val = codeBuffer[sym];
        } else if (codeBuffer[sym] >= match) {
            code_data_op = (uint8_t)(extra[codeBuffer[sym] - match]);
            code_data_val = base[codeBuffer[sym] - match];
        } else {
            code_data_op = (uint8_t)(96);
            code_data_val = 0;
        }

        incr = 1 << (len - drop);
        fill = 1 << curr;
        min = fill;

        do {
            fill -= incr;
            nptr_op[(huff >> drop) + fill] = code_data_op;
            nptr_bits[(huff >> drop) + fill] = code_data_bits;
            nptr_val[(huff >> drop) + fill] = code_data_val;
        } while (fill != 0);

        incr = 1 << (len - 1);

        while (huff & incr) incr >>= 1;

        if (incr != 0) {
            huff &= incr - 1;
            huff += incr;
        } else
            huff = 0;

        sym++;

        if (--(count[len]) == 0) {
            if (len == max) break;
            len = lens[codeBuffer[sym]];
        }

        if (len > root && (huff & mask) != low) {
            if (drop == 0) drop = root;

            nptr_op += min;
            nptr_bits += min;
            nptr_val += min;

            curr = len - drop;
            left = (int)(1 << curr);

            for (int i = curr; i + drop < max; i++, curr++) {
                left -= count[curr + drop];
                if (left <= 0) break;
                left <<= 1;
            }

            *used += 1 << curr;

            low = huff & mask;
            table_op[low] = (uint8_t)curr;
            table_bits[low] = (uint8_t)root;
            table_val[low] = (uint16_t)(nptr_val - table_val);
        }
    }

    *bits = root;
}
} // end details

/**
 * @brief This module is ZLIB/GZIP Fixed Huffman Decoder. It takes ZLIB/GZIP Huffman encoded data as input and
 * generates decoded data in LZ77 format (literal, length, offset)
 *
 *
 * @param inStream input bit packed data
 * @param outStream output lz77 compressed output in the form of 32bit packets
 * (Literals, Match Length, Distances)
 * @param endOfStream output completion of execution
 * @param input_size input data size
 */
void huffmanDecoderFixed(hls::stream<ap_uint<16> >& inStream,
                         hls::stream<compressd_dt>& outStream,
                         hls::stream<bool>& endOfStream,
                         uint32_t input_size) {
    enum decomp_stages {
        HEADER_STATE = 1,
        TREE_PMBL_STATE,
        STORE_STATE,
        BYTEGEN_FIXED_STATE,
        COMPLETE_STATE,
        FIXED_STATE
    };

    uint64_t bitbuffer = 0;
    uint32_t curInSize = input_size;
    uint8_t bits_cntr = 0;

    uint8_t current_op = 0;
    uint8_t current_bits = 0;
    uint16_t current_val = 0;

    uint8_t len = 0;

    const uint16_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint8_t dynamic_last = 0;
    uint32_t dynamic_nlen = 0;
    uint32_t dynamic_ndist = 0;
    uint32_t dynamic_ncode = 0;
    uint32_t dynamic_curInSize = 0;
    uint16_t dynamic_lens[512];

    uint32_t in_cntr = 0;

    uint32_t dynamic_lenbits = 0;
    uint32_t dynamic_distbits = 0;
    uint8_t copy = 0;

    bool done = true;

    uint8_t block_mode;
    int cntr = 0;
    uint32_t used = 0;
    uint8_t next_state = HEADER_STATE;

    while (done) {
        done = false;

        if (next_state == HEADER_STATE) {
            done = true;
            uint16_t lcl_tmp = inStream.read();
            in_cntr += 2;
            curInSize -= 2;

            next_state = TREE_PMBL_STATE;

            bitbuffer = 0;
            bits_cntr = 0;
        } else if (next_state == TREE_PMBL_STATE) {
            done = true;

            if (bits_cntr == 0) {
                // if first block
                uint16_t temp = inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(temp) << bits_cntr;
                curInSize -= 2;
                bits_cntr += 16;
            }

            dynamic_last = bitbuffer & 1;
            bitbuffer = bitbuffer >> 1; // dump the bit read
            uint8_t cb_type = (uint8_t)(bitbuffer)&3;

            switch (cb_type) {
                case 0:
                    next_state = STORE_STATE;
                    break;
                case 1:
                    next_state = FIXED_STATE;
                    break;
                case 2:
                    done = false;
                    break;
                default:
                    done = false;
                    break;
            }
            bitbuffer >>= 2;
            bits_cntr -= 3; // previously dumped 1 bit + current dumped 2 bits

        } else if (next_state == STORE_STATE) {
            done = true;
            bitbuffer >>= bits_cntr & 7;
            bits_cntr -= bits_cntr & 7;

            while (bits_cntr < 32) {
                uint16_t tmp_dt = (uint16_t)inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(tmp_dt) << bits_cntr;
                curInSize -= 2;
                bits_cntr += 16;
            }
            bitbuffer >>= 32;
            bits_cntr = bits_cntr - 32;

            if (dynamic_last) {
                next_state = COMPLETE_STATE;
            } else {
                next_state = TREE_PMBL_STATE;
            }
        } else if (next_state == FIXED_STATE) {
            done = true;
            next_state = BYTEGEN_FIXED_STATE;
        } else if (next_state == BYTEGEN_FIXED_STATE) {
            done = true;

#include "fixed_codes.hpp"

            // mask length codes 1st level
            uint32_t lit_mask = (1 << 9) - 1;
            // mask length codes 2nd level
            uint32_t dist_mask = (1 << 5) - 1;

            // Read from the table
            uint8_t current_op = 0;
            uint8_t current_bits = 0;
            uint16_t current_val = 0;

            // ********************************
            //  Create Packets Below
            //  [LIT|ML|DIST|DIST] --> 32 Bit
            //  Read data from inStream - 8bits
            //  at a time. Decode the literals,
            //  ML, Distances based on tables
            // ********************************
            compressd_dt tmpVal;
            bool done = false;
            // Read from inStream
            while (bits_cntr < 32) {
                uint16_t temp = inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(temp) << bits_cntr;
                bits_cntr += 16;
            }

            used = 512;
            // ByteGeneration module
            details::bytegen(bitbuffer, bits_cntr, outStream, endOfStream, lit_mask, dist_mask, next_state, in_cntr,
                             inStream, fixed_litml_op, fixed_litml_bits, fixed_litml_val, used);

            if (next_state == 77) done = false;

            if (dynamic_last) {
                next_state = COMPLETE_STATE;
            }

        } else if (next_state == COMPLETE_STATE) {
            done = false;
            break;
        }
    } // While end
    // Handle leftover bytes
    if (input_size > in_cntr) {
        for (int i = 0; i < (input_size - in_cntr); i += 2) {
            uint16_t c = inStream.read();
        }
    }
    outStream << 0; // Adding Dummy Data for last end of stream case
    endOfStream << 1;
}

/**
 * @brief This module is ZLIB/GZIP Dynamic Huffman Decoder. It takes ZLIB/GZIP
 * Huffman encoded data as input and generates decoded data in LZ77
 * format(literal, length, offset).
 *
 *
 * @param inStream input bit packed data
 * @param outStream output lz77 compressed output in the form of 32bit packets
 * (Literals, Match Length, Distances)
 * @param endOfStream output completion of execution
 * @param input_size input data size
 */
template <int ByteGenLoopII = 2>
void huffmanDecoderDynamic(hls::stream<ap_uint<16> >& inStream,
                           hls::stream<compressd_dt>& outStream,
                           hls::stream<bool>& endOfStream,
                           uint32_t input_size) {
    enum decomp_stages {
        TREE_PMBL_STATE,
        STORE_STATE,
        DYNAMIC_STATE,
        BYTEGEN_DYNAMIC_STATE,
    };
    // printf("Inside %s Size=%d\n", __FUNCTION__ , input_size);

    uint64_t bitbuffer = 0;
    uint32_t curInSize = input_size;
    uint8_t bits_cntr = 0;

    uint8_t current_op = 0;
    uint8_t current_bits = 0;
    uint16_t current_val = 0;

    uint8_t len = 0;

    const uint16_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint8_t dynamic_last = 0;
    uint32_t dynamic_nlen = 0;
    uint32_t dynamic_ndist = 0;
    uint32_t dynamic_ncode = 0;
    uint32_t dynamic_curInSize = 0;
    uint16_t dynamic_lens[512];

    uint32_t in_cntr = 0;

    uint32_t dynamic_lenbits = 0;
    uint32_t dynamic_distbits = 0;
    uint8_t copy = 0;

    bool done = true;
    const uint16_t c_tcodesize = 2048;

    uint8_t array_codes_op[c_tcodesize];
    uint8_t array_codes_bits[c_tcodesize];
    uint16_t array_codes_val[c_tcodesize];
    uint8_t block_mode;
    int cntr = 0;
    uint32_t used = 0;
    done = true;
    uint16_t lcl_tmp = inStream.read();
    in_cntr += 2;
    curInSize -= 2;
    while (done) {
        done = true;

        while (bits_cntr < 32) {
            // if first block
            uint16_t temp = inStream.read();
            in_cntr += 2;
            bitbuffer += (uint64_t)(temp) << bits_cntr;
            curInSize -= 2;
            bits_cntr += 16;
        }

        dynamic_last = bitbuffer & 1;
        uint8_t cb_type = ((uint8_t)(bitbuffer)&6) >> 1;
        bitbuffer >>= 3;
        bits_cntr -= 3; // previously dumped 1 bit + current dumped 2 bits
        if (cb_type != 0 && cb_type != 2) {
            done = false;
            break;
        }
        if (cb_type == 0) {
            // STORE_STATE
            bitbuffer >>= bits_cntr & 7;
            bits_cntr -= bits_cntr & 7;

            while (bits_cntr < 32) {
                uint16_t tmp_dt = (uint16_t)inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(tmp_dt) << bits_cntr;
                curInSize -= 2;
                bits_cntr += 16;
            }
            bitbuffer >>= 32;
            bits_cntr = bits_cntr - 32;

            if (dynamic_last) {
                done = false;
            } else {
                done = true;
            }
        } else if (cb_type == 2) {
            done = true;
            // Read 14 bits HLIT(5-bits), HDIST(5-bits) and HCLEN(4-bits)
            if (bits_cntr < 16) {
                uint16_t tmp_data = inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)tmp_data << bits_cntr;
                curInSize -= 2;  // read 2 bytes above
                bits_cntr += 16; // read 16 bits
            }
            dynamic_nlen = ((uint32_t)bitbuffer & ((1 << 5) - 1)) + 257; // Max 288
            bitbuffer >>= 5;

            dynamic_ndist = ((uint32_t)bitbuffer & ((1 << 5) - 1)) + 1; // Max 30
            bitbuffer >>= 5;

            dynamic_ncode = ((uint32_t)bitbuffer & ((1 << 4) - 1)) + 4; // Max 19
            bitbuffer >>= 4;
            bits_cntr -= 14;

            dynamic_curInSize = 0;

            while (dynamic_curInSize < dynamic_ncode) {
                if (bits_cntr < 16) {
                    uint16_t tmp_data = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(tmp_data << bits_cntr);
                    curInSize -= 2;
                    bits_cntr += 16;
                }
                dynamic_lens[order[dynamic_curInSize++]] = (uint16_t)((uint32_t)bitbuffer & ((1 << 3) - 1));
                bitbuffer >>= 3;
                bits_cntr -= 3;
            }

            while (dynamic_curInSize < 19) dynamic_lens[order[dynamic_curInSize++]] = 0;

            dynamic_lenbits = 7;

            details::code_generator_array(1, dynamic_lens, 19, array_codes_op, array_codes_bits, array_codes_val,
                                          &dynamic_lenbits, &used);

            dynamic_curInSize = 0;
            uint32_t dlenb_mask = ((1 << dynamic_lenbits) - 1);

            // Figure out codes for LIT/ML and DIST
            while (dynamic_curInSize < dynamic_nlen + dynamic_ndist) {
                // check if bits in bitbuffer are enough
                current_bits = array_codes_bits[(bitbuffer & dlenb_mask)];
                if (current_bits > bits_cntr) {
                    // read 2-bytes
                    uint16_t tmp_data = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                    curInSize -= 2;
                    bits_cntr += 16;
                }
                current_op = array_codes_op[(bitbuffer & dlenb_mask)];
                current_bits = array_codes_bits[(bitbuffer & dlenb_mask)];
                current_val = array_codes_val[(bitbuffer & dlenb_mask)];

                if (current_val < 16) {
                    bitbuffer >>= current_bits;
                    bits_cntr -= current_bits;
                    dynamic_lens[dynamic_curInSize++] = current_val;
                } else {
                    switch (current_val) {
                        case 16:
                            if (bits_cntr < current_bits + 2) {
                                uint16_t tmp_data = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                                curInSize -= 2;
                                bits_cntr += 16;
                            }
                            bitbuffer >>= current_bits;

                            len = dynamic_lens[dynamic_curInSize - 1];
                            copy = 3 + ((uint32_t)bitbuffer & 3); // use 2 bits
                            bitbuffer >>= 2;                      // dump 2 bits
                            bits_cntr -= (current_bits + 2);      // update bits_cntr
                            break;

                        case 17:
                            if (bits_cntr < current_bits + 3) {
                                uint16_t tmp_data = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                                curInSize -= 2;
                                bits_cntr += 16;
                            }
                            bitbuffer >>= current_bits;
                            len = 0;
                            copy = 3 + ((uint32_t)bitbuffer & 7); // use 3 bits
                            bitbuffer >>= 3;
                            bits_cntr -= (current_bits + 3);
                            break;

                        default:
                            if (bits_cntr < current_bits + 7) {
                                uint16_t tmp_data = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                                curInSize -= 2;
                                bits_cntr += 16;
                            }
                            bitbuffer >>= current_bits;
                            len = 0;
                            copy = 11 + ((uint32_t)bitbuffer & ((1 << 7) - 1)); // use 7 bits
                            bitbuffer >>= 7;
                            bits_cntr -= (current_bits + 7);
                    }

                    while (copy--) dynamic_lens[dynamic_curInSize++] = (uint16_t)len;
                }
            } // End of while
            dynamic_lenbits = 9;
            details::code_generator_array(2, dynamic_lens, dynamic_nlen, array_codes_op, array_codes_bits,
                                          array_codes_val, &dynamic_lenbits, &used);

            dynamic_distbits = 6;
            uint32_t dused = 0;
            details::code_generator_array(3, dynamic_lens + dynamic_nlen, dynamic_ndist, &array_codes_op[used],
                                          &array_codes_bits[used], &array_codes_val[used], &dynamic_distbits, &dused);
            done = true;
            if (curInSize >= 6) {
                // mask length codes 1st level
                uint32_t lit_mask = (1 << dynamic_lenbits) - 1;
                // mask length codes 2nd level
                uint32_t dist_mask = (1 << dynamic_distbits) - 1;

                // ********************************
                //  Create Packets Below
                //  [LIT|ML|DIST|DIST] --> 32 Bit
                //  Read data from inStream - 8bits
                //  at a time. Decode the literals,
                //  ML, Distances based on tables
                // ********************************
                // bool done = false;
                // Read from inStream
                while (bits_cntr < 32) {
                    uint16_t temp = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(temp) << bits_cntr;
                    bits_cntr += 16;
                }

                uint8_t ret = details::huffmanBytegen<ByteGenLoopII>(
                    bitbuffer, bits_cntr, outStream, endOfStream, lit_mask, dist_mask, in_cntr, inStream,
                    array_codes_op, array_codes_bits, array_codes_val, used);
            }
        }
    } // While end

    // Handle leftover bytes
    if (input_size > in_cntr) {
        for (int i = 0; i < (input_size - in_cntr); i += 2) {
            uint16_t c = inStream.read();
        }
    }
    outStream << 0; // Adding Dummy Data for last end of stream case
    endOfStream << 1;
}

/**
 * @brief This module is ZLIB/GZIP Fixed, Dynamic and Stored block supported
 * decoder. It takes ZLIB/GZIP Huffman encoded data as input and generates
 * decoded data in LZ77 format (Literal, Length, Offset).
 *
 * @param inStream input bit packed data
 * @param outStream output lz77 compressed output in the form of 32bit packets
 * (Literals, Match Length, Distances)
 * @param endOfStream output completion of execution
 * @param input_size input data size
 */
void huffmanDecoderFull(hls::stream<ap_uint<16> >& inStream,
                        hls::stream<compressd_dt>& outStream,
                        hls::stream<bool>& endOfStream,
                        uint32_t input_size) {
    enum decomp_stages {
        HEADER_STATE = 1,
        TREE_PMBL_STATE,
        STORE_STATE,
        DYNAMIC_STATE,
        BYTEGEN_DYNAMIC_STATE,
        BYTEGEN_FIXED_STATE,
        COMPLETE_STATE,
        FIXED_STATE
    };

    uint64_t bitbuffer = 0;
    uint32_t curInSize = input_size;
    uint8_t bits_cntr = 0;

    uint8_t current_op = 0;
    uint8_t current_bits = 0;
    uint16_t current_val = 0;

    uint8_t len = 0;

    const uint16_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint8_t dynamic_last = 0;
    uint32_t dynamic_nlen = 0;
    uint32_t dynamic_ndist = 0;
    uint32_t dynamic_ncode = 0;
    uint32_t dynamic_curInSize = 0;
    uint16_t dynamic_lens[512];

    uint32_t in_cntr = 0;

    uint32_t dynamic_lenbits = 0;
    uint32_t dynamic_distbits = 0;
    uint8_t copy = 0;

    bool done = true;

    const uint16_t c_tcodesize = 2048;

    uint8_t array_codes_op[c_tcodesize];
    uint8_t array_codes_bits[c_tcodesize];
    uint16_t array_codes_val[c_tcodesize];

    uint8_t block_mode;
    int cntr = 0;
    uint32_t used = 0;
    uint8_t next_state = HEADER_STATE;

    while (done) {
        done = false;

        if (next_state == HEADER_STATE) {
            done = true;
            uint16_t lcl_tmp = inStream.read();
            in_cntr += 2;
            curInSize -= 2;

            next_state = TREE_PMBL_STATE;

            bitbuffer = 0;
            bits_cntr = 0;
        } else if (next_state == TREE_PMBL_STATE) {
            done = true;
            if (bits_cntr == 0) {
                // if first block
                uint16_t temp = inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(temp) << bits_cntr;
                curInSize -= 2;
                bits_cntr += 16;
            }

            dynamic_last = bitbuffer & 1;
            bitbuffer = bitbuffer >> 1; // dump the bit read
            uint8_t cb_type = (uint8_t)(bitbuffer)&3;

            switch (cb_type) {
                case 0:
                    next_state = STORE_STATE;
                    break;
                case 1:
                    next_state = FIXED_STATE;
                    break;
                case 2:
                    next_state = DYNAMIC_STATE;
                    break;
                default:
                    done = false;
                    break;
            }
            bitbuffer >>= 2;
            bits_cntr -= 3; // previously dumped 1 bit + current dumped 2 bits

        } else if (next_state == STORE_STATE) {
            done = true;
            bitbuffer >>= bits_cntr & 7;
            bits_cntr -= bits_cntr & 7;

            while (bits_cntr < 32) {
                uint16_t tmp_dt = (uint16_t)inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(tmp_dt) << bits_cntr;
                curInSize -= 2;
                bits_cntr += 16;
            }
            uint32_t store_length = bitbuffer & 0xffff;

            if (bits_cntr > 32) {
                bitbuffer >>= 32;
                bits_cntr = bits_cntr - 32;
            } else {
                bitbuffer = 0;
                bits_cntr = 0;
            }

            // Stored block data
            if (bits_cntr < 8) {
                uint16_t tmp_dt = (uint16_t)inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(tmp_dt) << bits_cntr;
                curInSize -= 2;
                bits_cntr += 16;
            }
            compressd_dt tmpVal = 0;
        strcpy:
            for (uint32_t i = 0; i < store_length; i++) {
                tmpVal.range(7, 0) = ((uint32_t)bitbuffer & ((1 << 8) - 1));
                tmpVal.range(31, 8) = 0;
                outStream << tmpVal;
                endOfStream << 0;
                bitbuffer >>= 8;
                bits_cntr -= 8;
                if (bits_cntr < 8) {
                    uint16_t tmp_dt = (uint16_t)inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(tmp_dt) << bits_cntr;
                    curInSize -= 2;
                    bits_cntr += 16;
                }
            }

            if (dynamic_last) {
                next_state = COMPLETE_STATE;
            } else {
                next_state = TREE_PMBL_STATE;
            }

        } else if (next_state == FIXED_STATE) {
            done = true;
            next_state = BYTEGEN_FIXED_STATE;
        } else if (next_state == DYNAMIC_STATE) {
            done = true;
            // Read 14 bits HLIT(5-bits), HDIST(5-bits) and HCLEN(4-bits)
            if (bits_cntr < 14) {
                uint16_t tmp_data = inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)tmp_data << bits_cntr;
                curInSize -= 2;  // read 2 bytes above
                bits_cntr += 16; // read 16 bits
            }
            dynamic_nlen = ((uint32_t)bitbuffer & ((1 << 5) - 1)) + 257; // Max 288
            bitbuffer >>= 5;

            dynamic_ndist = ((uint32_t)bitbuffer & ((1 << 5) - 1)) + 1; // Max 30
            bitbuffer >>= 5;

            dynamic_ncode = ((uint32_t)bitbuffer & ((1 << 4) - 1)) + 4; // Max 19
            bitbuffer >>= 4;
            bits_cntr -= 14;

            dynamic_curInSize = 0;

            while (dynamic_curInSize < dynamic_ncode) {
                if (bits_cntr < 3) {
                    uint16_t tmp_data = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(tmp_data << bits_cntr);
                    curInSize -= 2;
                    bits_cntr += 16;
                }
                dynamic_lens[order[dynamic_curInSize++]] = (uint16_t)((uint32_t)bitbuffer & ((1 << 3) - 1));
                bitbuffer >>= 3;
                bits_cntr -= 3;
            }

            while (dynamic_curInSize < 19) dynamic_lens[order[dynamic_curInSize++]] = 0;

            dynamic_lenbits = 7;

            details::code_generator_array(1, dynamic_lens, 19, array_codes_op, array_codes_bits, array_codes_val,
                                          &dynamic_lenbits, &used);

            dynamic_curInSize = 0;
            uint32_t dlenb_mask = ((1 << dynamic_lenbits) - 1);

            // Figure out codes for LIT/ML and DIST
            while (dynamic_curInSize < dynamic_nlen + dynamic_ndist) {
                // check if bits in bitbuffer are enough
                current_bits = array_codes_bits[(bitbuffer & dlenb_mask)];
                if (current_bits > bits_cntr) {
                    // read 2-bytes
                    uint16_t tmp_data = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                    curInSize -= 2;
                    bits_cntr += 16;
                }
                current_op = array_codes_op[(bitbuffer & dlenb_mask)];
                current_bits = array_codes_bits[(bitbuffer & dlenb_mask)];
                current_val = array_codes_val[(bitbuffer & dlenb_mask)];

                if (current_val < 16) {
                    bitbuffer >>= current_bits;
                    bits_cntr -= current_bits;
                    dynamic_lens[dynamic_curInSize++] = current_val;
                } else {
                    switch (current_val) {
                        case 16:
                            if (bits_cntr < current_bits + 2) {
                                uint16_t tmp_data = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                                curInSize -= 2;
                                bits_cntr += 16;
                            }
                            bitbuffer >>= current_bits;

                            if (dynamic_curInSize == 0) done = false;

                            len = dynamic_lens[dynamic_curInSize - 1];
                            copy = 3 + ((uint32_t)bitbuffer & 3); // use 2 bits
                            bitbuffer >>= 2;                      // dump 2 bits
                            bits_cntr -= (current_bits + 2);      // update bits_cntr
                            break;

                        case 17:
                            if (bits_cntr < current_bits + 3) {
                                uint16_t tmp_data = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                                curInSize -= 2;
                                bits_cntr += 16;
                            }
                            bitbuffer >>= current_bits;
                            len = 0;
                            copy = 3 + ((uint32_t)bitbuffer & 7); // use 3 bits
                            bitbuffer >>= 3;
                            bits_cntr -= (current_bits + 3);
                            break;

                        default:
                            if (bits_cntr < current_bits + 7) {
                                uint16_t tmp_data = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                                curInSize -= 2;
                                bits_cntr += 16;
                            }
                            bitbuffer >>= current_bits;
                            len = 0;
                            copy = 11 + ((uint32_t)bitbuffer & ((1 << 7) - 1)); // use 7 bits
                            bitbuffer >>= 7;
                            bits_cntr -= (current_bits + 7);
                    }

                    while (copy--) dynamic_lens[dynamic_curInSize++] = (uint16_t)len;
                }
            } // End of while
            dynamic_lenbits = 9;
            details::code_generator_array(2, dynamic_lens, dynamic_nlen, array_codes_op, array_codes_bits,
                                          array_codes_val, &dynamic_lenbits, &used);

            dynamic_distbits = 6;
            uint32_t dused = 0;
            details::code_generator_array(3, dynamic_lens + dynamic_nlen, dynamic_ndist, &array_codes_op[used],
                                          &array_codes_bits[used], &array_codes_val[used], &dynamic_distbits, &dused);
            next_state = BYTEGEN_DYNAMIC_STATE;
        } else if (next_state == BYTEGEN_DYNAMIC_STATE) {
            done = true;
            if (curInSize >= 6) {
                // mask length codes 1st level
                uint32_t lit_mask = (1 << dynamic_lenbits) - 1;
                // mask length codes 2nd level
                uint32_t dist_mask = (1 << dynamic_distbits) - 1;

                // Read from the table
                uint8_t current_op = 0;
                uint8_t current_bits = 0;
                uint16_t current_val = 0;

                // ********************************
                //  Create Packets Below
                //  [LIT|ML|DIST|DIST] --> 32 Bit
                //  Read data from inStream - 8bits
                //  at a time. Decode the literals,
                //  ML, Distances based on tables
                // ********************************
                compressd_dt tmpVal;
                bool done = false;
                // Read from inStream
                while (bits_cntr < 32) {
                    uint16_t temp = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(temp) << bits_cntr;
                    bits_cntr += 16;
                }

                details::bytegen(bitbuffer, bits_cntr, outStream, endOfStream, lit_mask, dist_mask, next_state, in_cntr,
                                 inStream, array_codes_op, array_codes_bits, array_codes_val, used);

                if (next_state == 77) done = false;

                if (dynamic_last) break; // next_state = COMPLETE_STATE;

            } else {
                break;
                // done = false;
            }
        } else if (next_state == BYTEGEN_FIXED_STATE) {
            done = true;

#include "fixed_codes.hpp"

            // mask length codes 1st level
            uint32_t lit_mask = (1 << 9) - 1;
            // mask length codes 2nd level
            uint32_t dist_mask = (1 << 5) - 1;

            // Read from the table
            uint8_t current_op = 0;
            uint8_t current_bits = 0;
            uint16_t current_val = 0;

            // ********************************
            //  Create Packets Below
            //  [LIT|ML|DIST|DIST] --> 32 Bit
            //  Read data from inStream - 8bits
            //  at a time. Decode the literals,
            //  ML, Distances based on tables
            // ********************************
            compressd_dt tmpVal;
            bool done = false;
            // Read from inStream
            while (bits_cntr < 32) {
                uint16_t temp = inStream.read();
                in_cntr += 2;
                bitbuffer += (uint64_t)(temp) << bits_cntr;
                bits_cntr += 16;
            }
            used = 512;
            details::bytegen(bitbuffer, bits_cntr, outStream, endOfStream, lit_mask, dist_mask, next_state, in_cntr,
                             inStream, fixed_litml_op, fixed_litml_bits, fixed_litml_val, used);

            if (next_state == 77) done = false;

            if (dynamic_last) {
                next_state = COMPLETE_STATE;
            }
        } else if (next_state == COMPLETE_STATE) {
            done = false;
            break;
        }
    } // While end

    // Handle leftover bytes
    if (input_size > in_cntr) {
        for (int i = 0; i < (input_size - in_cntr); i += 2) {
            uint16_t c = inStream.read();
        }
    }

    outStream << 0; // Adding Dummy Data for last end of stream case
    endOfStream << 1;
}

} // Compression
} // XF
#endif // _XFCOMPRESSION_HUFFMAN_DECODER_HPP_
