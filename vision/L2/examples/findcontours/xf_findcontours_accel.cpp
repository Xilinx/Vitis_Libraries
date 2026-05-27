/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

#include <ap_int.h>
#include <stdint.h>
#include "xf_findcontours_accel_config.h"

void findcontours_accel(ap_uint<INPUT_PTR_WIDTH>* img,
                        int rows,
                        int cols,
                        ap_uint<OUTPUT_PTR_WIDTH>* points_packed,
                        ap_uint<OUTPUT_PTR_WIDTH>* contour_offsets,
                        ap_uint<OUTPUT_PTR_WIDTH>* num_contours) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img             offset=slave bundle=gmem0 depth=XF_CV_DEPTH_IN
#pragma HLS INTERFACE m_axi     port=points_packed   offset=slave bundle=gmem1 depth=XF_CV_DEPTH_OUT_1
#pragma HLS INTERFACE m_axi     port=contour_offsets offset=slave bundle=gmem2 depth=XF_CV_DEPTH_OUT_2
#pragma HLS INTERFACE m_axi     port=num_contours    offset=slave bundle=gmem3 depth=1
#pragma HLS INTERFACE s_axilite port=img             bundle=control
#pragma HLS INTERFACE s_axilite port=rows            bundle=control
#pragma HLS INTERFACE s_axilite port=cols            bundle=control
#pragma HLS INTERFACE s_axilite port=points_packed   bundle=control
#pragma HLS INTERFACE s_axilite port=contour_offsets bundle=control
#pragma HLS INTERFACE s_axilite port=num_contours    bundle=control
#pragma HLS INTERFACE s_axilite port=return          bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, MAX_H, MAX_W, NPPCX, XF_CV_DEPTH_IN_MAT> in_mat(rows, cols);
// clang-format off
// clang-format on

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    ap_uint<OUTPUT_PTR_WIDTH> local_num = 0;
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, MAX_H, MAX_W, NPPCX, XF_CV_DEPTH_IN_MAT>(img, in_mat);
    xf::cv::findcontours<IN_TYPE, INPUT_PTR_WIDTH, OUTPUT_PTR_WIDTH, MAX_H, MAX_W, MAX_TOTAL_POINTS, MAX_CONTOURS,
                         NPPCX, XF_CV_DEPTH_IN_MAT>(in_mat, rows, cols, points_packed, contour_offsets, local_num);
    num_contours[0] = local_num;
}
