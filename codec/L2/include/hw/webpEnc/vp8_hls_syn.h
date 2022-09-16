/**********
           Copyright (c) 2017, Xilinx, Inc.
           All rights reserved.
           Redistribution and use in source and binary forms, with or without modification,
           are permitted provided that the following conditions are met:
           1. Redistributions of source code must retain the above copyright notice,
           this list of conditions and the following disclaimer.
           2. Redistributions in binary form must reproduce the above copyright notice,
           this list of conditions and the following disclaimer in the documentation
           and/or other materials provided with the distribution.
           3. Neither the name of the copyright holder nor the names of its contributors
           may be used to endorse or promote products derived from this software
           without specific prior written permission.
           THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
           ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
           THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
           IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
           INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
           PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
           HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
           OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
           EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#ifndef _VP8_HLS_SYN_H_
#define _VP8_HLS_SYN_H_

#define _KEEP_PSNR_

#include <hls_stream.h>
#include <ap_int.h>

#define WD_PIX (8)
#define WD_MODE (4)
#define WD_SUB (WD_PIX + 1)
#define WD_DCT (12)
#define WD_WHT (15)
#define WD_IWHT (16)
#define WD_Q (WD_DCT)
#define WD_IQ (WD_DCT)
#define WD_IQT (WD_DCT)
#define WD_IDCT (WD_DCT)
#define WD_SSE4 (8 + 8 + 4)
#define WD_SSE16 (WD_SSE4 + 4)
#define WD_TTW (6)
#define WD_TTR (WD_TTW + WD_PIX + 3 + 1 + 1 - 1 + 4)
#define WD_DISTO (WD_TTR + 1 - 1 - 5)
#define MAX_WIDTH (65536 * 2)
#define MAX_HEIGHT (65536 * 2)
#define WD_sharpen (MY_SHARPEN_BITS)
#define WD_QT (32)
#define WD_q (7)
#define WD_iq (16)
#define WD_bias (32)
#define WD_zthresh (32)
#define WD_MLEVEL (32)
#define WD_LEVEL (12)
#define WD_LMD (12)
#define WD_FAST (WD_LEVEL + 4)
#define MAX_W_PIX (4096)
#define MAX_H_PIX (4096)
#define LG2_MAX_W_PIX (12)
#define LG2_MAX_H_PIX (12)
#define MAX_NUM_MB_W ((MAX_W_PIX + 15) / 16)
#define MAX_NUM_MB_H ((MAX_H_PIX + 15) / 16)
#define MAX_NUM_MB (MAX_NUM_MB_W * MAX_NUM_MB_H)
#define LG2_MAX_NUM_MB_W (LG2_MAX_W_PIX - 4)
#define LG2_MAX_NUM_MB_H (LG2_MAX_H_PIX - 4)
#define SIZE_P_INFO (256 * 4)
#define SIZE_P_YSRC (MAX_NUM_MB * 256 * 1)
#define SIZE_P_USRC (MAX_NUM_MB * 64 * 1)
#define SIZE_P_VSRC (MAX_NUM_MB * 64 * 1)
#define SIZE_P_OUT (MAX_NUM_MB * 512 * 2)
#define WD_RD_SCORE (40)

#define MAX_DEEP_LOOP (20) // clock cycles for PRLoop;

#define NUM_BURST_READ (64)
#define LG2_NUM_BURST_READ (6)

#define OFF_NUM_MB_32 (1056 / 4)
#define OFF_PID_PROB_8BIT (1200)

typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;
#define SB_GET(sb, line, col, wd) (sb(wd - 1 + ((line)*4 + (col)) * (wd), ((line)*4 + (col)) * (wd)))
#define VCT_GET(vect, mi, wd) (vect(wd - 1 + (mi) * (wd), (mi) * (wd)))
#define VCT_SET_VAL(vect, mi, wd, val) (vect(wd - 1 + (mi) * (wd), (mi) * (wd)) = val)
#define SB_SET_VAL(sb, line, col, wd, val) SB_GET(sb, line, col, wd) = val
#define SB_SET_COL_VAL(sb, col, wd, val) \
    SB_SET_VAL(sb, 0, col, wd, val);     \
    SB_SET_VAL(sb, 1, col, wd, val);     \
    SB_SET_VAL(sb, 2, col, wd, val);     \
    SB_SET_VAL(sb, 3, col, wd, val)
#define SB_SET(sb, line, col, wd, val) (sb(wd - 1 + ((line)*4 + (col)) * (wd), ((line)*4 + (col)) * (wd)) = val)

#define VCT_SET_COL_SB(sb, col, wd, vect)                                                          \
    (VCT_GET(vect, 0, wd) = SB_GET(sb, 0, col, wd); VCT_GET(vect, 1, wd) = SB_GET(sb, 1, col, wd); \
     VCT_GET(vect, 2, wd) = SB_GET(sb, 2, col, wd); VCT_GET(vect, 3, wd) = SB_GET(sb, 3, col, wd))

#define SB_SET_COL_VCT(sb, col, wd, vect)                                                                \
    (SB_SET_VAL(sb, 0, col, wd, VCT_GET(vect, 0, wd)); SB_SET_VAL(sb, 1, col, wd, VCT_GET(vect, 1, wd)); \
     SB_SET_VAL(sb, 2, col, wd, VCT_GET(vect, 2, wd)); SB_SET_VAL(sb, 3, col, wd, VCT_GET(vect, 3, wd)))

#define SB_SET_COL_SB(sb, col, wd, sbs)                                                                        \
    (SB_SET_VAL(sb, 0, col, wd, SB_GET(sbs, 0, col, wd)); SB_SET_VAL(sb, 1, col, wd, SB_GET(sbs, 1, col, wd)); \
     SB_SET_VAL(sb, 2, col, wd, SB_GET(sbs, 2, col, wd)); SB_SET_VAL(sb, 3, col, wd, SB_GET(sbs, 3, col, wd)))
#define VCT_SET_VAL_ALL(vect, mii, wd, val) \
    for (int mi = 0; mi < mii; mi++) {      \
        VCT_SET_VAL(vect, mi, wd, val);     \
    }
#define VCT4_SET_LINE_SAME_VAL(vector, wd, val) (SB_SET_LINE_VAL((vector), (0), (wd), (val)))
#define SB_SET_LINE_VAL(sb, line, wd, val) \
    SB_SET_VAL(sb, line, 0, wd, val);      \
    SB_SET_VAL(sb, line, 1, wd, val);      \
    SB_SET_VAL(sb, line, 2, wd, val);      \
    SB_SET_VAL(sb, line, 3, wd, val)

#define SB_SET_LINE_VCT(sb, line, wd, val) sb(((line)*4 + 1) * (wd)-1, (line) * (wd)*4) = val((wd)*4 - 1, 0)
#define SB_SET_LINE_SB(sb, line, wd, sbs) \
    sb(((line)*4 + 1) * (wd)-1, (line) * (wd)*4) = sbs(((line)*4 + 1) * (wd)-1, (line) * (wd)*4)

#define AP_DST(sb, x, y, wd) SB_GET(sb, y, x, wd)
#define AP_AVG3(a, b, c, wd) (((a) + (ap_uint<wd + 2>(b) << 1) + (c) + 2) >> 2)
#define AP_AVG2(a, b, wd) ((ap_uint<wd + 1>(a) + ap_uint<wd + 1>(b) + 1) >> 1)

#define AP_TREEADD2(v0, v1, wd) ((ap_uint<wd + 1>(v0)) + (ap_uint<wd + 1>(v1)))
#define AP_TREEADD4(v0, v1, v2, v3, wd) \
    ((ap_uint<wd + 2> AP_TREEADD2(v0, v1, wd)) + (ap_uint<wd + 2> AP_TREEADD2(v2, v3, wd)))
#define AP_TREEADD4_VCT(vct4, wd) \
    (AP_TREEADD4((VCT_GET(vct4, 0, wd)), (VCT_GET(vct4, 1, wd)), (VCT_GET(vct4, 2, wd)), (VCT_GET(vct4, 3, wd)), wd))

#define A44 (VCT_GET(abcd, 0, WD_PIX))
#define B44 (VCT_GET(abcd, 1, WD_PIX))
#define C44 (VCT_GET(abcd, 2, WD_PIX))
#define D44 (VCT_GET(abcd, 3, WD_PIX))

#define E44 (VCT_GET(efgh, 0, WD_PIX))
#define F44 (VCT_GET(efgh, 1, WD_PIX))
#define G44 (VCT_GET(efgh, 2, WD_PIX))
#define H44 (VCT_GET(efgh, 3, WD_PIX))

#define I44 (VCT_GET(ijkl, 0, WD_PIX))
#define J44 (VCT_GET(ijkl, 1, WD_PIX))
#define K44 (VCT_GET(ijkl, 2, WD_PIX))
#define L44 (VCT_GET(ijkl, 3, WD_PIX))

#define BPS 32 // this is the common stride for enc/dec
#define DST(x, y) dst[(x) + (y)*BPS]
#define AVG3(a, b, c) (((a) + 2 * (b) + (c) + 2) >> 2)
#define AVG2(a, b) (((a) + (b) + 1) >> 1)
// Get pix position from Block numb, Sub-block number and pix number in sub-block
#define SB2MB_X(Bn, Sbn) ((Bn)&2 + (Sbn) / 2)
#define SB2MB_Y(Bn, Sbn) (((Bn) % 2) * 2 + (Sbn) % 2)
#define SB2MB_XP(Bn, Sbn, pos) ((SB2MB_H(Bn, Sbn)) * 4 + (pos) % 4)
#define SB2MB_YP(Bn, Sbn, pos) ((SB2MB_V(Bn, Sbn)) * 4 + (pos) / 4)
// From pix position of MB, get Block numb, Sub-block number, and pix number in sub-block;
#define NUM_B2SB(bx, by) ((by / 4) * 2 + (bx) / 4)
#define NUM_MB2B(mx, my) (((my) / 8) * 2 + (mx) / 8)
#define NUM_MB2SB(mx, my) (NUM_B2SB((mx % 8), (my % 8)))
#define NUM_MB2PIX(mx, my) ((my % 4) * 4 + (mx % 4))
#define NUM_B2PIX(bx, by) ((by % 4) * 4 + (bx % 4))
#define NUM_Bn(mbn) (((mbn / 4) & 2) + (((mbn % 4) & 2) / 2))
#define NUM_Sn(mbn) ((((mbn / 4) & 1) * 2) + ((mbn % 4) & 1))
#define NUM_MBn2SBn(mbn) (NUM_Bn(mbn) * 4 + NUM_Sn(mbn))
#define NUM_MBSB(n_sb) \
    (n_sb) //((n_sb==2||n_sb==3||n_sb==10||n_sb==11)?(n_sb+2):(n_sb==4||n_sb==5||n_sb==12||n_sb==13)?(n_sb-2):n_sb)
typedef unsigned int uint32_t;
#define MY_MAX_LEVEL (2047)
#define MY_SHARPEN_BITS (11)
#define ZIGZAG(k)                                                                          \
    k == 2 ? 4                                                                             \
           : k == 3 ? 8                                                                    \
                    : k == 4 ? 5                                                           \
                             : k == 5 ? 2                                                  \
                                      : k == 6 ? 3                                         \
                                               : k == 7 ? 6                                \
                                                        : k == 8 ? 9                       \
                                                                 : k == 9 ? 12             \
                                                                          : k == 10        \
                                                                                ? 13       \
                                                                                : k == 11  \
                                                                                      ? 10 \
                                                                                      : k == 12 ? 7 : k == 13 ? 11 : k
#define CPY16(vc, ap, wd) \
    for (int i = 0; i < 16; i++) vc[i] = (ap_int<wd>)VCT_GET(ap, i, wd)
#define CPY16U(vc, ap, wd) \
    for (int i = 0; i < 16; i++) vc[i] = (ap_uint<wd>)VCT_GET(ap, i, wd)
#define SET16(vc, ap, wd) \
    for (int i = 0; i < 16; i++) VCT_GET(ap, i, wd) = vc[i]

typedef unsigned short int uint16_t;
#define SIZE32_MEM_INFO (256)
#define SIZE32_MEM_YSRC (MAX_NUM_MB_W * MAX_NUM_MB_H * 256 / 4)
#define SIZE32_MEM_UVSRC (MAX_NUM_MB_W * MAX_NUM_MB_H * 64 / 4)
#define SIZE32_MEM_UVSRC (MAX_NUM_MB_W * MAX_NUM_MB_H * 64 / 4)
#define SIZE32_MEM_LEVEL (MAX_NUM_MB_W * MAX_NUM_MB_H * 512 / 2)
#define SIZE32_AC_STATE (8)
#define SYSTEM_MIN_COMP_RATIO (4)
#define SYSTEM_MAX_COMP_BPP (12 / SYSTEM_MIN_COMP_RATIO) // 3
#define WD_BUS_BYTE (4)
#define SIZE32_MEM_BW (MAX_NUM_MB_W * MAX_NUM_MB_H * 384 / SYSTEM_MIN_COMP_RATIO / WD_BUS_BYTE)
#define SIZE32_MEM_RET (MAX_NUM_MB_W * MAX_NUM_MB_H * 1 / WD_BUS_BYTE)
#define SIZE32_MEM_PRED (MAX_NUM_MB_W * MAX_NUM_MB_H * 8 / WD_BUS_BYTE)
#define SIZE8_MEM_PROB 2048 // instead of (4*8*11*3)
#define SIZE8_MEM_BW (SIZE32_MEM_BW * 4)
#define SIZE8_MEM_RET (SIZE32_MEM_RET * 4)
#define SIZE8_MEM_PRED (SIZE32_MEM_PRED * 4)

#define TOKEN_ID2(t, b) (((b) << 5) + (b) + ((((t) << 5) + (t)) << 3))

struct hls_QMatrix {
    ap_uint<WD_q> q_0; // quantizer steps
    ap_uint<WD_q> q_n;
    ap_uint<WD_iq> iq_0; // reciprocals, fixed point.
    ap_uint<WD_iq> iq_n;
    ap_uint<WD_bias> bias_0; // rounding bias
    ap_uint<WD_bias> bias_n;
};

typedef struct {
    ap_uint<WD_SSE4 + 4> d;     // 24bit
    ap_uint<WD_DISTO + 4> sd;   // 21b   // distortion, spectral distortion
    ap_uint<WD_FAST + 1 + 4> r; // 21=15+4+1
    ap_uint<12> h;
    ap_uint<25> nz;
    ap_uint<WD_RD_SCORE + 4> score;
    void init() {
        d = 0;
        sd = 0;
        r = 0;
        h = 0;
        nz = 0;
    };
    void ca_score(ap_uint<WD_LMD> lmbda) {
        score = (((ap_uint<WD_RD_SCORE + 4>)(d + (ap_uint<WD_SSE4 + 4>)sd)) << 8) +
                ((ap_uint<WD_RD_SCORE + 4>)(r + h)) * lmbda;
    };
} str_dis;

typedef struct {
    ap_uint<25> nz;
    ap_uint<WD_RD_SCORE + 4> score; //[16];
    ap_uint<4> mode;
    void init() {
        nz = 0;
        mode = 15;
        score = -1;
    };
    void ca_score(ap_uint<WD_LMD> lmbda, str_dis* dis, ap_uint<4> m) {
        nz = dis->nz;
        score = (((ap_uint<WD_RD_SCORE + 4>)(dis->d + (ap_uint<WD_SSE4 + 4>)(dis->sd))) << 8) +
                ((ap_uint<WD_RD_SCORE + 4>)(dis->r + dis->h)) * lmbda;
        mode = m;
    }; // ca_score
    void ca_score2(ap_uint<WD_LMD> lmbda,
                   ap_uint<WD_SSE4 + 4> d,
                   ap_uint<WD_DISTO + 4> sd,
                   ap_uint<WD_FAST + 1 + 4> r,
                   ap_uint<12> h,
                   ap_uint<4> m) {
        score = (((ap_uint<WD_RD_SCORE + 4>)(d + (ap_uint<WD_SSE4 + 4>)sd)) << 8) +
                ((ap_uint<WD_RD_SCORE + 4>)(r + h)) * lmbda;
        mode = m;
    }; // ca_score

} str_rd;

typedef struct {
    ap_uint<25> nz;
    ap_uint<WD_RD_SCORE + 4> score; //[16];
    ap_uint<WD_MODE> mode[16];
    void init() {
        nz = 0;
        score = 0;
    };
    void acc_rd(str_rd* rd_sb, ap_uint<4> n) {
        nz |= rd_sb->nz;
        score += rd_sb->score;
        mode[n] = rd_sb->mode;
    }
} str_rd_i4;

struct AllPicInfo {
    int id_pic;         // 0
    int cnt_line_mb;    //
    int y_stride;       // = pic->y_stride;
    int uv_stride;      // = pic->uv_stride
    int width;          //      = p_info[4];  // = pic->width
    int height;         //    = p_info[5];  // = pic->height
    int mb_w;           // = enc->mb_w_;//
    int mb_h;           // = enc->mb_h_;
    int seg_lambda_p16; // = dqm->lambda_i16_;
    int seg_lambda_p44; // = dqm->lambda_i4_;
    int seg_tlambda;    // = dqm->tlambda_;
    int seg_lambda_uv;  // = dqm->lambda_uv_;
    int seg_tlambda_m;  // = dqm->lambda_mode_;//10
    int seg_y1_q_0;     // = dqm->lambda_mode_;
    int seg_y1_q_n;     // = dqm->lambda_mode_;
    int seg_y1_iq_0;    // = dqm->lambda_mode_;
    int seg_y1_iq_n;    // = dqm->lambda_mode_;
    int seg_y1_bias_0;  // = dqm->lambda_mode_;
    int seg_y1_bias_n;  // = dqm->lambda_mode_;//16
    int seg_y2_q_0;     // = dqm->lambda_mode_;
    int seg_y2_q_n;     // = dqm->lambda_mode_;
    int seg_y2_iq_0;    // = dqm->lambda_mode_;
    int seg_y2_iq_n;    // = dqm->lambda_mode_;
    int seg_y2_bias_0;  // = dqm->lambda_mode_;
    int seg_y2_bias_n;  // = dqm->lambda_mode_;//22
    int seg_uv_q_0;     // = dqm->lambda_mode_;
    int seg_uv_q_n;     // = dqm->lambda_mode_;
    int seg_uv_iq_0;    // = dqm->lambda_mode_;
    int seg_uv_iq_n;    // = dqm->lambda_mode_;
    int seg_uv_bias_0;  // = dqm->lambda_mode_;
    int seg_uv_bias_n;  // = dqm->lambda_mode_;//28
    int seg_y1_sharpen[16];
    int seg_uv_sharpen[16];
};

struct AllPicInfo_kernel {
    ap_uint<32> id_pic;               // p_info[0];
    ap_uint<32> mb_line;              // p_info[1];
    ap_uint<LG2_MAX_W_PIX> y_stride;  // p_info[2];  // ;//pic->y_stride;
    ap_uint<LG2_MAX_W_PIX> uv_stride; // p_info[3]; // ;//pic->uv_stride
    ap_uint<LG2_MAX_W_PIX> width;     // p_info[4];  // ;//pic->width
    ap_uint<LG2_MAX_W_PIX> height;    // p_info[5];  // ;//pic->height
    ap_uint<LG2_MAX_NUM_MB_W> mb_w;   // p_info[2+2+2];///;
    ap_uint<LG2_MAX_NUM_MB_H> mb_h;   // p_info[3+2+2];//;
    ap_uint<WD_LMD> lambda_p16;       // p_info[4+2+2];//dqm->lambda_i16_;
    ap_uint<WD_LMD> lambda_p44;       // p_info[5+2+2];//dqm->lambda_i4_;
    ap_uint<WD_LMD> tlambda;          // p_info[6+2+2];//dqm->tlambda_;
    ap_uint<WD_LMD> lambda_uv;        // p_info[7+2+2];//dqm->lambda_uv_;
    ap_uint<WD_LMD> tlambda_m;        // p_info[8+2+2];//dqm->lambda_mode_;

    hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
    ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;
    void SetData(int* p_info) {
        id_pic = p_info[0];
        mb_line = p_info[1];
        y_stride = p_info[2];           // = pic->y_stride;
        uv_stride = p_info[3];          // = pic->uv_stride
        width = p_info[4];              // = pic->width
        height = p_info[5];             // = pic->height
        mb_w = p_info[2 + 2 + 2];       ///;
        mb_h = p_info[3 + 2 + 2];       //;
        lambda_p16 = p_info[4 + 2 + 2]; // dqm->lambda_i16_;
        lambda_p44 = p_info[5 + 2 + 2]; // dqm->lambda_i4_;
        tlambda = p_info[6 + 2 + 2];    // dqm->tlambda_;
        lambda_uv = p_info[7 + 2 + 2];  // dqm->lambda_uv_;
        tlambda_m = p_info[8 + 2 + 2];  // dqm->lambda_mode_;

        hls_qm1.q_0 = p_info[11 + 2];    // pm->q_[0];     // quantizer steps
        hls_qm1.q_n = p_info[12 + 2];    // pm->q_[1];
        hls_qm1.iq_0 = p_info[13 + 2];   // pm->iq_[0];    // reciprocals fixed point.
        hls_qm1.iq_n = p_info[14 + 2];   // pm->iq_[1];
        hls_qm1.bias_0 = p_info[15 + 2]; // pm->bias_[0];  // rounding bias
        hls_qm1.bias_n = p_info[16 + 2]; // pm->bias_[1];

        hls_qm2.q_0 = p_info[17 + 2];    // pm->q_[0];     // quantizer steps
        hls_qm2.q_n = p_info[18 + 2];    // pm->q_[1];
        hls_qm2.iq_0 = p_info[19 + 2];   // pm->iq_[0];    // reciprocals fixed point.
        hls_qm2.iq_n = p_info[20 + 2];   // pm->iq_[1];
        hls_qm2.bias_0 = p_info[21 + 2]; // pm->bias_[0];  // rounding bias
        hls_qm2.bias_n = p_info[22 + 2]; // pm->bias_[1];

        hls_qm_uv.q_0 = p_info[23 + 2];    // pm->q_[0];     // quantizer steps
        hls_qm_uv.q_n = p_info[24 + 2];    // pm->q_[1];
        hls_qm_uv.iq_0 = p_info[25 + 2];   // pm->iq_[0];    // reciprocals fixed point.
        hls_qm_uv.iq_n = p_info[26 + 2];   // pm->iq_[1];
        hls_qm_uv.bias_0 = p_info[27 + 2]; // pm->bias_[0];  // rounding bias
        hls_qm_uv.bias_n = p_info[28 + 2]; // pm->bias_[1];
    SHARPEN0:
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
    SHARPEN1:
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_info[29 + 2 + 16 + i];
    }
};

struct ap_NoneZero {
    ap_uint<25> nz_current;
    ap_uint<9> top_nz, left_nz;
    ap_uint<25> line_nz[MAX_NUM_MB_W];
    void set_nz_y(ap_uint<25> nz_y) {
        nz_current(15, 0) = nz_y(15, 0);
        nz_current[24] = nz_y[24];
    };
    void set_nz_uv(ap_uint<25> nz_uv) { nz_current(23, 16) = nz_uv(23, 16); };
    void store_nz(ap_uint<25> nz, ap_uint<LG2_MAX_NUM_MB_W> x_mb) { line_nz[x_mb] = nz; };
    void store_nz(ap_uint<LG2_MAX_NUM_MB_W> x_mb) { line_nz[x_mb] = nz_current; };
    ap_uint<9> load_top9(ap_uint<LG2_MAX_NUM_MB_W> x_mb, ap_uint<LG2_MAX_NUM_MB_H> y_mb) {
        if (y_mb == 0) return 0;
        ap_uint<25> BIT = line_nz[x_mb];
        top_nz[0] = BIT(12, 12);
        top_nz[1] = BIT(13, 13);
        top_nz[2] = BIT(14, 14);
        top_nz[3] = BIT(15, 15);
        // Top-U
        top_nz[4] = BIT(18, 18);
        top_nz[5] = BIT(19, 19);
        // Top-V
        top_nz[6] = BIT(22, 22);
        top_nz[7] = BIT(23, 23);
        // DC
        top_nz[8] = BIT(24, 24);
        return top_nz;
    };
    ap_uint<9> load_left9(ap_uint<LG2_MAX_NUM_MB_W> x_mb) {
        // ap_uint<9> left_nz;
        if (x_mb == 0) return 0;
        ap_uint<25> BIT = line_nz[x_mb - 1];
        left_nz[0] = BIT(3, 3);
        left_nz[1] = BIT(7, 7);
        left_nz[2] = BIT(11, 11);
        left_nz[3] = BIT(15, 15);
        // left-U
        left_nz[4] = BIT(17, 17);
        left_nz[5] = BIT(19, 19);
        // left-V
        left_nz[6] = BIT(21, 21);
        left_nz[7] = BIT(23, 23);
        return left_nz;
    }
};

// Used for creating xclbin
extern "C" {

void kernel_IntraPredLoop2_NoOut(int32_t* p_info,    // 256
                                 uint32_t* ysrc,     // 4096x4096//32bb
                                 uint32_t* usrc,     // 2048x2048//32bb
                                 uint32_t* vsrc,     // 2048x2048//32bb
                                 int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                 ,
                                 uint8_t* pout_prob);

void kernel_2_ArithmeticCoding(uint32_t pin_level[SIZE32_MEM_BW],
                               uint32_t pin_prob[2048 / 4], // with some reduncency
                               uint32_t pout_bw[SIZE32_MEM_BW],
                               uint32_t pout_ret[SIZE32_MEM_RET],
                               uint32_t pout_pred[SIZE32_MEM_PRED]);

void kernel_IntraPredLoop2_NoOut_2(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_2(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
void kernel_IntraPredLoop2_NoOut_3(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_3(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
/////////////////////////////
void kernel_IntraPredLoop2_NoOut_4(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_4(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
/////////////////////////////
void kernel_IntraPredLoop2_NoOut_5(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_5(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
/////////////////////////////
void kernel_IntraPredLoop2_NoOut_6(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_6(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
/////////////////////////////
void kernel_IntraPredLoop2_NoOut_7(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_7(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
/////////////////////////////
void kernel_IntraPredLoop2_NoOut_8(int32_t* p_info,    // 256
                                   uint32_t* ysrc,     // 4096x4096//32bb
                                   uint32_t* usrc,     // 2048x2048//32bb
                                   uint32_t* vsrc,     // 2048x2048//32bb
                                   int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                   ,
                                   uint8_t* pout_prob);

void kernel_2_ArithmeticCoding_8(uint32_t pin_level[SIZE32_MEM_BW],
                                 uint32_t pin_prob[2048 / 4], // with some reduncency
                                 uint32_t pout_bw[SIZE32_MEM_BW],
                                 uint32_t pout_ret[SIZE32_MEM_RET],
                                 uint32_t pout_pred[SIZE32_MEM_PRED]);
}
/*
 * //Other used for host convenience
 */
