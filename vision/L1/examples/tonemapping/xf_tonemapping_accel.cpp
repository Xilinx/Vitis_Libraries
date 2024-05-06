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

#include "xf_tonemapping_accel_config.h"

static constexpr int __XF_DEPTH_IN = (HEIGHT * WIDTH * XF_PIXELWIDTH(IN_TYPE, NPPCX)) / INPUT_PTR_WIDTH;
static constexpr int __XF_DEPTH_OUT = (HEIGHT * WIDTH * XF_PIXELWIDTH(OUT_TYPE, NPPCX)) / OUTPUT_PTR_WIDTH;
static constexpr int MinMaxVArrSize = LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, HEIGHT, WIDTH, NPPCX>::MinMaxVArrSize;
static constexpr int MinMaxHArrSize = LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, HEIGHT, WIDTH, NPPCX>::MinMaxHArrSize;
static constexpr int BILINEAR_INTERPOLATE_TYPE_C = XF_32FC3;

static bool flg = false;
static XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, NPPCX) omin[2][MinMaxVArrSize][MinMaxHArrSize];
static XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, NPPCX) omax[2][MinMaxVArrSize][MinMaxHArrSize];

void tonemapping_accel_i(ap_uint<INPUT_PTR_WIDTH>* in_ptr,
                         ap_uint<OUTPUT_PTR_WIDTH>* out_ptr,
                         XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, NPPCX) omin_r[MinMaxVArrSize][MinMaxHArrSize],
                         XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, NPPCX) omax_r[MinMaxVArrSize][MinMaxHArrSize],
                         XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, NPPCX) omin_w[MinMaxVArrSize][MinMaxHArrSize],
                         XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, NPPCX) omax_w[MinMaxVArrSize][MinMaxHArrSize],
                         int height,
                         int width,
                         int blk_height,
                         int blk_width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(in_ptr, imgInput);
    xf::cv::LTM<IN_TYPE, OUT_TYPE, BLOCK_HEIGHT, BLOCK_WIDTH, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1,
                XF_CV_DEPTH_OUT_1>::process(imgInput, blk_height, blk_width, omin_r, omax_r, omin_w, omax_w, imgOutput);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, out_ptr);
}

void tonemapping_accel(ap_uint<INPUT_PTR_WIDTH>* in_ptr,
                       ap_uint<OUTPUT_PTR_WIDTH>* out_ptr,
                       int height,
                       int width,
                       int blk_height,
                       int blk_width) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=in_ptr  offset=slave bundle=gmem_in  depth=__XF_DEPTH_IN
#pragma HLS INTERFACE m_axi      port=out_ptr offset=slave bundle=gmem_out depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE s_axilite  port=height
#pragma HLS INTERFACE s_axilite  port=width
#pragma HLS INTERFACE s_axilite  port=return
    
#pragma HLS ARRAY_PARTITION variable=omin dim=1 complete
#pragma HLS ARRAY_PARTITION variable=omin dim=2 cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omin dim=3 cyclic factor=2

#pragma HLS ARRAY_PARTITION variable=omax dim=1 complete
#pragma HLS ARRAY_PARTITION variable=omax dim=2 cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omax dim=3 cyclic factor=2
    // clang-format on

    if (flg) {
        tonemapping_accel_i(in_ptr, out_ptr, omin[0], omax[0], omin[1], omax[1], height, width, blk_height, blk_width);
    } else {
        tonemapping_accel_i(in_ptr, out_ptr, omin[1], omax[1], omin[0], omax[0], height, width, blk_height, blk_width);
    }
    flg = flg ? false : true;

    return;
}
