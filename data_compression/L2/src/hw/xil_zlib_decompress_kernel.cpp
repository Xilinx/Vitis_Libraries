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
#include "zlib_decompress_kernel.hpp"

#define READBITS(n)                              \
    do {                                         \
        while (bits < (unsigned)(n)) NEXTBYTE(); \
    } while (0)

#define NEXTBYTE()                                  \
    do {                                            \
        curInSize--;                                \
        uint8_t temp = (uint8_t)inStream.read();    \
        in_cntr++;                                  \
        bitbuffer += (unsigned long)(temp) << bits; \
        bits += 8;                                  \
    } while (0)

#define BITS(n) ((unsigned)bitbuffer & ((1U << (n)) - 1))

#define DUMPBITS(n)            \
    do {                       \
        bitbuffer >>= (n);     \
        bits -= (unsigned)(n); \
    } while (0)

#define INITBITS()     \
    do {               \
        bitbuffer = 0; \
        bits = 0;      \
    } while (0)

#define BYTEALIGN()             \
    do {                        \
        bitbuffer >>= bits & 7; \
        bits -= bits & 7;       \
    } while (0)

#define BITS(n) ((unsigned)bitbuffer & ((1U << (n)) - 1))

#if (D_COMPUTE_UNIT == 1)
namespace cu1
#elif (D_COMPUTE_UNIT == 2)
namespace cu2
#endif
{

int inflate_table(packcodedata type, uint16_t* lens, unsigned codes, code** table, unsigned* bits, uint16_t* work) {
    unsigned len;
    unsigned sym;
    unsigned min, max;
    unsigned root;
    unsigned curr;
    unsigned drop;
    int left;
    unsigned used;
    unsigned huff;
    unsigned incr;
    unsigned fill;
    unsigned low;
    unsigned mask;
    code here;
    code* nptr;
    const uint16_t* base;
    const uint16_t* extra;
    unsigned match;
    uint16_t count[MAXBITS + 1];
    uint16_t offs[MAXBITS + 1];
    const uint16_t lbase[31] = {3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
                                35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};
    static const uint16_t lext[31] = {16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
                                      19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 77, 202};
    static const uint16_t dbase[32] = {1,    2,    3,    4,    5,    7,     9,     13,    17,  25,   33,
                                       49,   65,   97,   129,  193,  257,   385,   513,   769, 1025, 1537,
                                       2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0,   0};
    static const uint16_t dext[32] = {16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
                                      23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 64, 64};

    for (len = 0; len <= MAXBITS; len++) count[len] = 0;
    for (sym = 0; sym < codes; sym++) count[lens[sym]]++;

    root = *bits;
    for (max = MAXBITS; max >= 1; max--)
        if (count[max] != 0) break;
    if (root > max) root = max;
    if (max == 0) {
        here.op = (uint8_t)64;
        here.bits = (uint8_t)1;
        here.val = (uint16_t)0;
        *(*table)++ = here;
        *(*table)++ = here;
        *bits = 1;
        return 0;
    }
    for (min = 1; min < max; min++)
        if (count[min] != 0) break;
    if (root < min) root = min;

    left = 1;
    for (len = 1; len <= MAXBITS; len++) {
        left <<= 1;
        left -= count[len];
        if (left < 0) return -1;
    }
    if (left > 0 && (type == CODES || max != 1)) return -1;

    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++) offs[len + 1] = offs[len] + count[len];

    for (sym = 0; sym < codes; sym++)
        if (lens[sym] != 0) work[offs[lens[sym]]++] = (uint16_t)sym;

    switch (type) {
        case CODES:
            base = extra = work;
            match = 20;
            break;
        case LENS:
            base = lbase;
            extra = lext;
            match = 257;
            break;
        default: /* DISTS */
            base = dbase;
            extra = dext;
            match = 0;
    }

    huff = 0;
    sym = 0;
    len = min;
    nptr = *table;
    curr = root;
    drop = 0;
    low = (unsigned)(-1);
    used = 1U << root;
    mask = used - 1;

    if ((type == LENS && used > TOTAL_LENS) || (type == DISTS && used > TOTAL_DISTS)) return 1;

    for (;;) {
        here.bits = (uint8_t)(len - drop);
        if (work[sym] + 1U < match) {
            here.op = (uint8_t)0;
            here.val = work[sym];
        } else if (work[sym] >= match) {
            here.op = (uint8_t)(extra[work[sym] - match]);
            here.val = base[work[sym] - match];
        } else {
            here.op = (uint8_t)(32 + 64);
            here.val = 0;
        }

        incr = 1U << (len - drop);
        fill = 1U << curr;
        min = fill;
        do {
            fill -= incr;
            nptr[(huff >> drop) + fill] = here;
        } while (fill != 0);

        incr = 1U << (len - 1);
        while (huff & incr) incr >>= 1;
        if (incr != 0) {
            huff &= incr - 1;
            huff += incr;
        } else
            huff = 0;

        sym++;
        if (--(count[len]) == 0) {
            if (len == max) break;
            len = lens[work[sym]];
        }

        if (len > root && (huff & mask) != low) {
            if (drop == 0) drop = root;

            nptr += min;

            curr = len - drop;
            left = (int)(1 << curr);
            while (curr + drop < max) {
                left -= count[curr + drop];
                if (left <= 0) break;
                curr++;
                left <<= 1;
            }

            used += 1U << curr;
            if ((type == LENS && used > TOTAL_LENS) || (type == DISTS && used > TOTAL_DISTS)) return 1;

            low = huff & mask;
            (*table)[low].op = (uint8_t)curr;
            (*table)[low].bits = (uint8_t)root;
            (*table)[low].val = (uint16_t)(nptr - *table);
        }
    }

    if (huff != 0) {
        here.op = (uint8_t)64;
        here.bits = (uint8_t)(len - drop);
        here.val = (uint16_t)0;
        nptr[huff] = here;
    }

    *table += used;
    *bits = root;
    return 0;
}