void set_vect_to(ap_uint<8 * 16> src, unsigned char* des, int strip);
inline uint32_t Get_Busoffset_info_32bits() {
    return 64;
};
inline uint32_t Get_Busoffset_ysrc(uint32_t y_size) {
    return (y_size + 3) / sizeof(uint32_t);
};
inline uint32_t Get_Busoffset_uvsrc(uint32_t uv_size) {
    return (uv_size + 3) / sizeof(uint32_t);
};

inline uint32_t Get_Busoffset_level(uint32_t num_mb) {
    return num_mb * (512 * sizeof(int16_t)) / sizeof(uint32_t);
};
inline uint32_t Get_Busoffset_prob_32bits() {
    return 2048 / sizeof(uint32_t);
};
inline uint32_t Get_Busoffset_prob_8bits() {
    return 2048;
};
inline uint32_t Get_Busoffset_pout_bw(uint32_t num_mb) {
    return num_mb * (384 / SYSTEM_MIN_COMP_RATIO / sizeof(uint32_t)) + 1000;
};
inline uint32_t Get_Busoffset_pout_ret(uint32_t num_mb) {
    return (num_mb + 3) / sizeof(uint32_t);
};
inline uint32_t Get_Busoffset_pout_pred(uint32_t num_mb) {
    return (num_mb * 8 + 3) / sizeof(uint32_t);
};

