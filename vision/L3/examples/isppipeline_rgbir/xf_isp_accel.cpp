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

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
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
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void fifo_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
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
	xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in> impop(height, width);
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
                             XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(demosaic_out, impop, hist0, thresh, inputMin,
                                                                       inputMax, outputMin, outputMax);
        xf::cv::AWBNormalization<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, WB_TYPE, HIST_SIZE,
                                 XFCVDEPTH_ltm_in, XFCVDEPTH_ltm_in>(impop, ltm_in, hist1, thresh, inputMin, inputMax,
                                                                     outputMin, outputMax);
    } else {
        xf::cv::AWBChannelGain<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XFCVDEPTH_demosaic_out,
                               XFCVDEPTH_ltm_in>(demosaic_out, impop, thresh, gain0);
        xf::cv::AWBGainUpdate<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XFCVDEPTH_ltm_in, XFCVDEPTH_ltm_in>(
            impop, ltm_in, thresh, gain1);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void function_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
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
        fifo_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(
            demosaic_out, ltm_in, hist0, hist1, gain0, gain1, height, width, thresh);
    } else {
        fifo_copy<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(
            demosaic_out, ltm_in, height, width);
    }
}

void ISPpipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                 ap_uint<OUTPUT_PTR_WIDTH>* ir_img_out,
                 unsigned short height,
                 unsigned short width,
                 uint32_t hist0[3][HIST_SIZE],
                 uint32_t hist1[3][HIST_SIZE],
                 int gain0[3],
                 int gain1[3],
                 uint16_t rgain,
                 uint16_t bgain,
                 char R_IR_C1_wgts[25],
                 char R_IR_C2_wgts[25],
                 char B_at_R_wgts[25],
                 char IR_at_R_wgts[9],
                 char IR_at_B_wgts[9],
                 char sub_wgts[4],
                 unsigned char gamma_lut[256 * 3],
                 unsigned char mode_reg,
                 uint16_t pawb,
                 unsigned short ggain) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput> imgInput(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInputCopy1> imgInputCopy1(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInputCopy2> imgInputCopy2(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_fullir_out> fullir_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_rggb_out> rggb_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_gain_out> gain_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_demosaic_out> demosaic_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_demoOut_final> demoOut_final(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ltm_in> ltm_in(height, width);
    xf::cv::Mat<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH__dst> _dst(height, width);
    xf::cv::Mat<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_aecin> aecin(height, width);
    xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH__imgOutput> _imgOutput(height, width);
//

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    const int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX));

    float thresh = (float)pawb / 256;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;

    float mul_fact = (inputMax / (inputMax - BLACK_LEVEL));

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput>(img_inp,
                                                                                                       imgInput);

    xf::cv::rgbir2bayer<FILTERSIZE1, FILTERSIZE2, XF_BAYER_PATTERN, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX,
                        XF_BORDER_CONSTANT, XF_USE_URAM, XF_CV_DEPTH_imgInput, XF_CV_DEPTH_rggb_out,
                        XF_CV_DEPTH_fullir_out, XF_CV_DEPTH_3XWIDTH>(
        imgInput, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, rggb_out, fullir_out);

    xf::cv::gaincontrol<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_rggb_out, XF_CV_DEPTH_gain_out>(
        rggb_out, gain_out, rgain, bgain, ggain, bformat);
    xf::cv::demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XF_CV_DEPTH_gain_out,
                        XF_CV_DEPTH_demosaic_out>(gain_out, demosaic_out, bformat);

    function_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_demosaic_out, XF_CV_DEPTH_ltm_in>(
        demosaic_out, ltm_in, hist0, hist1, gain0, gain1, height, width, mode_reg, thresh);

    if (OUT_TYPE == XF_8UC3) {
        fifo_copy<OUT_TYPE, XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ltm_in, XF_CV_DEPTH_aecin>(
            ltm_in, aecin, height, width);
    } else {
        xf::cv::xf_QuatizationDithering<OUT_TYPE, XF_LTM_T, XF_HEIGHT, XF_WIDTH, 256, Q_VAL, XF_NPPCX, XF_USE_URAM,
                                        XF_CV_DEPTH_ltm_in, XF_CV_DEPTH_aecin>(ltm_in, aecin);
    }
    xf::cv::gammacorrection<XF_LTM_T, XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_aecin, XF_CV_DEPTH__dst>(
        aecin, _dst, gamma_lut);
    xf::cv::rgb2yuyv<XF_LTM_T, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH__dst, XF_CV_DEPTH__imgOutput>(
        _dst, _imgOutput);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH__imgOutput>(_imgOutput,
                                                                                                           img_out);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_fullir_out>(fullir_out,
                                                                                                          ir_img_out);
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
                       ap_uint<OUTPUT_PTR_WIDTH>* ir_img_out,
                       int height,
                       int width,
                       uint16_t rgain,
                       uint16_t bgain,
                       char R_IR_C1_wgts[25],
                       char R_IR_C2_wgts[25],
                       char B_at_R_wgts[25],
                       char IR_at_R_wgts[9],
                       char IR_at_B_wgts[9],
                       char sub_wgts[4],
                       unsigned char gamma_lut[256 * 3],
                       unsigned char mode_reg,
                       uint16_t pawb,
                       unsigned short bformat,
                       unsigned short ggain) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=ir_img_out  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=R_IR_C1_wgts  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=R_IR_C2_wgts  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=B_at_R_wgts  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=IR_at_R_wgts  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=IR_at_B_wgts  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=sub_wgts  offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi     port=gamma_lut  offset=slave bundle=gmem6

// clang-format on

// clang-format off
#pragma HLS ARRAY_PARTITION variable=hist0_awb complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1_awb complete dim=1

    // clang-format on

    if (!flag) {
        ISPpipeline(img_inp, img_out, ir_img_out, height, width, hist0_awb, hist1_awb, igain_0, igain_1, rgain, bgain,
                    R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, gamma_lut, mode_reg,
                    pawb, bformat, ggain);
        flag = 1;

    } else {
        ISPpipeline(img_inp, img_out, ir_img_out, height, width, hist1_awb, hist0_awb, igain_1, igain_0, rgain, bgain,
                    R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, gamma_lut, mode_reg,
                    pawb, bformat, ggain);
        flag = 0;
    }
}
}
