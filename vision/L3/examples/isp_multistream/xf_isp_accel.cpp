/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_isp_types.h"

static uint32_t hist0_awb[NUM_STREAMS][3][HIST_SIZE] = {0};
static uint32_t hist1_awb[NUM_STREAMS][3][HIST_SIZE] = {0};
// static uint32_t histogram0[1][256] = {0};
// static uint32_t histogram1[1][256] = {0};
static int igain_0[3] = {0};
static int igain_1[3] = {0};

bool flag[NUM_STREAMS] = {0};

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC>& ltm_in,
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
            ltm_in.write(writeindex++, tmp_src);
        }
    }
}
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC>
void fifo_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& demosaic_out,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC>& ltm_in,
              uint32_t hist0[3][HIST_SIZE],
              uint32_t hist1[3][HIST_SIZE],
              int gain0[3],
              int gain1[3],
              unsigned short height,
              unsigned short width,
              float thresh) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on	
	xf::cv::Mat<XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> impop(height, width);

	float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1; // 65535.0f;
	
	// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    if (WB_TYPE) {
        xf::cv::AWBhistogram<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC, WB_TYPE, HIST_SIZE>(
            demosaic_out, impop, hist0, thresh, inputMin, inputMax, outputMin, outputMax);
        xf::cv::AWBNormalization<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC, WB_TYPE, HIST_SIZE>(
            impop, ltm_in, hist1, thresh, inputMin, inputMax, outputMin, outputMax);
    } else {
        xf::cv::AWBChannelGain<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC, 0>(demosaic_out, impop, thresh,
                                                                                      gain0);
        xf::cv::AWBGainUpdate<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC, 0>(impop, ltm_in, thresh, gain1);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC>
void function_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& demosaic_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC>& ltm_in,
                  uint32_t hist0[3][HIST_SIZE],
                  uint32_t hist1[3][HIST_SIZE],
                  int gain0[3],
                  int gain1[3],
                  unsigned short height,
                  unsigned short width,
                  // unsigned char mode_reg,
                  float thresh,
                  bool flag) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    // ap_uint<8> mode = (ap_uint<8>)mode_reg;
    // ap_uint<1> mode_flg = mode.range(0, 0);

    if (!flag) {
        fifo_awb<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(demosaic_out, ltm_in, hist0, hist1, gain0, gain1,
                                                                     height, width, thresh);

        flag = 1;

    } else {
        fifo_awb<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(demosaic_out, ltm_in, hist1, hist0, gain1, gain0,
                                                                     height, width, thresh);

        flag = 0;
    }
}

void Streampipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                    ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                    unsigned short height,
                    unsigned short width,
                    uint32_t hist0[3][HIST_SIZE],
                    uint32_t hist1[3][HIST_SIZE],
                    int gain0[3],
                    int gain1[3],
                    struct ispparams_config params,
                    unsigned char _gamma_lut[256 * 3],
                    bool flag) {
    xf::cv::Mat<XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> imgInput(height, width);
    xf::cv::Mat<XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> blc_out(height, width);
    xf::cv::Mat<XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> bpc_out(height, width);
    xf::cv::Mat<XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> gain_out(height, width);
    xf::cv::Mat<XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> demosaic_out(height, width);
    xf::cv::Mat<XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> impop(height, width);
    xf::cv::Mat<XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> ltm_in(height, width);
    xf::cv::Mat<XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> lsc_out(height, width);
    xf::cv::Mat<XF_LTM_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> _dst(height, width);
    xf::cv::Mat<XF_LTM_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC> aecin(height, width);
    xf::cv::Mat<XF_16UC1, STRM_HEIGHT, XF_WIDTH, XF_NPPC> imgOutput(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    const int Q_VAL = 1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC));

    float thresh = (float)params.pawb / 256;
    float inputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1; // 65535.0f;

    float mul_fact = (inputMax / (inputMax - params.black_level));

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(img_inp, imgInput);

    xf::cv::blackLevelCorrection<XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC, 16, 15, 1>(imgInput, blc_out,
                                                                                      params.black_level, mul_fact);
    // xf::cv::badpixelcorrection<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0, 0>(imgInput2, bpc_out);

    xf::cv::gaincontrol<XF_SRC_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(blc_out, gain_out, params.rgain, params.bgain,
                                                                  params.ggain, params.bayer_p);
    xf::cv::demosaicing<XF_SRC_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC, 0>(gain_out, demosaic_out, params.bayer_p);

    function_awb<XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(demosaic_out, ltm_in, hist0, hist1, gain0, gain1,
                                                                     height, width, thresh, flag);
    xf::cv::colorcorrectionmatrix<XF_CCM_TYPE, XF_DST_T, XF_DST_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(ltm_in, lsc_out);
    if (XF_DST_T == XF_8UC3) {
        fifo_copy<XF_DST_T, XF_LTM_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(lsc_out, aecin, height, width);
    } else {
        xf::cv::xf_QuatizationDithering<XF_DST_T, XF_LTM_T, STRM_HEIGHT, XF_WIDTH, 256, Q_VAL, XF_NPPC>(lsc_out, aecin);
    }
    xf::cv::gammacorrection<XF_LTM_T, XF_LTM_T, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(aecin, _dst, _gamma_lut);
    // ColorMat2AXIvideo<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(_dst, m_axis_video);
    xf::cv::rgb2yuyv<XF_LTM_T, XF_16UC1, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(_dst, imgOutput);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, STRM_HEIGHT, XF_WIDTH, XF_NPPC>(imgOutput, img_out);

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
                       int height,
                       int width,
                       unsigned short array_params[NUM_STREAMS][6],
                       unsigned char gamma_lut[NUM_STREAMS][256 * 3]) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp1  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_inp2  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=img_inp3  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=img_inp4  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=img_out1  offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi     port=img_out2  offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi     port=img_out3  offset=slave bundle=gmem7
