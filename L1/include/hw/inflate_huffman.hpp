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
#ifndef _XFCOMPRESSION_INFLATE_HUFFMAN_HPP_
#define _XFCOMPRESSION_INFLATE_HUFFMAN_HPP_

/**
 * @file inflate_huffman.h
 * @brief Header for defining functions used in inflate algorithm used in zlib decompression kernel.
 *
 * This file is part of XF Compression Library.
 */

//#include "common.h"
//#include "hls_stream.h"
//#include <ap_int.h>

#define MAXBITS 15
#define TCODESIZE 2048

#define HEADER_STATE 1
#define TREE_PMBL_STATE 2
#define STORE_STATE 3
#define DYNAMIC_STATE 4
#define BYTEGEN_STATE 5
#define COMPLETE_STATE 6
#define STATIC_STATE 7

#define LITERAL_STAGE 1
#define MATCH_LENGTH_STAGE 2
#define MATCH_DIST_STAGE 3
#define MATCH_DIST_WRITE_STAGE 4

#define BIT 8

#define READBITS(n) \
    while (bits < (uint32_t)(n)) NEXTBYTE();

#define NEXTBYTE()                                     \
    {                                                  \
        curInSize -= 2;                                \
        uint16_t temp_lcl = (uint16_t)inStream.read(); \
        in_cntr += 2;                                  \
        bitbuffer += (uint64_t)(temp_lcl) << bits;     \
        bits += 16;                                    \
    }

#define BITS(n) ((uint32_t)bitbuffer & ((1 << (n)) - 1))

#define DUMPBITS(n)    \
    bitbuffer >>= (n); \
    bits -= (uint32_t)(n);