namespace xf {
namespace codec {
// For multi-instance
/**
 * @brief Level 2 : kernel for WebP intra prediction
 *
 * @tparam p_info basic information of image and compression parameters. More details can be found in function
 * kernel_IntraPredLoop2_NoOut_core.
 * @param ysrc the Y sample of image as input.
 * @param usrc the U sample of image as input.
 * @param vsrc the V sample of image as input.
 * @param pout_level point to structures contains coefficients from a MB for output.
 * @param pout_prob probability table for output.
 */
void webp_IntraPredLoop2_NoOut_1(int32_t* p_info,    // 256
                                 uint32_t* ysrc,     // 4096x4096//32bb
                                 uint32_t* usrc,     // 2048x2048//32bb
                                 uint32_t* vsrc,     // 2048x2048//32bb
                                 int32_t* pout_level // 65536*512/2 int16_t* pout_level//65536*512
                                 ,
                                 uint8_t* pout_prob);

/**
 * @brief Level 2 : kernel for WebP arithmetic coding
 *
 * @param pin_level point to structures contains coefficients from a MB as an input.
 * @param pin_prob probability table as an input.
 * @param pout_bw byte-stream created by arithmetic coding
 * @param pout_ret output stream in which the element indicates the non-zero status of 6 blocks of a MB
 * @param pout_pred output stream of prediction mode of Y.
 */
void webp_2_ArithmeticCoding_1(uint32_t pin_level[SIZE32_MEM_BW],
                               uint32_t pin_prob[2048 / 4], // with some reduncency
                               uint32_t pout_bw[SIZE32_MEM_BW],
                               uint32_t pout_ret[SIZE32_MEM_RET],
                               uint32_t pout_pred[SIZE32_MEM_PRED]);
} // namespace codec
} // namespace xf

#endif
