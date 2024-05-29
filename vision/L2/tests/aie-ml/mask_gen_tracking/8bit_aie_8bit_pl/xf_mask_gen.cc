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
#include "imgproc/xf_mask_gen.hpp"

void maskGen_api(adf::input_buffer<uint8_t>& input,
                 adf::output_buffer<uint8_t>& output,
                 uint8_t depth_min,
                 uint8_t depth_max,
                 uint16_t thres_f_new,
                 uint16_t thres_b_new) {
    uint8_t* img_in_ptr_1 = (uint8_t*)::aie::begin(input);
    uint8_t* img_out_ptr = (uint8_t*)::aie::begin(output);

    const uint16 img_width = xf::cv::aie::xfGetTileWidth(img_in_ptr_1);
    const uint16 img_height = xf::cv::aie::xfGetTileHeight(img_in_ptr_1);

    xf::cv::aie::xfCopyMetaData(img_in_ptr_1, img_out_ptr);

    uint8_t* in_ptr_1 = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_in_ptr_1);
    uint8_t* out_ptr = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_out_ptr);

    xf::cv::aie::MaskGen maskGen = xf::cv::aie::MaskGen();
    maskGen.runImplMaskGen(in_ptr_1, out_ptr, depth_min, depth_max, thres_f_new, thres_b_new, img_height, img_width);
}
