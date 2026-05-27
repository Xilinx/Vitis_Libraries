/*
 * Copyright 2022 Xilinx, Inc.
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
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>
#include <common/xf_aie_hw_utils.hpp>

#ifndef __XF_RGBA2HSV__HPP__
#define __XF_RGBA2HSV__HPP__

namespace xf {
namespace cv {
namespace aie {

class Rgba2Hsv {
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    static constexpr int N = 64;             // Vectorization factor
#else
    static constexpr int N = 64; // Vectorization factor
#endif

   public:
    void runImpl(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out);
    void xf_rgba2hsv(uint8_t* in_ptr, uint8_t* out_ptr, uint16_t tile_width, uint16_t tile_height);
};

__attribute__((noinline)) void Rgba2Hsv::xf_rgba2hsv(uint8_t* restrict in_ptr,
                                                     uint8_t* restrict out_ptr,
                                                     uint16_t tile_width,
                                                     uint16_t tile_height) {
    ::aie::vector<uint8_t, N> rgba_channel0, rgba_channel1, rgba_channel2, rgba_channel3;
    ::aie::vector<uint8_t, N> r, g, b, a, h, s, v;
    ::aie::accum<acc64, N> acc_h, acc_s, acc_v;
    ::aie::accum<acc32, N> acc1;
    ::aie::vector<uint8_t, N> v_max, v_min;
    ::aie::vector<uint8_t, N> delta;
    ::aie::vector<int, N> delta1;
    ::aie::vector<int, N> sdiv_180_sel;
    uint16_t more_pixels = 0, loop_count;
    loop_count = (tile_height * tile_width) / N; // Divide by VECTORIZATION-FACTOR
    float one_by_510 = (float)1 / (2.0f * 255.0f);
    bfloat16 one_by_510_bf16 = (bfloat16)1 / (2.0f * 255.0f);
    bfloat16 one_by_255 = (bfloat16)1 / (255.0f);
    ::aie::accum<accfloat, N> acc_f;
    ::aie::vector<uint8_t, N> num_zeroes = ::aie::broadcast<uint8_t, N>(0);

    bfloat16 two_bf16 = 2.0;
    bfloat16 one_bf16 = 1.0;
    bfloat16 four_bf16 = 4.0;
    bfloat16 sixty_bf16 = 60.0;
    bfloat16 zero_bf16 = 0.0;
    bfloat16 three_sixty_bf16 = 360.0;
    bfloat16 one_eighty_bf16 = 180.0;
    bfloat16 point_five_bf16 = 0.5;
    bfloat16 two_five_five_bf16 = 255;
    int zero_int = 0;
    uint8_t zero_uint8 = 0;

    for (int j = 0; j < loop_count; j++) chess_prepare_for_pipelining {

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
            auto [rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(rgba_channel0, rgba_channel1),
                                                              ::aie::concat(rgba_channel2, rgba_channel3), 2);
            r = ::aie::filter_even(rg_temp, 1);
            g = ::aie::filter_odd(rg_temp, 1);
            b = ::aie::filter_even(ba_temp, 1);
            a = ::aie::filter_odd(ba_temp, 1);
            ::aie::vector<bfloat16, N> a_bf16 = ::aie::to_float<bfloat16>(a);

            v_max = ::aie::max(::aie::max(b, r), g);
            v_min = ::aie::min(::aie::min(b, r), g);
            delta = ::aie::sub(v_max, v_min);

            // ************************  S CALCULATION  START *************************//
            // if v_max != 0 then s = (vmax-vmin/vmax) else s = 0
            ::aie::vector<uint16_t, N> add_vmax_vmin = ::aie::add(::aie::unpack(v_max), ::aie::unpack(v_min));

            ::aie::vector<bfloat16, N> denom = ::aie::inv(::aie::to_float<bfloat16>(v_max));
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(delta), denom);
            acc_f = ::aie::mul(acc_f.template to_vector<bfloat16>(), two_five_five_bf16);
            ::aie::vector<uint8_t, N> s_uint8_1 = ::aie::to_fixed<uint8_t>(acc_f.template to_vector<bfloat16>());

            auto m1 = ::aie::eq(v_max, num_zeroes); // ::aie::gt(l_uint8, 127);
            ::aie::vector<uint8_t, N> s_uint8 = ::aie::select(s_uint8_1, (uint8_t)0, m1);

            // ************************  H CALCULATION  START *************************//

            // *** h1 = 60.0f * std::fmod(((g - b) / delta), 6.0f);
            ::aie::vector<int16, N> temp2 = (::aie::sub(::aie::unpack(g), ::aie::unpack(b))).cast_to<int16>();
            ::aie::vector<bfloat16, N> delta_inv = ::aie::inv(::aie::to_float<bfloat16>(delta));
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(temp2), delta_inv);
            acc_f = ::aie::mul(sixty_bf16, acc_f.template to_vector<bfloat16>());
            ::aie::vector<bfloat16, N> h1_bf16 = acc_f.template to_vector<bfloat16>();


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
            acc_f = ::aie::mul(h3_bf16, point_five_bf16);
            h3_bf16 = acc_f.template to_vector<bfloat16>();

            m1 = ::aie::lt(h3_bf16, zero_bf16);
            auto temp1 = ::aie::add(h3_bf16, one_eighty_bf16);
            h3_bf16 = ::aie::select(h3_bf16, temp1, m1);
            //acc_f = ::aie::mul(h3_bf16, point_five_bf16);
            //::aie::vector<uint8_t, N> h_uint8 = ::aie::to_fixed<uint8>(acc_f.template to_vector<bfloat16>(), 0);
            ::aie::vector<uint8_t, N> h_uint8 = ::aie::to_fixed<uint8>(h3_bf16, 0);

            m1 = ::aie::eq(delta, num_zeroes);
            s_uint8 = ::aie::select(s_uint8, zero_uint8, m1);

            h_uint8 = ::aie::select(h_uint8, zero_uint8, m1);


            auto [rg1_tmp, rg2_tmp] = ::aie::interleave_zip(h_uint8, s_uint8, 1);
            auto [ba1_tmp, ba2_tmp] = ::aie::interleave_zip(v_max, a, 1);
            auto [x1, y1] = ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
            ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
            out_ptr = out_ptr + 4 * N;
        }

}

void Rgba2Hsv::runImpl(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out) {
    uint8_t* img_in = (uint8_t*)::aie::begin(in);
    uint8_t* img_out = (uint8_t*)::aie::begin(out);

    int16_t tile_width = xfGetTileWidth(img_in);
    int16_t tile_height = xfGetTileHeight(img_in);

    if (tile_width == 0 || tile_height == 0) return;

    xfCopyMetaData(img_in, img_out);
    xfSetTileWidth(img_out, tile_width);

    xfUnsignedSaturation(img_out);

    uint8_t* in_ptr = (uint8_t*)xfGetImgDataPtr(img_in);
    uint8_t* out_ptr = (uint8_t*)xfGetImgDataPtr(img_out);

    xf_rgba2hsv(in_ptr, out_ptr, tile_width, tile_height);
}

} // aie
} // cv
} // xf
#endif
