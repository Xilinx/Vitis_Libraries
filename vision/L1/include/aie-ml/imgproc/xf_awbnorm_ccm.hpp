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
#ifndef _AIE_AWB_NORM_H_
#define _AIE_AWB_NORM_H_

typedef uint16 COEFF_T;
namespace xf {
namespace cv {
namespace aie {

template <typename T, int N, typename OUT_T>
::aie::vector<OUT_T, N> normUtility(::aie::vector<T, N> in_data, COEFF_T* coeff) {
    COEFF_T fp_coeff = coeff[0];
    COEFF_T fp = coeff[1];
    COEFF_T coeff2 = coeff[2];

    ::aie::accum<acc32, N> acc = ::aie::mul(in_data, fp_coeff);
    ::aie::vector<int16, N> tmp1 = ::aie::sub(acc.template to_vector<int16>(fp), (int16)coeff2);

    ::aie::accum<acc32, N> acc2(tmp1);
    ::aie::vector<OUT_T, N> out_data = acc2.template to_vector<OUT_T>(0);
    return out_data;
}

template <typename T, unsigned int N>
__attribute__((noinline)) void awbNormCCMImpl(
    const T* restrict img_in, const T* restrict img_out, const int16_t (&coeff)[25], int tile_width, int tile_height) {
    COEFF_T* coeff_awb_r = (COEFF_T*)(coeff + 16);
    COEFF_T* coeff_awb_g = (COEFF_T*)(coeff + 16 + 3);
    COEFF_T* coeff_awb_b = (COEFF_T*)(coeff + 16 + 6);
    int16_t* coeff_ccm = (int16_t*)(coeff);

    ::aie::vector<int16_t, N> vcoeff = *(v32int16*)(&coeff_ccm[0]);
    ::aie::vector<int16_t, N> r_off = ::aie::broadcast<int16_t, N>(vcoeff[9]);
    ::aie::vector<int16_t, N> g_off = ::aie::broadcast<int16_t, N>(vcoeff[10]);
    ::aie::vector<int16_t, N> b_off = ::aie::broadcast<int16_t, N>(vcoeff[11]);
    auto zerovec = ::aie::zeros<T, N>();

    set_sat();

    ::aie::vector<T, (N << 1)> x, y;
    for (int i = 0; i < tile_height; i++) chess_prepare_for_pipelining {
            for (int j = 0; j < tile_width; j += N) {
                // load N*4 data in two vectors
                ::aie::vector<T, (N << 1)> load_vec1 = ::aie::load_v<(N << 1)>(img_in);
                ::aie::vector<T, (N << 1)> load_vec2 = ::aie::load_v<(N << 1)>(img_in + (N << 1));
                img_in += (N << 2);

                // pixel interleaved data to planar vectors
                auto[tmp1, tmp2] = ::aie::interleave_unzip(load_vec1, load_vec2, 2);
                auto[r, g] = tmp1.template split<N>();
                auto[b, a] = tmp2.template split<N>();
                std::tie(r, g) = ::aie::interleave_unzip(r, g, 1);
                std::tie(b, a) = ::aie::interleave_unzip(b, a, 1);

                // awb norm
                auto r_norm = normUtility<T, N, T>(r, coeff_awb_r);
                auto g_norm = normUtility<T, N, T>(g, coeff_awb_g);
                auto b_norm = normUtility<T, N, T>(b, coeff_awb_b);

                // CCM
                ::aie::accum<acc32, N> acc_r(r_off);
                ::aie::accum<acc32, N> acc_g(g_off);
                ::aie::accum<acc32, N> acc_b(b_off);
                acc_r = ::aie::accumulate<N>(acc_r, vcoeff, 0, r_norm, g_norm, b_norm);
                acc_g = ::aie::accumulate<N>(acc_g, vcoeff, 3, r_norm, g_norm, b_norm);
                acc_b = ::aie::accumulate<N>(acc_b, vcoeff, 6, r_norm, g_norm, b_norm);

                r = acc_r.template to_vector<T>(12);
                g = acc_g.template to_vector<T>(12);
                b = acc_b.template to_vector<T>(12);

                // planar to pixel interleaved
                std::tie(r, g) = ::aie::interleave_zip(r, g, 1);
                std::tie(b, a) = ::aie::interleave_zip(b, zerovec, 1);
                std::tie(x, y) = ::aie::interleave_zip(::aie::concat(r, g), ::aie::concat(b, a), 2);
                ::aie::store_v((T*)img_out, x);
                ::aie::store_v((T*)(img_out + (N << 1)), y);

                img_out += (N << 2);
            }
        }
}

template <typename T, unsigned int N>
__attribute__((noinline)) void awbNormCCMImpl1(
    T* img_in, T* img_out, const int16_t (&coeff)[25], int tile_width, int tile_height) {
    T* img_in_ptr = img_in;
    T* img_out_ptr = img_out;

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    uint8_t* img_in1 = (uint8_t*)xfGetImgDataPtr(img_in_ptr);
    uint8_t* img_out1 = (uint8_t*)xfGetImgDataPtr(img_out_ptr);

    COEFF_T* coeff_awb_r = (COEFF_T*)(coeff + 16);
    COEFF_T* coeff_awb_g = (COEFF_T*)(coeff + 16 + 3);
    COEFF_T* coeff_awb_b = (COEFF_T*)(coeff + 16 + 6);
    int16_t* coeff_ccm = (int16_t*)(coeff);

    ::aie::vector<int16_t, N> vcoeff = *(v32int16*)(&coeff_ccm[0]);
    ::aie::vector<int16_t, N> r_off = ::aie::broadcast<int16_t, N>(vcoeff[9]);
    ::aie::vector<int16_t, N> g_off = ::aie::broadcast<int16_t, N>(vcoeff[10]);
    ::aie::vector<int16_t, N> b_off = ::aie::broadcast<int16_t, N>(vcoeff[11]);
    auto zerovec = ::aie::zeros<T, N>();

    set_sat();

    ::aie::vector<T, (N << 1)> x, y;
    for (int i = 0; i < tile_height; i++) chess_prepare_for_pipelining {
            for (int j = 0; j < tile_width; j += N) {
                // load N*4 data in two vectors
                ::aie::vector<T, (N << 1)> load_vec1 = ::aie::load_v<(N << 1)>(img_in1);
                ::aie::vector<T, (N << 1)> load_vec2 = ::aie::load_v<(N << 1)>(img_in1 + (N << 1));
                img_in1 += (N << 2);

                // pixel interleaved data to planar vectors
                auto[tmp1, tmp2] = ::aie::interleave_unzip(load_vec1, load_vec2, 2);
                auto[r, g] = tmp1.template split<N>();
                auto[b, a] = tmp2.template split<N>();
                std::tie(r, g) = ::aie::interleave_unzip(r, g, 1);
                std::tie(b, a) = ::aie::interleave_unzip(b, a, 1);

                // awb norm
                auto r_norm = normUtility<T, N, T>(r, coeff_awb_r);
                auto g_norm = normUtility<T, N, T>(g, coeff_awb_g);
                auto b_norm = normUtility<T, N, T>(b, coeff_awb_b);

                // CCM
                ::aie::accum<acc32, N> acc_r(r_off);
                ::aie::accum<acc32, N> acc_g(g_off);
                ::aie::accum<acc32, N> acc_b(b_off);
                acc_r = ::aie::accumulate<N>(acc_r, vcoeff, 0, r_norm, g_norm, b_norm);
                acc_g = ::aie::accumulate<N>(acc_g, vcoeff, 3, r_norm, g_norm, b_norm);
                acc_b = ::aie::accumulate<N>(acc_b, vcoeff, 6, r_norm, g_norm, b_norm);

                r = acc_r.template to_vector<T>(12);
                g = acc_g.template to_vector<T>(12);
                b = acc_b.template to_vector<T>(12);

                // planar to pixel interleaved
                std::tie(r, g) = ::aie::interleave_zip(r, g, 1);
                std::tie(b, a) = ::aie::interleave_zip(b, zerovec, 1);
                std::tie(x, y) = ::aie::interleave_zip(::aie::concat(r, g), ::aie::concat(b, a), 2);
                ::aie::store_v((T*)img_out1, x);
                ::aie::store_v((T*)(img_out1 + (N << 1)), y);

                img_out1 += (N << 2);
            }
        }
}

template <typename T, unsigned int N>
__attribute__((noinline)) void awbnorm_ccm_api(adf::input_buffer<T>& img_in,
                                               const int16_t (&coeff)[25],
                                               adf::output_buffer<T>& img_out) {
    T* restrict img_in_ptr = (T*)::aie::begin(img_in);
    T* restrict img_out_ptr = (T*)::aie::begin(img_out);

    const int16_t img_width = xfGetTileWidth(img_in_ptr);
    const int16_t img_height = xfGetTileHeight(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);
    xfUnsignedSaturation(img_out_ptr);

    T* restrict ptr_in = (T*)xfGetImgDataPtr(img_in_ptr);
    T* restrict ptr_out = (T*)xfGetImgDataPtr(img_out_ptr);

    awbNormCCMImpl<T, N>(ptr_in, ptr_out, coeff, img_width, img_height);
}

} // aie
} // cv
} // xf
#endif // _AIE_AWB_NORM_H_
