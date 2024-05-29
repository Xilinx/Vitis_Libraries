/*
 * Copyright 2021 Xilinx, Inc.
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

#include "kernels.h"
#include "imgproc/xf_mask_gen_track.hpp"

void maskGenTrack_api(adf::input_buffer<uint8_t>& input1,
                      adf::input_buffer<uint8_t>& input2,
                      adf::output_buffer<uint8_t>& output,
                      uint8_t depth_min,
                      uint8_t depth_max,
                      uint16_t thres_f_new,
                      uint16_t thres_b_new,
                      uint8_t pred_seg_thresh) {
    uint8_t* img_in_ptr_1 = (uint8_t*)::aie::begin(input1);
    uint8_t* img_in_ptr_2 = (uint8_t*)::aie::begin(input2);
    uint8_t* img_out_ptr = (uint8_t*)::aie::begin(output);

    const uint16 img_width = xf::cv::aie::xfGetTileWidth(img_in_ptr_1);
    const uint16 img_height = xf::cv::aie::xfGetTileHeight(img_in_ptr_1);

    xf::cv::aie::xfCopyMetaData(img_in_ptr_1, img_out_ptr);

    xf::cv::aie::xfSetTileOutTWidth(img_out_ptr, 16);
    xf::cv::aie::xfSetTileOutTHeight(img_out_ptr, 1);
    xf::cv::aie::xfSetTileOutOffset_L(img_out_ptr, 0);
    xf::cv::aie::xfSetTileOutOffset_U(img_out_ptr, 0);

    uint8_t* in_ptr_1 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_in_ptr_1);
    uint8_t* in_ptr_2 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_in_ptr_2);
    uint32_t* out_ptr = (uint32_t*)xf::cv::aie::xfGetImgDataPtr(img_out_ptr);

    xf::cv::aie::MaskGenTrack maskGenTrack = xf::cv::aie::MaskGenTrack();
    maskGenTrack.runImplTracking(in_ptr_1, in_ptr_2, pred_seg_thresh, depth_min, depth_max, img_height, img_width);

    out_ptr[0] = maskGenTrack.non_zero_count;
    out_ptr[1] = maskGenTrack.sum;
}
