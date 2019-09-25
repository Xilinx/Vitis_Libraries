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
#ifndef _XFCOMPRESSION_INFLATE_TREES_HPP_
#define _XFCOMPRESSION_INFLATE_TREES_HPP_
#define MAXBITS 15

namespace xf {
namespace compression {
void code_generator_array(uint8_t curr_table,
                          uint16_t* lens,
                          uint32_t codes,
                          uint8_t* table_op,
                          uint8_t* table_bits,
                          uint16_t* table_val,
                          uint32_t* bits,
                          uint32_t* used) {
#pragma HLS INLINE REGION
    uint32_t sym = 0;
    uint32_t min, max;
    uint32_t root = *bits;
    uint32_t curr;
    uint32_t drop;
    uint32_t huff = 0;
    uint32_t incr;
    uint32_t fill;
    uint32_t low;
    uint32_t mask;

    uint8_t code_data_op = 0;
    uint8_t code_data_bits = 0;
    uint16_t code_data_val = 0;

    uint8_t* nptr_op;
    uint8_t* nptr_bits;
    uint16_t* nptr_val;

    const uint16_t* base;
    const uint16_t* extra;
    uint32_t match;
    uint16_t count[MAXBITS + 1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = count

    uint16_t offs[MAXBITS + 1];
#pragma HLS ARRAY_PARTITION variable = offs

    uint16_t work[512];

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
    for (max = MAXBITS; max >= 1; max--)
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
    for (uint16_t i = 1; i <= MAXBITS; i++) {
#pragma HLS PIPELINE II = 1
        left <<= 1;
        left -= count[i];
    }

    offs[1] = 0;
offs_loop:
    for (uint16_t i = 1; i < MAXBITS; i++)
#pragma HLS PIPELINE II = 1
        offs[i + 1] = offs[i] + count[i];

codes_loop:
    for (uint16_t i = 0; i < codes; i++) {
#pragma HLS PIPELINE II = 1
        if (lens[i] != 0) work[offs[lens[i]]++] = (uint16_t)i;
    }

    switch (curr_table) {
        case 1:
            base = extra = work;
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

        if (work[sym] + 1 < match) {
            code_data_op = (uint8_t)0;
            code_data_val = work[sym];
        } else if (work[sym] >= match) {
            code_data_op = (uint8_t)(extra[work[sym] - match]);
            code_data_val = base[work[sym] - match];
        } else {
            code_data_op = (uint8_t)(32 + 64);
            code_data_val = 0;
        }

        incr = 1 << (len - drop);
        fill = 1 << curr;
        min = fill;

        do {
            fill -= incr;
            ////printme("Array Index %d\n", (huff >> drop) + fill);
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
            len = lens[work[sym]];
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

} // Compression namespace
} // xf namespace
#endif // _XFCOMPRESSION_INFLATE_TREES_HPP_
