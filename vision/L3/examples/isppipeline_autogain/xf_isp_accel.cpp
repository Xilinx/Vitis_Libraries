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

static bool flag = 0;

static uint32_t hist0_awb[3][HIST_SIZE] = {0};
static uint32_t hist1_awb[3][HIST_SIZE] = {0};
// static uint32_t histogram0[1][256] = {0};
// static uint32_t histogram1[1][256] = {0};
static int igain_0[3] = {0};
static int igain_1[3] = {0};

uint16_t gain0_agc[3] = {MAX_PIX_VAL, MAX_PIX_VAL, MAX_PIX_VAL};
uint16_t gain1_agc[3] = {MAX_PIX_VAL, MAX_PIX_VAL, MAX_PIX_VAL};

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& ltm_in,
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
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void fifo_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& demosaic_out,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& ltm_in,
              uint32_t hist0[3][HIST_SIZE],
              uint32_t hist1[3][HIST_SIZE],
              int gain0[3],
              int gain1[3],
              unsigned short height,
              unsigned short width,
              float thresh_awb) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on	
	xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_IN_1> impop(height, width);
    uint32_t thresh = (int)(thresh_awb * 256); // thresh_awb int Q24_8 format change to Q16_16 format

	float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
	
	// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    if (WB_TYPE) {
        xf::cv::AWBhistogram<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, WB_TYPE, HIST_SIZE,
                             XFCVDEPTH_IN_1, XFCVDEPTH_IN_1>(demosaic_out, impop, hist0, thresh, inputMin, inputMax,
                                                             outputMin, outputMax);
        xf::cv::AWBNormalization<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, WB_TYPE, HIST_SIZE, XFCVDEPTH_IN_1,
                                 XFCVDEPTH_OUT_1>(impop, ltm_in, hist1, thresh, inputMin, inputMax, outputMin,
                                                  outputMax);
    } else {
        xf::cv::AWBChannelGain<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XFCVDEPTH_IN_1, XFCVDEPTH_IN_1>(
            demosaic_out, impop, thresh, gain0);
        xf::cv::AWBGainUpdate<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            impop, ltm_in, thresh, gain1);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN_1, int XFCVDEPTH_OUT_1>
void function_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& demosaic_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& ltm_in,
                  uint32_t hist0[3][HIST_SIZE],
                  uint32_t hist1[3][HIST_SIZE],
                  int gain0[3],
                  int gain1[3],
                  unsigned short height,
                  unsigned short width,
                  unsigned char mode_reg,
                  float thresh) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<8> mode = (ap_uint<8>)mode_reg;
    ap_uint<1> mode_flg = mode.range(0, 0);

    if (mode_flg) {
        fifo_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            demosaic_out, ltm_in, hist0, hist1, gain0, gain1, height, width, thresh);
    } else {
        fifo_copy<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
            demosaic_out, ltm_in, height, width);
    }
}

void ISPpipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                 unsigned short height,
                 unsigned short width,
                 uint32_t hist0[3][HIST_SIZE],
                 uint32_t hist1[3][HIST_SIZE],
                 int gain0[3],
                 int gain1[3],
                 uint16_t gain_agc_0[3],
                 uint16_t gain_agc_1[3],
                 unsigned char gamma_lut[256 * 3],
                 unsigned char mode_reg,
                 uint16_t pawb,
                 signed int ccm_config_1[3][3],
                 signed int ccm_config_2[3],
                 unsigned short bformat) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_0> imgInput1(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1> imgInput2(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_2> bpc_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_3> gain_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_0> demosaic_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> impop(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_2> ltm_in(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_3> lsc_out(height, width);
    xf::cv::Mat<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_4> _dst(height, width);
    xf::cv::Mat<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_5> aecin(height, width);
    xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_6> _imgOutput(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    const int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX));

    float thresh = (float)pawb / 256;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;

    float mul_fact = (inputMax / (inputMax - BLACK_LEVEL));
    unsigned int blc_config_1 = (int)(mul_fact * 65536); // mul_fact int Q16_16 format
    unsigned int blc_config_2 = BLACK_LEVEL;

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_0>(img_inp, imgInput1);
    xf::cv::blackLevelCorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 16, 15, 1, XF_CV_DEPTH_IN_0, XF_CV_DEPTH_IN_1>(
        imgInput1, imgInput2, blc_config_2, blc_config_1);
    // xf::cv::badpixelcorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, 0>(imgInput2, bpc_out);

    xf::cv::autogain<OUT_TYPE, XF_NPPCX, HIST_SIZE>(hist1, gain_agc_0);

    xf::cv::gaincontrol<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_3>(
        imgInput2, gain_out, gain_agc_1[0], gain_agc_1[2], gain_agc_1[1], bformat);
    xf::cv::demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XF_CV_DEPTH_IN_3, XF_CV_DEPTH_OUT_0>(
        gain_out, demosaic_out, bformat);

    function_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_0, XF_CV_DEPTH_OUT_2>(
        demosaic_out, ltm_in, hist0, hist1, gain0, gain1, height, width, mode_reg, thresh);

    xf::cv::colorcorrectionmatrix<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_2,
                                  XF_CV_DEPTH_OUT_3>(ltm_in, lsc_out, ccm_config_1, ccm_config_2);

    if (OUT_TYPE == XF_8UC3) {
        fifo_copy<OUT_TYPE, XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_3, XF_CV_DEPTH_OUT_5>(
            lsc_out, aecin, height, width);
    } else {
        xf::cv::xf_QuatizationDithering<OUT_TYPE, XF_LTM_T, XF_HEIGHT, XF_WIDTH, 256, Q_VAL, XF_NPPCX, XF_USE_URAM,
                                        XF_CV_DEPTH_OUT_3, XF_CV_DEPTH_OUT_5>(lsc_out, aecin);
    }
    xf::cv::gammacorrection<XF_LTM_T, XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_5, XF_CV_DEPTH_OUT_4>(
        aecin, _dst, gamma_lut);
    // ColorMat2AXIvideo<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX>(_dst, m_axis_video);
    xf::cv::rgb2yuyv<XF_LTM_T, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_4, XF_CV_DEPTH_OUT_6>(
        _dst, _imgOutput);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_6>(_imgOutput,
                                                                                                      img_out);
}
/*********************************************************************************
 * Function:    ISPPipeline_accel
 * Parameters:  input and output image pointers, image resolution
 * Return:
 * Description:
 **********************************************************************************/
extern "C" {
void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                       int height,
                       int width,
                       unsigned char gamma_lut[256 * 3],
                       unsigned char mode_reg,
                       uint16_t pawb,
                       signed int ccm_config_1[3][3],
                       signed int ccm_config_2[3],
                       uint16_t bformat) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2

#pragma HLS INTERFACE m_axi port=ccm_config_1     bundle=gmem3 offset=slave
#pragma HLS INTERFACE m_axi port=ccm_config_2     bundle=gmem4 offset=slave

// clang-format on

// clang-format off
#pragma HLS ARRAY_PARTITION variable=hist0_awb complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1_awb complete dim=1

    // clang-format on

    if (!flag) {
        ISPpipeline(img_inp, img_out, height, width, hist0_awb, hist1_awb, igain_0, igain_1, gain0_agc, gain1_agc,
                    gamma_lut, mode_reg, pawb, ccm_config_1, ccm_config_2, bformat);
        flag = 1;

    } else {
        ISPpipeline(img_inp, img_out, height, width, hist1_awb, hist0_awb, igain_1, igain_0, gain1_agc, gain0_agc,
                    gamma_lut, mode_reg, pawb, ccm_config_1, ccm_config_2, bformat);
        flag = 0;
    }
}
}
