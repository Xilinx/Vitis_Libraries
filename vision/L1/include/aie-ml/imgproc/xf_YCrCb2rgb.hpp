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

#ifndef __XF_YCrCb2rgb_
#define __XF_YCrCb2rgb_

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
class YCrCb2rgb {
   private:
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    static constexpr int N = 64;
#else
    static constexpr int N = 32;
#endif
   public:
    void runImpl(adf::input_buffer<uint8_t>& input,
                 adf::output_buffer<uint8_t>& output,
                 int img_height_in,
                 int nChannels);
    void xg_YcrCb2Rgb(uint8_t* in_ptr, uint8_t* out_ptr, int16_t tile_width, int16_t tile_height);
};
//#define DBG
template <uint8_t IS_RGB>
__attribute__((noinline)) void YCrCb2rgb<IS_RGB>::xg_YcrCb2Rgb(uint8_t* restrict ptr_rgba,
                                                               uint8_t* restrict out_ptr,
                                                               int16_t tile_width,
                                                               int16_t tile_height) {
    ::aie::vector<int16_t, 16> WT(359, -183, -88, 454, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ::aie::vector<uint8_t, N> channel0, channel1, channel3, channel2;
    ::aie::vector<uint8_t, N> y, Cr, Cb, a, r, g, b;
    ::aie::accum<acc32, N> acc, acc_y;

    ::aie::vector<int16, N> const_128 = ::aie::broadcast<int16, N>(128);
    int16 int16_359 = 359;
    int16 int16_183 = 183;
    int16 int16_88 = 88;
    int16 int16_454 = 454;
    int16 int16_256 = 256;
    uint8_t uint8_255 = 255;
    int int_255 = 255;

    for (int j = 0; j < (tile_width / N); j += 1) chess_prepare_for_pipelining { // (tile_width / N)
            channel0 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;
            channel1 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;
            channel2 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;
            channel3 = ::aie::load_v<N>(ptr_rgba);
            ptr_rgba += N;

            auto[temp0, temp1] =
                ::aie::interleave_unzip(::aie::concat(channel0, channel1), ::aie::concat(channel2, channel3), 2);

            y = ::aie::filter_even(temp0, 1);
            Cr = ::aie::filter_odd(temp0, 1);
            Cb = ::aie::filter_even(temp1, 1);
            a = ::aie::filter_odd(temp1, 1);
#ifdef DBG

            ::aie::print(y, true, " y = ");
            ::aie::print(Cr, true, " Cr = ");
            ::aie::print(Cb, true, " Cb = ");

#endif

            ::aie::vector<int16, N> Cr_128 = ::aie::sub(::aie::unpack(Cr).cast_to<int16>(), const_128);
            ::aie::vector<int16, N> Cb_128 = ::aie::sub(::aie::unpack(Cb).cast_to<int16>(), const_128);
            acc_y = ::aie::mul(int16_256, ::aie::unpack(y).cast_to<int16>());

#ifdef DBG
            ::aie::print(Cr_128, true, " Cr_128 = ");
            ::aie::print(Cb_128, true, " Cb_128 = ");
#endif

            acc = ::aie::accumulate<N>(acc_y, WT, 0, Cr_128);
            r = acc.template to_vector<uint8_t>(8);
#ifdef DBG
            ::aie::print(acc.template to_vector<uint8_t>(8), true, " acc = ");
            ::aie::print(r, true, " r = ");
#endif

            acc = ::aie::accumulate<N>(acc_y, WT, 1, Cr_128, Cb_128);
            g = acc.template to_vector<uint8_t>(8);

#ifdef DBG
            ::aie::print(acc.template to_vector<uint8_t>(8), true, " acc = ");
            ::aie::print(g, true, " g = ");
#endif

            acc = ::aie::accumulate<N>(acc_y, WT, 3, Cb_128);
            b = acc.template to_vector<uint8_t>(8);
#ifdef DBG
            ::aie::print(acc.template to_vector<int>(0), true, " acc2 = ");
            ::aie::print(acc.template to_vector<uint8_t>(8), true, " acc = ");
            ::aie::print(b, true, " b = ");
#endif

            if (IS_RGB == 1) {
                auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(r, g, 1);
                auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(b, a, 1);
                auto[x1, y1] =
                    ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
                ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
                out_ptr = out_ptr + 4 * N;
#ifdef DBG
                ::aie::print(::aie::concat(x1, y1), true, " out = ");
#endif

            } else {
                auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(b, g, 1);
                auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(r, a, 1);
                auto[x1, y1] =
                    ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
                ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
                out_ptr = out_ptr + 4 * N;
#ifdef DBG
                ::aie::print(::aie::concat(x1, y1), true, " out = ");
#endif
            }
        }
}

template <uint8_t IS_RGB>
void YCrCb2rgb<IS_RGB>::runImpl(adf::input_buffer<uint8_t>& input,
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
    xg_YcrCb2Rgb(ptr_in, ptr_out, tile_width_in, tile_height_in);
    clr_sat();
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
