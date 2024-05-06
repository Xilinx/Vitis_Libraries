/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "xf_isp_accel_config.h"

static uint32_t hist0_awb[NUM_STREAMS][3][HIST_SIZE] = {0};
static uint32_t hist1_awb[NUM_STREAMS][3][HIST_SIZE] = {0};
static uint32_t hist0_aec[NUM_STREAMS][AEC_HIST_SIZE] = {0};
static uint32_t hist1_aec[NUM_STREAMS][AEC_HIST_SIZE] = {0};

static constexpr int MinMaxVArrSize =
    LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, SLICE_HEIGHT, XF_WIDTH, XF_NPPC>::MinMaxVArrSize;
static constexpr int MinMaxHArrSize =
    LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, SLICE_HEIGHT, XF_WIDTH, XF_NPPC>::MinMaxHArrSize;
static XF_CTUNAME(OUT_TYPE, XF_NPPC) omin_r[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize];
static XF_CTUNAME(OUT_TYPE, XF_NPPC) omax_r[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize];
static XF_CTUNAME(OUT_TYPE, XF_NPPC) omin_w[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize];
static XF_CTUNAME(OUT_TYPE, XF_NPPC) omax_w[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize];

static ap_ufixed<16, 4> mean1[NUM_STREAMS] = {0, 0, 0, 0};
static ap_ufixed<16, 4> mean2[NUM_STREAMS] = {0, 0, 0, 0};
static ap_ufixed<16, 4> L_max1[NUM_STREAMS] = {0.1, 0.1, 0.1, 0.1};
static ap_ufixed<16, 4> L_max2[NUM_STREAMS] = {0.1, 0.1, 0.1, 0.1};
static ap_ufixed<16, 4> L_min1[NUM_STREAMS] = {10, 10, 10, 10};
static ap_ufixed<16, 4> L_min2[NUM_STREAMS] = {10, 10, 10, 10};
static ap_ufixed<32, 24> acc_sum[NUM_STREAMS] = {0, 0, 0, 0};

static int igain_0[NUM_STREAMS][3] = {0};
static int igain_1[NUM_STREAMS][3] = {0};

static bool flag_awb[NUM_STREAMS] = {0};
static bool flag_tm[NUM_STREAMS] = {0};
static bool flag_aec[NUM_STREAMS] = {0};

static XF_TNAME(IN_TYPE, XF_NPPC) bpc_buff[NUM_STREAMS][4][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)] = {0};
static XF_TNAME(IN_TYPE, XF_NPPC) demo_buffs[NUM_STREAMS][4][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)];
static XF_TNAME(IN_TYPE, XF_NPPC) rgbir_buffs[NUM_STREAMS][4][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)];
static XF_TNAME(IN_TYPE, XF_NPPC) rgbir_ir_buffs[NUM_STREAMS][2][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)];
static XF_TNAME(IN_TYPE, XF_NPPC) rgbir_wgt_buffs[NUM_STREAMS][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)];

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& awb_out,
               unsigned short height,
               unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    ap_uint<13> row, col;
    int readindex = 0, writeindex = 0;

    ap_uint<13> img_width = width >> XF_BITSHIFT(NPC);

Row_Loop:
    for (row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
    // clang-format on
    Col_Loop:
        for (col = 0; col < img_width; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS PIPELINE
            // clang-format on
            XF_TNAME(SRC_T, NPC) tmp_src;
            tmp_src = demosaic_out.read(readindex++);
            awb_out.write(writeindex++, tmp_src);
        }
    }
}

template <int SRC_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int STREAMS = 2,
          int SLICES = 2,
          int XFCVDEPTH_IN_1 = 2,
          int XFCVDEPTH_OUT_1 = 2,
          int XFCVDEPTH_OUT_IR = 2,
          int XFCVDEPTH_3XWIDTH = 12000>