uint32_t s2mm_simple(uint512_t* out, hls::stream<uint512_t>& inStream512, hls::stream<uint32_t>& inStream512Size) {
    const int c_byte_size = 8;
    const int c_factor = GMEM_DWIDTH / c_byte_size;

    uint32_t outIdx = 0;
    uint32_t size = 1;
    uint32_t sizeIdx = 0;
    uint32_t total_size = 0;

    for (outIdx = 0; size != 0;) {
        size = inStream512Size.read();
        total_size += size;
        uint32_t sizeInWord = size ? ((size - 1) / c_factor + 1) : 0;

        for (int i = 0; i < sizeInWord; i++) {
            out[outIdx + i] = inStream512.read();
        }
        outIdx += sizeInWord;
    }

    return total_size;
}

void bitUnPacker(hls::stream<ap_uint<BIT> >& inStream,
                 hls::stream<compressd_dt>& outStream,
                 hls::stream<bool>& endOfStream,
                 uint32_t input_size) {
    uint64_t bitbuffer = 0;
    uint32_t curInSize = input_size;
    uint32_t left = -1;
    uint8_t bits = 0;

    // Present decoding entry
    code here;

    // Parent table entry
    code last;

    // Length to copy for repeats and bits to drop
    uint8_t len = 0;

    const uint16_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    uint8_t dynamic_mode = TOP;
    uint8_t dynamic_wrap = 5;
    uint8_t dynamic_wbits = 15;
    uint8_t dynamic_last = 0;
    uint32_t dynamic_length = 0;
    uint32_t dynamic_nlen = 0;      // No of length code lengths
    uint32_t dynamic_ndist = 0;     // No of dist code lengths
    uint32_t dynamic_ncode = 0;     // No of code length code lenghts
    uint32_t dynamic_curInSize = 0; // No of code lengths in lens
    uint16_t dynamic_lens[320];     // temp storatge for code lengths
    uint16_t dynamic_work[288];     // work area for code table building

    uint32_t in_cntr = 0;

    code dynamic_codes[TCODESIZE]; // All codes are held in this buffer

    code* dynamic_nptr;
    code* dynamic_lencode;
    code* dynamic_distcode;

    uint32_t dynamic_lenbits = 0;
    uint32_t dynamic_distbits = 0;
    int dynamic_back = 0;
    uint16_t ret = 0;
    uint32_t dest_len = 0;
    uint8_t copy = 0;

    // TODO:nptr_state switch based on dataflow
    for (;;) {
        ////printme("In side forlloop \n");
        switch (dynamic_mode) {
            case TOP:
                READBITS(16);

                if (((BITS(8) << 8) + (bitbuffer >> 8)) % 31) {
                    ////printme("In First IF \n");
                    dynamic_mode = WRONG;
                    break;
                }

                if (BITS(4) != Z_DEFLATED) {
                    dynamic_mode = WRONG;
                    ////printme("Un known method \n");
                    break;
                }

                DUMPBITS(4);
                len = BITS(4) + 8;

                if (dynamic_wbits == 0) dynamic_wbits = len;
                if (len > 15 || len > dynamic_wbits) {
                    dynamic_mode = WRONG;
                    ////printme("Invalid window size \n");
                    break;
                }

                dynamic_mode = PATH;

                INITBITS();
                break;
            case PATH:
            // printme("In PATH \n");
            case PATHDO:
                // printme("In PATHDO stage - Kali\n");
                if (dynamic_last) {
                    //////printme("PATHDO \n");
                    BYTEALIGN();
                    dynamic_mode = CHECK;
                    break;
                }

                READBITS(3);
                dynamic_last = BITS(1);
                //////printme("dynamic_last %d \n", dynamic_last);
                DUMPBITS(1);

                //////printme("Before switch statement \n");
                switch (BITS(2)) {
                    case 0: /* stored block */
                        // printme("stored block \n");
                        dynamic_mode = STORED;
                        break;
                    case 1: /* fixed block */
                        ////printme("fixed block \n");
                        // fixedtables(state);
                        dynamic_mode = BITLENGTH; /* decode codes */
                        break;
                    case 2: /* dynamic block */
                        // printme("dynamic block \n");
                        dynamic_mode = TABLE;
                        break;
                    case 3:
                        ////printme("Invalid block %d\n", BITS(2));
                        dynamic_mode = WRONG;
                }

                DUMPBITS(2);
                break;

            case STORED:
                ////printme("STORED \n");
                BYTEALIGN(); /* go to byte boundary */
                READBITS(32);
                if ((bitbuffer & 0xffff) != ((bitbuffer >> 16) ^ 0xffff)) {
                    dynamic_mode = WRONG;
                    ////printme("Invalid storedblock \n");
                    break;
                }
                dynamic_length = (unsigned)bitbuffer & 0xffff;
                INITBITS();
                dynamic_mode = COPY_;

            case COPY_:
                ////printme("COPY_ \n");
                dynamic_mode = COPY;
            case COPY:
                ////printme("COPY \n");
                copy = dynamic_length;
                ////printme("dynamic_length %d \n", dynamic_length);
                //////printme("left %d \n", left);
                if (copy) {
                    if (copy > curInSize) copy = curInSize;
                    if (copy > left) copy = left;

                    ////printme("copy %d \n", copy);
                    // In stored block we need to
                    // copy data as it is
                    for (int i = 0; i < copy; i++)
                        // put[i] = nptr[i];

                        curInSize -= copy;
                    // nptr += copy;
                    left -= copy;
                    // put += copy;
                    dynamic_length -= copy;
                    break;
                }
                dynamic_mode = PATH;
                break;
            case TABLE:
                ////printme("TABLE\n");
                READBITS(14);
                dynamic_nlen = BITS(5) + 257;
                DUMPBITS(5);
                dynamic_ndist = BITS(5) + 1;
                DUMPBITS(5);
                dynamic_ncode = BITS(4) + 4;
                DUMPBITS(4);
                dynamic_curInSize = 0;
                dynamic_mode = BITLENHUFF;
            case BITLENHUFF:
                //////printme("BITLENHUFF \n");
                while (dynamic_curInSize < dynamic_ncode) {
                    //////printme("In BITLENHUFF \n");
                    READBITS(3);
                    dynamic_lens[order[dynamic_curInSize++]] = (uint16_t)BITS(3);
                    DUMPBITS(3);
                }
                while (dynamic_curInSize < 19) dynamic_lens[order[dynamic_curInSize++]] = 0;

                dynamic_nptr = dynamic_codes;
                dynamic_lencode = (code*)(dynamic_nptr);
                dynamic_lenbits = 7;

                ret = inflate_table(CODES, dynamic_lens, 19, &(dynamic_nptr), &(dynamic_lenbits), dynamic_work);
                //////printme("ret inflate_table %d\n", ret);
                if (ret) {
                    dynamic_mode = WRONG;
                    //////printme("Invalid code length set \n");
                    break;
                }
                dynamic_curInSize = 0;
                dynamic_mode = CODELENS;
            case CODELENS:
                //////printme("CODELENS \n");
                while (dynamic_curInSize < dynamic_nlen + dynamic_ndist) {
                    for (;;) {
                        here = dynamic_lencode[BITS(dynamic_lenbits)];
                        if ((unsigned)(here.bits) <= bits) break;
                        NEXTBYTE();
                    }

                    if (here.val < 16) {
                        DUMPBITS(here.bits);
                        dynamic_lens[dynamic_curInSize++] = here.val;
                    } else {
                        if (here.val == 16) {
                            READBITS(here.bits + 2);
                            DUMPBITS(here.bits);
                            if (dynamic_curInSize == 0) {
                                dynamic_mode = WRONG;
                                //////printme("Invalid bit length repeat\n");
                                break;
                            }
                            len = dynamic_lens[dynamic_curInSize - 1];
                            copy = 3 + BITS(2);
                            DUMPBITS(2);
                        } else if (here.val == 17) {
                            READBITS(here.bits + 3);
                            DUMPBITS(here.bits);
                            len = 0;
                            copy = 3 + BITS(3);
                            DUMPBITS(3);
                        } else {
                            READBITS(here.bits + 7);
                            DUMPBITS(here.bits);
                            len = 0;
                            copy = 11 + BITS(7);
                            DUMPBITS(7);
                        }

                        if (dynamic_curInSize + copy > dynamic_nlen + dynamic_ndist) {
                            dynamic_mode = WRONG;
                            //////printme("Invalid bit length repeat \n");
                            break;
                        }
                        while (copy--) dynamic_lens[dynamic_curInSize++] = (uint16_t)len;
                    }
                } // End of while

                if (dynamic_mode == WRONG) break;

                //////printme("dynamic_lens[256] %d \n", state->lens[256]);
                if (dynamic_lens[256] == 0) {
                    //////printme("missing end of block \n");
                    dynamic_mode = WRONG;
                    break;
                }

                dynamic_nptr = dynamic_codes;
                dynamic_lencode = (code*)(dynamic_nptr);
                dynamic_lenbits = 9;
                ret =
                    inflate_table(LENS, dynamic_lens, dynamic_nlen, &(dynamic_nptr), &(dynamic_lenbits), dynamic_work);
                if (ret) {
                    dynamic_mode = WRONG;
                    ////printme("invalid literal/length set \n");
                    break;
                }
                dynamic_distcode = (code*)(dynamic_nptr);
                dynamic_distbits = 6;
                ret = inflate_table(DISTS, dynamic_lens + dynamic_nlen, dynamic_ndist, &(dynamic_nptr),
                                    &(dynamic_distbits), dynamic_work);
                //////printme("ret %d build distance tables\n", ret);
                if (ret) {
                    dynamic_mode = WRONG;
                    //////printme("invalid distance set \n");
                    break;
                }
                dynamic_mode = BITLENGTH;
            case BITLENGTH:
                ////printme("BITLENGTH \n");
                dynamic_mode = LEN;
            case LEN:
                if (curInSize >= 6 && left >= 258) {
                    ////printme("LEN inside if dynamic_mode %d \n", dynamic_mode);
                    uint32_t inDataStride = 0;
                    uint32_t outDataStride = 0;

                    // Read from the table
                    code here;

                    // mask length codes 1st level
                    uint32_t lmask = (1U << dynamic_lenbits) - 1;
                    // mask length codes 2nd level
                    uint32_t dmask = (1U << dynamic_distbits) - 1;

                    unsigned op;  /* code bits, operation, extra bits, or */
                                  /*  window position, window bytes to copy */
                    uint16_t len; /* match length, unused bytes */
                    uint16_t dist;

                    // ********************************
                    //  Create Packets Below
                    //  [LIT|ML|DIST|DIST] --> 32 Bit
                    //  Read data from inStream - 8bits
                    //  at a time. Decode the literals,
                    //  ML, Distances based on tables
                    // ********************************

                    compressd_dt tmpVal;
                    for (;;) {
                        if (bits < 15) {
                            uint8_t temp = inStream.read();
                            in_cntr++;
                            bitbuffer += (unsigned long)(temp) << bits;
                            bits += 8;
                            temp = inStream.read();
                            in_cntr++;
                            bitbuffer += (unsigned long)(temp) << bits;
                            bits += 8;
                        }
                        here = dynamic_lencode[bitbuffer & lmask];

                    dolen:
                        // Literals
                        op = (unsigned)(here.bits);
                        bitbuffer >>= op;
                        bits -= op;
                        op = (unsigned)(here.op);

                        if (op == 0) {
                            tmpVal.range(7, 0) = (uint16_t)(here.val);
                            tmpVal.range(31, 8) = 0;
                            // Literal Section
                            // outStreamSize << 1;
                            outStream << tmpVal;
                            endOfStream << 0;
                        } else if (op & 16) {
                            len = (uint16_t)(here.val);
                            op &= 15;

                            if (op) { // Figure out if you need extra bits
                                if (bits < op) {
                                    uint8_t temp = inStream.read();
                                    in_cntr++;
                                    bitbuffer += (unsigned long)temp << bits;
                                    bits += 8;
                                }
                                len += (unsigned)bitbuffer & ((1U << op) - 1);
                                bitbuffer >>= op;
                                bits -= op;
                            }
                            tmpVal.range(31, 16) = len;
                            tmpVal.range(7, 0) = 0;
                            // printme("len %d \n", len);
                            for (int i = 0; i < len - 1; i++) endOfStream << 0;

                            if (bits < 15) {
                                uint8_t temp = inStream.read();
                                in_cntr++;
                                bitbuffer += (unsigned long)(temp) << bits;
                                bits += 8;
                                temp = inStream.read();
                                in_cntr++;
                                bitbuffer += (unsigned long)(temp) << bits;
                                bits += 8;
                            }

                            // Length is decoded
                            // Lets decode offset
                            here = dynamic_distcode[bitbuffer & dmask];

                        dodist:
                            op = (unsigned)(here.bits);
                            bitbuffer >>= op;
                            bits -= op;
                            op = (unsigned)(here.op);

                            if (op & 16) {
                                dist = (unsigned)(here.val);
                                op &= 15;

                                if (bits < op) {
                                    uint8_t temp = inStream.read();
                                    in_cntr++;
                                    bitbuffer += (unsigned long)(temp) << bits;
                                    bits += 8;

                                    if (bits < op) {
                                        uint8_t temp = inStream.read();
                                        in_cntr++;
                                        bitbuffer += (unsigned long)(temp) << bits;
                                        bits += 8;
                                    }
                                }
                                dist += (unsigned)bitbuffer & ((1U << op) - 1);
                                bitbuffer >>= op;
                                bits -= op;

                                ////printme("len %d offset %d dist \n", len, dist);

                                tmpVal.range(15, 0) = dist;

                                outStream << tmpVal;
                                endOfStream << 0;
                            } else if ((op & 64) == 0) {
                                here = dynamic_distcode[here.val + (bitbuffer & ((1U << op) - 1))];
                                goto dodist;
                            } else {
                                dynamic_mode = WRONG;
                                break;
                            }

                            // ML, Dist section ends
                        } else if ((op & 64) == 0) {
                            here = dynamic_lencode[here.val + (bitbuffer & ((1U << op) - 1))];
                            goto dolen;
                        } else if (op & 32) {
                            dynamic_mode = PATH;
                            break;
                        } else {
                            dynamic_mode = WRONG;
                            break;
                        }

                    } // For loop ends here

                    if (dynamic_mode == PATH) dynamic_back = -1;
                    break;
                }

                dynamic_mode = PATH;
                break;

            case CHECK:
                // printme("CHECK \n");
                //////printme("CHECK stage \n");
                dynamic_mode = DONE;
                break;
            case DONE:
                ////printme("Its done \n");
                break;
            case WRONG:
                ////printme("Its bad \n");
                break;
            default:
                ////printme("default case \n");
                break;
        }

        if (dynamic_mode == WRONG || dynamic_mode == DONE) break;
    } // End of infinite for loop

    uint32_t leftover = input_size - in_cntr;

    if (leftover) {
        for (int i = 0; i < leftover; i++) {
            uint8_t c = inStream.read();
        }
    }

    // printme("WRITING endOfStream \n");
    // outStream << 0;
    endOfStream << 1;

    // printme("Done with bitUnPacker \n");
}

