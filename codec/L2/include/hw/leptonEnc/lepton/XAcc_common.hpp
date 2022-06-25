/*
 * Copyright 2021 Xilinx, Inc.
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
 * @file XAcc_common.hpp
 * @brief lepton common include struct, top of prepare_engine and push_engine function.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XAcc_common.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XACC_COMMON_HPP_
#define _XACC_COMMON_HPP_
#include <ap_int.h>
#include <hls_stream.h>
#include <stdint.h>

enum { NZ_CNT_7x7, NZ_CNT_1x8, NZ_CNT_8x1, NOIS_CNT, NOIS_CNT_DC, THRE_CNT, EXP_CNT, EXP_CNT_X, EXP_CNT_DC, SIGN_CNT };

const static uint8_t hls_raster_to_jpeg_zigzag[64] = {0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42,
                                                      3,  8,  12, 17, 25, 30, 41, 43, 9,  11, 18, 24, 31, 40, 44, 53,
                                                      10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
                                                      21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};

const static uint8_t hls_jpeg_zigzag_to_raster[64] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
                                                      12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
                                                      35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
                                                      58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
const static uint8_t hls_raster_to_aligned[64] = {49, 50, 51, 52, 53, 54, 55, 56, 57, 0,  1,  5,  6,  14, 15, 27,
                                                  58, 2,  4,  7,  13, 16, 26, 28, 59, 3,  8,  12, 17, 25, 29, 38,
                                                  60, 9,  11, 18, 24, 30, 37, 39, 61, 10, 19, 23, 31, 36, 40, 45,
                                                  62, 20, 22, 32, 35, 41, 44, 46, 63, 21, 33, 34, 42, 43, 47, 48};

const static uint8_t hls_zigzag_to_aligned[64] = {49, 50, 57, 58, 0,  51, 52, 1,  2,  59, 60, 3,  4,  5,  53, 54,
                                                  6,  7,  8,  9,  61, 62, 10, 11, 12, 13, 14, 55, 56, 15, 16, 17,
                                                  18, 19, 20, 63, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
                                                  33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48};

#define _MACRO_MIN(a, b) ((a > b) ? (b) : (a))
#define _MACRO_ABS(a) ((a > 0) ? (a) : (-a))

typedef int16_t coef_t;
struct coeff_64 {
    coef_t data[64];
    coeff_64() { ; };
    coeff_64(int16_t* psrc) {
        for (int i = 0; i < 64; i++) data[i] = psrc[i];
    };
    void ExportTo(int16_t* pdes) {
        for (int i = 0; i < 64; i++) pdes[i] = data[i];
    };
};
struct coeff_77_t {
    coef_t data[49];
};

struct coeff_edge_t {
    coef_t data[8];
};

typedef uint8_t pix_t;
typedef int16_t epix_t;
struct pix_edge_t {
    epix_t data[8];
};

#define MAX_EXPONENT_PIX (11)
#define MAX_NUM_COLOR (3)
#define MAX_NUM_BLOCK88_W (1024)
#define MAX_NUM_BLOCK88_H (1024)
#define MAX_NUM_BLOCK88 (MAX_NUM_BLOCK88_W * MAX_NUM_BLOCK88_H)
#define MAX_PIX_W (MAX_NUM_BLOCK88_W << 3)
#define MAX_PIX_H (MAX_NUM_BLOCK88_H << 3)
#define MAX_NUM_PIX (MAX_NUM_BLOCK88 * MAX_NUM_COLOR * 64)
#define MAX_NUM_COEF (MAX_NUM_BLOCK88 * MAX_NUM_COLOR * 64)
#define MAX_SIZE_COEF (MAX_NUM_COEF * 2)

#define BITS_DDR (64)
#define SCAL_AXI (1)
#define BITS_AXI (BITS_DDR * SCAL_AXI) // Must be (WD_DDR*SCAL_AXI)
#define AXI_WIDTH (16)

#define DIV_FOR_WD_AXI (SCAL_AXI)
#define NUM_COEF_AXI (BITS_AXI / 16)
#define MAX_COEF_AXI (MAX_NUM_COEF / NUM_COEF_AXI)
#define STRIP_COEFF_AXI (64 / NUM_COEF_AXI)
struct WD_AXI {
    coef_t data[NUM_COEF_AXI]; // 4
};

#define MAX_NUM_AXI (MAX_NUM_COEF / NUM_COEF_AXI)

struct hls_Branch {
   public:
    uint8_t counts_[2];
    // uint8_t probability_;
   public:
    // uint8_t prob() const { return probability_; }
    void set_identity() {
        counts_[0] = 1;
        counts_[1] = 1;
        //  probability_ = 128;
    }
    // bool is_identity() const {
    //  return counts_[0] == 1 && counts_[1] == 1 && probability_ == 128;
    //}
    static hls_Branch identity() {
        hls_Branch retval;
        retval.set_identity();
        return retval;
    }
    uint32_t true_count() const { return counts_[1]; }
    uint32_t false_count() const { return counts_[0]; }
    struct ProbUpdate {
        struct ProbOutcome {
            uint8_t log_prob;
        };
        uint8_t prob;
        ProbOutcome next[2];
        uint8_t& log_prob_false() { return next[0].log_prob; }
        uint8_t& log_prob_true() { return next[1].log_prob; }
    };

    void record_obs_and_update(bool obs) {
        bool isFull = counts_[obs] == 0xff;
        bool isOne = counts_[!obs] == 1;
        if (!isFull)
            counts_[obs]++;
        else if (isOne) {
            counts_[obs] = 0xff;
        } else {
            counts_[obs] = 129;
            counts_[1 - obs] = ((1 + counts_[1 - obs]) >> 1);
        }
    }
    void record_obs_and_update_almost_org(bool obs) {
        // unsigned int fcount = counts_[0];
        //  unsigned int tcount = counts_[1];
        bool overflow = (counts_[obs]++ == 0xff);
        if (overflow) { // check less than 512
            bool neverseen = counts_[!obs] == 1;
            if (neverseen) {
                counts_[obs] = 0xff;
                //    probability_ = obs ? 0 : 255;
            } else {
                // counts_[0] = ((1 + (unsigned int)fcount) >> 1);
                //  counts_[1] = ((1 + (unsigned int)tcount) >> 1);
                counts_[1 - obs] = ((1 + (unsigned int)counts_[1 - obs]) >> 1);
                counts_[obs] = 129;
                //     probability_ = optimize(counts_[0] + counts_[1]);
            }
        } else {
            //   probability_ = optimize(counts_[0] + counts_[1]);
        }
    }
    uint8_t prob2() {
        unsigned int fcount = counts_[0];
        unsigned int tcount = counts_[1];
        if (fcount == 1 && tcount == 255) return 0;
        if (fcount == 255 && tcount == 1) return 255;
        uint8_t normal = optimize();
        return normal;
    }
    void normalize() {
        counts_[0] = ((1 + (unsigned int)counts_[0]) >> 1);
        counts_[1] = ((1 + (unsigned int)counts_[1]) >> 1);
    }

    uint8_t optimize() const {
        uint16_t cnt_lsb = counts_[0];
        uint16_t sum = counts_[0] + counts_[1];
        const int prob = (cnt_lsb << 8) / sum; // fast_divide18bit_by_10bit(cnt_lsb << 8, sum);
        return (uint8_t)prob;
    }

    hls_Branch() {}
};
struct tmp_struct {
    bool value;
    hls_Branch* branch;
    int bill;
};

// template <class T>
// static void hls_cp8(T des[8], T src[8])
//{
//#pragma HLS inline
//#pragma HLS PIPELINE
//	INNER_LOOP:
//	for(int i=0;i<8;i++)
//		des[i] = src[i];
//}

struct struct_arith {
    int count;
    unsigned int value;
    unsigned char pre_byte;
    unsigned short run;
    unsigned int pos;
    unsigned char range;
    bool isFirst;
};

enum COLOR_FORMAT { C400 = 0, C420, C422, C444 };

struct decOutput {
    COLOR_FORMAT format;
    uint16_t axi_width[MAX_NUM_COLOR];  // colldata->block_width(i);
    uint16_t axi_height[MAX_NUM_COLOR]; // colldata->block_width(i);
    uint8_t axi_map_row2cmp[4];         //     AXI                   2,1,0,0 2,1,0
    uint8_t min_nois_thld_x[MAX_NUM_COLOR][64];
    uint8_t min_nois_thld_y[MAX_NUM_COLOR][64];
    uint8_t q_tables[MAX_NUM_COLOR][8][8]; //[64],
    int32_t idct_q_table_x[MAX_NUM_COLOR][8][8];
    int32_t idct_q_table_y[MAX_NUM_COLOR][8][8];
    int32_t idct_q_table_l[MAX_NUM_COLOR][8][8];

    uint16_t axi_mcuv;
    uint8_t axi_num_cmp_mcu;
    uint8_t axi_num_cmp;
};

const short hls_icos_base_8192_scaled[64] = {
    8192,   8192,  8192,   8192,  8192, 8192,  8192,   8192,   11363, 9633,  6436,   2260,  -2260,
    -6436,  -9633, -11363, 10703, 4433, -4433, -10703, -10703, -4433, 4433,  10703,  9633,  -2260,
    -11363, -6436, 6436,   11363, 2260, -9633, 8192,   -8192,  -8192, 8192,  8192,   -8192, -8192,
    8192,   6436,  -11363, 2260,  9633, -9633, -2260,  11363,  -6436, 4433,  -10703, 10703, -4433,
    -4433,  10703, -10703, 4433,  2260, -6436, 9633,   -11363, 11363, -9633, 6436,   -2260,
};

const short hls_icos_idct_linear_8192_scaled[64] = {
    1024, 1420,  1338,  1204,  1024,  805,   554,   283,  1024, 1204,  554,   -283,  -1024, -1420, -1338, -805,
    1024, 805,   -554,  -1420, -1024, 283,   1338,  1204, 1024, 283,   -1338, -805,  1024,  1204,  -554,  -1420,
    1024, -283,  -1338, 805,   1024,  -1204, -554,  1420, 1024, -805,  -554,  1420,  -1024, -283,  1338,  -1204,
    1024, -1204, 554,   283,   -1024, 1420,  -1338, 805,  1024, -1420, 1338,  -1204, 1024,  -805,  554,   -283,
};
#if 0
/////template PackStr2Mem_t////////////////////////
template < int W_STR,int B_LAST, int N_BURST>
int PackStr2Mem_t(
    uint8_t* pdes,
	struct_arith    axi_arith,
    hls::stream<ap_uint<W_STR> > &str_s)
{
    const int N_BYTE = ((W_STR-1+7)/8);
    const int N_PACK = (4/N_BYTE);
    uint8_t* ptmp = pdes;//uint32_t
    int num_w = 0;
    ap_uint<1> isLast = 0;
    uint8_t buff[512];//uint32_t
#ifndef __SYNTHESIS__
    assert(N_BURST<=512);
#endif
    ap_uint<N_BYTE * 8 * N_PACK> tmp;
    //hls::stream<ap_uint<1>> str_last_buff;
    do {
        for (int i = 0; i < N_BURST * N_PACK; i++) {
#pragma HLS PIPELINE II = 1
            ap_uint<2> bs = i % N_PACK;
            if (isLast == 0) {
                ap_uint<W_STR> w = str_s.read();
                isLast = w(W_STR-1, W_STR-1);
                tmp( bs*N_BYTE*8+W_STR-2 , bs*N_BYTE*8) = w(W_STR-2, 0);
                num_w++;
            } else
                tmp( bs*N_BYTE*8+W_STR-2 , bs*N_BYTE*8 ) = 0;
            if (bs == N_PACK - 1) {
                buff[i / N_PACK] = tmp;
                //str_last_buff.write(isLast);
            }
        }
        //memcpy((void*)ptmp, (void*)buff, N_BURST*N_PACK);
        for (int j = 0; j < N_BURST; j++)
#pragma HLS PIPELINE II = 1
            ptmp[j] = buff[j];
        ptmp += N_BURST;
    } while (isLast == 0);

    pdes[axi_arith.pos++] = axi_arith.pre_byte;
	for(; axi_arith.run > 0; axi_arith.run--)
		pdes[axi_arith.pos++] = 0xff;

    return num_w;
}

/////template PackStr2Mem_t////////////////////////
template < int W_STR,int B_LAST, int N_BURST>
int PackStr2Mem_t(
    uint8_t* pdes,
    hls::stream<ap_uint<W_STR> > &str_s)
{
    const int N_BYTE = ((W_STR-1+7)/8);
    const int N_PACK = (4/N_BYTE);
    uint8_t* ptmp = pdes;//uint32_t
    int num_w = 0;
    ap_uint<1> isLast = 0;
    uint8_t buff[512];//uint32_t
#ifndef __SYNTHESIS__
    assert(N_BURST<=512);
#endif
    ap_uint<N_BYTE * 8 * N_PACK> tmp;
    //hls::stream<ap_uint<1>> str_last_buff;
    do {
        for (int i = 0; i < N_BURST * N_PACK; i++) {
#pragma HLS PIPELINE II = 1
            ap_uint<2> bs = i % N_PACK;
            if (isLast == 0) {
                ap_uint<W_STR> w = str_s.read();
                isLast = w(W_STR-1, W_STR-1);
                tmp( bs*N_BYTE*8+W_STR-2 , bs*N_BYTE*8) = w(W_STR-2, 0);
                num_w++;
            } else
                tmp( bs*N_BYTE*8+W_STR-2 , bs*N_BYTE*8 ) = 0;
            if (bs == N_PACK - 1) {
                buff[i / N_PACK] = tmp;
                //str_last_buff.write(isLast);
            }
        }
        //memcpy((void*)ptmp, (void*)buff, N_BURST*N_PACK);
        for (int j = 0; j < N_BURST; j++)
#pragma HLS PIPELINE II = 1
            ptmp[j] = buff[j];
        ptmp += N_BURST;
    } while (isLast == 0);

    return num_w;
}
#endif

void kernel_LeptonE_strmIn(
    // input
    hls::stream<ap_int<11> > coef[8],

    uint16_t axi_width[MAX_NUM_COLOR], // colldata->block_width(i);
    uint8_t axi_map_row2cmp[4],        //     AXI                   2,1,0,0 2,1,0
    uint8_t min_nois_thld_x[MAX_NUM_COLOR][64],
    uint8_t min_nois_thld_y[MAX_NUM_COLOR][64],
    uint8_t q_tables[MAX_NUM_COLOR][8][8], //[64],
    int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],
    int32_t idct_q_table_y[MAX_NUM_COLOR][8][8],

    uint16_t axi_mcuv,
    uint8_t axi_num_cmp_mcu,

    // output
    struct_arith& axi_arith,
    hls::stream<bool>& strm_pos_o_e,
    hls::stream<unsigned char>& strm_pos_o_byte

    );

namespace xf {
namespace codec {
namespace details {

void kernel_LeptonE_strmIn_engine(
    // input
    hls::stream<ap_int<11> > coef[8],

    uint16_t axi_width[MAX_NUM_COLOR], // colldata->block_width(i);
    uint8_t axi_map_row2cmp[4],        //     AXI                   2,1,0,0 2,1,0
    uint8_t min_nois_thld_x[MAX_NUM_COLOR][64],
    uint8_t min_nois_thld_y[MAX_NUM_COLOR][64],
    uint8_t q_tables[MAX_NUM_COLOR][8][8], //[64],
    int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],
    int32_t idct_q_table_y[MAX_NUM_COLOR][8][8],

    uint16_t axi_mcuv,
    uint8_t axi_num_cmp_mcu,

    // output
    struct_arith& axi_arith,
    hls::stream<bool>& strm_pos_o_e,
    hls::stream<ap_uint<8> >& strm_pos_o_byte);

} // namespace details
} // namespace codec
} // namespace xf
#endif
