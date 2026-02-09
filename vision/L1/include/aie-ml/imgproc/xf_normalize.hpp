/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>

#ifndef _AIE_NORMALIZE_H_
#define _AIE_NORMALIZE_H_

namespace xf {
namespace cv {
namespace aie {

int last_blk_repeat(int tile_width_out) {
    int last_blk = ((((tile_width_out + 32 - 1) / 32) * 32) - tile_width_out);
    return last_blk;
}

__attribute__((noinline)) void xf_normalize(
    uint8_t* input, int8_t* output, int tile_width_out, int nChannels, const int16_t* coeff, uint8_t FBits[3]) {
    uint8_t alpha[4] = {(uint8_t)coeff[0], (uint8_t)coeff[1], (uint8_t)coeff[2], (uint8_t)coeff[3]};
    int8_t beta[4] = {(int8_t)coeff[4], (int8_t)coeff[5], (int8_t)coeff[6], (int8_t)coeff[7]};

    uint32_t alpha_i = *((uint32_t*)(alpha));
    int32_t beta_i = *((int32_t*)(beta));

    auto a_reg = ::aie::broadcast<uint32_t, 8>(alpha_i).template cast_to<uint8_t>();
    auto b_tmp = ::aie::broadcast<int32_t, 8>(beta_i).template cast_to<int8_t>();
    ::aie::vector<int8_t, 64> b_reg = ::aie::concat(b_tmp, ::aie::neg(b_tmp));

    ::aie::vector<uint8_t, 64> data_vec;
    ::aie::accum<acc32, 32> acc;
    data_vec.insert(0, ::aie::load_v<32>(input));
    data_vec.insert(1, a_reg);
    uint8_t* restrict img_in_ptr = (uint8_t*)input + 32;
    int8_t* restrict img_out_ptr = (int8_t*)output;
    int nFBits = FBits[0] + FBits[1] - FBits[2];
    int pixel_width = tile_width_out * nChannels;
    for (int i = 0; i < pixel_width / 32; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
            acc = mul_elem_32_2(data_vec, b_reg);
            ::aie::store_v(img_out_ptr, acc.template to_vector<int8_t>(nFBits));
            data_vec.insert(0, ::aie::load_v<32>(img_in_ptr));
            img_in_ptr += 32;
            img_out_ptr += 32;
        }

    if ((pixel_width % 32) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(pixel_width);
        img_in_ptr -= (32 + LAST_BLK_REPEAT);
        img_out_ptr -= LAST_BLK_REPEAT;
        data_vec.insert(0, ::aie::load_unaligned_v<32>(img_in_ptr));
        acc = mul_elem_32_2(data_vec, b_reg);

        auto last_vec = acc.template to_vector<int8_t>(nFBits);
        ::aie::store_unaligned_v(img_out_ptr,
                                 ::aie::select(::aie::load_unaligned_v<32>(img_out_ptr), last_vec,
                                               ::aie::mask<32>::from_uint32((0xFFFFFFFF << LAST_BLK_REPEAT))));
    }
}

void Normalize_runImpl(adf::input_buffer<uint8_t>& input,
                       adf::output_buffer<int8_t>& output,
                       const int16_t* coeff,
                       uint8_t FBits[3],
                       int nChannels) {
    uint8* img_in_ptr = (uint8*)::aie::begin(input);
    int8* img_out_ptr = (int8*)::aie::begin(output);

    const int16_t tile_width_in = xfGetTileWidth(img_in_ptr);
    const int16_t tile_width_out = xfGetTileOutTWidth(img_in_ptr);
    const int16_t tile_height_in = xfGetTileHeight(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    uint8* restrict ptr_in = (uint8*)xfGetImgDataPtr(img_in_ptr);
    int8* restrict ptr_out = (int8*)xfGetImgDataPtr(img_out_ptr);

    for (int i = 0; i < tile_height_in; i++) {
        xf_normalize((uint8_t*)ptr_in, (int8_t*)ptr_out, tile_width_out, nChannels, coeff, FBits);
        ptr_in += tile_width_in * nChannels;
        ptr_out += tile_width_out * nChannels;
    }
}

} // aie
} // cv
} // xf
#endif