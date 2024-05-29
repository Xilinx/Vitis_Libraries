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

#include <adf.h>
#include <aie_api/aie.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <aie_api/utils.hpp>
#ifndef _AIE_BLACKLEVEL_H_
#define _AIE_BLACKLEVEL_H_

namespace xf {
namespace cv {
namespace aie {
template <typename T, int N, typename OUT_T>
void blackLevelCorrection(const T* restrict img_in,
                          T* restrict img_out,
                          int image_width,
                          int image_height,
                          const uint8_t& black_level,
                          const uint16_t& mul_fact) {
    int32_t const_val = mul_fact * (-black_level);
    ::aie::vector<int32_t, N> blk_val = ::aie::broadcast<int32_t, N>(const_val);
    ::aie::vector<T, N> in_data;
    ::aie::vector<T, N> temp_out1;
    ::aie::accum<acc32, N> acc;
    ::aie::accum<acc32, N> acc1;
    acc.from_vector(blk_val);
    set_sat();

    for (int i = 0; i < image_height; i++) chess_prepare_for_pipelining {
            for (int j = 0; j < image_width; j += N) {
                in_data = ::aie::load_v<N>(img_in);
                img_in += N;
                acc1 = ::aie::mac(acc, in_data, mul_fact);
                temp_out1 = acc1.template to_vector<uint8_t>(15);
                ::aie::store_v(img_out, temp_out1);
                img_out += N;
            }
        }
}
void blackLevelCorrection_api(input_window_uint8* img_in,
                              output_window_uint8* img_out,
                              const uint8_t& black_level,
                              const uint16_t& mul_fact) {
    uint8_t* img_in_ptr = (uint8_t*)img_in->ptr;
    uint8_t* img_out_ptr = (uint8_t*)img_out->ptr;

    const uint16_t img_width = xfGetTileWidth(img_in_ptr);
    const uint16_t img_height = xfGetTileHeight(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);
    xfUnsignedSaturation(img_out_ptr);

    uint8_t* in_ptr = (uint8_t*)xfGetImgDataPtr(img_in_ptr);
    uint8_t* out_ptr = (uint8_t*)xfGetImgDataPtr(img_out_ptr);

    blackLevelCorrection<uint8_t, 32, uint8_t>(in_ptr, out_ptr, img_width, img_height, black_level, mul_fact);
}

} // aie
} // cv
} // xf
#endif
