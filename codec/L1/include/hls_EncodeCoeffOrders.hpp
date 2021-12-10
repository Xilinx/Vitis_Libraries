/*
 * Copyright 2019 Xilinx, Inc.
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
 */
/**
 * @file hls_EncodeCoeffOrders.hpp
 *
 * @brief This file contains top function of test case.
 */

#ifndef _HLS_ENCODE_TOKENS_HPP_
#define _HLS_ENCODE_TOKENS_HPP_

#include <iostream>
#include <ap_int.h>
#include <hls_stream.h>

namespace xf {
namespace codec {
namespace internal {

const size_t LENGTH = 1024;
const size_t kBlockDim = 8;
const size_t kDCTBlockSize = 64;
const size_t kMaxCoeffBlocks = 32;
const size_t kMaxBlockDim = kBlockDim * kMaxCoeffBlocks;
const size_t kMaxCoeffArea = kMaxBlockDim * kMaxBlockDim;
const uint32_t kPermutationContexts = 8;

struct HybridUint {
    HybridUint(ap_uint<32> split, ap_uint<32> msb, ap_uint<32> lsb)
        : split_exponent(split), msb_in_token(msb), lsb_in_token(lsb) {}
    ap_uint<32> split_exponent;
    ap_uint<32> msb_in_token;
    ap_uint<32> lsb_in_token;
};

enum Type {
    // Regular block size DCT
    DCT = 0,
    // Encode pixels without transforming
    IDENTITY = 1,
    // Use 2-by-2 DCT
    DCT2X2 = 2,
    // Use 4-by-4 DCT
    DCT4X4 = 3,
    // Use 16-by-16 DCT
    DCT16X16 = 4,
    // Use 32-by-32 DCT
    DCT32X32 = 5,
    // Use 16-by-8 DCT
    DCT16X8 = 6,
    // Use 8-by-16 DCT
    DCT8X16 = 7,
    // Use 32-by-8 DCT
    DCT32X8 = 8,
    // Use 8-by-32 DCT
    DCT8X32 = 9,
    // Use 32-by-16 DCT
    DCT32X16 = 10,
    // Use 16-by-32 DCT
    DCT16X32 = 11,
    // 4x8 and 8x4 DCT
    DCT4X8 = 12,
    DCT8X4 = 13,
    // Corner-DCT.
    AFV0 = 14,
    AFV1 = 15,
    AFV2 = 16,
    AFV3 = 17,
    // Larger DCTs
    DCT64X64 = 18,
    DCT64X32 = 19,
    DCT32X64 = 20,
    DCT128X128 = 21,
    DCT128X64 = 22,
    DCT64X128 = 23,
    DCT256X256 = 24,
    DCT256X128 = 25,
    DCT128X256 = 26,
    // Marker for num of valid strategies.
    kNumValidStrategies
};

static constexpr size_t kOffset[kNumValidStrategies + 1] = {
    0,  1,  2,  3,  4,  8,   24,  26,  28,  32,  36,  44,   52,   53,
    54, 55, 56, 57, 58, 122, 154, 186, 442, 570, 698, 1722, 2234, 2746,
};

constexpr uint8_t kStrategyOrder[] = {0, 1, 1, 1, 2, 3, 4, 4, 5,  5,  6,  6,  1, 1,
                                      1, 1, 1, 1, 7, 8, 8, 9, 10, 10, 11, 12, 12};

static constexpr size_t coeffOrderLut[1536] = {
    0,    1,    5,   6,   14,  15,  27,  28,  2,    4,    7,    13,   16,   26,  29,  42,  3,   8,   12,   17,   25,
    30,   41,   43,  9,   11,  18,  24,  31,  40,   44,   53,   10,   19,   23,  32,  39,  45,  52,  54,   20,   22,
    33,   38,   46,  51,  55,  60,  21,  34,  37,   47,   50,   56,   59,   61,  35,  36,  48,  49,  57,   58,   62,
    63,   0,    1,   5,   6,   14,  15,  27,  28,   2,    4,    7,    13,   16,  26,  29,  42,  3,   8,    12,   17,
    25,   30,   41,  43,  9,   11,  18,  24,  31,   40,   44,   53,   10,   19,  23,  32,  39,  45,  52,   54,   20,
    22,   33,   38,  46,  51,  55,  60,  21,  34,   37,   47,   50,   56,   59,  61,  35,  36,  48,  49,   57,   58,
    62,   63,   0,   1,   5,   6,   14,  15,  27,   28,   2,    4,    7,    13,  16,  26,  29,  42,  3,    8,    12,
    17,   25,   30,  41,  43,  9,   11,  18,  24,   31,   40,   44,   53,   10,  19,  23,  32,  39,  45,   52,   54,
    20,   22,   33,  38,  46,  51,  55,  60,  21,   34,   37,   47,   50,   56,  59,  61,  35,  36,  48,   49,   57,
    58,   62,   63,  0,   1,   5,   6,   14,  15,   27,   28,   2,    4,    7,   13,  16,  26,  29,  42,   3,    8,
    12,   17,   25,  30,  41,  43,  9,   11,  18,   24,   31,   40,   44,   53,  10,  19,  23,  32,  39,   45,   52,
    54,   20,   22,  33,  38,  46,  51,  55,  60,   21,   34,   37,   47,   50,  56,  59,  61,  35,  36,   48,   49,
    57,   58,   62,  63,  0,   1,   5,   6,   14,   15,   27,   28,   44,   45,  65,  66,  90,  91,  119,  120,  2,
    3,    7,    13,  16,  26,  29,  43,  46,  64,   67,   89,   92,   118,  121, 150, 4,   8,   12,  17,   25,   30,
    42,   47,   63,  68,  88,  93,  117, 122, 149,  151,  9,    11,   18,   24,  31,  41,  48,  62,  69,   87,   94,
    116,  123,  148, 152, 177, 10,  19,  23,  32,   40,   49,   61,   70,   86,  95,  115, 124, 147, 153,  176,  178,
    20,   22,   33,  39,  50,  60,  71,  85,  96,   114,  125,  146,  154,  175, 179, 200, 21,  34,  38,   51,   59,
    72,   84,   97,  113, 126, 145, 155, 174, 180,  199,  201,  35,   37,   52,  58,  73,  83,  98,  112,  127,  144,
    156,  173,  181, 198, 202, 219, 36,  53,  57,   74,   82,   99,   111,  128, 143, 157, 172, 182, 197,  203,  218,
    220,  54,   56,  75,  81,  100, 110, 129, 142,  158,  171,  183,  196,  204, 217, 221, 234, 55,  76,   80,   101,
    109,  130,  141, 159, 170, 184, 195, 205, 216,  222,  233,  235,  77,   79,  102, 108, 131, 140, 160,  169,  185,
    194,  206,  215, 223, 232, 236, 245, 78,  103,  107,  132,  139,  161,  168, 186, 193, 207, 214, 224,  231,  237,
    244,  246,  104, 106, 133, 138, 162, 167, 187,  192,  208,  213,  225,  230, 238, 243, 247, 252, 105,  134,  137,
    163,  166,  188, 191, 209, 212, 226, 229, 239,  242,  248,  251,  253,  135, 136, 164, 165, 189, 190,  210,  211,
    227,  228,  240, 241, 249, 250, 254, 255, 0,    1,    2,    3,    17,   18,  27,  28,  44,  45,  65,   66,   90,
    91,   119,  120, 152, 153, 189, 190, 230, 231,  275,  276,  324,  325,  377, 378, 434, 435, 495, 496,  4,    5,
    6,    7,    19,  26,  29,  43,  46,  64,  67,   89,   92,   118,  121,  151, 154, 188, 191, 229, 232,  274,  277,
    323,  326,  376, 379, 433, 436, 494, 497, 558,  8,    9,    10,   11,   25,  30,  42,  47,  63,  68,   88,   93,
    117,  122,  150, 155, 187, 192, 228, 233, 273,  278,  322,  327,  375,  380, 432, 437, 493, 498, 557,  559,  12,
    13,   14,   15,  31,  41,  48,  62,  69,  87,   94,   116,  123,  149,  156, 186, 193, 227, 234, 272,  279,  321,
    328,  374,  381, 431, 438, 492, 499, 556, 560,  617,  16,   20,   24,   32,  40,  49,  61,  70,  86,   95,   115,
    124,  148,  157, 185, 194, 226, 235, 271, 280,  320,  329,  373,  382,  430, 439, 491, 500, 555, 561,  616,  618,
    21,   23,   33,  39,  50,  60,  71,  85,  96,   114,  125,  147,  158,  184, 195, 225, 236, 270, 281,  319,  330,
    372,  383,  429, 440, 490, 501, 554, 562, 615,  619,  672,  22,   34,   38,  51,  59,  72,  84,  97,   113,  126,
    146,  159,  183, 196, 224, 237, 269, 282, 318,  331,  371,  384,  428,  441, 489, 502, 553, 563, 614,  620,  671,
    673,  35,   37,  52,  58,  73,  83,  98,  112,  127,  145,  160,  182,  197, 223, 238, 268, 283, 317,  332,  370,
    385,  427,  442, 488, 503, 552, 564, 613, 621,  670,  674,  723,  36,   53,  57,  74,  82,  99,  111,  128,  144,
    161,  181,  198, 222, 239, 267, 284, 316, 333,  369,  386,  426,  443,  487, 504, 551, 565, 612, 622,  669,  675,
    722,  724,  54,  56,  75,  81,  100, 110, 129,  143,  162,  180,  199,  221, 240, 266, 285, 315, 334,  368,  387,
    425,  444,  486, 505, 550, 566, 611, 623, 668,  676,  721,  725,  770,  55,  76,  80,  101, 109, 130,  142,  163,
    179,  200,  220, 241, 265, 286, 314, 335, 367,  388,  424,  445,  485,  506, 549, 567, 610, 624, 667,  677,  720,
    726,  769,  771, 77,  79,  102, 108, 131, 141,  164,  178,  201,  219,  242, 264, 287, 313, 336, 366,  389,  423,
    446,  484,  507, 548, 568, 609, 625, 666, 678,  719,  727,  768,  772,  813, 78,  103, 107, 132, 140,  165,  177,
    202,  218,  243, 263, 288, 312, 337, 365, 390,  422,  447,  483,  508,  547, 569, 608, 626, 665, 679,  718,  728,
    767,  773,  812, 814, 104, 106, 133, 139, 166,  176,  203,  217,  244,  262, 289, 311, 338, 364, 391,  421,  448,
    482,  509,  546, 570, 607, 627, 664, 680, 717,  729,  766,  774,  811,  815, 852, 105, 134, 138, 167,  175,  204,
    216,  245,  261, 290, 310, 339, 363, 392, 420,  449,  481,  510,  545,  571, 606, 628, 663, 681, 716,  730,  765,
    775,  810,  816, 851, 853, 135, 137, 168, 174,  205,  215,  246,  260,  291, 309, 340, 362, 393, 419,  450,  480,
    511,  544,  572, 605, 629, 662, 682, 715, 731,  764,  776,  809,  817,  850, 854, 887, 136, 169, 173,  206,  214,
    247,  259,  292, 308, 341, 361, 394, 418, 451,  479,  512,  543,  573,  604, 630, 661, 683, 714, 732,  763,  777,
    808,  818,  849, 855, 886, 888, 170, 172, 207,  213,  248,  258,  293,  307, 342, 360, 395, 417, 452,  478,  513,
    542,  574,  603, 631, 660, 684, 713, 733, 762,  778,  807,  819,  848,  856, 885, 889, 918, 171, 208,  212,  249,
    257,  294,  306, 343, 359, 396, 416, 453, 477,  514,  541,  575,  602,  632, 659, 685, 712, 734, 761,  779,  806,
    820,  847,  857, 884, 890, 917, 919, 209, 211,  250,  256,  295,  305,  344, 358, 397, 415, 454, 476,  515,  540,
    576,  601,  633, 658, 686, 711, 735, 760, 780,  805,  821,  846,  858,  883, 891, 916, 920, 945, 210,  251,  255,
    296,  304,  345, 357, 398, 414, 455, 475, 516,  539,  577,  600,  634,  657, 687, 710, 736, 759, 781,  804,  822,
    845,  859,  882, 892, 915, 921, 944, 946, 252,  254,  297,  303,  346,  356, 399, 413, 456, 474, 517,  538,  578,
    599,  635,  656, 688, 709, 737, 758, 782, 803,  823,  844,  860,  881,  893, 914, 922, 943, 947, 968,  253,  298,
    302,  347,  355, 400, 412, 457, 473, 518, 537,  579,  598,  636,  655,  689, 708, 738, 757, 783, 802,  824,  843,
    861,  880,  894, 913, 923, 942, 948, 967, 969,  299,  301,  348,  354,  401, 411, 458, 472, 519, 536,  580,  597,
    637,  654,  690, 707, 739, 756, 784, 801, 825,  842,  862,  879,  895,  912, 924, 941, 949, 966, 970,  987,  300,
    349,  353,  402, 410, 459, 471, 520, 535, 581,  596,  638,  653,  691,  706, 740, 755, 785, 800, 826,  841,  863,
    878,  896,  911, 925, 940, 950, 965, 971, 986,  988,  350,  352,  403,  409, 460, 470, 521, 534, 582,  595,  639,
    652,  692,  705, 741, 754, 786, 799, 827, 840,  864,  877,  897,  910,  926, 939, 951, 964, 972, 985,  989,  1002,
    351,  404,  408, 461, 469, 522, 533, 583, 594,  640,  651,  693,  704,  742, 753, 787, 798, 828, 839,  865,  876,
    898,  909,  927, 938, 952, 963, 973, 984, 990,  1001, 1003, 405,  407,  462, 468, 523, 532, 584, 593,  641,  650,
    694,  703,  743, 752, 788, 797, 829, 838, 866,  875,  899,  908,  928,  937, 953, 962, 974, 983, 991,  1000, 1004,
    1013, 406,  463, 467, 524, 531, 585, 592, 642,  649,  695,  702,  744,  751, 789, 796, 830, 837, 867,  874,  900,
    907,  929,  936, 954, 961, 975, 982, 992, 999,  1005, 1012, 1014, 464,  466, 525, 530, 586, 591, 643,  648,  696,
    701,  745,  750, 790, 795, 831, 836, 868, 873,  901,  906,  930,  935,  955, 960, 976, 981, 993, 998,  1006, 1011,
    1015, 1020, 465, 526, 529, 587, 590, 644, 647,  697,  700,  746,  749,  791, 794, 832, 835, 869, 872,  902,  905,
    931,  934,  956, 959, 977, 980, 994, 997, 1007, 1010, 1016, 1019, 1021, 527, 528, 588, 589, 645, 646,  698,  699,
    747,  748,  792, 793, 833, 834, 870, 871, 903,  904,  932,  933,  957,  958, 978, 979, 995, 996, 1008, 1009, 1017,
    1018, 1022, 1023};

static constexpr size_t kTotalTableSize = kOffset[kNumValidStrategies] * kDCTBlockSize;

void hls_Encode(HybridUint& config, ap_uint<32> value, ap_uint<32>& token, ap_uint<32>& nbits, ap_uint<32>& bits) {
    ap_uint<32> split_exponent = config.split_exponent;
    ap_uint<32> split_token = 1 << split_exponent;
    ap_uint<32> msb_in_token = config.msb_in_token;
    ap_uint<32> lsb_in_token = config.lsb_in_token;

    // if(split_exponent >= msb_in_token + lsb_in_token)
    //   std::cout << "split_exponent" << "\n";
    if (value < split_token) {
        token = value;
        nbits = 0;
        bits = 0;
    } else {
        ap_uint<32> n = 32 - value.countLeadingZeros() - 1; // FloorLog2Nonzero(value);
        ap_uint<32> m = value - (1 << n);
        token = split_token + ((n - split_exponent) << (msb_in_token + lsb_in_token)) +
                ((m >> (n - msb_in_token)) << lsb_in_token) + (m & ((1 << lsb_in_token) - 1));
        nbits = n - msb_in_token - lsb_in_token;
        bits = (value >> lsb_in_token) & ((1UL << nbits) - 1);
    }
}

ap_uint<32> hls_CoeffOrderContext(ap_uint<32> val) {
    ap_uint<32> token, nbits, bits;
    ap_uint<32> mins = 7;
    HybridUint config(0, 0, 0);
    hls_Encode(config, val, token, nbits, bits);
    return (token < mins) ? token : mins;
}

void scanStrategy(uint16_t used_orders,
                  ap_uint<32>& num_strategy,
                  hls::stream<ap_uint<8> >& strategyStrm,
                  hls::stream<ap_uint<32> > skipStrm[3]) {
#pragma HLS INLINE off
    uint16_t computed = 0;
    ap_uint<32> order;
SCAN_STRATEGY:
    for (ap_uint<3> o = 0; o < 6; o++) {
#pragma HLS PIPELINE II = 1
        ap_uint<8> order = kStrategyOrder[o];
        if (!(computed & (1 << order))) {
            computed |= 1 << order;
            if ((used_orders & (1 << order)) != 0) {
                num_strategy++;
                strategyStrm.write(o);
                if (o == Type::DCT) {
                    skipStrm[0].write(1);
                    skipStrm[1].write(1);
                    skipStrm[2].write(1);
                } else if (o == Type::IDENTITY) {
                    skipStrm[0].write(1);
                    skipStrm[1].write(1);
                    skipStrm[2].write(1);
                } else if (o == Type::DCT2X2) {
                    skipStrm[0].write(1);
                    skipStrm[1].write(1);
                    skipStrm[2].write(1);
                } else if (o == Type::DCT4X4) {
                    skipStrm[0].write(1);
                    skipStrm[1].write(1);
                    skipStrm[2].write(1);
                } else if (o == Type::DCT16X16) {
                    skipStrm[0].write(4);
                    skipStrm[1].write(4);
                    skipStrm[2].write(4);
                } else if (o == Type::DCT32X32) {
                    skipStrm[0].write(16);
                    skipStrm[1].write(16);
                    skipStrm[2].write(16);
                }
            }
        }
    }
}

void loadZigzag(ap_uint<32>& num_strategy,
                hls::stream<ap_uint<8> >& strategyStrm,
                hls::stream<ap_uint<32> >& skipStrm,
                hls::stream<ap_uint<32> >& orderStrm,
                hls::stream<ap_uint<32> >& zigzagStrm) {
#pragma HLS INLINE off

    ap_uint<32> skip;
    ap_uint<32> order;
    ap_uint<32> zigzag;
    ap_uint<32> kOffset;
    ap_uint<8> strategy;
    for (ap_uint<8> o = 0; o < num_strategy; o++) {
        strategyStrm.read(strategy);
        if (strategy == Type::DCT) {
            kOffset = 0;
        } else if (strategy == Type::IDENTITY) {
            kOffset = 64;
        } else if (strategy == Type::DCT2X2) {
            kOffset = 128;
        } else if (strategy == Type::DCT4X4) {
            kOffset = 192;
        } else if (strategy == Type::DCT16X16) {
            kOffset = 256;
        } else if (strategy == Type::DCT32X32) {
            kOffset = 512;
        } else {
            std::cerr << "The strategy is invaild!" << std::endl;
        }
        skipStrm.read(skip);
        ap_uint<32> size = skip << 6;
    CHANNELS:
        for (ap_uint<2> c = 0; c < 3; c++) {
        ZIGZAG_LOOP:
            for (ap_uint<32> i = 0; i < size; i++) {
#pragma HLS PIPELINE II = 1
                orderStrm.read(order);
                zigzag = coeffOrderLut[kOffset + order];
                zigzagStrm.write(zigzag);
            }
        }
    }
}

void initTemp(ap_uint<16> temp[LENGTH + 1]) {
#pragma HLS INLINE off
INIT_TEMP_PONG:
    for (ap_uint<32> i = 0; i < LENGTH + 1; i++) {
#pragma HLS PIPELINE II = 1
        temp[i] = 0;
    }
}

void lehmerCore(ap_uint<32> skip,
                ap_uint<32> size,
                ap_uint<16> temp[LENGTH + 1],
                ap_uint<32>& end,
                hls::stream<ap_uint<32> >& zigzagStrm,
                hls::stream<ap_uint<32> >& lehmerStrm) {
#pragma HLS INLINE off
    ap_uint<32> lehmer; // [LENGTH];
    for (ap_uint<32> idx = 0; idx < size; idx++) {
        ap_uint<32> penalty = 0;
        ap_uint<32> s = zigzagStrm.read();
        uint32_t i = s + 1;
    LEHMER_LOOP0:
        while (i != 0) {
#pragma HLS PIPELINE II = 1
            penalty += temp[i];
            i &= i - 1;
        }
        // if(s >= penalty)
        //   std::cout << "Warning: s>=penalty!" << std::endl;
        lehmer = s - penalty;
        if (lehmer != 0) end = idx;
        if (idx >= skip) lehmerStrm.write(lehmer);
        i = s + 1;
    LEHMER_LOOP1:
        while (i < size + 1) {
#pragma HLS PIPELINE II = 1
            temp[i] += 1;
            i += (i & -i);
        }
    }
}

void updateCore(ap_uint<32> skip,
                ap_uint<32> size,
                ap_uint<16> ping[LENGTH + 1],
                ap_uint<16> pong[LENGTH + 1],
                ap_uint<32>& end,
                hls::stream<ap_uint<32> >& zigzagStrm,
                hls::stream<ap_uint<32> >& lehmerStrm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW
    initTemp(ping);
    lehmerCore(skip, size, pong, end, zigzagStrm, lehmerStrm);
}

void updateLehmer(ap_uint<32> acs,
                  hls::stream<ap_uint<32> >& skipStrm,
                  hls::stream<ap_uint<32> >& endStrm,
                  hls::stream<ap_uint<32> >& zigzagStrm,
                  hls::stream<ap_uint<32> >& lehmerStrm) {
#pragma HLS INLINE off
    ap_uint<1> flag = 0;
    ap_uint<16> temp[2][LENGTH + 1];
#pragma HLS RESOURCE variable = temp core = RAM_2P_BRAM
#pragma HLS ARRAY_PARTITION variable = temp complete dim = 1
INIT_TEMP_PING:
    for (ap_uint<32> i = 0; i < LENGTH + 1; i++) {
#pragma HLS PIPELINE II = 1
        temp[1][i] = 0;
    }
    for (ap_uint<8> o = 0; o < acs; o++) {
        ap_uint<32> skip = skipStrm.read();
        ap_uint<32> size = kDCTBlockSize * skip;
        for (ap_uint<2> c = 0; c < 3; c++) {
            ap_uint<32> end = 0;
            if (flag == 0)
                updateCore(skip, size, temp[0], temp[1], end, zigzagStrm, lehmerStrm);
            else
                updateCore(skip, size, temp[1], temp[0], end, zigzagStrm, lehmerStrm);
            flag++;
            endStrm.write(++end);
        }
    }
}

void updateToken(ap_uint<32> acs,
                 hls::stream<ap_uint<32> >& skipStrm,
                 hls::stream<ap_uint<32> >& endStrm,
                 hls::stream<ap_uint<32> >& lehmerStrm,
                 hls::stream<ap_uint<64> >& tokenStrm,
                 hls::stream<bool>& e_token) {
#pragma HLS INLINE off

    ap_uint<64> token;
    ap_uint<32> lehmer;
    ap_uint<32> skip;
    for (ap_uint<8> o = 0; o < acs; o++) {
        skipStrm.read(skip);
        ap_uint<32> size = kDCTBlockSize * skip;
        for (ap_uint<2> c = 0; c < 3; c++) {
            ap_uint<32> last = 0;
            token.range(31, 0) = hls_CoeffOrderContext(size);
            ap_uint<32> end = endStrm.read();
            token.range(63, 32) = end - skip;
            tokenStrm.write(token);
            e_token.write(false);

        TOKEN_LOOP:
            for (ap_uint<32> i = skip; i < end; ++i) {
#pragma HLS PIPELINE II = 1
                token.range(31, 0) = hls_CoeffOrderContext(last);
                lehmerStrm.read(lehmer);
                token.range(63, 32) = lehmer;
                tokenStrm.write(token);
                e_token.write(false);
                last = lehmer;
            }
        TOKEN_LEFTOVER:
            for (ap_uint<32> i = end; i < size; ++i)
#pragma HLS PIPELINE II = 1
                lehmerStrm.read(lehmer);
        }
    }
    e_token.write(true);
}

} // internal

/**
 * @brief jpegxl order tokenize function.
 *
 * @tparam used_orders the input for complete ac type
 * @tparam orderStrm   the input for image scan order
 * @tparam tokenStrm   the output for tokens initialization
 * @tparam e_tokenStrm the output for tokens stream
 */
void hls_EncodeCoeffOrders(ap_uint<32> used_orders,
                           hls::stream<ap_uint<32> >& orderStrm,
                           hls::stream<ap_uint<64> >& tokenStrm,
                           hls::stream<bool>& e_tokenStrm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    hls::stream<ap_uint<8> > strategyStrm("strategyStrm");

#pragma HLS ARRAY_PARTITION variable = strategyStrm complete dim = 0
#pragma HLS RESOURCE variable = strategyStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = strategyStrm depth = 32

    hls::stream<ap_uint<32> > zigzagStrm("zigzagStrm");

#pragma HLS ARRAY_PARTITION variable = zigzagStrm complete dim = 0
#pragma HLS RESOURCE variable = zigzagStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = zigzagStrm depth = 128

    hls::stream<ap_uint<32> > skipStrm[3];

#pragma HLS ARRAY_PARTITION variable = skipStrm complete dim = 0
#pragma HLS RESOURCE variable = skipStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = skipStrm depth = 32

    hls::stream<ap_uint<32> > endStrm("endStrm");

#pragma HLS ARRAY_PARTITION variable = endStrm complete dim = 0
#pragma HLS RESOURCE variable = endStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = endStrm depth = 32

    hls::stream<ap_uint<32> > lehmerStrm("lehmerStrm");

#pragma HLS ARRAY_PARTITION variable = lehmerStrm complete dim = 0
#pragma HLS RESOURCE variable = lehmerStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = lehmerStrm depth = 1024

    ap_uint<32> num_strategy = 0;
    internal::scanStrategy(used_orders, num_strategy, strategyStrm, skipStrm);
    internal::loadZigzag(num_strategy, strategyStrm, skipStrm[0], orderStrm, zigzagStrm);
    internal::updateLehmer(num_strategy, skipStrm[1], endStrm, zigzagStrm, lehmerStrm);
    internal::updateToken(num_strategy, skipStrm[2], endStrm, lehmerStrm, tokenStrm, e_tokenStrm);
}
} // namespace
} // xf

#endif // _HLS_ENCODE_TOKENS_HPP_
