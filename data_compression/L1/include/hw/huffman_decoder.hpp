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

enum eHuffmanType { FIXED = 0, DYNAMIC, FULL };

namespace details {
template <int ByteGenLoopII = 2>
inline uint8_t huffmanBytegenStatic(uint64_t& _bitbuffer,
                                    uint8_t& bits_cntr,
                                    hls::stream<compressd_dt>& outStream,
                                    hls::stream<bool>& endOfStream,
                                    uint32_t& in_cntr,
                                    hls::stream<ap_uint<16> >& inStream,
                                    const uint8_t* array_codes_op,
                                    const uint8_t* array_codes_bits,
                                    const uint16_t* array_codes_val) {
#pragma HLS INLINE
    const int c_byteGenLoopII = ByteGenLoopII;
    uint16_t used = 512;
    uint16_t lit_mask = 511;
    uint16_t dist_mask = 31;
    uint64_t bitbuffer = _bitbuffer;
    ap_uint<10> lidx = bitbuffer & lit_mask;
    uint8_t current_op = array_codes_op[lidx];
    uint8_t current_bits = array_codes_bits[lidx];
    uint16_t current_val = array_codes_val[lidx];
    bool is_length = true;
    compressd_dt tmpVal;
    uint8_t ret = 0;

    bool done = false;
ByteGenStatic:
    for (; !done;) {
#pragma HLS PIPELINE II = c_byteGenLoopII
        ap_uint<4> len1 = current_bits;
        ap_uint<4> len2 = 0;
        ap_uint<4> ml_op = current_op;
        uint64_t bitbuffer1 = bitbuffer >> current_bits;
        ap_uint<9> bitbuffer3 = bitbuffer >> current_bits;
        uint64_t bitbuffer2 = bitbuffer >> (current_bits + ml_op);
        bits_cntr -= current_bits;

        if (current_op == 0) {
            tmpVal.range(7, 0) = (uint8_t)(current_val);
            tmpVal.range(31, 8) = 0;
            outStream << tmpVal;
            endOfStream << 0;
            lidx = bitbuffer3;
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
            ap_uint<9> mask = (is_length) ? dist_mask : lit_mask;
            lidx = array_offset + (bitbuffer2 & mask);
            is_length = !(is_length);
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

template <int ByteGenLoopII = 2>
uint8_t huffmanBytegen(uint64_t& _bitbuffer,
                       uint8_t& bits_cntr,
                       hls::stream<compressd_dt>& outStream,
                       hls::stream<bool>& endOfStream,
                       uint32_t& in_cntr,
                       hls::stream<ap_uint<16> >& inStream,
                       const uint32_t* array_codes,
                       const uint32_t* array_codes_extra,
                       const uint32_t* array_codes_dist,
                       const uint32_t* array_codes_dist_extra) {
#pragma HLS INLINE
    uint16_t lit_mask = 511; // Adjusted according to 8 bit
    uint16_t dist_mask = 511;
    const int c_byteGenLoopII = ByteGenLoopII;
    ap_uint<64> bitbuffer = _bitbuffer;
    ap_uint<9> lidx = bitbuffer;
    ap_uint<9> lidx1;
    ap_uint<32> current_array_val = array_codes[lidx];
    uint8_t current_op = current_array_val.range(31, 24);
    uint8_t current_bits = current_array_val.range(23, 16);
    uint16_t current_val = current_array_val.range(15, 0);
    bool is_length = true;
    compressd_dt tmpVal;
    uint8_t ret = 0;
    bool dist_extra = false;
    bool len_extra = false;

    bool done = false;
ByteGen:
    for (; !done;) {
#pragma HLS PIPELINE II = c_byteGenLoopII
        ap_uint<4> len1 = current_bits;
        ap_uint<4> len2 = 0;
        ap_uint<4> ml_op = current_op;
        uint8_t current_op1 = (current_op == 0 || current_op >= 64) ? 1 : current_op;
        ap_uint<64> bitbuffer1 = bitbuffer >> current_bits;
        ap_uint<9> bitbuffer3 = bitbuffer >> current_bits;
        lidx1 = bitbuffer1.range(current_op1 - 1, 0) + current_val;
        ap_uint<9> bitbuffer2 = bitbuffer.range(current_bits + ml_op + 8, current_bits + ml_op);
        bits_cntr -= current_bits;
        dist_extra = false;
        len_extra = false;

        if (current_op == 0) {
            tmpVal.range(7, 0) = (uint8_t)(current_val);
            tmpVal.range(31, 8) = 0;
            outStream << tmpVal;
            endOfStream << 0;
            lidx = bitbuffer3;
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
            lidx = bitbuffer2;
            is_length = !(is_length);
        } else if ((current_op & 64) == 0) {
            if (is_length) {
                len_extra = true;
            } else {
                dist_extra = true;
            }
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
        if (len_extra) {
            ap_uint<32> val = array_codes_extra[lidx1];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        } else if (dist_extra) {
            ap_uint<32> val = array_codes_dist_extra[lidx1];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        } else if (is_length) {
            ap_uint<32> val = array_codes[lidx];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        } else {
            ap_uint<32> val = array_codes_dist[lidx];
            current_op = val.range(31, 24);
            current_bits = val.range(23, 16);
            current_val = val.range(15, 0);
        }
    }
    _bitbuffer = bitbuffer;
    return ret;
}

void code_generator_array_dyn(
    uint8_t curr_table, uint16_t* lens, uint32_t codes, uint32_t* table, uint32_t* table_extra, uint32_t bits) {
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
    uint16_t extra_idx = 0;
    uint32_t root = bits;
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
    uint32_t* nptr;
    uint32_t* nptr_extra;

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

min_loop:
    for (min = 1; min < max; min++) {
#pragma HLS PIPELINE II = 1
        if (count[min] != 0) break;
    }

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

    nptr = table;
    nptr_extra = table_extra;

    curr = root;
    drop = 0;
    low = (uint32_t)(-1);
    mask = (1 << root) - 1;
    bool is_extra = false;

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

        uint32_t code_val = ((uint32_t)code_data_op << 24) | ((uint32_t)code_data_bits << 16) | code_data_val;
        do {
            fill -= incr;
            if (is_extra) {
                nptr_extra[(huff >> drop) + fill] = code_val;
            } else {
                nptr[(huff >> drop) + fill] = code_val;
            }
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
            if (drop == 0) {
                drop = root;
                min = 0;
                is_extra = true;
            }

            extra_idx += min;
            nptr_extra += min;
            curr = len - drop;
            left = (int)(1 << curr);

            for (int i = curr; i + drop < max; i++, curr++) {
                left -= count[curr + drop];
                if (left <= 0) break;
                left <<= 1;
            }

            low = huff & mask;
            table[low] = ((uint32_t)curr << 24) | ((uint32_t)root << 16) | extra_idx;
        }
    }
}
} // end details

/**
 * @brief This module is ZLIB/GZIP Fixed, Dynamic and Stored block supported
 * decoder. It takes ZLIB/GZIP Huffman encoded data as input and generates
 * decoded data in LZ77 format (Literal, Length, Offset).
 *
 * @tparam DECODER Fixed, Full, Dynamic huffman block support
 * @tparam ByteGenLoopII core bytegenerator loop initiation interval
 * @tparam USE_GZIP switch that controls GZIP/ZLIB header processing
 *
 *
 * @param inStream input bit packed data
 * @param outStream output lz77 compressed output in the form of 32bit packets
 * (Literals, Match Length, Distances)
 * @param endOfStream output completion of execution
 * @param input_size input data size
 */
template <eHuffmanType DECODER = FULL, int ByteGenLoopII = 2, bool USE_GZIP = 0>
void huffmanDecoder(hls::stream<ap_uint<16> >& inStream,
                    hls::stream<compressd_dt>& outStream,
                    hls::stream<bool>& endOfStream,
                    uint32_t input_size) {
    uint64_t bitbuffer = 0;
    uint32_t curInSize = input_size;
    uint8_t bits_cntr = 0;

    uint8_t current_op = 0;
    uint8_t current_bits = 0;
    uint16_t current_val = 0;
    ap_uint<32> current_table_val;

    uint8_t len = 0;

    const uint16_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint8_t dynamic_last = 0;
    uint32_t dynamic_nlen = 0;
    uint32_t dynamic_ndist = 0;
    uint32_t dynamic_ncode = 0;
    uint32_t dynamic_curInSize = 0;
    uint16_t dynamic_lens[512];

    uint32_t dynamic_lenbits = 0;
    uint32_t dynamic_distbits = 0;
    uint8_t copy = 0;

    bool blocks_processed = false;
    uint32_t in_cntr = 0; // number of bytes read from input stream

    const uint16_t c_tcodesize = 2048;

    uint32_t array_codes[512];
    uint32_t array_codes_extra[512];
    uint32_t array_codes_dist[512];
    uint32_t array_codes_dist_extra[256];

    uint8_t block_mode;
    int cntr = 0;
    uint32_t used = 0;

    const bool include_fixed_block = (DECODER == FIXED || DECODER == FULL);
    const bool include_dynamic_block = (DECODER == DYNAMIC || DECODER == FULL);
    bool skip_fname = false;
    if (USE_GZIP) {
        // GZIP Header Processing
        // Magic Header
        uint16_t lcl_tmp = inStream.read();
        in_cntr += 2;
        // Deflate mode & file name flag
        lcl_tmp = inStream.read();
        in_cntr += 2;

        // Check for fnam content
        skip_fname = (lcl_tmp >> 8) ? true : false;

        // MTIME - must
        lcl_tmp = inStream.read();
        in_cntr += 2;
        lcl_tmp = inStream.read();
        in_cntr += 2;
        // XFL (2 for high compress, 4 fast)
        // OS code (3Unix, 0Fat)
        lcl_tmp = inStream.read();
        in_cntr += 2;
        uint8_t h, l;

        // If FLG is set to zero by using -n
        if (skip_fname) {
            // Read file name
            do {
                lcl_tmp = inStream.read();
                in_cntr += 2;
                h = (lcl_tmp >> 8);
                l = (lcl_tmp & 0x00ff);
            } while ((l != 0) && (h != 0));

            if (h == 0) {
                bitbuffer = 0;
                bits_cntr = 0;
            } else if (l == 0) {
                bitbuffer = h;
                bits_cntr = 8;
            }
        }

        curInSize -= (in_cntr);
    } else {
        // ZLIB Header Processing
        uint16_t lcl_tmp = inStream.read();
        in_cntr += 2;
        curInSize -= 2;
    }

    while (!blocks_processed) {
        // one block per iteration
        // check if the following block is stored block or compressed block
        if (bits_cntr == 0) {
            // if first block
            uint16_t temp = inStream.read();
            in_cntr += 2;
            bitbuffer += (uint64_t)(temp) << bits_cntr;
            curInSize -= 2;
            bits_cntr += 16;
        }
        // read the last bit in bitbuffer to check if this is last block
        dynamic_last = bitbuffer & 1;
        bitbuffer >>= 1; // dump the bit read
        uint8_t cb_type = (uint8_t)(bitbuffer)&3;
        bitbuffer >>= 2;
        bits_cntr -= 3; // previously dumped 1 bit + current dumped 2 bits

        if (cb_type == 0) { // stored block
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

            bitbuffer >>= 32;
            bits_cntr -= 32;

            if (DECODER == FULL) {
                // Stored block data
                if (bits_cntr < 8) {
                    uint16_t tmp_dt = (uint16_t)inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(tmp_dt) << bits_cntr;
                    curInSize -= 2;
                    bits_cntr += 16;
                }
                compressd_dt tmpVal = 0;
            strd_blk_cpy:
                for (uint32_t i = 0; i < store_length; i++) {
#pragma HLS PIPELINE II = 1
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
            }

        } else if (cb_type == 2) {       // dynamic huffman compressed block
            if (include_dynamic_block) { // compile if decoder should be dynamic/full
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

            dyn_len_bits:
                while (dynamic_curInSize < dynamic_ncode) {
#pragma HLS PIPELINE II = 1
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

                details::code_generator_array_dyn(1, dynamic_lens, 19, array_codes, array_codes_extra, 7);

                dynamic_curInSize = 0;
                uint32_t dlenb_mask = ((1 << dynamic_lenbits) - 1);

                // Figure out codes for LIT/ML and DIST
                while (dynamic_curInSize < dynamic_nlen + dynamic_ndist) {
                    current_table_val = array_codes[(bitbuffer & dlenb_mask)];
                    current_bits = current_table_val.range(23, 16);
                    if (current_bits > bits_cntr) {
                        // read 2-bytes
                        uint16_t tmp_data = inStream.read();
                        in_cntr += 2;
                        bitbuffer += (uint64_t)(tmp_data) << bits_cntr;
                        curInSize -= 2;
                        bits_cntr += 16;
                    }

                    current_table_val = array_codes[(bitbuffer & dlenb_mask)];
                    current_bits = current_table_val.range(23, 16);
                    current_op = current_table_val.range(24, 31);
                    current_val = current_table_val.range(15, 0);

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

                                if (dynamic_curInSize == 0) blocks_processed = true;

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

                    cp_dyn_len:
                        while (copy--) {
#pragma HLS PIPELINE II = 1
                            dynamic_lens[dynamic_curInSize++] = (uint16_t)len;
                        }
                    }
                } // End of while
                details::code_generator_array_dyn(2, dynamic_lens, dynamic_nlen, array_codes, array_codes_extra, 9);

                details::code_generator_array_dyn(3, dynamic_lens + dynamic_nlen, dynamic_ndist, array_codes_dist,
                                                  array_codes_dist_extra, 9);
                // BYTEGEN dynamic state
                if (curInSize >= 6) {
                    // ********************************
                    //  Create Packets Below
                    //  [LIT|ML|DIST|DIST] --> 32 Bit
                    //  Read data from inStream - 8bits
                    //  at a time. Decode the literals,
                    //  ML, Distances based on tables
                    // ********************************

                    // Read from inStream
                    while (bits_cntr < 32) {
                        uint16_t temp = inStream.read();
                        in_cntr += 2;
                        bitbuffer += (uint64_t)(temp) << bits_cntr;
                        bits_cntr += 16;
                    }

                    uint8_t ret = details::huffmanBytegen<ByteGenLoopII>(
                        bitbuffer, bits_cntr, outStream, endOfStream, in_cntr, inStream, array_codes, array_codes_extra,
                        array_codes_dist, array_codes_dist_extra);

                    if (ret == 77) blocks_processed = true;

                } else {
                    blocks_processed = true;
                }
            } else {
                blocks_processed = true;
            }
        } else if (cb_type == 1) {     // fixed huffman compressed block
            if (include_fixed_block) { // compile if decoder should be fixed/full
#include "fixed_codes.hpp"
                // ********************************
                //  Create Packets Below
                //  [LIT|ML|DIST|DIST] --> 32 Bit
                //  Read data from inStream - 8bits
                //  at a time. Decode the literals,
                //  ML, Distances based on tables
                // ********************************
                // Read from inStream
                while (bits_cntr < 32) {
                    uint16_t temp = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(temp) << bits_cntr;
                    bits_cntr += 16;
                }

                // ByteGeneration module
                uint8_t ret = details::huffmanBytegenStatic<ByteGenLoopII>(bitbuffer, bits_cntr, outStream, endOfStream,
                                                                           in_cntr, inStream, fixed_litml_op,
                                                                           fixed_litml_bits, fixed_litml_val);

                if (ret == 77) blocks_processed = true;

            } else {
                blocks_processed = true;
            }
        } else {
            blocks_processed = true;
        }
        if (dynamic_last) blocks_processed = true;
    } // While end

    // Handle leftover bytes
    if (input_size > in_cntr) {
    leftover_bytes:
        for (int i = 0; i < (input_size - in_cntr); i += 2) {
#pragma HLS PIPELINE II = 1
            uint16_t c = inStream.read();
        }
    }

    outStream << 0; // Adding Dummy Data for last end of stream case
    endOfStream << 1;
}

} // Compression
} // XF
#endif // _XFCOMPRESSION_HUFFMAN_DECODER_HPP_