namespace xf {
namespace compression {
void bitUnPacker(hls::stream<ap_uint<2 * BIT> >& inStream,
                 hls::stream<xf::compression::compressd_dt>& outStream,
                 hls::stream<bool>& endOfStream,
                 uint32_t input_size) {
    uint64_t bitbuffer = 0;
    uint32_t curInSize = input_size;
    uint32_t left = -1;
    uint8_t bits = 0;

    uint8_t here_op = 0;
    uint8_t here_bits = 0;
    uint16_t here_val = 0;

    uint8_t len = 0;

    const uint16_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint8_t dynamic_last = 0;
    uint32_t dynamic_nlen = 0;      // No of length code lengths
    uint32_t dynamic_ndist = 0;     // No of dist code lengths
    uint32_t dynamic_ncode = 0;     // No of code length code lenghts
    uint32_t dynamic_curInSize = 0; // No of code lengths in lens
    uint16_t dynamic_lens[512];     // temp storatge for code lengths

    uint32_t in_cntr = 0;

    uint32_t dynamic_lenbits = 0;
    uint32_t dynamic_distbits = 0;
    uint8_t copy = 0;

    bool done = true;

    uint8_t array_codes_op[TCODESIZE];
    uint8_t array_codes_bits[TCODESIZE];
    uint16_t array_codes_val[TCODESIZE];

    uint8_t block_mode;
    int cntr = 0;
    uint32_t used = 0;
    uint8_t next_state = HEADER_STATE;

    while (done) {
        done = false;

        if (next_state == HEADER_STATE) {
            // printme("HEADER_STATE \n");
            done = true;
            READBITS(16);
            DUMPBITS(4);

            next_state = TREE_PMBL_STATE;

            bitbuffer = 0;
            bits = 0;
        } else if (next_state == TREE_PMBL_STATE) {
            // printme("tree PMBL State %d bits %d input_size %d in_cntr %d\n", bitbuffer, bits, input_size, in_cntr);
            done = true;

            if (bitbuffer == 0) READBITS(3);

            dynamic_last = BITS(1);
            // printme("[PMBL]: dynamic_last %d bitbuffer %d \n", dynamic_last, bitbuffer);
            DUMPBITS(1);

            switch (BITS(2)) {
                case 0:
                    next_state = STORE_STATE;
                    break;
                case 1:
                    done = false;
                    break;
                case 2:
                    next_state = DYNAMIC_STATE;
                    break;
                case 3:
                    done = false;
                    break;
            }
            DUMPBITS(2);

        } else if (next_state == STORE_STATE) {
            // printme("store state bits %d bitbuffer %d\n", bits, bitbuffer);
            done = true;
            bitbuffer >>= bits & 7;
            bits -= bits & 7;
            // printme("bits %d bitbuffer %d\n", bits, bitbuffer);

            READBITS(32);
            // printme("[Read32]: bitbuffer %d bits %d \n", bitbuffer, bits);
            if (bits > 32) {
                bitbuffer >>= 32;
                bits = bits - 32;
            } else {
                bitbuffer = 0;
                bits = 0;
            }

            // next_state = TREE_PMBL_STATE;
            if (dynamic_last) {
                next_state = COMPLETE_STATE;
            } else {
                next_state = TREE_PMBL_STATE;
            }

        } else if (next_state == DYNAMIC_STATE) {
            // printme("dynamic state \n");
            done = true;
            READBITS(14);
            dynamic_nlen = BITS(5) + 257; // Max 288
            DUMPBITS(5);

            dynamic_ndist = BITS(5) + 1; // Max 30
            DUMPBITS(5);

            dynamic_ncode = BITS(4) + 4; // Max 19
            DUMPBITS(4);

            dynamic_curInSize = 0;

            while (dynamic_curInSize < dynamic_ncode) {
                //////printme("In BITLENHUFF \n");
                READBITS(3);
                dynamic_lens[order[dynamic_curInSize++]] = (uint16_t)BITS(3);
                DUMPBITS(3);
            }

            while (dynamic_curInSize < 19) dynamic_lens[order[dynamic_curInSize++]] = 0;

            dynamic_lenbits = 7;

            xf::compression::code_generator_array(1, dynamic_lens, 19, array_codes_op, array_codes_bits,
                                                  array_codes_val, &dynamic_lenbits, &used);

            dynamic_curInSize = 0;

            // Figure out codes for LIT/ML and DIST
            while (dynamic_curInSize < dynamic_nlen + dynamic_ndist) {
                for (;;) {
                    here_op = array_codes_op[BITS(dynamic_lenbits)];
                    here_bits = array_codes_bits[BITS(dynamic_lenbits)];
                    here_val = array_codes_val[BITS(dynamic_lenbits)];
                    if ((uint32_t)(here_bits) <= bits) break;
                    NEXTBYTE();
                }

                if (here_val < 16) {
                    DUMPBITS(here_bits);
                    dynamic_lens[dynamic_curInSize++] = here_val;
                } else {
                    if (here_val == 16) {
                        READBITS(here_bits + 2);
                        DUMPBITS(here_bits);

                        if (dynamic_curInSize == 0) done = false;

                        len = dynamic_lens[dynamic_curInSize - 1];
                        copy = 3 + BITS(2);
                        DUMPBITS(2);

                    } else if (here_val == 17) {
                        READBITS(here_bits + 3);
                        DUMPBITS(here_bits);
                        len = 0;
                        copy = 3 + BITS(3);
                        DUMPBITS(3);
                    } else {
                        READBITS(here_bits + 7);
                        DUMPBITS(here_bits);
                        len = 0;
                        copy = 11 + BITS(7);
                        DUMPBITS(7);
                    }

                    while (copy--) dynamic_lens[dynamic_curInSize++] = (uint16_t)len;
                }
            } // End of while
            dynamic_lenbits = 9;
            xf::compression::code_generator_array(2, dynamic_lens, dynamic_nlen, array_codes_op, array_codes_bits,
                                                  array_codes_val, &dynamic_lenbits, &used);

            dynamic_distbits = 6;
            uint32_t dused = 0;
            xf::compression::code_generator_array(3, dynamic_lens + dynamic_nlen, dynamic_ndist, &array_codes_op[used],
                                                  &array_codes_bits[used], &array_codes_val[used], &dynamic_distbits,
                                                  &dused);
            next_state = BYTEGEN_STATE;
            // printme("Done with code_gen \n");
            // printme("incntr %d \n", in_cntr);
            // done = false;
        } else if (next_state == BYTEGEN_STATE) {
            // printme("byte_gen state \n");
            done = true;
            if (curInSize >= 6) {
                // mask length codes 1st level
                uint32_t lit_mask = (1 << dynamic_lenbits) - 1;
                // mask length codes 2nd level
                uint32_t dist_mask = (1 << dynamic_distbits) - 1;

                // Read from the table
                uint8_t here_op = 0;
                uint8_t here_bits = 0;
                uint16_t here_val = 0;

                uint8_t* mltable_op = array_codes_op;
                uint8_t* mltable_bits = array_codes_bits;
                uint16_t* mltable_val = array_codes_val;

                uint8_t* disttable_op = &array_codes_op[used];
                uint8_t* disttable_bits = &array_codes_bits[used];
                uint16_t* disttable_val = &array_codes_val[used];

                uint32_t op;
                uint16_t len;
                uint16_t dist;

                uint32_t ml_op = 0;
                uint32_t dist_op = 0;

                // ********************************
                //  Create Packets Below
                //  [LIT|ML|DIST|DIST] --> 32 Bit
                //  Read data from inStream - 8bits
                //  at a time. Decode the literals,
                //  ML, Distances based on tables
                // ********************************
                uint8_t curr_stage = LITERAL_STAGE;
                xf::compression::compressd_dt tmpVal;
                bool done = false;
                uint32_t cntr = 0;

                // Read from inStream
                if (bits < 15) {
                    uint16_t temp = inStream.read();
                    in_cntr += 2;
                    bitbuffer += (uint64_t)(temp) << bits;
                    bits += 16;
                }

                uint32_t lidx = bitbuffer & lit_mask;
                here_op = mltable_op[lidx];
                here_bits = mltable_bits[lidx];
                here_val = mltable_val[lidx];
                bool read_ml_bram = false;
                bool read_dist_bram = false;
                uint32_t didx = 0;

            ByteGen:
                for (; !done;) {
#pragma HLS PIPELINE II = 2
                    // In this stage read literals
                    // if length is read then push out
                    // if its single length if its extra
                    // move to match length stage
                    // This stage generates output
                    // INSTREAM: 1
                    // OUTSTREAM: 1
                    if (curr_stage == LITERAL_STAGE) {
                        uint32_t temp = (uint32_t)(here_bits);
                        bitbuffer >>= temp;
                        bits -= temp;
                        ml_op = (uint32_t)(here_op);

                        // Push Literal
                        // Otherwise fill length
                        if (ml_op == 0) {
                            tmpVal.range(7, 0) = (uint16_t)(here_val);
                            tmpVal.range(31, 8) = 0;
                            // printf("%c", here_val);
                            outStream << tmpVal;
                            endOfStream << 0;
                            curr_stage = LITERAL_STAGE;

                            if (bits < 15) {
                                uint16_t temp = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(temp) << bits;
                                bits += 16;
                            }

                            lidx = bitbuffer & lit_mask;
                            read_ml_bram = true;
                        } else {
                            curr_stage = MATCH_LENGTH_STAGE;
                        }
                    } else if (curr_stage == MATCH_LENGTH_STAGE) {
                        // In this stage read the extra match length
                        // update the tmpVal
                        // This stage reads but doesnt write
                        // INSTREAM: 1
                        // OUTSTREAM: NIL
                        if (ml_op & 16) {
                            len = (uint16_t)(here_val);
                            ml_op &= 15;
                            if (ml_op) {
                                len += (uint32_t)bitbuffer & ((1 << ml_op) - 1);
                                bitbuffer >>= ml_op;
                                bits -= ml_op;
                            }
                            tmpVal.range(31, 16) = len;
                            tmpVal.range(7, 0) = 0;
                            // printf("\tlen %d ", len);
                            curr_stage = MATCH_DIST_STAGE;

                            if (bits < 15) {
                                uint16_t temp = inStream.read();
                                in_cntr += 2;
                                bitbuffer += (uint64_t)(temp) << bits;
                                bits += (2 * BIT);
                            }

                            didx = bitbuffer & dist_mask;
                            read_dist_bram = true;
                        } else if ((ml_op & 64) == 0) {
                            curr_stage = LITERAL_STAGE;
                            lidx = here_val + (bitbuffer & ((1 << ml_op) - 1));
                            read_ml_bram = true;
                        } else if (ml_op & 32) {
                            // Termination Condition
                            next_state = 2;
                            done = 1;
                        }

                    } else if (curr_stage == MATCH_DIST_STAGE) {
                        // In this stage read tmpVal - filled with match length
                        // data, work on match dist, read once or twice based
                        // on extra match dist
                        // This stage generates output
                        // INSTREAM: 1
                        // OUTSTREAM: 1
                        uint32_t temp = (uint32_t)(here_bits);
                        bitbuffer >>= temp;
                        bits -= temp;
                        dist_op = (uint32_t)(here_op);

                        if (dist_op & 16) {
                            curr_stage = MATCH_DIST_WRITE_STAGE;
                        } else if ((dist_op & 64) == 0) {
                            didx = here_val + (bitbuffer & ((1 << dist_op) - 1));
                            read_dist_bram = true;
                            curr_stage = MATCH_DIST_STAGE;
                        } else {
                            next_state = 77;
                            done = 1;
                        }
                    } else if (curr_stage == MATCH_DIST_WRITE_STAGE) {
                        dist = (uint32_t)(here_val);
                        dist_op &= 15;
                        dist += (uint32_t)bitbuffer & ((1 << dist_op) - 1);
                        bitbuffer >>= dist_op;
                        bits -= dist_op;

                        tmpVal.range(15, 0) = dist;
                        outStream << tmpVal;
                        endOfStream << 0;
                        curr_stage = LITERAL_STAGE;

                        if (bits < 15) {
                            uint16_t temp = inStream.read();
                            in_cntr += 2;
                            bitbuffer += (uint64_t)(temp) << bits;
                            bits += (2 * BIT);
                        }

                        lidx = bitbuffer & lit_mask;
                        read_ml_bram = true;
                        // printf("dist %d \n", dist);
                    }
                    if (read_ml_bram) {
                        read_ml_bram = false;
                        here_op = mltable_op[lidx];
                        here_bits = mltable_bits[lidx];
                        here_val = mltable_val[lidx];
                    }

                    if (read_dist_bram) {
                        read_dist_bram = false;
                        here_op = disttable_op[didx];
                        here_bits = disttable_bits[didx];
                        here_val = disttable_val[didx];
                    }

                    // Read inStream
                    if (bits < 15) {
                        uint16_t temp = inStream.read();
                        in_cntr += 2;
                        bitbuffer += (uint64_t)(temp) << bits;
                        bits += (2 * BIT);
                    }

                } // Top for-loop ends hre

                if (next_state == 77) done = false;
            } else
                done = false;
        } else if (next_state == COMPLETE_STATE) {
            done = false;
            break;
        }

    } // While end

    // printme("input_size %d in_cntr %d \n", input_size, in_cntr);

    uint32_t leftover = input_size - in_cntr;
    if (leftover) {
        for (int i = 0; i < leftover; i += 2) {
            uint16_t c = inStream.read();
        }
    }

    outStream << 0; // Adding Dummy Data for last end of stream case
    endOfStream << 1;
}
} // Compression
} // XF
#endif // _XFCOMPRESSION_INFLATE_HUFFMAN_HPP_
