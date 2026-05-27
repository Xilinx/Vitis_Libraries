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

#ifndef __XF_RGB2YCrCb_
#define __XF_RGB2YCrCb_

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

template <uint8_t IS_RGB>
class RGB2YCrCb {
   private:
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    static constexpr int N = 64;
#else
    static constexpr int N = 64;
#endif
   public:
    void runImpl(adf::input_buffer<uint8_t>& input,
                 adf::output_buffer<uint8_t>& output,
                 int img_height_in,
                 int nChannels);
    void xf_Rgb2YCrCb(uint8_t* in_ptr, uint8_t* out_ptr, int16_t tile_width, int16_t tile_height);
};

template <uint8_t IS_RGB>
__attribute__((noinline)) void RGB2YCrCb<IS_RGB>::xf_Rgb2YCrCb(uint8_t* restrict ptr_rgba,
                                                               uint8_t* restrict out_ptr,
                                                               int16_t tile_width,
                                                               int16_t tile_height) {
    ::aie::vector<int16_t, 16> WT(77, 150, 29, 183, -183, 144, -144, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ::aie::vector<uint8_t, N> c1 = ::aie::broadcast<uint8_t, N>(128);
    ::aie::vector<uint8_t, N> rgba_channel0, rgba_channel1, rgba_channel3, rgba_channel2;
    ::aie::vector<uint8_t, N> r, g, b, a, y, Cr, Cb;
    ::aie::vector<int, N> v1 = ::aie::broadcast<int, N>(32768);
    ::aie::accum<acc32, N> acc, acc_const(v1);

    for (int j = 0; j < (tile_width / N); j += 1) chess_prepare_for_pipelining {
            rgba_channel0 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;
            rgba_channel1 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;
            rgba_channel2 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;
            rgba_channel3 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;

            auto[rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(rgba_channel0, rgba_channel1),
                                                             ::aie::concat(rgba_channel2, rgba_channel3), 2);
            if (IS_RGB == 1) {
                r = ::aie::filter_even(rg_temp, 1);
                b = ::aie::filter_even(ba_temp, 1);

            } else {
                b = ::aie::filter_even(rg_temp, 1);
                r = ::aie::filter_even(ba_temp, 1);
            }
            g = ::aie::filter_odd(rg_temp, 1);
            a = ::aie::filter_odd(ba_temp, 1);

            acc = ::aie::accumulate<N>(WT, 0, r, g, b);
            y = acc.template to_vector<uint8_t>(8);

            acc = ::aie::accumulate<N>(acc_const, WT, 3, r, y);
            Cr = acc.template to_vector<uint8_t>(8);

            acc = ::aie::accumulate<N>(acc_const, WT, 5, b, y);
            Cb = acc.template to_vector<uint8_t>(8);

            auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(y, Cr, 1);
            auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(Cb, a, 1);
            auto[x1, y1] = ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
            ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
            out_ptr = out_ptr + 4 * N;
        }
}

template <uint8_t IS_RGB>
void RGB2YCrCb<IS_RGB>::runImpl(adf::input_buffer<uint8_t>& input,
                                adf::output_buffer<uint8_t>& output,
                                int img_height_in,
                                int nChannels) {
    uint8* img_in_ptr = (uint8*)::aie::begin(input);
    uint8* img_out_ptr = (uint8*)::aie::begin(output);

    const int16_t tile_width_in = xfGetTileWidth(img_in_ptr);
    const int16_t tile_width_out = xfGetTileOutTWidth(img_in_ptr);
    const int16_t tile_height_in = xfGetTileHeight(img_in_ptr);
    const int16_t tile_height_out = xfGetTileOutTHeight(img_in_ptr);
    xfCopyMetaData(img_in_ptr, img_out_ptr);
    xfSetTileOutTWidth(img_out_ptr, tile_width_out);
    uint8* restrict ptr_in = (uint8*)xfGetImgDataPtr(img_in_ptr);
    uint8* restrict ptr_out = (uint8*)xfGetImgDataPtr(img_out_ptr);
    int row = xfGetTileOutPosV(img_in_ptr);
    set_sat();
    xf_Rgb2YCrCb(ptr_in, ptr_out, tile_width_in, tile_height_in);
    clr_sat();
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
