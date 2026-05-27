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

#ifndef __XF_HSV2RGBA__HPP__
#define __XF_HSV2RGBA__HPP__

namespace xf {
namespace cv {
namespace aie {

class Hsv2Rgba {
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    static constexpr int N = 64;             // Vectorization factor
#else
    static constexpr int N = 64; // Vectorization factor
#endif

   public:
   private:

   public:
    void runImpl(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out);
    void xf_hsv2rgba(uint8_t* in_ptr, uint8_t* out_ptr, uint16_t tile_width, uint16_t tile_height);
};

__attribute__((noinline)) void Hsv2Rgba::xf_hsv2rgba(uint8_t* restrict in_ptr,
                                                     uint8_t* restrict out_ptr,
                                                     uint16_t tile_width,
                                                     uint16_t tile_height) {
    ::aie::vector<uint8_t, N> hsv_channel0, hsv_channel1, hsv_channel2, hsv_channel3;
    ::aie::vector<uint8_t, N> r, g, b, a, h, s, v;
    ::aie::accum<acc64, N> acc_s, acc_h, acc_v;
    ::aie::accum<acc32, N> acc_h1,acc_h2,acc_h3;
    ::aie::accum<acc32, N> acc1;
    uint16_t more_pixels = 0, loop_count;
    loop_count = (tile_height * tile_width) / N; // Divide by VECTORIZATION-FACTOR
    ::aie::accum<accfloat, N> acc_f;
    ::aie::accum<accfloat, N> acc_vs;
    ::aie::accum<accfloat, N> acc_vsh;

    bfloat16 two_bf16 = 2.0;
    bfloat16 one_bf16 = 1.0;
    bfloat16 four_bf16 = 4.0;
    bfloat16 six_bf16 = 6.0;
    bfloat16 sixty_bf16 = 60.0;
    bfloat16 thirty_bf16 = 30.0;
    bfloat16 zero_bf16 = 0.0;
    bfloat16 one_eighty_bf16 = 180.0;
    bfloat16 three_sixty_bf16 = 360.0;
    bfloat16 point_five_bf16 = 0.5;
    bfloat16 two_five_five_bf16 = 255.0;
    bfloat16 one_by_255 = one_bf16/two_five_five_bf16;
    bfloat16 one_by_30  = one_bf16/thirty_bf16;
    bfloat16 h_scale = six_bf16/one_eighty_bf16;
    ::aie::vector<bfloat16, N> out_tab[4];
    ::aie::vector<uint8_t, N> vec_add_sub;
    uint16_t one_uint16 = 1;
    ::aie::vector<uint8_t, N> num_zeroes = ::aie::broadcast<uint8_t, N>(0);
    ::aie::vector<uint8_t, N> num_sixes = ::aie::broadcast<uint8_t, N>(6);
    ::aie::vector<uint8_t, N> num_ones = ::aie::broadcast<uint8_t, N>(1);
    ::aie::vector<uint8_t, N> num_twos = ::aie::broadcast<uint8_t, N>(2);
    ::aie::vector<uint8_t, N> num_threes = ::aie::broadcast<uint8_t, N>(3);
    ::aie::vector<uint8_t, N> num_fours = ::aie::broadcast<uint8_t, N>(4);
    ::aie::vector<uint8_t, N> num_fives = ::aie::broadcast<uint8_t, N>(5);

    for (int j = 0; j < loop_count; j++) {
        // h,s,v extraction 
        hsv_channel0 = ::aie::load_v<N>(in_ptr);
        in_ptr += N;
        hsv_channel1 = ::aie::load_v<N>(in_ptr);
        in_ptr += N;
        hsv_channel2 = ::aie::load_v<N>(in_ptr);
        in_ptr += N;
        hsv_channel3 = ::aie::load_v<N>(in_ptr);
        in_ptr += N;

        // Unzip the interleaved channels
        auto[hs_temp, va_temp] = ::aie::interleave_unzip(::aie::concat(hsv_channel0, hsv_channel1),
                                                         ::aie::concat(hsv_channel2, hsv_channel3), 2);
        h = ::aie::filter_even(hs_temp, 1);
        s = ::aie::filter_odd(hs_temp, 1);
        v = ::aie::filter_even(va_temp, 1);
        a = ::aie::filter_odd(va_temp, 1);
            
        //Find the region which is divided into 6 parts each of 30deg
        acc_f = ::aie::mul(::aie::to_float<bfloat16>(h), one_by_30);
        ::aie::vector<bfloat16, N> region_bf16 = acc_f.template to_vector<bfloat16>(0);
        ::aie::vector<uint8_t, N> region = ::aie::to_fixed<uint8_t>(region_bf16, 0);
        ::aie::vector<bfloat16, N> region_bfloat16 = ::aie::to_float<bfloat16>(region);
        ::aie::mask<N> m1 = ::aie::gt(region_bfloat16,region_bf16);
        region = ::aie::select(region,::aie::sub(region, (uint8_t)1), m1);

        //Scale remainder to 0-255
        ::aie::accum<acc32, N> acc_v = ::aie::mul(region, (uint8_t)30);
        ::aie::vector<uint8_t, N> h_mod_30 = ::aie::sub(h, acc_v.template to_vector<uint8_t>(0));
        auto acc_v1 = ::aie::mul(h_mod_30, (uint8_t)255);
        acc_f = ::aie::mul(::aie::to_float<bfloat16>(acc_v1.template to_vector<uint16_t>(0)), one_by_30);
        ::aie::vector<bfloat16, N> remainder_bf16 = acc_f.template to_vector<bfloat16>(0);
        ::aie::vector<uint8_t, N> remainder = ::aie::to_fixed<uint8_t>(remainder_bf16, 0);
        
        //get floor value of h_bfloat16 and do h = h - floor(h)
        // if h < 0 => h = h + 6.0
        m1 = ::aie::lt(region, num_zeroes);
        vec_add_sub = ::aie::add(region, (uint8_t)6);
        region = ::aie::select(region, vec_add_sub, m1);

        // if h >= 6.0 => h = h - 6.0
        m1 = ::aie::ge(region, num_sixes);
        vec_add_sub = ::aie::sub(region, (uint8_t)6);
        region = ::aie::select(region, vec_add_sub, m1);
        
        //p = (v * (255 - s)) / 255;
        ::aie::vector<uint8_t, N> vec_sub = ::aie::sub((uint8_t)255, s);
        acc_v = ::aie::mul(v, vec_sub);
        ::aie::vector<bfloat16, N> vec_mul = ::aie::to_float<bfloat16>(acc_v.template to_vector<uint16_t>(0));
        acc_f = ::aie::mul(vec_mul, one_by_255);
        ::aie::vector<bfloat16, N> p_bf16 = acc_f.template to_vector<bfloat16>(0);
        ::aie::vector<uint8_t, N> p = ::aie::to_fixed<uint8_t>(p_bf16, 0);
        
        //q = (v * (255 - (s * remainder) / 255)) / 255;
        acc_v = ::aie::mul(s, remainder);
        acc_f = ::aie::mul(::aie::to_float<bfloat16>(acc_v.template to_vector<uint16_t>(0)), one_by_255);
        ::aie::vector<bfloat16, N> q_bf16 = acc_f.template to_vector<bfloat16>(0);
        ::aie::vector<uint8_t, N> q = ::aie::to_fixed<uint8_t>(q_bf16, 0);
        vec_sub = ::aie::sub((uint8_t)255, q);
        acc_v = ::aie::mul(v, vec_sub);
        vec_mul = ::aie::to_float<bfloat16>(acc_v.template to_vector<uint16_t>(0));
        acc_f = ::aie::mul(vec_mul, one_by_255);
        q_bf16 = acc_f.template to_vector<bfloat16>(0);
        q = ::aie::to_fixed<uint8_t>(q_bf16, 0);
        
        //t = (v * (255 - (s * (255 - remainder)) / 255)) / 255;
        vec_sub = ::aie::sub((uint8_t)255, remainder);
        acc_v = ::aie::mul(s, vec_sub);
        vec_mul = ::aie::to_float<bfloat16>(acc_v.template to_vector<uint16_t>(0));
        acc_f = ::aie::mul(vec_mul, one_by_255);
        ::aie::vector<bfloat16, N> t_bf16 = acc_f.template to_vector<bfloat16>(0);
        ::aie::vector<uint8_t, N> t = ::aie::to_fixed<uint8_t>(t_bf16, 0);
        vec_sub = ::aie::sub((uint8_t)255, t);
        acc_v = ::aie::mul(v, vec_sub);
        vec_mul = ::aie::to_float<bfloat16>(acc_v.template to_vector<uint16_t>(0));
        acc_f = ::aie::mul(vec_mul, one_by_255);
        t_bf16 = acc_f.template to_vector<bfloat16>(0);
        t = ::aie::to_fixed<uint8_t>(t_bf16, 0);

        // region == 0 => b = p, g = t, r = v
        // region == 1 => b = p, g = v, r = q
        // region == 2 => b = t, g = v, r = p
        // region == 3 => b = v, g = q, r = p
        // region == 4 => b = v, g = p, r = t
        // region == 5 => b = q, g = p, r = v
        auto b = ::aie::select(::aie::select(::aie::select(::aie::select(::aie::select(q
                               , v, ::aie::eq(region, num_fours))
                               , v, ::aie::eq(region, num_threes))
                               , t, ::aie::eq(region, num_twos))
                               , p, ::aie::eq(region, num_ones))
                               , p, ::aie::eq(region, num_zeroes));

        auto g = ::aie::select(::aie::select(::aie::select(::aie::select(::aie::select(p
                               , p, ::aie::eq(region, num_fours))
                               , q, ::aie::eq(region, num_threes))
                               , v, ::aie::eq(region, num_twos))
                               , v, ::aie::eq(region, num_ones))
                               , t, ::aie::eq(region, num_zeroes));

        auto r = ::aie::select(::aie::select(::aie::select(::aie::select(::aie::select(v
                               , t, ::aie::eq(region, num_fours))
                               , p, ::aie::eq(region, num_threes))
                               , p, ::aie::eq(region, num_twos))
                               , q, ::aie::eq(region, num_ones))
                               , v, ::aie::eq(region, num_zeroes));

        // s == 0 => b = v, g = v, r = v else above values
        b = ::aie::select(b, v, ::aie::eq(s, num_zeroes));
        g = ::aie::select(g, v, ::aie::eq(s, num_zeroes));
        r = ::aie::select(r, v, ::aie::eq(s, num_zeroes));

        auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(r, g, 1);
        auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(b, a, 1);
        auto[x1, y1] = ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
        ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
        out_ptr = out_ptr + 4 * N;
    }
}

void Hsv2Rgba::runImpl(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out) {
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

    xf_hsv2rgba(in_ptr, out_ptr, tile_width, tile_height);
}

} // aie
} // cv
} // xf
#endif