#pragma HLS INTERFACE m_axi     port=img_out4  offset=slave bundle=gmem8
// clang-format on

// clang-format off
#pragma HLS ARRAY_PARTITION variable=hist0_awb complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1_awb complete dim=1
    // clang-format on

    struct ispparams_config params[NUM_STREAMS];

    for (int i = 0; i < NUM_STREAMS; i++) {
// clang-format off
#pragma HLS UNROLL
        // clang-format on

        params[i].rgain = array_params[i][0];
        params[i].bgain = array_params[i][1];
        params[i].ggain = array_params[i][2];
        params[i].pawb = array_params[i][3];
        params[i].bayer_p = array_params[i][4];
        params[i].black_level = array_params[i][5];
    }

    uint32_t tot_rows = NUM_STREAMS * height;
    const uint16_t pt[NUM_STREAMS] = {STRM1_ROWS, STRM2_ROWS, STRM3_ROWS, STRM4_ROWS};
    uint16_t max = STRM1_ROWS;
    for (int i = 1; i < NUM_STREAMS; i++) {
        if (pt[i] > max) max = pt[i];
    }

    const uint16_t TC = tot_rows / max;

    uint32_t addrbound, num_rows;

    int strm_id = 0, idx = 0;

    int rem_rows[NUM_STREAMS] = {height, height, height, height};

    uint32_t offset1 = 0, offset2 = 0, offset3 = 0, offset4 = 0;

    for (int r = 0; r < tot_rows;) {
// clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=0 max=TC
        #pragma HLS LOOP_FLATTEN off
        // clang-format on

        // Compute no.of rows to process
        if (rem_rows[idx] > pt[idx])
            num_rows = pt[idx];
        else
            num_rows = rem_rows[idx];

        // Compute
        addrbound = num_rows * (width >> XF_BITSHIFT(XF_NPPC));
        strm_id = idx;

        if (idx == 0 && num_rows > 0) {
            Streampipeline(img_inp1 + offset1, img_out1 + offset1, num_rows, width, hist0_awb[idx], hist1_awb[idx],
                           igain_0, igain_1, params[idx], gamma_lut[idx], flag[idx]);
            offset1 += addrbound;
        } else if (idx == 1 && num_rows > 0) {
            Streampipeline(img_inp2 + offset2, img_out2 + offset2, num_rows, width, hist0_awb[idx], hist1_awb[idx],
                           igain_0, igain_1, params[idx], gamma_lut[idx], flag[idx]);

            offset2 += addrbound;
        } else if (idx == 2 && num_rows > 0) {
            Streampipeline(img_inp3 + offset3, img_out3 + offset3, num_rows, width, hist0_awb[idx], hist1_awb[idx],
                           igain_0, igain_1, params[idx], gamma_lut[idx], flag[idx]);

            offset3 += addrbound;
        } else if (idx == 3 && num_rows > 0) {
            Streampipeline(img_inp4 + offset4, img_out4 + offset4, num_rows, width, hist0_awb[idx], hist1_awb[idx],
                           igain_0, igain_1, params[idx], gamma_lut[idx], flag[idx]);

            offset4 += addrbound;
        }
        // Update remaining rows to process
        rem_rows[idx] = rem_rows[idx] - num_rows;

        // Next stream selection
        if (idx == NUM_STREAMS - 1)
            idx = 0;
        else
            idx++;

        // Update total rows to process
        r += num_rows;
    }

    return;
}
}
