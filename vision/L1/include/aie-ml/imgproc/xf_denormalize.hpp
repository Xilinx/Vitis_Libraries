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

#ifndef _AIE_DENORMALIZE_H_
#define _AIE_DENORMALIZE_H_

namespace xf {
namespace cv {
namespace aie {

int last_blk_repeat(int tile_width_out) {
    int last_blk = ((((tile_width_out + 32 - 1) / 32) * 32) - tile_width_out);
    return last_blk;
}

__attribute__((noinline)) void xf_denormalize(int8_t* input1,
                                              int8_t* input2,
                                              uint8_t* output1,
                                              uint8_t* output2,
                                              int tile_width_in,
                                              int nChannels,
                                              const int16_t* coeff,
                                              uint8_t FBits[3]) {
    uint8_t alpha[4] = {(uint8_t)coeff[0], (uint8_t)coeff[1], (uint8_t)coeff[2], (uint8_t)coeff[3]};
    int8_t beta[4] = {(int8_t)coeff[4], (int8_t)coeff[5], (int8_t)coeff[6], (int8_t)coeff[7]};

    uint32_t alpha_i = *((uint32_t*)(alpha));
    int32_t beta_i = *((int32_t*)(beta));

    auto a_reg = ::aie::broadcast<uint32_t, 8>(alpha_i).template cast_to<uint8_t>();
    auto b_reg = ::aie::broadcast<int32_t, 8>(beta_i).template cast_to<int8_t>();

    ::aie::accum<acc32, 32> acc_init;
    uint8_t nFBits = FBits[0] + FBits[2];
    uint8_t mFBits = nFBits - FBits[1];
    uint8_t nFBits_x256 = nFBits - 8;
    uint8_t a_scale = (1 << mFBits);
    acc_init = ::aie::mul(a_reg, a_scale);

    ::aie::vector<int8_t, 32> data_vec1 = ::aie::load_v<32>(input1);
    //::aie::print(data_vec1, true, "data_vec1: ");
    ::aie::vector<int8_t, 32> data_vec2 = ::aie::load_v<32>(input2);
    int8_t* restrict img_in_ptr1 = (int8_t*)input1 + 32;
    int8_t* restrict img_in_ptr2 = (int8_t*)input2 + 32;
    uint8_t* restrict img_out_ptr1 = (uint8_t*)output1;
    uint8_t* restrict img_out_ptr2 = (uint8_t*)output2;
    set_rnd(rnd_floor);
    int pixel_width = tile_width_in * nChannels;
    for (int i = 0; i < pixel_width / 32; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
            ::aie::accum<acc32, 32> acc1 = ::aie::mac(acc_init, data_vec1, b_reg);
            //::aie::print(acc1, true, "acc1: ");
            //::aie::print(acc1.template to_vector<uint8_t>(nFBits_x256), true, "acc1_to_vec: ");
            ::aie::accum<acc32, 32> acc2 = ::aie::mac(acc_init, data_vec2, b_reg);
            ::aie::store_v(img_out_ptr1, acc1.template to_vector<uint8_t>(nFBits_x256));
            ::aie::store_v(img_out_ptr2, acc2.template to_vector<uint8_t>(nFBits_x256));
            data_vec1 = ::aie::load_v<32>(img_in_ptr1);
            //::aie::print(data_vec1, true, "data_vec1: ");
            data_vec2 = ::aie::load_v<32>(img_in_ptr2);
            img_in_ptr1 += 32;
            img_out_ptr1 += 32;
            img_in_ptr2 += 32;
            img_out_ptr2 += 32;
        }

    if ((pixel_width % 32) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(pixel_width);
        img_in_ptr1 -= (32 + LAST_BLK_REPEAT);
        img_in_ptr2 -= (32 + LAST_BLK_REPEAT);
        img_out_ptr1 -= LAST_BLK_REPEAT;
        img_out_ptr2 -= LAST_BLK_REPEAT;

        data_vec1 = ::aie::load_unaligned_v<32>(img_in_ptr1);
        data_vec2 = ::aie::load_unaligned_v<32>(img_in_ptr2);
        ::aie::accum<acc32, 32> acc1 = ::aie::mac(acc_init, data_vec1, b_reg);
        ::aie::accum<acc32, 32> acc2 = ::aie::mac(acc_init, data_vec2, b_reg);

        auto last_vec1 = acc1.template to_vector<uint8_t>(nFBits_x256);
        auto last_vec2 = acc2.template to_vector<uint8_t>(nFBits_x256);
        auto m = ::aie::mask<32>::from_uint32((0xFFFFFFFF << LAST_BLK_REPEAT));
        ::aie::store_unaligned_v(img_out_ptr1, ::aie::select(::aie::load_unaligned_v<32>(img_out_ptr1), last_vec1, m));
        ::aie::store_unaligned_v(img_out_ptr2, ::aie::select(::aie::load_unaligned_v<32>(img_out_ptr2), last_vec2, m));
    }
    set_rnd(rnd_conv_even);
}

void Denormalize_runImpl(adf::input_buffer<int8_t>& input,
                         adf::output_buffer<uint8_t>& output,
                         const int16_t* coeff,
                         uint8_t FBits[3],
                         int nChannels) {
    int8* img_in_ptr = (int8*)::aie::begin(input);
    uint8* img_out_ptr = (uint8*)::aie::begin(output);

    const int16_t tile_width_in = xfGetTileWidth(img_in_ptr);
    const int16_t tile_width_out = xfGetTileOutTWidth(img_in_ptr);
    const int16_t tile_height_in = xfGetTileHeight(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    int8* restrict ptr_in = (int8*)xfGetImgDataPtr(img_in_ptr);
    uint8* restrict ptr_out = (uint8*)xfGetImgDataPtr(img_out_ptr);

    int8_t* ptr_in_2 = ptr_in + tile_width_in * nChannels;
    uint8_t* ptr_out_2 = ptr_out + tile_width_in * nChannels;

    for (int i = 0; i < tile_height_in / 2; i++) {
        xf_denormalize((int8_t*)ptr_in, (int8_t*)ptr_in_2, (uint8_t*)ptr_out, (uint8_t*)ptr_out_2, tile_width_out,
                       nChannels, coeff, FBits);
        ptr_in += 2 * tile_width_in * nChannels;
        ptr_in_2 = ptr_in + tile_width_in * nChannels;
        ptr_out += 2 * tile_width_out * nChannels;
        ptr_out_2 = ptr_out + tile_width_in * nChannels;
    }
}

} // aie
} // cv
} // xf
#endif