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
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>
#include <common/xf_aie_hw_utils.hpp>

void rgba2rgb(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out) {
    uint8_t* img_in = (uint8_t*)::aie::begin(in);
    uint8_t* img_out = (uint8_t*)::aie::begin(out);

    int16_t tile_width = xf::cv::aie::xfGetTileWidth(img_in);
    int16_t tile_height = xf::cv::aie::xfGetTileHeight(img_in);

    if (tile_width == 0 || tile_height == 0) return;

    xf::cv::aie::xfCopyMetaData(img_in, img_out);
    xf::cv::aie::xfSetTileWidth(img_out, tile_width);

    xf::cv::aie::xfUnsignedSaturation(img_out);

    uint8_t* in_ptr = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_in);
    uint8_t* out_ptr = (uint8_t*)xf::cv::aie::xfGetImgDataPtr(img_out);

    ::aie::vector<uint8_t, N> rgba;
    uint16_t loop_count = (tile_height * tile_width * 4) / N; // Divide by VECTORIZATION-FACTOR

    for (int j = 0; j < loop_count; j++) chess_prepare_for_pipelining {
            rgba = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            ::aie::store_v((uint8_t*)out_ptr, rgba);
            out_ptr += N;
        }
};