uint32_t uncomp(hls::stream<ap_uint<BIT> >& inStream,
                hls::stream<ap_uint<BIT> >& outStream,
                hls::stream<bool>& byteEndOfStream,
                uint32_t input_size) {
    hls::stream<compressd_dt> bitUnPackStream("unCompOutStream");
    hls::stream<uint32_t> bitUnPackStreamSize("unCompOutStreamSize");
    hls::stream<bool> bitEndOfStream("bitEndOfStream");

#pragma HLS dataflow
    // printme("BitUnPacker \n");
    bitUnPacker(inStream, bitUnPackStream, bitEndOfStream, input_size);
    // printme("lz_decompress_zlib_eos \n");
    uint32_t outSize =
        xf::compression::lzDecompressZlibEos<HISTORY_SIZE, READ_STATE, MATCH_STATE, LOW_OFFSET_STATE, LOW_OFFSET>(
            bitUnPackStream, bitEndOfStream, outStream, byteEndOfStream);

    // printme("outSize %d \n", outSize);
    return outSize;
}

template <int DATAWIDTH, int BURST_SIZE>
void mm2sSimple(const uint512_t* in, hls::stream<uint512_t>& outStream, uint32_t input_size) {
    const int c_byte_size = 8;
    const int c_word_size = DATAWIDTH / c_byte_size;
    const int inSize_gmemwidth = (input_size - 1) / c_word_size + 1;

    for (int i = 0; i < inSize_gmemwidth; i++) {
        outStream << in[i];
    }
}

