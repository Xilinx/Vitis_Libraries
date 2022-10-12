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

#include "xf_ispstats_config.h"

extern "C" {

void ispstats_accel(ap_uint<PTR_WIDTH>* img_in,
                    unsigned int* stats,
                    unsigned int* max_bins,
                    int rows,
                    int cols,
                    int roi_tlx,
                    int roi_tly,
                    int roi_brx,
                    int roi_bry,
                    int zone_col_num,   // N
                    int zone_row_num) { // M
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
#pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(rows, cols);
// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in, imgInput);

    xf::cv::ispStats<MAX_ZONES, STATS_SIZE, FINAL_BINS_NUM, MERGE_BINS, TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN>(
        imgInput, stats, max_bins, roi_tlx, roi_tly, roi_brx, roi_bry, zone_col_num, zone_row_num);

    return;
} // End of kernel

} // End of extern "C"