void function_rgbir(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& hdr_out,
                    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& rggb_out,
                    ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,
                    char R_IR_C1_wgts[STREAMS][25],
                    char R_IR_C2_wgts[STREAMS][25],
                    char B_at_R_wgts[STREAMS][25],
                    char IR_at_R_wgts[STREAMS][9],
                    char IR_at_B_wgts[STREAMS][9],
                    char sub_wgts[STREAMS][4],
                    unsigned short bayer_form[STREAMS],
                    unsigned short height,
                    unsigned short width,
                    int stream_id,
                    int slice_id,
                    XF_TNAME(SRC_T, NPC) rgbir_buffs[STREAMS][4][COLS >> XF_BITSHIFT(NPC)],
                    XF_TNAME(SRC_T, NPC) rgbir_ir_buffs[STREAMS][2][COLS >> XF_BITSHIFT(NPC)],
                    XF_TNAME(SRC_T, NPC) rgbir_wgt_buffs[STREAMS][COLS >> XF_BITSHIFT(NPC)],
                    uint16_t strm_rows) {
// clang-format off
 #pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_OUT_IR> fullir_out(height, width);
// clang-format off
 #pragma HLS DATAFLOW
    // clang-format on
    if (USE_RGBIR) {
        xf::cv::rgbir2bayer_multi_wrap<FILTERSIZE1, FILTERSIZE2, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC,
                                       XF_BORDER_CONSTANT, USE_URAM, NUM_STREAMS, NUM_SLICES, XFCVDEPTH_IN_1,
                                       XFCVDEPTH_OUT_1, XFCVDEPTH_OUT_IR, XFCVDEPTH_3XWIDTH>(
            hdr_out, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, bayer_form,
            rggb_out, fullir_out, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, stream_id, slice_id, strm_rows);

        xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_OUT_IR>(fullir_out,
                                                                                                           img_out_ir);

    } else {
        fifo_copy<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(hdr_out, rggb_out,
                                                                                                      height, width);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int STREAMS, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void fifo_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& demosaic_out,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& awb_out,
              uint32_t awb_hist0[STREAMS][3][HIST_SIZE],
              uint32_t awb_hist1[STREAMS][3][HIST_SIZE],
              int awb_gain0[STREAMS][3],
              int awb_gain1[STREAMS][3],
              unsigned short height,
              unsigned short width,
              bool awb_flg[STREAMS],
              bool eof_awb[STREAMS],
              unsigned short pawb[STREAMS],
              int stream_id,
              int slice_id,
              unsigned short full_height) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC))) - 1; // 65535.0f;

    if (WB_TYPE) {
        xf::cv::hist_nor_awb_multi<OUT_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, WB_TYPE, HIST_SIZE, NUM_STREAMS,
                                   XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            demosaic_out, awb_out, awb_hist0, awb_hist1, height, width, inputMin, inputMax, outputMin, outputMax,
            awb_flg, eof_awb, pawb, stream_id, slice_id, full_height);
    } else {
        xf::cv::chgain_update_awb_multi<OUT_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, 0, NUM_STREAMS,
                                        XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            demosaic_out, awb_out, awb_gain0, awb_gain1, height, width, awb_flg, eof_awb, pawb, stream_id);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int STREAMS, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void function_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& demosaic_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& awb_out,
                  uint32_t awb_hist0[STREAMS][3][HIST_SIZE],
                  uint32_t awb_hist1[STREAMS][3][HIST_SIZE],
                  int awb_gain0[STREAMS][3],
                  int awb_gain1[STREAMS][3],
                  unsigned short height,
                  unsigned short width,
                  bool awb_flg[STREAMS],
                  bool eof_awb[STREAMS],
                  unsigned short pawb[STREAMS],
                  int stream_id,
                  int slice_id,
                  unsigned short full_height) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    if (USE_AWB) {
        fifo_awb<OUT_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            demosaic_out, awb_out, awb_hist0, awb_hist1, awb_gain0, awb_gain1, height, width, awb_flg, eof_awb, pawb,
            stream_id, slice_id, full_height);
    } else {
        fifo_copy<OUT_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            demosaic_out, awb_out, height, width);
    }
}

template <int DST_T, int LTM_T, int ROWS, int COLS, int NPC, int STREAMS, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void function_tm(xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& awb_out,
                 xf::cv::Mat<LTM_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& aecin,
                 XF_CTUNAME(OUT_TYPE, XF_NPPC) omin_r[STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(OUT_TYPE, XF_NPPC) omax_r[STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(OUT_TYPE, XF_NPPC) omin_w[STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(OUT_TYPE, XF_NPPC) omax_w[STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                 unsigned short blk_height[STREAMS],
                 unsigned short blk_width[STREAMS],
                 ap_ufixed<16, 4> mean1[STREAMS],
                 ap_ufixed<16, 4> mean2[STREAMS],
                 ap_ufixed<16, 4> L_max1[STREAMS],
                 ap_ufixed<16, 4> L_max2[STREAMS],
                 ap_ufixed<16, 4> L_min1[STREAMS],
                 ap_ufixed<16, 4> L_min2[STREAMS],
                 unsigned int c1[STREAMS],
                 unsigned int c2[STREAMS],
                 bool tm_flg[STREAMS],
                 bool eof_tm[STREAMS],
                 int stream_id,
                 unsigned short full_height,
                 int slice_id,
                 ap_ufixed<32, 24> acc_sum[STREAMS]) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC)); /* Used in xf_QuatizationDithering */
    if (USE_GTM) {
        xf::cv::gtm_multi_wrap<OUT_TYPE, XF_LTM_T, IN_TYPE, SIN_CHANNEL_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC,
                               NUM_STREAMS, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            awb_out, aecin, mean1, mean2, L_max1, L_max2, L_min1, L_min2, c1, c2, tm_flg, eof_tm, stream_id,
            full_height, slice_id, acc_sum);

    }
    //  else if (USE_LTM) {
    // xf::cv::LTM_multi_wrap<OUT_TYPE, XF_LTM_T, BLOCK_HEIGHT, BLOCK_WIDTH, SLICE_HEIGHT, XF_WIDTH, XF_NPPC,
    //                        NUM_STREAMS, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>::LTM_multistream_wrap(awb_out, blk_height,
    //                                                                                            blk_width, omin_r,
    //                                                                                            omax_r, omin_w,
    //                                                                                            omax_w, aecin,
    //                                                                                            tm_flg, eof_tm,
    //                                                                                            stream_id);

    // }
    else if (USE_QnD) {
        xf::cv::xf_QuatizationDithering<OUT_TYPE, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, 256, Q_VAL, XF_NPPC, XF_USE_URAM,
                                        XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(awb_out, aecin);
    }
}

template <int DST_T,
          int LTM_T,
          int ROWS,
          int COLS,
          int NPC,
          int STREAMS,
          int XFCVDEPTH_IN_1,
          int XFCVDEPTH_OUT_1,
          int XFCVDEPTH_3dlut>
void function_3dlut(xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _dst,
                    xf::cv::Mat<LTM_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& lut_out,
                    ap_uint<LUT_PTR_WIDTH>* lut,
                    unsigned short lutDim[STREAMS],
                    unsigned short height,
                    unsigned short width,
                    int stream_id) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<XF_32FC3, SQ_LUTDIM, LUT_DIM, XF_NPPC, XFCVDEPTH_3dlut> lutMat(lutDim[stream_id] * lutDim[stream_id],
                                                                               lutDim[stream_id]);

