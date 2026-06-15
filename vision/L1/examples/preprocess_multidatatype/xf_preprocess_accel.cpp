/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#include "xf_config_params.h"
#include "common/xf_infra.hpp"

void preprocess_accel(InStream& s_axis_video,  // Input AXI stream
                      OutStream& m_axis_video, // output image pointer
                      uint32_t params_int[2 * XF_CHANNELS(OUT_TYPE, NPPCX)],
                      int in_img_width,
                      int in_img_height,
                      int resize_width,
                      int resize_height,
                      uint32_t datatype) {
// clang-format off
#pragma HLS INTERFACE axis port=s_axis_video register
#pragma HLS INTERFACE axis port=m_axis_video register
#pragma HLS INTERFACE s_axilite port=params_int bundle=CTRL offset= 0x80 

#pragma HLS INTERFACE s_axilite port=in_img_width bundle=CTRL offset= 0x10     
#pragma HLS INTERFACE s_axilite port=in_img_height bundle=CTRL offset= 0x18     
#pragma HLS INTERFACE s_axilite port=resize_width bundle=CTRL offset= 0x40     
#pragma HLS INTERFACE s_axilite port=resize_height bundle=CTRL offset= 0x48     
#pragma HLS INTERFACE s_axilite port=datatype bundle=CTRL offset= 0x20

#pragma HLS INTERFACE s_axilite port=return bundle=CTRL

    float params[2 * XF_CHANNELS(OUT_TYPE, NPPCX)];
    for (int i=0 ;i< 2 * XF_CHANNELS(OUT_TYPE, NPPCX);i++){
        params[i] = ((float)params_int[i] / 8388608);   //division with 2^23;
    }

    // clang-format on
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(in_img_height, in_img_width);

    xf::cv::Mat<IN_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_CH_SWAP> ch_swap_mat(resize_height, resize_width);

    xf::cv::Mat<OUT_TYPE_NEW, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_RESIZE_OUT> resize_out_mat(resize_height,
                                                                                                 resize_width);

    xf::cv::Mat<OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_OUT> out_mat(resize_height, resize_width);

#pragma HLS DATAFLOW

    xf::cv::AXIvideo2xfMat(s_axis_video, imgInput);

/**** RGB to X ****/
#if RGB2RGBA
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPPCX, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(imgInput, ch_swap_mat);
    xf::cv::rgb2rgba<IN_TYPE, OUT_TYPE_NEW, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_CH_SWAP>(
        ch_swap_mat, resize_out_mat);
#else
    xf::cv::resize<INTERPOLATION, IN_TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPPCX, XF_USE_URAM, MAXDOWNSCALE,
                   XF_CV_DEPTH_CH_SWAP, XF_CV_DEPTH_RESIZE_OUT>(imgInput, resize_out_mat);
#endif

/**** Preprocessing ****/

#if (SELECT & 0x0001) // INT8

    if (datatype == data_types::INT8) { // Runtime

        xf::cv::preProcess_int8<OUT_TYPE_NEW, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, WIDTH_A, IBITS_A, WIDTH_B, IBITS_B,
                                WIDTH_OUT, IBITS_OUT, XF_CV_DEPTH_RESIZE_OUT, XF_CV_DEPTH_OUT>(resize_out_mat, out_mat,
                                                                                               params);
    }

#endif

#if (SELECT & 0x0002) // FP16

    if (datatype == data_types::FP16) { // Runtime

        xf::cv::preProcess_fp16<OUT_TYPE_NEW, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_RESIZE_OUT,
                                XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);
    }

#endif

#if (SELECT & 0x0004) // BF16

    if (datatype == data_types::BF16) { // Runtime

        xf::cv::preProcess_bf16<OUT_TYPE_NEW, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_RESIZE_OUT,
                                XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);
    }

#endif

#if (SELECT & 0x0008) // FP32

    if (datatype == data_types::FP32) { // Runtime

        xf::cv::preProcess_fp32<OUT_TYPE_NEW, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_RESIZE_OUT,
                                XF_CV_DEPTH_OUT>(resize_out_mat, out_mat, params);
    }

#endif

    xf::cv::xfMat2AXIvideo<AXI_WIDTH_OUT, OUT_TYPE, NEWHEIGHT, NEWWIDTH, NPPCX, XF_CV_DEPTH_OUT, XF_AXI_GBR>(
        out_mat, m_axis_video);
}
