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
#include "imgproc/xf_resize_bicubic_16bit.hpp"

void ResizeRunner::run(adf::input_buffer<DATA_TYPE>& input,
                       adf::output_buffer<DATA_TYPE>& metadata,
                       adf::output_buffer<DATA_TYPE>& output,
                       int channels,
                       uint32_t scale_y,
                       int img_height_in,
                       int img_height_out,
                       float scale_y_f) {
    DATA_TYPE* img_in_ptr = (DATA_TYPE*)::aie::begin(input);
    DATA_TYPE* metadata_ptr = (DATA_TYPE*)::aie::begin(metadata);
    DATA_TYPE* img_out_ptr = (DATA_TYPE*)::aie::begin(output);

    xf::cv::aie::Resizebicubic<DATA_TYPE, VF> resize(mwtsY);
    resize.runImpl(img_in_ptr, metadata_ptr, img_out_ptr, channels, scale_y, img_height_in, img_height_out, scale_y_f);
}