template <class SIZE_DT, int IN_WIDTH, int OUT_WIDTH>
void upsizerSimple(hls::stream<ap_uint<BIT> >& inStream,
                    hls::stream<uint32_t>& inStreamSize,
                    hls::stream<uint512_t>& outStream,
                    hls::stream<uint32_t>& outStreamSize) {
    const int c_byte_width = IN_WIDTH;
    const int c_upsize_factor = OUT_WIDTH / c_byte_width;
    const int c_in_size = IN_WIDTH / c_byte_width;

    ap_uint<2 * OUT_WIDTH> outBuffer = 0;
    uint32_t byteIdx = 0;

    for (int size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
        uint32_t outSize = ((size + byteIdx) / c_upsize_factor) * c_upsize_factor;
        if (outSize) outStreamSize << outSize;

        for (int i = 0; i < size; i += c_in_size) {
            int chunk_size = c_in_size;
            if (chunk_size + i > size) chunk_size = size - i;

            ap_uint<IN_WIDTH> tmpValue = inStream.read();
            outBuffer.range((byteIdx + c_in_size) * c_byte_width - 1, byteIdx * c_byte_width) = tmpValue;
            byteIdx += chunk_size;

            if (byteIdx >= c_upsize_factor) {
                outStream << outBuffer.range(OUT_WIDTH - 1, 0);
                outBuffer >>= OUT_WIDTH;
                byteIdx -= c_upsize_factor;
            }
        }
    }

    if (byteIdx) {
        outStreamSize << byteIdx;
        outStream << outBuffer.range(OUT_WIDTH - 1, 0);
    }

    outStreamSize << 0;
}

