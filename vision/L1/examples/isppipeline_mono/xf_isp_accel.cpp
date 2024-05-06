/* Copyright 2023 Xilinx, Inc.
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

static bool flag = 0;

#define CLAHE_T                                                                                            \
    xf::cv::clahe::CLAHEImpl<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, CLIPLIMIT, TILES_Y_MAX, TILES_X_MAX, \
                             TILES_Y_MIN, TILES_X_MIN, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>

static constexpr int HIST_COUNTER_BITS = CLAHE_T::HIST_COUNTER_BITS;
static constexpr int CLIP_COUNTER_BITS = CLAHE_T::CLIP_COUNTER_BITS;

static ap_uint<HIST_COUNTER_BITS> _lut1[TILES_Y_MAX][TILES_X_MAX][(XF_NPIXPERCYCLE(XF_NPPCX) << 1)]
                                       [1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX)];
static ap_uint<HIST_COUNTER_BITS> _lut2[TILES_Y_MAX][TILES_X_MAX][(XF_NPIXPERCYCLE(XF_NPPCX) << 1)]
                                       [1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX)];
static ap_uint<CLIP_COUNTER_BITS> _clipCounter[TILES_Y_MAX][TILES_X_MAX];

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN, int XFCVDEPTH_OUT>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& ltm_in,
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
    #pragma HLS pipeline
            // clang-format on
            XF_TNAME(SRC_T, NPC) tmp_src;
            tmp_src = demosaic_out.read(readindex++);
            ltm_in.write(writeindex++, tmp_src);
        }
    }
}

template <int WINDOW_SIZE_V,
          int XF_BORDER_REPLICATE_V,
          int IN_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_USE_URAM_V,
          int XF_CV_DEPTH_IN_2_V,
          int XF_CV_DEPTH_DPC_OUT_V>
void function_medianblur(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_2_V>& in_img,
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_DPC_OUT_V>& median_out_mat,
    uint16_t height,
    uint16_t width,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_MEDIAN_EN_INDEX, XF_MEDIAN_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_2_V, XF_CV_DEPTH_DPC_OUT_V>(
            in_img, median_out_mat, height, width);
    } else {
        xf::cv::medianBlur<WINDOW_SIZE_V, XF_BORDER_REPLICATE, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V,
                           XF_USE_URAM_V, XF_CV_DEPTH_IN_2_V, XF_CV_DEPTH_DPC_OUT_V>(in_img, median_out_mat);
    }
}

template <int IN_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_CV_DEPTH_LSC_OUT_V,
          int XF_CV_DEPTH_GAIN_OUT_V>
void function_gaincontrol_mono(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V>& in_img,
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>& gaincontrol_mono_out_mat,
    uint32_t gain_control_config_1,
    uint16_t height,
    uint16_t width,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_GAIN_EN_INDEX, XF_GAIN_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V,
                  XF_CV_DEPTH_LSC_OUT_V>(in_img, gaincontrol_mono_out_mat, height, width);
    } else {
        xf::cv::gaincontrol_mono<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V,
                                 XF_CV_DEPTH_GAIN_OUT_V>(in_img, gaincontrol_mono_out_mat, gain_control_config_1);
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_CV_DEPTH_GAIN_OUT_V,
          int XF_CV_DEPTH_OUT_V>
void function_gammacorrection(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>& gaincontrol_mono_out_mat,
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& gammacorrection_out_mat,
    unsigned char gamma_lut[256],
    uint16_t height,
    uint16_t width,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_GAMMA_EN_INDEX, XF_GAMMA_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V,
                  XF_CV_DEPTH_GAIN_OUT_V>(gaincontrol_mono_out_mat, gammacorrection_out_mat, height, width);
    } else {
        xf::cv::gammacorrection<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V,
                                XF_CV_DEPTH_OUT_V>(gaincontrol_mono_out_mat, gammacorrection_out_mat, gamma_lut);
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_CV_DEPTH_GAIN_OUT_V,
          int XF_CV_DEPTH_OUT_V>
void function_clahe(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>& clahe_out_mat,
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& xf_QuatizationDithering_out_mat,
    ap_uint<HIST_COUNTER_BITS> _lutw[TILES_Y_MAX][TILES_X_MAX][(XF_NPIXPERCYCLE(XF_NPPCX) << 1)]
                                    [1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX)],
    ap_uint<HIST_COUNTER_BITS> _lutr[TILES_Y_MAX][TILES_X_MAX][(XF_NPIXPERCYCLE(XF_NPPCX) << 1)]
                                    [1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX)],
    ap_uint<CLIP_COUNTER_BITS> _clipCounter[TILES_Y_MAX][TILES_X_MAX],
    uint16_t height,
    uint16_t width,
    int clip,
    int tilesY,
    int tilesX,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_CLAHE_EN_INDEX, XF_CLAHE_EN_INDEX);
// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    CLAHE_T obj;
    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V,
                  XF_CV_DEPTH_GAIN_OUT_V>(xf_QuatizationDithering_out_mat, clahe_out_mat, height, width);
    } else {
        obj.process(clahe_out_mat, xf_QuatizationDithering_out_mat, _lutw, _lutr, _clipCounter, height, width, clip,
                    tilesY, tilesX);
    }
}

template <int OUT_TYPE_V,
          int XF_GTM_T_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int SCALE_FACTOR_V,
          int MAX_REPRESENTED_VALUE_V,
          int XF_NPPCX_V,
          bool XF_USE_URAM_V,
          int XF_CV_DEPTH_IN_V,
          int XF_CV_DEPTH_OUT_V>
void function_xf_QuatizationDithering(
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V>& gammacorrection_out_mat,
    xf::cv::Mat<XF_GTM_T_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& xf_QuatizationDithering_out_mat,
    uint16_t height,
    uint16_t width,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_TM_EN_INDEX, XF_TM_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<OUT_TYPE_V, XF_GTM_T_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_IN_V>(
            gammacorrection_out_mat, xf_QuatizationDithering_out_mat, height, width);
    } else {
        xf::cv::xf_QuatizationDithering<OUT_TYPE_V, XF_GTM_T_V, XF_HEIGHT_V, XF_WIDTH_V, SCALE_FACTOR_V,
                                        MAX_REPRESENTED_VALUE_V, XF_NPPCX_V, XF_USE_URAM_V, XF_CV_DEPTH_IN_V,
                                        XF_CV_DEPTH_OUT_V>(gammacorrection_out_mat, xf_QuatizationDithering_out_mat);
    }
}

template <int IN_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int MUL_VALUE_WIDTH_V,
          int FL_POS_V,
          int USE_DSP_V,
          int XF_CV_DEPTH_LSC_OUT_V,
          int XF_CV_DEPTH_GAIN_OUT_V>
void function_blackLevelCorrection(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V>& in_img,
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>& blackLevelCorrection_out_mat,
    unsigned int blc_config_1,
    unsigned int blc_config_2,
    uint16_t height,
    uint16_t width,
    unsigned int funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_BLC_EN_INDEX, XF_BLC_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V,
                  XF_CV_DEPTH_LSC_OUT_V>(in_img, blackLevelCorrection_out_mat, height, width);
    } else {
        xf::cv::blackLevelCorrection<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, MUL_VALUE_WIDTH_V, FL_POS_V,
                                     USE_DSP_V, XF_CV_DEPTH_GAIN_OUT_V, XF_CV_DEPTH_GAIN_OUT_V>(
            in_img, blackLevelCorrection_out_mat, blc_config_2, blc_config_1);
    }
}

void ISPpipeline(InVideoStrm_t& s_axis_video,
                 OutVideoStrm_t& m_axis_video,
                 uint16_t height,
                 uint16_t width,

                 uint16_t new_height,
                 uint16_t new_width,
#if XF_BLC_EN
                 unsigned int blc_config_1,
                 unsigned int blc_config_2,
#endif

#if XF_GAIN_EN
                 uint32_t gain_control_config_1,
#endif
#if XF_GAMMA_EN
                 unsigned char gamma_lut[256],
#endif
#if XF_CLAHE_EN
                 ap_uint<HIST_COUNTER_BITS> _lutw[TILES_Y_MAX][TILES_X_MAX][(XF_NPIXPERCYCLE(XF_NPPCX) << 1)]
                                                 [1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX)],
                 ap_uint<HIST_COUNTER_BITS> _lutr[TILES_Y_MAX][TILES_X_MAX][(XF_NPIXPERCYCLE(XF_NPPCX) << 1)]
                                                 [1 << XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX)],
                 ap_uint<CLIP_COUNTER_BITS> _clipCounter[TILES_Y_MAX][TILES_X_MAX],
                 int clip,
                 int tilesY,
                 int tilesX,
#endif
                 unsigned int funcs_bypass_config) {
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT> in_img(height, width);

    AXIvideo2xfMat<AXI_WIDTH_IN, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT>(s_axis_video, in_img);

// blacklevel functioncall
#if XF_BLC_EN
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT> blackLevelCorrection_out_mat(height,
                                                                                                           width);

    function_blackLevelCorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, MUL_VALUE_WIDTH, FL_POS, USE_DSP,
                                  XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_GAIN_OUT>(
        in_img, blackLevelCorrection_out_mat, blc_config_1, blc_config_2, height, width, funcs_bypass_config);
#endif

#if XF_MEDIAN_EN
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT> median_out_mat(height, width);
#if XF_BLC_EN
    function_medianblur<WINDOW_SIZE, XF_BORDER_REPLICATE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM,
                        XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_GAIN_OUT>(blackLevelCorrection_out_mat, median_out_mat,
                                                                    height, width, funcs_bypass_config);
#else
    function_medianblur<WINDOW_SIZE, XF_BORDER_REPLICATE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM,
                        XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_GAIN_OUT>(in_img, median_out_mat, height, width,
                                                                    funcs_bypass_config);
#endif
#endif

#if XF_GAIN_EN
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT> gaincontrol_mono_out_mat(height, width);
#if XF_MEDIAN_EN

    function_gaincontrol_mono<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT, XF_CV_DEPTH_GAIN_OUT>(
        median_out_mat, gaincontrol_mono_out_mat, gain_control_config_1, height, width, funcs_bypass_config);
#elif XF_BLC_EN

    function_gaincontrol_mono<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT, XF_CV_DEPTH_GAIN_OUT>(
        blackLevelCorrection_out_mat, gaincontrol_mono_out_mat, gain_control_config_1, height, width,
        funcs_bypass_config);
#else

    function_gaincontrol_mono<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT, XF_CV_DEPTH_GAIN_OUT>(
        in_img, gaincontrol_mono_out_mat, gain_control_config_1, height, width, funcs_bypass_config);

#endif
#endif

    // QnD section

    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> xf_QuatizationDithering_out_mat(height,
                                                                                                          width);
#if XF_GAIN_EN
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        gaincontrol_mono_out_mat, xf_QuatizationDithering_out_mat, height, width);
#elif T_16U
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        gaincontrol_mono_out_mat, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#elif XF_MEDIAN_EN
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        median_out_mat, xf_QuatizationDithering_out_mat, height, width);
#elif T_16U
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        median_out_mat, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#elif XF_BLC_EN
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        blackLevelCorrection_out_mat, xf_QuatizationDithering_out_mat, height, width);
#elif T_16U
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        blackLevelCorrection_out_mat, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#else
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        in_img, xf_QuatizationDithering_out_mat, height, width);
#elif T_16U
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        in_img, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#endif

#if XF_CLAHE_EN
    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> clahe_out_mat(height, width);
    function_clahe<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_OUT>(
        clahe_out_mat, xf_QuatizationDithering_out_mat, _lutw, _lutr, _clipCounter, height, width, clip, tilesY, tilesX,
        funcs_bypass_config);

#endif

#if XF_GAMMA_EN
    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> gammacorrection_out_mat(height, width);
#if XF_CLAHE_EN
    function_gammacorrection<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_OUT>(
        clahe_out_mat, gammacorrection_out_mat, gamma_lut, height, width, funcs_bypass_config);
#else
    function_gammacorrection<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_OUT>(
        xf_QuatizationDithering_out_mat, gammacorrection_out_mat, gamma_lut, height, width, funcs_bypass_config);
#endif
#endif
// resize section

#if XF_GAMMA_EN
    xf::cv::Mat<XF_GTM_T, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_CV_DEPTH_LUT_OUT> resize_out_mat(new_height,
                                                                                                   new_width);

    xf::cv::resize<INTERPOLATION, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_USE_URAM,
                   MAXDOWNSCALE, XF_CV_DEPTH_LUT_IN, XF_CV_DEPTH_LUT_OUT>(gammacorrection_out_mat, resize_out_mat);
#elif XF_CLAHE_EN
    xf::cv::Mat<XF_GTM_T, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_CV_DEPTH_LUT_OUT> resize_out_mat(new_height,
                                                                                                   new_width);

    xf::cv::resize<INTERPOLATION, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_USE_URAM,
                   MAXDOWNSCALE, XF_CV_DEPTH_LUT_IN, XF_CV_DEPTH_LUT_OUT>(clahe_out_mat, resize_out_mat);
#else
    xf::cv::Mat<XF_GTM_T, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_CV_DEPTH_LUT_OUT> resize_out_mat(new_height,
                                                                                                   new_width);

    xf::cv::resize<INTERPOLATION, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_USE_URAM,
                   MAXDOWNSCALE, XF_CV_DEPTH_LUT_IN, XF_CV_DEPTH_LUT_OUT>(xf_QuatizationDithering_out_mat,
                                                                          resize_out_mat);
#endif

    xfMat2AXIvideo<AXI_WIDTH_OUT, XF_GTM_T, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_CV_DEPTH_LUT_OUT>(resize_out_mat,
                                                                                                      m_axis_video);
}

void ISPPipeline_accel(InVideoStrm_t& s_axis_video,
                       OutVideoStrm_t& m_axis_video,
                       unsigned int common_config,
#if XF_BLC_EN
                       unsigned int blc_config_1,
                       unsigned int blc_config_2,
#endif

                       unsigned int resize_config,

#if XF_GAIN_EN
                       unsigned int gain_control_config_1,
#endif
#if XF_GAMMA_EN
                       unsigned char gamma_lut[256],
#endif
#if XF_CLAHE_EN
                       int clahe_config_1,
                       int clahe_config_2,
#endif
                       unsigned int& pipeline_config_info,
                       unsigned int& max_supported_size,
                       unsigned int& funcs_available,
                       unsigned int& funcs_bypassable,
                       unsigned int funcs_bypass_config) {
// clang-format off
#pragma HLS INTERFACE axis port=s_axis_video register
#pragma HLS INTERFACE axis port=m_axis_video register
#pragma HLS INTERFACE s_axilite port=common_config bundle=CTRL offset=0x00010

#pragma HLS INTERFACE s_axilite port=resize_config bundle=CTRL offset=0x00078 

#if XF_GAIN_EN
#pragma HLS INTERFACE s_axilite port=gain_control_config_1 bundle=CTRL offset=0x00038
#endif
#if XF_GAMMA_EN
#pragma HLS INTERFACE s_axilite port=gamma_lut	bundle=CTRL offset=0x02000
#endif

#if XF_BLC_EN
#pragma HLS INTERFACE s_axilite port=blc_config_1 bundle=CTRL offset=0x00028 
#pragma HLS INTERFACE s_axilite port=blc_config_2 bundle=CTRL offset=0x00030 
#endif

#pragma HLS INTERFACE s_axilite  port=pipeline_config_info bundle=CTRL offset=0x00080 
#pragma HLS INTERFACE mode=ap_none port=pipeline_config_info 
#pragma HLS INTERFACE s_axilite  port=max_supported_size bundle=CTRL offset=0x00090 
#pragma HLS INTERFACE mode=ap_none port=max_supported_size 
#pragma HLS INTERFACE s_axilite  port=funcs_available bundle=CTRL offset=0x000A0 
#pragma HLS INTERFACE mode=ap_none port=funcs_available 
#pragma HLS INTERFACE s_axilite  port=funcs_bypassable bundle=CTRL offset=0x000B0 
#pragma HLS INTERFACE mode=ap_none port=funcs_bypassable 
#pragma HLS INTERFACE s_axilite  port=funcs_bypass_config bundle=CTRL offset=0x000C0
#pragma HLS INTERFACE s_axilite  port=return bundle=CTRL
    //   clang-format on

// extracting height and width  // 
uint16_t  height,width;
height = (uint16_t)(common_config >> 16);
width = (uint16_t)(common_config );


// extracting height and width  // 
uint16_t  new_height,new_width;
new_height = (uint16_t)(resize_config >> 16);
new_width = (uint16_t)(resize_config);

#if XF_CLAHE_EN
#pragma HLS INTERFACE s_axilite  port=clahe_config_1 bundle=CTRL offset=0x00068
#pragma HLS INTERFACE s_axilite  port=clahe_config_2 bundle=CTRL offset=0x00070
#pragma HLS ARRAY_PARTITION variable=_lut1 dim=3 complete
#pragma HLS ARRAY_PARTITION variable=_lut2 dim=3 complete
int clip, tilesX, tilesY;
clip = clahe_config_1;
tilesX = (uint16_t)(clahe_config_2 >> 16);
tilesY = (uint16_t)(clahe_config_2 );


#endif


max_supported_size 	= (XF_WIDTH << MAX_WIDTH_INDEX) | (XF_HEIGHT << MAX_HEIGHT_INDEX) ;

pipeline_config_info =  (IN_C_TYPE << IN_C_TYPE_INDEX) | (IN_BW_MODE << IN_BW_MODE_INDEX) | (OUT_C_TYPE << OUT_C_TYPE_INDEX) | (OUT_BW_MODE << OUT_BW_MODE_INDEX) | (NPPCX << NPPCX_INDEX) | (NUM_STREAMS << NUM_STREAMS_INDEX);

funcs_available  = (XF_BLC_EN << XF_BLC_EN_INDEX) | (XF_MEDIAN_EN << XF_MEDIAN_EN_INDEX) | (XF_GAIN_EN << XF_GAIN_EN_INDEX) | (XF_GAMMA_EN << XF_GAMMA_EN_INDEX) | (XF_TM_EN << XF_TM_EN_INDEX) | (XF_TM_TYPE << XF_TM_TYPE_INDEX) | (XF_CLAHE_EN << XF_CLAHE_EN_INDEX);
funcs_bypassable  = (XF_BLC_BYPASS_EN << XF_BLC_EN_INDEX) | (XF_MEDIAN_BYPASS_EN << XF_MEDIAN_EN_INDEX) | (XF_GAIN_BYPASS_EN << XF_GAIN_EN_INDEX) | (XF_GAMMA_BYPASS_EN << XF_GAMMA_EN_INDEX) | (XF_CLAHE_BYPASS_EN << XF_CLAHE_EN_INDEX);
if (!flag) {
        ISPpipeline(s_axis_video
                    , m_axis_video
                    , height
                    , width
                    , new_height
                    , new_width,
                    #if XF_BLC_EN
                    blc_config_1, blc_config_2,
                    #endif
                    #if XF_GAIN_EN
		            gain_control_config_1,
                    #endif
                    #if XF_GAMMA_EN
		            gamma_lut,
                    #endif
                    #if XF_CLAHE_EN
                    _lut1, _lut2, _clipCounter, clip, tilesX, tilesY,
                    #endif
		            funcs_bypass_config);
	flag = 1;
    } else {
        ISPpipeline(s_axis_video
                    , m_axis_video
                    , height
                    , width
                    , new_height
                    , new_width,
                    #if XF_BLC_EN
                    blc_config_1, blc_config_2,
                    #endif
                    #if XF_GAIN_EN
		            gain_control_config_1,
                    #endif
                    #if XF_GAMMA_EN
		            gamma_lut,
                    #endif
                    #if XF_CLAHE_EN
                    _lut2, _lut1, _clipCounter, clip, tilesX, tilesY,
                    #endif
		            funcs_bypass_config);
        flag = 0;
        }

}