#pragma HLS DATAFLOW
    if (USE_3DLUT) {
        xf::cv::Array2xfMat<LUT_PTR_WIDTH, XF_32FC3, SQ_LUTDIM, LUT_DIM, XF_NPPC, XFCVDEPTH_3dlut>(lut, lutMat);
        xf::cv::lut3d_multi<LUT_DIM, SQ_LUTDIM, XF_LTM_T, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_USE_URAM,
                            NUM_STREAMS, XFCVDEPTH_IN_1, XFCVDEPTH_3dlut, XFCVDEPTH_OUT_1>(_dst, lutMat, lut_out,
                                                                                           lutDim, stream_id);
    } else {
        fifo_copy<XF_LTM_T, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(_dst, lut_out,
                                                                                                        height, width);
    }
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_IN_1,
          int XFCVDEPTH_OUT_1,
          int N,
          int STREAMS>
void function_degamma(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& bpc_out,
                      xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& dgamma_out,
                      unsigned int dgam_params[STREAMS][3][N][3],
                      unsigned short dgam_bayer[STREAMS],
                      unsigned short height,
                      unsigned short width,
                      int stream_id) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    if (USE_DEGAMMA) {
        degamma_multi<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XF_CV_DEPTH_OUT_1, DGAMMA_KP,
                      NUM_STREAMS>(bpc_out, dgamma_out, dgam_params, dgam_bayer, stream_id);
    } else {
        fifo_copy<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            bpc_out, dgamma_out, height, width);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1, int STREAMS>
void function_aec(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& rggb_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& aec_out,
                  uint32_t aec_hist0[STREAMS][AEC_HIST_SIZE],
                  uint32_t aec_hist1[STREAMS][AEC_HIST_SIZE],
                  unsigned short height,
                  unsigned short width,
                  bool aec_flg[STREAMS],
                  bool eof_aec[STREAMS],
                  unsigned short paec[STREAMS],
                  int stream_id,
                  int slice_id,
                  unsigned short full_height) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    float aec_inputMin = 0.0f;
    float aec_inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC))) - 1; // 65535.0f;
    float aec_outputMin = 0.0f;
    float aec_outputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC))) - 1; // 65535.0f;

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    if (USE_AEC) {
        xf::cv::aec_multi<IN_TYPE, IN_TYPE, AEC_SIN_CHANNEL_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_USE_URAM,
                          XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1, AEC_HIST_SIZE, NUM_STREAMS>(
            rggb_out, aec_out, aec_hist0, aec_hist1, paec, aec_inputMin, aec_inputMax, aec_outputMin, aec_outputMax,
            aec_flg, eof_aec, stream_id, slice_id, full_height);

    } else {
        fifo_copy<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(rggb_out, aec_out,
                                                                                                      height, width);
    }
}
template <int LTM_T, int ROWS, int COLS, int NPC, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void function_csc(xf::cv::Mat<LTM_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& lut_out,
                  ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                  unsigned short height,
                  unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<XF_16UC1, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_OUT_1> imgOutput(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    if (USE_CSC) {
        xf::cv::rgb2yuyv<XF_LTM_T, XF_16UC1, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            lut_out, imgOutput);
        xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_OUT_1>(imgOutput,
                                                                                                          img_out);
    } else {
        xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XFCVDEPTH_IN_1>(lut_out,
                                                                                                         img_out);
    }
}

void Streampipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                    ap_uint<OUTPUT_PTR_WIDTH>* img_out,

                    ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,
                    ap_uint<LUT_PTR_WIDTH>* lut,
                    unsigned short height,
                    unsigned short full_height,
                    unsigned short width,
                    uint16_t strm_rows,

                    unsigned int dgam_params[NUM_STREAMS][3][DGAMMA_KP][3],
                    uint32_t awb_hist0[NUM_STREAMS][3][HIST_SIZE],
                    uint32_t awb_hist1[NUM_STREAMS][3][HIST_SIZE],
                    int awb_gain0[NUM_STREAMS][3],
                    int awb_gain1[NUM_STREAMS][3],
                    bool awb_flg[NUM_STREAMS],
                    bool eof_awb[NUM_STREAMS],
                    unsigned short array_params[NUM_STREAMS][11],
                    unsigned char gamma_lut[NUM_STREAMS][256 * 3],
                    short wr_hls[NUM_STREAMS][NO_EXPS * XF_NPPC * W_B_SIZE],
                    char R_IR_C1_wgts[NUM_STREAMS][25],
                    char R_IR_C2_wgts[NUM_STREAMS][25],
                    char B_at_R_wgts[NUM_STREAMS][25],
                    char IR_at_R_wgts[NUM_STREAMS][9],
                    char IR_at_B_wgts[NUM_STREAMS][9],
                    char sub_wgts[NUM_STREAMS][4],
                    int dcp_params_12to16[NUM_STREAMS][3][4][3],

                    uint32_t aec_hist0[NUM_STREAMS][AEC_HIST_SIZE], /* function_aec */
                    uint32_t aec_hist1[NUM_STREAMS][AEC_HIST_SIZE], /* function_aec */
                    bool aec_flg[NUM_STREAMS],
                    bool eof_aec[NUM_STREAMS],

                    XF_CTUNAME(OUT_TYPE, XF_NPPC) omin_r[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                    XF_CTUNAME(OUT_TYPE, XF_NPPC) omax_r[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                    XF_CTUNAME(OUT_TYPE, XF_NPPC) omin_w[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize],
                    XF_CTUNAME(OUT_TYPE, XF_NPPC) omax_w[NUM_STREAMS][MinMaxVArrSize][MinMaxHArrSize],

                    XF_TNAME(IN_TYPE, XF_NPPC) demo_buffs[NUM_STREAMS][4][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)],
                    XF_TNAME(IN_TYPE, XF_NPPC) rgbir_buffs[NUM_STREAMS][4][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)],
                    XF_TNAME(IN_TYPE, XF_NPPC) rgbir_ir_buffs[NUM_STREAMS][2][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)],
                    XF_TNAME(IN_TYPE, XF_NPPC) rgbir_wgt_buffs[NUM_STREAMS][XF_WIDTH >> XF_BITSHIFT(XF_NPPC)],

                    ap_ufixed<16, 4> mean1[NUM_STREAMS],
                    ap_ufixed<16, 4> mean2[NUM_STREAMS],
                    ap_ufixed<16, 4> L_max1[NUM_STREAMS],
                    ap_ufixed<16, 4> L_max2[NUM_STREAMS],
                    ap_ufixed<16, 4> L_min1[NUM_STREAMS],
                    ap_ufixed<16, 4> L_min2[NUM_STREAMS],
                    unsigned int c1[NUM_STREAMS],
                    unsigned int c2[NUM_STREAMS],
                    bool tm_flg[NUM_STREAMS],
                    bool eof_tm[NUM_STREAMS],
                    int stream_id,
                    int slice_id,
                    ap_ufixed<32, 24> acc_sum[NUM_STREAMS],
                    signed int ccm_config_1[NUM_STREAMS][3][3],
                    signed int ccm_config_2[NUM_STREAMS][3]) {
    int max_height, max_width;
    static unsigned short black_level[NUM_STREAMS];
    static unsigned short rgain[NUM_STREAMS];
    static unsigned short bgain[NUM_STREAMS];
    static unsigned short ggain[NUM_STREAMS];
    static unsigned short bayer_p[NUM_STREAMS];
    static unsigned short pawb[NUM_STREAMS];
    static unsigned short paec[NUM_STREAMS];
    static unsigned short block_height[NUM_STREAMS];
    static unsigned short block_width[NUM_STREAMS];
    static unsigned short lutDim[NUM_STREAMS];

    max_height = height * 2;
    max_width = width + NUM_H_BLANK;

    for (int i = 0; i < NUM_STREAMS; i++) {
        black_level[i] = array_params[i][5];
        rgain[i] = array_params[i][0];
        bgain[i] = array_params[i][1];
        ggain[i] = array_params[i][2];
        bayer_p[i] = array_params[i][4];
        pawb[i] = array_params[i][3];
        paec[i] = array_params[i][3];
        block_height[i] = array_params[i][8];
        block_width[i] = array_params[i][9];
        lutDim[i] = array_params[i][10];
    }

    xf::cv::Mat<IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_0_1> imgInput1(max_height, max_width);
    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_0_2> imgInput2(height, width);
    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_LEF> LEF_Img(height, width);
    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_SEF> SEF_Img(height, width);
    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_1> hdr_out(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    //    const int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC));
    if (USE_HDR_FUSION) {
        xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_0_1>(img_inp,
                                                                                                          imgInput1);

        xf::cv::extractExposureFrames<IN_TYPE, NUM_V_BLANK_LINES, NUM_H_BLANK, SLICE_HEIGHT, XF_WIDTH, XF_NPPC,
                                      XF_USE_URAM, XF_CV_DEPTH_IN_0_1, XF_CV_DEPTH_LEF, XF_CV_DEPTH_SEF>(
            imgInput1, LEF_Img, SEF_Img);

        xf::cv::Hdrmerge_bayer_multi<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_USE_URAM, NO_EXPS, W_B_SIZE,
                                     NUM_STREAMS, XF_CV_DEPTH_LEF, XF_CV_DEPTH_SEF, XF_CV_DEPTH_IN_1>(
            LEF_Img, SEF_Img, hdr_out, wr_hls, stream_id);
    } else {
        xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_0_2>(img_inp,
                                                                                                           imgInput2);
        xf::cv::hdr_decompand_multi<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_0_2,
                                    XF_CV_DEPTH_IN_1, NUM_STREAMS>(imgInput2, hdr_out, dcp_params_12to16, bayer_p,
                                                                   stream_id);
    }
    if (USE_RGBIR) {
        if (NUM_SLICES > 1) {
            if (slice_id == 0) {
                height = height - 3;
            } else if (slice_id == NUM_SLICES - 1) {
                height = height + 3;
            }
        }
    }

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_2> rggb_out(height, width);
    function_rgbir<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_USE_URAM, NUM_STREAMS, NUM_SLICES, XF_CV_DEPTH_IN_1,
                   XF_CV_DEPTH_IN_2, XF_CV_DEPTH_OUT_IR, XF_CV_DEPTH_3XWIDTH>(
        hdr_out, rggb_out, img_out_ir, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts,
        bayer_p, height, width, stream_id, slice_id, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, strm_rows);

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_3> aec_out(height, width);
    function_aec<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_IN_3, NUM_STREAMS>(
        rggb_out, aec_out, aec_hist0, aec_hist1, height, width, aec_flg, eof_aec, paec, stream_id, slice_id,
        full_height);

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_4> blc_out(height, width);
    xf::cv::blackLevelCorrection_multi<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, 16, 15, 1, NUM_STREAMS,
                                       XF_CV_DEPTH_IN_3, XF_CV_DEPTH_IN_4>(aec_out, blc_out, black_level, stream_id);

    if (NUM_SLICES > 1) {
        if (slice_id == 0) {
            height = height - 2;
        } else if (slice_id == NUM_SLICES - 1) {
            height = height + 2;
        }
    }

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_5> bpc_out(height, width);

    xf::cv::badpixelcorrection_multi<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, 0, 0, NUM_STREAMS, NUM_SLICES,
                                     XF_CV_DEPTH_IN_4, XF_CV_DEPTH_IN_5>(blc_out, bpc_out, stream_id, slice_id,
                                                                         bpc_buff);

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_6> dgamma_out(height, width);
    function_degamma<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_5, XF_CV_DEPTH_IN_6, DGAMMA_KP,
                     NUM_STREAMS>(bpc_out, dgamma_out, dgam_params, bayer_p, height, width, stream_id);

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_7> LscOut(height, width);
    xf::cv::Lscdistancebased_multi<IN_TYPE, IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_6,
                                   XF_CV_DEPTH_IN_7, NUM_SLICES>(dgamma_out, LscOut, full_height, width, slice_id,
                                                                 strm_rows);

    xf::cv::Mat<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_IN_8> gain_out(height, width);

    xf::cv::gaincontrol_multi_wrap<IN_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS, XF_CV_DEPTH_IN_7,
                                   XF_CV_DEPTH_IN_8>(LscOut, gain_out, rgain, bgain, ggain, bayer_p, stream_id);

    if (NUM_SLICES != 1) {
        if (slice_id == 0) {
            height = height - 2;
        } else if (slice_id == NUM_SLICES - 1) {
            height = height + 2;
        }
    }

    xf::cv::Mat<OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_0> demosaic_out(height, width);
    xf::cv::demosaicing_multi_wrap<IN_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, 0, NUM_STREAMS, NUM_SLICES,
                                   XF_CV_DEPTH_IN_8, XF_CV_DEPTH_OUT_0>(gain_out, demosaic_out, bayer_p, stream_id,
                                                                        slice_id, demo_buffs, strm_rows);

    xf::cv::Mat<OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_1> impop(height, width);
    xf::cv::Mat<OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_2> awb_out(height, width);
    function_awb<OUT_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS, XF_CV_DEPTH_OUT_0,
                 XF_CV_DEPTH_OUT_2>(demosaic_out, awb_out, awb_hist0, awb_hist1, awb_gain0, awb_gain1, height, width,
                                    awb_flg, eof_awb, pawb, stream_id, slice_id, full_height);

    xf::cv::Mat<OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_3> ccm_out(height, width);
    xf::cv::colorcorrectionmatrix_multi<OUT_TYPE, OUT_TYPE, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS,
                                        XF_CV_DEPTH_OUT_2, XF_CV_DEPTH_OUT_3>(awb_out, ccm_out, ccm_config_1,
                                                                              ccm_config_2, stream_id);

    xf::cv::Mat<XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_4> aecin(height, width);

    if (OUT_TYPE == XF_8UC3) {
        fifo_copy<OUT_TYPE, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_3, XF_CV_DEPTH_OUT_4>(
            ccm_out, aecin, height, width);
    } else {
        function_tm<OUT_TYPE, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS, XF_CV_DEPTH_OUT_3,
                    XF_CV_DEPTH_OUT_4>(ccm_out, aecin, omin_r, omax_r, omin_w, omax_w, block_height, block_width, mean1,
                                       mean2, L_max1, L_max2, L_min1, L_min2, c1, c2, tm_flg, eof_tm, stream_id,
                                       full_height, slice_id, acc_sum);
    }

    xf::cv::Mat<XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_5> _dst(height, width);
    xf::cv::gammacorrection_multi<XF_LTM_T, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS, XF_CV_DEPTH_OUT_4,
                                  XF_CV_DEPTH_OUT_5>(aecin, _dst, gamma_lut, stream_id);

    xf::cv::Mat<XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_6> lut_out(height, width);
    function_3dlut<XF_LTM_T, XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, NUM_STREAMS, XF_CV_DEPTH_OUT_5,
                   XF_CV_DEPTH_OUT_6, XF_CV_DEPTH_3dlut>(_dst, lut_out, lut, lutDim, height, width, stream_id);

    function_csc<XF_LTM_T, SLICE_HEIGHT, XF_WIDTH, XF_NPPC, XF_CV_DEPTH_OUT_6, XF_CV_DEPTH_OUT_7>(lut_out, img_out,
                                                                                                  height, width);

    return;
}

/*********************************************************************************
 * Function:    ISPPipeline_accel
 * Parameters:  input and output image pointers, image resolution
 * Return:
 * Description:
 **********************************************************************************/
extern "C" {
void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp1,
                       ap_uint<INPUT_PTR_WIDTH>* img_inp2,
                       ap_uint<INPUT_PTR_WIDTH>* img_inp3,
                       ap_uint<INPUT_PTR_WIDTH>* img_inp4,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out1,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out2,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out3,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out4,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir1,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir2,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir3,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir4,
                       short wr_hls[NUM_STREAMS][NO_EXPS * XF_NPPC * W_B_SIZE],
                       int dcp_params_12to16[NUM_STREAMS][3][4][3],
                       char R_IR_C1_wgts[NUM_STREAMS][25],
                       char R_IR_C2_wgts[NUM_STREAMS][25],
                       char B_at_R_wgts[NUM_STREAMS][25],
                       char IR_at_R_wgts[NUM_STREAMS][9],
                       char IR_at_B_wgts[NUM_STREAMS][9],
                       char sub_wgts[NUM_STREAMS][4],
                       unsigned int dgam_params[NUM_STREAMS][3][DGAMMA_KP][3],
                       unsigned int c1[NUM_STREAMS],
                       unsigned int c2[NUM_STREAMS],
                       unsigned short array_params[NUM_STREAMS][11],
                       unsigned char gamma_lut[NUM_STREAMS][256 * 3],
                       ap_uint<LUT_PTR_WIDTH>* lut1,
                       ap_uint<LUT_PTR_WIDTH>* lut2,
                       ap_uint<LUT_PTR_WIDTH>* lut3,
                       ap_uint<LUT_PTR_WIDTH>* lut4,
                       signed int ccm_config_1[NUM_STREAMS][3][3],
                       signed int ccm_config_2[NUM_STREAMS][3]) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp1             offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_inp2             offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=img_inp3             offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=img_inp4             offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=img_out1             offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi     port=img_out2             offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi     port=img_out3             offset=slave bundle=gmem7
#pragma HLS INTERFACE m_axi     port=img_out4             offset=slave bundle=gmem8

#pragma HLS INTERFACE m_axi     port=img_out_ir1          offset=slave bundle=gmem9
#pragma HLS INTERFACE m_axi     port=img_out_ir2          offset=slave bundle=gmem10
#pragma HLS INTERFACE m_axi     port=img_out_ir3          offset=slave bundle=gmem11
#pragma HLS INTERFACE m_axi     port=img_out_ir4          offset=slave bundle=gmem12
#pragma HLS INTERFACE m_axi     port=wr_hls               offset=slave bundle=gmem13
#pragma HLS INTERFACE m_axi     port=dcp_params_12to16    offset=slave bundle=gmem14
#pragma HLS INTERFACE m_axi     port=R_IR_C1_wgts         offset=slave bundle=gmem15
#pragma HLS INTERFACE m_axi     port=R_IR_C2_wgts         offset=slave bundle=gmem16
#pragma HLS INTERFACE m_axi     port=B_at_R_wgts          offset=slave bundle=gmem17
#pragma HLS INTERFACE m_axi     port=IR_at_R_wgts         offset=slave bundle=gmem18
#pragma HLS INTERFACE m_axi     port=IR_at_B_wgts         offset=slave bundle=gmem19
#pragma HLS INTERFACE m_axi     port=sub_wgts             offset=slave bundle=gmem20
#pragma HLS INTERFACE m_axi     port=dgam_params          offset=slave bundle=gmem21
#pragma HLS INTERFACE m_axi     port=c1                   offset=slave bundle=gmem22
#pragma HLS INTERFACE m_axi     port=c2                   offset=slave bundle=gmem23
#pragma HLS INTERFACE m_axi     port=array_params         offset=slave bundle=gmem24
#pragma HLS INTERFACE m_axi     port=gamma_lut            offset=slave bundle=gmem25
#pragma HLS INTERFACE m_axi     port=lut1                 offset=slave bundle=gmem26
#pragma HLS INTERFACE m_axi     port=lut2                 offset=slave bundle=gmem27
#pragma HLS INTERFACE m_axi     port=lut3                 offset=slave bundle=gmem28
#pragma HLS INTERFACE m_axi     port=lut4                 offset=slave bundle=gmem29
#pragma HLS INTERFACE m_axi port=ccm_config_1     bundle=gmem30 offset=slave
#pragma HLS INTERFACE m_axi port=ccm_config_2     bundle=gmem31 offset=slave
    // clang-format on

    // struct ispparams_config params[NUM_STREAMS];

    uint32_t tot_rows = 0;
    int rem_rows[NUM_STREAMS];

    static short wr_hls_tmp[NUM_STREAMS][NO_EXPS * XF_NPPC * W_B_SIZE];
    static unsigned char gamma_lut_tmp[NUM_STREAMS][256 * 3];
    static unsigned int c1_tmp[NUM_STREAMS], c2_tmp[NUM_STREAMS];
    unsigned int dgam_params_tmp[NUM_STREAMS][3][DGAMMA_KP][3];

    static int dcp_params_12to16_tmp[NUM_STREAMS][3][4][3];

    static char R_IR_C1_wgts_tmp[NUM_STREAMS][25], R_IR_C2_wgts_tmp[NUM_STREAMS][25], B_at_R_wgts_tmp[NUM_STREAMS][25],
        IR_at_R_wgts_tmp[NUM_STREAMS][9], IR_at_B_wgts_tmp[NUM_STREAMS][9], sub_wgts_tmp[NUM_STREAMS][4];

    unsigned short height_arr[NUM_STREAMS], width_arr[NUM_STREAMS];

    static int ccm_config_1_tmp[NUM_STREAMS][3][3];
    static int ccm_config_2_tmp[NUM_STREAMS][3];
    constexpr int ccm_row = 3, ccm_col = 3;

    constexpr int dg_parms_c1 = 3;
    constexpr int dg_parms_c2 = 3;

    constexpr int dcp_parms1 = 3;
    constexpr int dcp_parms2 = 4;
    constexpr int dcp_parms3 = 3;

CCM1_CCM2_INIT_LOOP:
    for (int i = 0; i < NUM_STREAMS; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on   
        for(int j=0; j< ccm_row; j++){
            // clang-format off
    #pragma HLS LOOP_TRIPCOUNT min=ccm_row max=ccm_row
           // clang-format on   
           for(int k=0; k< ccm_col; k++){
    // clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ccm_col max=ccm_col
                    // clang-format on    
            ccm_config_1_tmp[i][j][k] = ccm_config_1[i][j][k];
           }
            ccm_config_2_tmp[i][j] = ccm_config_2[i][j];
        }
    }
    
DEGAMMA_PARAMS_LOOP:
    for (int n = 0; n < NUM_STREAMS; n++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on

        for (int i = 0; i < dg_parms_c1; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=dg_parms_c1 max=dg_parms_c1
           // clang-format on   
           for(int j=0; j<DGAMMA_KP; j++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=DGAMMA_KP max=DGAMMA_KP
                // clang-format on             
                for(int k=0; k<dg_parms_c2; k++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=dg_parms_c2 max=dg_parms_c2
                    // clang-format on  
                    dgam_params_tmp[n][i][j][k] = dgam_params[n][i][j][k];
                }
            }                
        }
    }
    
DECOMPAND_PARAMS_LOOP:
   for(int n=0; n<NUM_STREAMS; n++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on

        for (int i = 0; i < dcp_parms1; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=dcp_parms1 max=dcp_parms1
           // clang-format on   
           for(int j=0; j<dcp_parms2; j++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=dcp_parms2 max=dcp_parms2
                // clang-format on             
                for(int k=0; k<dcp_parms3; k++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=dcp_parms3 max=dcp_parms3
                    // clang-format on  
                    dcp_params_12to16_tmp[n][i][j][k] = dcp_params_12to16[n][i][j][k];
                }
            }                
        }
    }
        
   
C1_C2_INIT_LOOP:   
   for(int i=0; i < NUM_STREAMS; i++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
                    // clang-format on    
        c1_tmp[i]=c1[i];
        c2_tmp[i]=c2[i];
        
    }  
    constexpr int R_B_count=25, IR_count=9, sub_count=4;    
      
RGBIR_INIT_LOOP_1:
  for(int n=0; n < NUM_STREAMS; n++){
      
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on

        for (int i = 0; i < R_B_count; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=R_B_count max=R_B_count
        // clang-format on         
            
          R_IR_C1_wgts_tmp[n][i] = R_IR_C1_wgts[n][i];
          R_IR_C2_wgts_tmp[n][i] = R_IR_C2_wgts[n][i];
          B_at_R_wgts_tmp[n][i]  = B_at_R_wgts[n][i];      
     }
}

RGBIR_INIT_LOOP_2:
for(int n=0; n < NUM_STREAMS; n++){
      
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on

        for (int i = 0; i < IR_count; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=IR_count max=IR_count
        // clang-format on         
            
          IR_at_R_wgts_tmp[n][i] = IR_at_R_wgts[n][i];
          IR_at_B_wgts_tmp[n][i] = IR_at_B_wgts[n][i];
        }
}

RGBIR_INIT_LOOP_3:
for(int n=0; n < NUM_STREAMS; n++){
      
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on

        for (int i = 0; i < sub_count; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=sub_count max=sub_count
        // clang-format on         
            
          sub_wgts_tmp[n][i] = sub_wgts[n][i];
        }
}

ARRAY_PARAMS_LOOP:
    for (int i = 0; i < NUM_STREAMS; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=NUM_STREAMS
        // clang-format on

        height_arr[i] = array_params[i][6];
        width_arr[i] = array_params[i][7];
        height_arr[i] = height_arr[i] * RD_MULT;
        tot_rows = tot_rows + height_arr[i];
        rem_rows[i] = height_arr[i];
    }
    constexpr int glut_TC = 256 * 3;

GAMMA_LUT_LOOP:
    for (int n = 0; n < NUM_STREAMS; n++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on        
       for(int i=0; i < glut_TC; i++){
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=glut_TC max=glut_TC
        // clang-format on           
           
       gamma_lut_tmp[n][i] = gamma_lut[n][i];
       
       }
   }     
           
WR_HLS_INIT_LOOP:  
    for(int n =0; n < NUM_STREAMS; n++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NUM_STREAMS max=NUM_STREAMS
        // clang-format on
        for (int k = 0; k < XF_NPPC; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=XF_NPPC max=XF_NPPC
            // clang-format on
            for (int i = 0; i < NO_EXPS; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NO_EXPS max=NO_EXPS
                // clang-format on
                for (int j = 0; j < (W_B_SIZE); j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=W_B_SIZE max=W_B_SIZE
                    // clang-format on
                    wr_hls_tmp[n][(i + k * NO_EXPS) * W_B_SIZE + j] = wr_hls[n][(i + k * NO_EXPS) * W_B_SIZE + j];
                }
            }
        }
    }

    const uint16_t pt[NUM_STREAMS] = {STRM1_ROWS, STRM2_ROWS, STRM3_ROWS, STRM4_ROWS};
    uint16_t max = STRM1_ROWS;
    for (int i = 1; i < NUM_STREAMS; i++) {
        if (pt[i] > max) max = pt[i];
    }

    const uint16_t TC = tot_rows / max;
    uint32_t addrbound, wr_addrbound, num_rows;

    int strm_id = 0, stream_idx = 0, slice_idx = 0;
    bool eof_awb[NUM_STREAMS] = {0};
    bool eof_tm[NUM_STREAMS] = {0};
    bool eof_aec[NUM_STREAMS] = {0};

    uint32_t rd_offset1 = 0, rd_offset2 = 0, rd_offset3 = 0, rd_offset4 = 0;
    uint32_t wr_offset1 = 0, wr_offset2 = 0, wr_offset3 = 0, wr_offset4 = 0;
    uint32_t wr_offset1_ir = 0, wr_offset2_ir = 0, wr_offset3_ir = 0, wr_offset4_ir = 0;

TOTAL_ROWS_LOOP:
    for (int r = 0; r < tot_rows;) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=(XF_HEIGHT/SLICE_HEIGHT)*NUM_STREAMS max=(XF_HEIGHT/SLICE_HEIGHT)*NUM_STREAMS
        // clang-format on

        // Compute no.of rows to process
        if (rem_rows[stream_idx] / RD_MULT > pt[stream_idx]) { // Check number for remaining rows of 1 interleaved image
            num_rows = pt[stream_idx];
            eof_awb[stream_idx] = 0; // 1 interleaved image/stream is not done
            eof_tm[stream_idx] = 0;
            eof_aec[stream_idx] = 0;
        } else {
            num_rows = rem_rows[stream_idx] / RD_MULT;
            eof_awb[stream_idx] = 1; // 1 interleaved image/stream done
            eof_tm[stream_idx] = 1;
            eof_aec[stream_idx] = 1;
        }

        strm_id = stream_idx;

        if (stream_idx == 0 && num_rows > 0) {
            Streampipeline(img_inp1 + rd_offset1, img_out1 + wr_offset1, img_out_ir1 + wr_offset1_ir, lut1, num_rows,
                           height_arr[stream_idx], width_arr[stream_idx], STRM1_ROWS, dgam_params_tmp, hist0_awb,
                           hist1_awb, igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp,
                           R_IR_C1_wgts_tmp, R_IR_C2_wgts_tmp, B_at_R_wgts_tmp, IR_at_R_wgts_tmp, IR_at_B_wgts_tmp,
                           sub_wgts_tmp, dcp_params_12to16_tmp, hist0_aec, hist1_aec, flag_aec, eof_aec, omin_r, omax_r,
                           omin_w, omax_w, demo_buffs, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, mean1, mean2,
                           L_max1, L_max2, L_min1, L_min2, c1_tmp, c2_tmp, flag_tm, eof_tm, stream_idx, slice_idx,
                           acc_sum, ccm_config_1_tmp, ccm_config_2_tmp);

            rd_offset1 += (RD_MULT * num_rows * ((width_arr[stream_idx] + RD_ADD) >> XF_BITSHIFT(XF_NPPC))) / 4;

            if (NUM_SLICES != 1 && slice_idx == 0) {
                wr_offset1 += (((num_rows - 7) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset1_ir += (((num_rows - 3) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            } else {
                wr_offset1 += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset1_ir += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            }

        } else if (stream_idx == 1 && num_rows > 0) {
            Streampipeline(img_inp2 + rd_offset2, img_out2 + wr_offset2, img_out_ir2 + wr_offset2_ir, lut2, num_rows,
                           height_arr[stream_idx], width_arr[stream_idx], STRM2_ROWS, dgam_params_tmp, hist0_awb,
                           hist1_awb, igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp,
                           R_IR_C1_wgts_tmp, R_IR_C2_wgts_tmp, B_at_R_wgts_tmp, IR_at_R_wgts_tmp, IR_at_B_wgts_tmp,
                           sub_wgts_tmp, dcp_params_12to16_tmp, hist0_aec, hist1_aec, flag_aec, eof_aec, omin_r, omax_r,
                           omin_w, omax_w, demo_buffs, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, mean1, mean2,
                           L_max1, L_max2, L_min1, L_min2, c1_tmp, c2_tmp, flag_tm, eof_tm, stream_idx, slice_idx,
                           acc_sum, ccm_config_1_tmp, ccm_config_2_tmp);

            rd_offset2 += (RD_MULT * num_rows * ((width_arr[stream_idx] + RD_ADD) >> XF_BITSHIFT(XF_NPPC))) / 4;

            if (NUM_SLICES != 1 && slice_idx == 0) {
                wr_offset2 += (((num_rows - 7) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset2_ir += (((num_rows - 3) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            } else {
                wr_offset2 += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset2_ir += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            }

        } else if (stream_idx == 2 && num_rows > 0) {
            Streampipeline(img_inp3 + rd_offset3, img_out3 + wr_offset3, img_out_ir3 + wr_offset3_ir, lut3, num_rows,
                           height_arr[stream_idx], width_arr[stream_idx], STRM3_ROWS, dgam_params_tmp, hist0_awb,
                           hist1_awb, igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp,
                           R_IR_C1_wgts_tmp, R_IR_C2_wgts_tmp, B_at_R_wgts_tmp, IR_at_R_wgts_tmp, IR_at_B_wgts_tmp,
                           sub_wgts_tmp, dcp_params_12to16_tmp, hist0_aec, hist1_aec, flag_aec, eof_aec, omin_r, omax_r,
                           omin_w, omax_w, demo_buffs, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, mean1, mean2,
                           L_max1, L_max2, L_min1, L_min2, c1_tmp, c2_tmp, flag_tm, eof_tm, stream_idx, slice_idx,
                           acc_sum, ccm_config_1_tmp, ccm_config_2_tmp);

            rd_offset3 += (RD_MULT * num_rows * ((width_arr[stream_idx] + RD_ADD) >> XF_BITSHIFT(XF_NPPC))) / 4;

            if (NUM_SLICES != 1 && slice_idx == 0) {
                wr_offset3 += (((num_rows - 7) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset3_ir += (((num_rows - 3) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;

            } else {
                wr_offset3 += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset3_ir += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            }

        } else if (stream_idx == 3 && num_rows > 0) {
            Streampipeline(img_inp4 + rd_offset4, img_out4 + wr_offset4, img_out_ir4 + wr_offset4_ir, lut4, num_rows,
                           height_arr[stream_idx], width_arr[stream_idx], STRM4_ROWS, dgam_params_tmp, hist0_awb,
                           hist1_awb, igain_0, igain_1, flag_awb, eof_awb, array_params, gamma_lut_tmp, wr_hls_tmp,
                           R_IR_C1_wgts_tmp, R_IR_C2_wgts_tmp, B_at_R_wgts_tmp, IR_at_R_wgts_tmp, IR_at_B_wgts_tmp,
                           sub_wgts_tmp, dcp_params_12to16_tmp, hist0_aec, hist1_aec, flag_aec, eof_aec, omin_r, omax_r,
                           omin_w, omax_w, demo_buffs, rgbir_buffs, rgbir_ir_buffs, rgbir_wgt_buffs, mean1, mean2,
                           L_max1, L_max2, L_min1, L_min2, c1_tmp, c2_tmp, flag_tm, eof_tm, stream_idx, slice_idx,
                           acc_sum, ccm_config_1_tmp, ccm_config_2_tmp);

            rd_offset4 += (RD_MULT * num_rows * ((width_arr[stream_idx] + RD_ADD) >> XF_BITSHIFT(XF_NPPC))) / 4;

            if (NUM_SLICES != 1 && slice_idx == 0) {
                wr_offset4 += (((num_rows - 7) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset4_ir += (((num_rows - 3) * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            } else {
                wr_offset4 += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / OUT_PORT) * CH_TYPE;
                wr_offset4_ir += ((num_rows * (width_arr[stream_idx] >> XF_BITSHIFT(XF_NPPC))) / 4) * 1;
            }
        }
        // Update remaining rows to process
        rem_rows[stream_idx] = rem_rows[stream_idx] - num_rows * RD_MULT;

        // Next stream selection
        if (stream_idx == NUM_STREAMS - 1) {
            stream_idx = 0;
            slice_idx++;

        } else {
            stream_idx++;
        }

        // Update total rows to process
        r += num_rows * RD_MULT;
    } // TOTAL_ROWS_LOOP

    return;

} // void isppipeline_accel
} // extern "C"
