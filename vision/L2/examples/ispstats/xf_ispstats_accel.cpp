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

#include "xf_ispstats_accel_config.h"

extern "C" {

void ispstats_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                    unsigned int* stats,
                    ap_uint<13>* max_bins,
                    int rows,
                    int cols,
                    int roi_tlx,
                    int roi_tly,
                    int roi_brx,
                    int roi_bry,
                    int zone_col_num, // N
                    int zone_row_num, // M
                    float inputMin,
                    float inputMax,
                    float outputMin,
                    float outputMax) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_in   offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi     port=stats    offset=slave bundle=gmem1 
#pragma HLS INTERFACE m_axi     port=max_bins offset=slave bundle=gmem2 
#pragma HLS INTERFACE s_axilite port=rows
#pragma HLS INTERFACE s_axilite port=cols
#pragma HLS INTERFACE s_axilite port=roi_tlx
#pragma HLS INTERFACE s_axilite port=roi_tly
#pragma HLS INTERFACE s_axilite port=roi_brx
#pragma HLS INTERFACE s_axilite port=roi_bry
#pragma HLS INTERFACE s_axilite port=zone_col_num
#pragma HLS INTERFACE s_axilite port=zone_row_num
#pragma HLS INTERFACE s_axilite port=inputMin 
#pragma HLS INTERFACE s_axilite port=inputMax 
#pragma HLS INTERFACE s_axilite port=outputMin
#pragma HLS INTERFACE s_axilite port=outputMax
#pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(rows, cols);
// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(img_in, imgInput);

    xf::cv::ispStats<MAX_ZONES, STATS_SIZE, FINAL_BINS_NUM, MERGE_BINS, IN_TYPE, NUM_OUT_CH, HEIGHT, WIDTH, NPPCX,
                     XF_CV_DEPTH_IN>(imgInput, stats, max_bins, roi_tlx, roi_tly, roi_brx, roi_bry, zone_col_num,
                                     zone_row_num, inputMin, inputMax, outputMin, outputMax);

    return;
} // End of kernel

} // End of extern "C"
