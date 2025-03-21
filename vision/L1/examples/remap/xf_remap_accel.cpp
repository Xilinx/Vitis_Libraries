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

#include "xf_remap_accel_config.h"

/************************************************************************************
 * Function:    AXIVideo2BayerMat
 * Parameters:  Multiple bayerWindow.getval AXI Stream, User Stream, Image Resolution
 * Return:      None
 * Description: Read data from multiple pixel/clk AXI stream into user defined stream
 ************************************************************************************/
template <int TYPE, int ROWS, int COLS, int NPPC, int XFCVDEPTH_BAYER>
void AXIVideo2BayerMat(InVideoStrm_t& bayer_strm, xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_BAYER>& bayer_mat) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    InVideoStrmBus_t axi;

    const int m_pix_width = XF_PIXELWIDTH(TYPE, NPPC) * XF_NPIXPERCYCLE(NPPC);

    int rows = bayer_mat.rows;
    int cols = bayer_mat.cols >> XF_BITSHIFT(NPPC);
    int idx = 0;

    bool start = false;
    bool last = false;

loop_start_hunt:
    while (!start) {
// clang-format off
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
        // clang-format on

        bayer_strm >> axi;
        start = axi.user.to_bool();
    }

loop_row_axi2mat:
    for (int i = 0; i < rows; i++) {
        last = false;
// clang-format off
#pragma HLS loop_tripcount avg=ROWS max=ROWS
    // clang-format on
    loop_col_zxi2mat:
        for (int j = 0; j < cols; j++) {
// clang-format off
#pragma HLS loop_flatten off
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=COLS/NPPC max=COLS/NPPC
            // clang-format on

            if (start || last) {
                start = false;
            } else {
                bayer_strm >> axi;
            }

            last = axi.last.to_bool();

            bayer_mat.write(idx++, axi.data(m_pix_width - 1, 0));
        }

    loop_last_hunt:
        while (!last) {
// clang-format off
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
            // clang-format on

            bayer_strm >> axi;
            last = axi.last.to_bool();
        }
    }

    return;
}

template <int TYPE, int ROWS, int COLS, int NPPC, int XFCVDEPTH_color>
void ColorMat2AXIvideo(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_color>& color_mat, OutVideoStrm_t& color_strm) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    OutVideoStrmBus_t axi;

    int rows = color_mat.rows;
    int cols = color_mat.cols >> XF_BITSHIFT(NPPC);
    int idx = 0;

    XF_TNAME(TYPE, NPPC) srcpixel;

    const int m_pix_width = XF_PIXELWIDTH(TYPE, NPPC) * XF_NPIXPERCYCLE(NPPC);

    int depth = XF_DTPIXELDEPTH(TYPE, NPPC);

    bool sof = true; // Indicates start of frame

loop_row_mat2axi:
    for (int i = 0; i < rows; i++) {
// clang-format off
#pragma HLS loop_tripcount avg=ROWS max=ROWS
    // clang-format on
    loop_col_mat2axi:
        for (int j = 0; j < cols; j++) {
// clang-format off
#pragma HLS loop_flatten off
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount avg=COLS/NPPC max=COLS/NPPC
            // clang-format on
            if (sof) {
                axi.user = 1;
            } else {
                axi.user = 0;
            }

            if (j == cols - 1) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }

            axi.data = 0;

            srcpixel = color_mat.read(idx++);

            for (int npc = 0; npc < NPPC; npc++) {
                for (int rs = 0; rs < 3; rs++) {
#if XF_AXI_GBR == 1
                    int kmap[3] = {1, 0, 2}; // GBR format
#else
                    int kmap[3] = {0, 1, 2}; // GBR format
#endif

                    int start = (rs + npc * 3) * depth;

                    int start_format = (kmap[rs] + npc * 3) * depth;

                    axi.data(start + (depth - 1), start) = srcpixel.range(start_format + (depth - 1), start_format);
                }
            }

            axi.keep = -1;
            color_strm << axi;

            sof = false;
        }
    }

    return;
}

void remap_accel(InVideoStrm_t& s_axis_video,
                 OutVideoStrm_t& m_axis_video,
                 MapxyStrm_t& map_x_axi,
                 MapxyStrm_t& map_y_axi,
                 int rows,
                 int cols) {
// clang-format off
    #pragma HLS INTERFACE axis port=s_axis_video register
    #pragma HLS INTERFACE axis port=m_axis_video register
    #pragma HLS INTERFACE axis  port=map_x_axi register  
    #pragma HLS INTERFACE axis  port=map_y_axi register  
    #pragma HLS INTERFACE s_axilite  port=rows 	          bundle=CTRL offset=0x00040              
    #pragma HLS INTERFACE s_axilite  port=cols 	          bundle=CTRL offset=0x00048              
    #pragma HLS INTERFACE s_axilite  port=return          bundle=CTRL
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(rows, cols);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_MAP_X> mapX(rows, cols);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_MAP_Y> mapY(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(rows, cols);

    const int HEIGHT_WIDTH_LOOPCOUNT = HEIGHT * WIDTH / XF_NPIXPERCYCLE(NPPCX);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    AXIVideo2BayerMat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(s_axis_video, imgInput);
    xf::cv::AXIvideo2xfMat(map_x_axi, mapX);
    xf::cv::AXIvideo2xfMat(map_y_axi, mapY);

    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION_TYPE, IN_TYPE, MAPXY_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX,
                  XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT, XF_CV_DEPTH_MAP_X, XF_CV_DEPTH_MAP_Y>(
        imgInput, imgOutput, mapX, mapY);

    xf::cv::xfMat2AXIvideo(imgOutput, m_axis_video);

    return;
} // End of kernel
