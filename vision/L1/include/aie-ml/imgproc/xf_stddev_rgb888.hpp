/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __XF_STDDEV_RGB888_
#define __XF_STDDEV_RGB888_

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>
#include <utility>
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21
#include <common/xf_aie_device_traits.hpp>
#endif

namespace xf {
namespace cv {
namespace aie {

class Stddev {
   private:
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    static constexpr int N = 64;
#else
    static constexpr int N = 32;
#endif
   public:
    void runImpl(adf::input_buffer<uint8_t>& input1,
                 adf::output_buffer<uint8_t>& output1,
                 uint32_t reset,
                 float m_r,
                 float m_g,
                 float m_b,
                 int img_height_in,
                 int nChannels);
};

::aie::accum<accfloat, 4> ACC_FLOAT_SUM_ALL_TILES;
int CTR = 0;

void Stddev::runImpl(adf::input_buffer<uint8_t>& input1,
                     adf::output_buffer<uint8_t>& output1,
                     uint32_t reset,
                     float m_r,
                     float m_g,
                     float m_b,
                     int img_height_in,
                     int nChannels) {
    if (reset == 1) {
        ACC_FLOAT_SUM_ALL_TILES = ::aie::zeros<accfloat, 4>();
    }

    //
    uint8* img_in_ptr_y = (uint8*)::aie::begin(input1);
    uint8* img_out_ptr_y = (uint8*)::aie::begin(output1);
    xfCopyMetaData(img_in_ptr_y, img_out_ptr_y);

    const int16_t tile_width_in = xfGetTileWidth(img_in_ptr_y);
    const int16_t tile_width_out = xfGetTileOutTWidth(img_in_ptr_y);
    const int16_t tile_height_in = xfGetTileHeight(img_in_ptr_y);
    const int16_t tile_height_out = xfGetTileOutTHeight(img_in_ptr_y);

    uint8* restrict RGBA_ptr = (uint8*)xfGetImgDataPtr(img_in_ptr_y);
    float* restrict Mean_ptr = (float*)xfGetImgDataPtr(img_out_ptr_y);

    float mean_val_red, mean_val_green, mean_val_blue;
    mean_val_red = m_r;
    mean_val_green = m_g;
    mean_val_blue = m_b;
    ::aie::vector<float, 2 * N> Mean_vec;
    {
        ::aie::vector<float, 4> Mean_vec_small(mean_val_red, mean_val_green, mean_val_blue, 255);
        for (int i = 0; i < 2 * N / 4; i++) Mean_vec.insert(i, Mean_vec_small);
    }

    ::aie::accum<accfloat, 2 * N> accf, accf_temp;
    ::aie::vector<uint8_t, 2 * N> in_vec = ::aie::load_v<2 * N>(RGBA_ptr);
    RGBA_ptr += 2 * N;
    ::aie::vector<float, 2 * N> in_vec_f = ::aie::to_float<float>(in_vec);
    accf = ::aie::mul_square(::aie::sub(in_vec_f, Mean_vec));
    for (int i = 0; i < (tile_height_in * tile_width_in * nChannels) / (N * 2) - 1; i++)
        chess_prepare_for_pipelining chess_loop_range(14, ) {
            // Load 2*N of total pixels per loop
            in_vec = ::aie::load_v<2 * N>(RGBA_ptr);
            accf_temp.template from_vector(::aie::to_float<float>(in_vec));
            RGBA_ptr += 2 * N;
            accf_temp = ::aie::sub(accf_temp, Mean_vec);
            ::aie::vector<bfloat16, 2 * N> sub_temp = accf_temp.template to_vector<bfloat16>(0);
            accf_temp = ::aie::mul_square(sub_temp);
            accf = ::aie::add(accf, accf_temp);
        }
    auto accf_0 = accf.extract<N>(0);
    auto accf_1 = accf.extract<N>(1);
    accf_0 = ::aie::add(accf_0, accf_1); // aie2p = 64, aie=32

    auto accf_00 = accf_0.extract<N / 2>(0);
    auto accf_11 = accf_0.extract<N / 2>(1);
    accf_00 = ::aie::add(accf_00, accf_11); // aie2p = 32, aie=16

    auto accf_000 = accf_00.extract<N / 4>(0);
    auto accf_111 = accf_00.extract<N / 4>(1);
    accf_000 = ::aie::add(accf_000, accf_111); // aie2p = 16, aie=8

    auto accf_0000 = accf_000.extract<N / 8>(0);
    auto accf_1111 = accf_000.extract<N / 8>(1);
    accf_0000 = ::aie::add(accf_0000, accf_1111); // aie2p = 8, aie=4

#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    auto accf_00000 = accf_0000.extract<N / 16>(0);
    auto accf_11111 = accf_0000.extract<N / 16>(1);
    accf_00000 = ::aie::add(accf_00000, accf_11111); // aie2p = 4
    ACC_FLOAT_SUM_ALL_TILES = ::aie::add(ACC_FLOAT_SUM_ALL_TILES, accf_00000);
#else
    ACC_FLOAT_SUM_ALL_TILES = ::aie::add(ACC_FLOAT_SUM_ALL_TILES, accf_0000);
#endif
    ::aie::vector<float, 4> vec_float = ACC_FLOAT_SUM_ALL_TILES.template to_vector<float>(0);
    ::aie::store_v(Mean_ptr, vec_float);
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