void xilInflate(const uint512_t* in, uint512_t* out, uint32_t* encoded_size, uint32_t input_size) {
    // printme("input_size %d \n", input_size);
    hls::stream<uint512_t> inStream512("inputStream");
    hls::stream<ap_uint<BIT> > outDownStream("outDownStream");
    hls::stream<ap_uint<BIT> > uncompOutStream("unCompOutStream");
    hls::stream<uint512_t> outStream512("outputStream");
    hls::stream<bool> outStream512_eos("outputStreamSize");
    hls::stream<bool> byte_eos("byteEndOfStream");

    uint32_t output_idx[PARALLEL_BLOCK];
    output_idx[0] = 0;

    hls::stream<bool> outStreamWidth_eos[PARALLEL_BLOCK];
    uint32_t output_size = 0;

#pragma HLS dataflow
    mm2sSimple<GMEM_DWIDTH, GMEM_BURST_SIZE>(in, inStream512, input_size);

    xf::compression::streamDownsizer<uint32_t, GMEM_DWIDTH, BIT>(inStream512, outDownStream, input_size);

    output_size = uncomp(outDownStream, uncompOutStream, byte_eos, input_size);

    // printme("upsizer \n");
    // upsizer_simple<uint16_t, BIT, GMEM_DWIDTH>(uncompOutStream, uncompOutStreamSize, outStream512, outStream512Size);
    xf::compression::upsizerEos<uint16_t, BIT, GMEM_DWIDTH>(uncompOutStream, byte_eos, outStream512, outStream512_eos);
    // printme("Upsizer done \n");

    // printme("s2mm_eos \n");
    xf::compression::s2mmEosNbZlib<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH, 1>(
        out, output_idx, outStream512, outStream512_eos, encoded_size, output_size);
    // printme("s2mm_eos done \n");
}

} // namespace cu1

extern "C" {
#if (D_COMPUTE_UNIT == 1)
void xilDecompress_cu1
#elif (D_COMPUTE_UNIT == 2)
void xilDecompress_cu2
#endif
    (uint512_t* in, uint512_t* out, uint32_t* encoded_size, uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = encoded_size offset = slave bundle = gmem0
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = encoded_size bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
////printme("In decompress kernel \n");
#if (D_COMPUTE_UNIT == 1)
    // Call for parallel compression
    cu1::xilInflate(in, out, encoded_size, input_size);
#elif (D_COMPUTE_UNIT == 2)
    // Call for parallel compression
    cu2::xilInflate(in, out, encoded_size, input_size);
#endif
}
}
