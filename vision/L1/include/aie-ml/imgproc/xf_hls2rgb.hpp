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

#ifndef __XF_Hls2rgb_
#define __XF_Hls2rgb_

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
class Hls2rgb {
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
    void xf_Hlsa2Rgba(uint8_t* in_ptr, uint8_t* out_ptr, int16_t tile_width, int16_t tile_height);
};
// #define DBG
template <uint8_t IS_RGB>
__attribute__((noinline)) void Hls2rgb<IS_RGB>::xf_Hlsa2Rgba(uint8_t* restrict in_ptr,
                                                             uint8_t* restrict out_ptr,
                                                             int16_t tile_width,
                                                             int16_t tile_height) {
    ::aie::vector<uint8_t, N> hlsa_channel0, hlsa_channel1, hlsa_channel2, hlsa_channel3;
    ::aie::vector<uint8_t, N> h, l, s, a;
    ::aie::accum<accfloat, N> acc_f;

    bfloat16 bf16_2_inv_60 = (bfloat16)(2 / 60.0);
    bfloat16 bf16_2_inv_255 = (bfloat16)2 / 255.0f;
    bfloat16 bf16_inv_255 = (bfloat16)1 / 255.0f;
    bfloat16 bf16_0 = (bfloat16)0;
    bfloat16 bf16_1 = (bfloat16)1;
    bfloat16 bf16_2 = (bfloat16)2;
    bfloat16 bf16_3 = (bfloat16)3;
    bfloat16 bf16_4 = (bfloat16)4;
    bfloat16 bf16_5 = (bfloat16)5;
    bfloat16 bf16_6 = (bfloat16)6;
    bfloat16 bf16_half = (bfloat16)0.5;
    bfloat16 bf16_255 = (bfloat16)255;

    uint16_t loop_count;
    loop_count = (tile_height * tile_width) / N;
    // printf(" Loop count = %d ", loop_count);
    ::aie::vector<uint8_t, N> rg1_tmp, rg2_tmp, ba1_tmp, ba2_tmp;

    for (int j = 0; j < loop_count; j++) chess_prepare_for_pipelining {
            // printf(" Loop count = %d ", j);

            hlsa_channel0 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            hlsa_channel1 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            hlsa_channel2 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;
            hlsa_channel3 = ::aie::load_v<N>(in_ptr);
            in_ptr += N;

            // Unzip the interleaved channels
            auto[rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(hlsa_channel0, hlsa_channel1),
                                                             ::aie::concat(hlsa_channel2, hlsa_channel3), 2);
            h = ::aie::filter_even(rg_temp, 1);
            l = ::aie::filter_odd(rg_temp, 1);
            s = ::aie::filter_even(ba_temp, 1);
            a = ::aie::filter_odd(ba_temp, 1);
#ifdef DBG

            ::aie::print(h, true, " h = ");
            ::aie::print(l, true, " l = ");
            ::aie::print(s, true, " s = ");

#endif

            // H, L, S
            // ::aie::vector<bfloat16, N> H_by_60 = ::aie::mul(::aie::to_float<bfloat16>(h), bf16_2_inv_60);
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(h), bf16_2_inv_60);
            ::aie::vector<bfloat16, N> H_by_60 = acc_f.template to_vector<bfloat16>();

            // ::aie::vector<bfloat16, N> TWO_L = ::aie::mul(::aie::to_float<bfloat16>(l), bf16_2_inv_255);
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(l), bf16_2_inv_255);
            ::aie::vector<bfloat16, N> TWO_L = acc_f.template to_vector<bfloat16>();

            //::aie::vector<bfloat16, N> S = ::aie::mul(::aie::to_float<bfloat16>(s), bf16_inv_255);
            acc_f = ::aie::mul(::aie::to_float<bfloat16>(s), bf16_inv_255);
            ::aie::vector<bfloat16, N> S = acc_f.template to_vector<bfloat16>();
#ifdef DBG

            ::aie::print(H_by_60, true, " H_by_60 = ");
            ::aie::print(TWO_L, true, " TWO_L = ");
            ::aie::print(S, true, " S = ");

