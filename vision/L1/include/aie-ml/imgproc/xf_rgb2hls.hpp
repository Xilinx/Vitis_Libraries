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

#ifndef __XF_RGB2HLS_
#define __XF_RGB2HLS_

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
class Rgb2hls {
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
    void xf_Rgba2Hls(uint8_t* in_ptr, uint8_t* out_ptr, int16_t tile_width, int16_t tile_height);
};

template <uint8_t IS_RGB>
__attribute__((noinline)) void Rgb2hls<IS_RGB>::xf_Rgba2Hls(uint8_t* restrict in_ptr,
                                                            uint8_t* restrict out_ptr,
                                                            int16_t tile_width,
                                                            int16_t tile_height) {
    ::aie::vector<uint8_t, N> rgba_channel0, rgba_channel1, rgba_channel2, rgba_channel3;
    ::aie::vector<uint8_t, N> r, g, b, a;
    ::aie::accum<acc32, N> acc1;
    ::aie::vector<uint8_t, N> v_max, v_min;
    ::aie::vector<uint8_t, N> delta;
    ::aie::vector<int, N> delta1;
    ::aie::vector<int, N> sdiv_180_sel;
    uint16_t more_pixels = 0, loop_count;
    loop_count = (tile_height * tile_width) / N;
    ::aie::accum<accfloat, N> acc_f;

    bfloat16 two_bf16 = 2.0;
    bfloat16 four_bf16 = 4.0;
    bfloat16 sixty_bf16 = 60.0;
    bfloat16 zero_bf16 = 0.0;
    bfloat16 three_sixty_bf16 = 360.0;
    bfloat16 point_five_bf16 = 0.5;
    bfloat16 two_five_five_bf16 = 255;
    // uint8_t zero_uint8 = 0;
    ::aie::vector<uint8_t, N> zero_uint8 = ::aie::zeros<uint8_t, N>();

    bfloat16 one_by_510_bf16 = (bfloat16)1 / (2.0f * 255.0f);
    uint16 uint16_510 = 510;

    for (int j = 0; j < loop_count; j++) chess_prepare_for_pipelining {
            // printf(" Loop count = %d " , j);

            /* r,g,b extraction */
            rgba_channel0 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            rgba_channel1 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            rgba_channel2 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            rgba_channel3 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;

            // Unzip the interleaved channels
            auto[rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(rgba_channel0, rgba_channel1),
                                                             ::aie::concat(rgba_channel2, rgba_channel3), 2);

            if (IS_RGB == 1) {
                r = ::aie::filter_even(rg_temp, 1);
                g = ::aie::filter_odd(rg_temp, 1);
            } else {
                g = ::aie::filter_even(rg_temp, 1);
                r = ::aie::filter_odd(rg_temp, 1);
            }
            b = ::aie::filter_even(ba_temp, 1);
            a = ::aie::filter_odd(ba_temp, 1);

            v_max = ::aie::max(::aie::max(b, r), g);
            v_min = ::aie::min(::aie::min(b, r), g);
            delta = ::aie::sub(v_max, v_min);

            // ************************  L CALCULATION  START *************************//

            //  *** float l = (maxVal + minVal) / 2.0f
            ::aie::vector<uint16_t, N> add_vmax_vmin = ::aie::add(::aie::unpack(v_max), ::aie::unpack(v_min));
            ::aie::vector<uint8_t, N> l_uint8 = ::aie::pack(::aie::downshift(add_vmax_vmin, 1));

            // l for s calculation
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(add_vmax_vmin), one_by_510_bf16);
            ::aie::vector<bfloat16, N> l_bf16_chk = acc_f.template to_vector<bfloat16>();

            // ************************  S CALCULATION  START *************************//

            // if l>1/2 then 255*(vmax-vmin/(510-(vmax+vim)))
            ::aie::vector<uint16_t, N> sub_vec = ::aie::sub(uint16_510, add_vmax_vmin);
            ::aie::vector<bfloat16, N> denom = ::aie::inv(::aie::to_float<bfloat16>(sub_vec));
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(delta), denom);

            acc_f = ::aie::mul(acc_f.template to_vector<bfloat16>(), two_five_five_bf16);
            ::aie::vector<uint8_t, N> s_uint8_1 = ::aie::to_fixed<uint8_t>(acc_f.template to_vector<bfloat16>());

            // if l<1/2 then 255*(vmax-vmin/(vmax+vim))
            denom = ::aie::inv(::aie::to_float<bfloat16>(add_vmax_vmin));
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(delta), denom);

            acc_f = ::aie::mul(acc_f.template to_vector<bfloat16>(), two_five_five_bf16);
            ::aie::vector<uint8_t, N> s_uint8_2 = ::aie::to_fixed<uint8_t>(acc_f.template to_vector<bfloat16>());

            // if l==1/2 then vmax-vmin
            ::aie::vector<uint8_t, N> s_uint8 = delta;
            ::aie::mask<N> m1 = ::aie::gt(l_bf16_chk, point_five_bf16); // ::aie::gt(l_uint8, 127);
            s_uint8 = ::aie::select(s_uint8, s_uint8_1, m1);

            m1 = ::aie::lt(l_bf16_chk, point_five_bf16);
            s_uint8 = ::aie::select(s_uint8, s_uint8_2, m1);

            // ************************  H CALCULATION  START *************************//

            // *** h1 = 60.0f * std::fmod(((g - b) / delta), 6.0f);
            ::aie::vector<int16, N> temp2 = (::aie::sub(::aie::unpack(g), ::aie::unpack(b))).cast_to<int16>();
            ::aie::vector<bfloat16, N> delta_inv = ::aie::inv(::aie::to_float<bfloat16>(delta));
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(temp2), delta_inv);
            ::aie::vector<bfloat16, N> h1_bf16 = acc_f.template to_vector<bfloat16>();

            // h1_bf16 = ::aie::mul(sixty_bf16, h1_bf16);
            acc_f = ::aie::mul(sixty_bf16, h1_bf16);
            h1_bf16 = acc_f.template to_vector<bfloat16>();

            // *** h2 = 60.0f * (((b - r) / delta) + 2.0f);
            temp2 = (::aie::sub(::aie::unpack(b), ::aie::unpack(r))).cast_to<int16>();
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(temp2), delta_inv);
            acc_f = ::aie::mul(sixty_bf16, ::aie::add(acc_f.template to_vector<bfloat16>(), two_bf16));
            ::aie::vector<bfloat16, N> h2_bf16 = acc_f.template to_vector<bfloat16>();

            //  *** h3 = 60.0f * (((r - g) / delta) + 4.0f);
            temp2 = (::aie::sub(::aie::unpack(r), ::aie::unpack(g))).cast_to<int16>();
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(temp2), delta_inv);
            acc_f = ::aie::mul(sixty_bf16, ::aie::add(acc_f.template to_vector<bfloat16>(), four_bf16));
            ::aie::vector<bfloat16, N> h3_bf16 = acc_f.template to_vector<bfloat16>();

            m1 = ::aie::eq(v_max, r);
            h3_bf16 = ::aie::select(h3_bf16, h1_bf16, m1);
            ::aie::mask<N> m2 = ::aie::eq(v_max, g);
            h3_bf16 = ::aie::select(h3_bf16, h2_bf16, ~m1 & m2);

            m1 = ::aie::lt(h3_bf16, zero_bf16);
            auto temp1 = ::aie::add(h3_bf16, three_sixty_bf16);
            h3_bf16 = ::aie::select(h3_bf16, temp1, m1);
            // h3_bf16 = ::aie::mul(h3_bf16, point_five_bf16);
            acc_f = ::aie::mul(h3_bf16, point_five_bf16);
            ::aie::vector<uint8_t, N> h_uint8 = ::aie::to_fixed<uint8>(acc_f.template to_vector<bfloat16>(), 0);

            // ************************  H CALCULATION  END *************************//

            m1 = ::aie::eq(delta, zero_uint8);
            s_uint8 = ::aie::select(s_uint8, zero_uint8, m1);
            h_uint8 = ::aie::select(h_uint8, zero_uint8, m1);

            auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(h_uint8, l_uint8, 1);
            auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(s_uint8, a, 1);
            auto[x1, y1] = ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
            ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
            out_ptr = out_ptr + 4 * N;
        }
}

template <uint8_t IS_RGB>
void Rgb2hls<IS_RGB>::runImpl(adf::input_buffer<uint8_t>& input,
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

    xf_Rgba2Hls(ptr_in, ptr_out, tile_width_in, tile_height_in);
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
