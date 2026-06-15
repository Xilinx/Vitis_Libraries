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
static constexpr int __XF_DEPTH_OUT =
    (((HEIGHT * WIDTH * XF_PIXELWIDTH(OUT_TYPE, NPPCX))) /
     OUTPUT_PTR_WIDTH); //(HEIGHT * WIDTH * (XF_PIXELWIDTH(OUT_TYPE, NPPCX)) / 8) / (OUTPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_OUT_NCHW =
    (HEIGHT * WIDTH * (XF_PIXELWIDTH(OUT_TYPE_NCHW, NPPCX)) / 8) / (OUTPUT_PTR_WIDTH_NCHW / 8);
void layout_formatter_accel(InStream& s_axis_video,                    // Input AXI stream
                            ap_uint<OUTPUT_PTR_WIDTH>* m_axi_mm_video, // Output M-AXI pointer
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* m_axi_mm_video1,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* m_axi_mm_video2,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* m_axi_mm_video3,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* m_axi_mm_video4,
                            int in_img_height,
                            int in_img_width,
                            int layout_format,
                            int datatype,
                            int channels_out) {
// clang-format off
#pragma HLS INTERFACE axis port=s_axis_video register
#pragma HLS INTERFACE m_axi port=m_axi_mm_video offset=slave bundle=gmem0 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE s_axilite port=m_axi_mm_video bundle=CTRL offset= 0x30
#pragma HLS INTERFACE m_axi port=m_axi_mm_video1 offset=slave bundle=gmem0 depth=__XF_DEPTH_OUT_NCHW
#pragma HLS INTERFACE m_axi port=m_axi_mm_video2 offset=slave bundle=gmem0 depth=__XF_DEPTH_OUT_NCHW
#pragma HLS INTERFACE m_axi port=m_axi_mm_video3 offset=slave bundle=gmem0 depth=__XF_DEPTH_OUT_NCHW

#pragma HLS INTERFACE m_axi port=m_axi_mm_video4 offset=slave bundle=gmem0 depth=__XF_DEPTH_OUT_NCHW
#pragma HLS INTERFACE s_axilite port=m_axi_mm_video4 bundle=CTRL offset= 0x70

#pragma HLS INTERFACE s_axilite port=m_axi_mm_video1 bundle=CTRL offset=0x3c
#pragma HLS INTERFACE s_axilite port=m_axi_mm_video2 bundle=CTRL offset=0x54
#pragma HLS INTERFACE s_axilite port=m_axi_mm_video3 bundle=CTRL offset=0x60
#pragma HLS INTERFACE s_axilite port=in_img_width bundle=CTRL offset=0x10    
#pragma HLS INTERFACE s_axilite port=in_img_height bundle=CTRL offset=0x18    
#pragma HLS INTERFACE s_axilite port=layout_format bundle=CTRL offset=0x80 
#pragma HLS INTERFACE s_axilite port=datatype bundle=CTRL offset=0x90
#pragma HLS INTERFACE s_axilite port=channels_out bundle=CTRL offset=0xA0
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(in_img_height, in_img_width);

    xf::cv::AXIvideo2xfMat(s_axis_video, imgInput); 
	
	#pragma HLS DATAFLOW
	
	layout_formatter<OUTPUT_PTR_WIDTH, OUTPUT_PTR_WIDTH_NCHW, SELECT_ORDER, SELECT_TYPE, IN_TYPE, OUT_TYPE, OUT_TYPE_NCHW,
		HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>
		(imgInput, m_axi_mm_video, m_axi_mm_video1, m_axi_mm_video2, m_axi_mm_video3, m_axi_mm_video4, layout_format, datatype, channels_out);
	
}	