#endif

            // Compute C. if(2L<=1) {C = 2LS }else {C = 2S-2LS}
            //::aie::vector<bfloat16, N> TWO_LS = ::aie::mul(TWO_L, S);
            acc_f = ::aie::mul(TWO_L, S);
            ::aie::vector<bfloat16, N> TWO_LS = acc_f.template to_vector<bfloat16>();

            //::aie::vector<bfloat16, N> TWO_S_LS = ::aie::sub(::aie::mul(S, bf16_2), TWO_LS);
            acc_f = ::aie::mul(S, bf16_2);
            acc_f = ::aie::sub(acc_f, TWO_LS);
            ::aie::vector<bfloat16, N> TWO_S_LS = acc_f.template to_vector<bfloat16>();

            ::aie::mask<N> m1 = ::aie::gt(TWO_L, bf16_1);
            ::aie::vector<bfloat16, N> C = ::aie::select(TWO_LS, TWO_S_LS, m1);

#ifdef DBG

            ::aie::print(TWO_LS, true, " TWO_LS = ");
            ::aie::print(TWO_S_LS, true, " TWO_S_LS = ");
            ::aie::print(C, true, " C  = ");

#endif

            // Compute X.
            /*if (H / 60.0 <= 1) {X = C * (H / 60.0);} else if (H/60.0 > 1 & H / 60.0 <= 3){X = C * std::abs(2 - H
             / 60.0);} else if (H/60.0 > 3 & H / 60.0 <= 5){ X = C * std::abs(4 - H / 60.0);} else {X = C * (6 - H
             / 60.0);}*/
            ::aie::vector<bfloat16, N> X = H_by_60;
            m1 = ::aie::gt(H_by_60, bf16_1);
            ::aie::mask<N> m2 = ::aie::le(H_by_60, bf16_3);
            X = ::aie::select(X, ::aie::abs(::aie::sub(bf16_2, H_by_60)), m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(X, true, " 1. X = ");

#endif

            m1 = ::aie::gt(H_by_60, bf16_3);
            m2 = ::aie::le(H_by_60, bf16_5);
            X = ::aie::select(X, ::aie::abs(::aie::sub(bf16_4, H_by_60)), m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(X, true, " 2.X = ");

#endif

            m1 = ::aie::gt(H_by_60, bf16_5);
            X = ::aie::select(X, ::aie::sub(bf16_6, H_by_60), m1);

#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(X, true, " 3. X = ");

#endif
            // X = ::aie::mul(X, C);
            acc_f = ::aie::mul(X, C);
            X = acc_f.template to_vector<bfloat16>();

#ifdef DBG
            ::aie::print(X, true, " 4. X = ");

#endif
            // Compute  R_prime, G_prime, B_prime
            ::aie::vector<bfloat16, N> R_prime = ::aie::zeros<bfloat16, N>();
            ::aie::vector<bfloat16, N> G_prime = ::aie::zeros<bfloat16, N>();
            ::aie::vector<bfloat16, N> B_prime = ::aie::zeros<bfloat16, N>();

            m1 = ::aie::ge(H_by_60, bf16_0);
            m2 = ::aie::lt(H_by_60, bf16_1);
            R_prime = ::aie::select(R_prime, C, m1 & m2);
            G_prime = ::aie::select(G_prime, X, m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(R_prime, true, " 1. R_prime = ");
            ::aie::print(G_prime, true, " 1. G_prime = ");

#endif

            m1 = ::aie::ge(H_by_60, bf16_1);
            m2 = ::aie::lt(H_by_60, bf16_2);
            R_prime = ::aie::select(R_prime, X, m1 & m2);
            G_prime = ::aie::select(G_prime, C, m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(R_prime, true, " 2. R_prime = ");
            ::aie::print(G_prime, true, " 2. G_prime = ");

#endif

            m1 = ::aie::ge(H_by_60, bf16_2);
            m2 = ::aie::lt(H_by_60, bf16_3);
            G_prime = ::aie::select(G_prime, C, m1 & m2);
            B_prime = ::aie::select(B_prime, X, m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(R_prime, true, " 3. G_prime = ");
            ::aie::print(G_prime, true, " 3. B_prime = ");

#endif

            m1 = ::aie::ge(H_by_60, bf16_3);
            m2 = ::aie::lt(H_by_60, bf16_4);
            G_prime = ::aie::select(G_prime, X, m1 & m2);
            B_prime = ::aie::select(B_prime, C, m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(R_prime, true, " 4. G_prime = ");
            ::aie::print(G_prime, true, " 4. B_prime = ");

#endif

            m1 = ::aie::ge(H_by_60, bf16_4);
            m2 = ::aie::lt(H_by_60, bf16_5);
            R_prime = ::aie::select(R_prime, X, m1 & m2);
            B_prime = ::aie::select(B_prime, C, m1 & m2);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(R_prime, true, " 5. R_prime = ");
            ::aie::print(G_prime, true, " 5. B_prime = ");

#endif

            m1 = ::aie::ge(H_by_60, bf16_5);
            R_prime = ::aie::select(R_prime, C, m1);
            B_prime = ::aie::select(B_prime, X, m1);
#ifdef DBG

            ::aie::print(m1, true, " m1 = ");
            ::aie::print(m2, true, " m2  = ");
            ::aie::print(R_prime, true, " 6. R_prime = ");
            ::aie::print(G_prime, true, " 6. B_prime = ");

#endif

            // m = L - C/2 = (2L - C) /2
            //::aie::vector<bfloat16, N> m = ::aie::mul(::aie::sub(TWO_L, C), bf16_half);
            acc_f = ::aie::mul(::aie::sub(TWO_L, C), bf16_half);
            ::aie::vector<bfloat16, N> m = acc_f.template to_vector<bfloat16>();

#ifdef DBG

            ::aie::print(m, true, " m = ");

#endif

            // compute R, G, B and store
            acc_f = ::aie::mul(::aie::add(R_prime, m), bf16_255);
            ::aie::vector<uint8_t, N> r_uint8 = ::aie::to_fixed<uint8>(acc_f.template to_vector<bfloat16>(), 0);
#ifdef DBG

            ::aie::print(acc_f.template to_vector<bfloat16>(), true, " 1.accf = ");
            ::aie::print(r_uint8, true, " r_uint8  = ");
#endif

            acc_f = ::aie::mul(::aie::add(G_prime, m), bf16_255);
            ::aie::vector<uint8_t, N> g_uint8 = ::aie::to_fixed<uint8>(acc_f.template to_vector<bfloat16>(), 0);
#ifdef DBG

            ::aie::print(acc_f.template to_vector<bfloat16>(), true, " 1.accf = ");
            ::aie::print(g_uint8, true, " r_uint8  = ");
#endif

            acc_f = ::aie::mul(::aie::add(B_prime, m), bf16_255);
            ::aie::vector<uint8_t, N> b_uint8 = ::aie::to_fixed<uint8>(acc_f.template to_vector<bfloat16>(), 0);
#ifdef DBG

            ::aie::print(acc_f.template to_vector<bfloat16>(), true, " 1.accf = ");
            ::aie::print(b_uint8, true, " r_uint8  = ");
#endif

            if (IS_RGB == 1) {
                auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(r_uint8, g_uint8, 1);
                auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(b_uint8, a, 1);
                auto[x1, y1] =
                    ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
                ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
                out_ptr = out_ptr + 4 * N;

            } else {
                auto[rg1_tmp, rg2_tmp] = ::aie::interleave_zip(b_uint8, g_uint8, 1);
                auto[ba1_tmp, ba2_tmp] = ::aie::interleave_zip(r_uint8, a, 1);
                auto[x1, y1] =
                    ::aie::interleave_zip(::aie::concat(rg1_tmp, rg2_tmp), ::aie::concat(ba1_tmp, ba2_tmp), 2);
                ::aie::store_v((uint8_t*)out_ptr, ::aie::concat(x1, y1));
                out_ptr = out_ptr + 4 * N;
            }
        }
}

template <uint8_t IS_RGB>
void Hls2rgb<IS_RGB>::runImpl(adf::input_buffer<uint8_t>& input,
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

    xf_Hlsa2Rgba(ptr_in, ptr_out, tile_width_in, tile_height_in);
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
