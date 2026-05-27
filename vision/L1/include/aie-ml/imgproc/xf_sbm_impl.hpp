/*

 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.

 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.

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

#ifndef __xf_sbm_IMPL_HPP__

#define __xf_sbm_IMPL_HPP__

// #define KERNEL_DEBUG

#include <adf.h>

#include <aie_api/aie.hpp>

#include <aie_api/utils.hpp>

#include <common/xf_aie_hw_utils.hpp>

#include <common/xf_aie_utils.hpp>

// Local Buffers

#define COST_BUF_SIZE 240

alignas(::aie::vector_decl_align) uint16_t vert_row_sum_buffer[COST_BUF_SIZE * 64] = {0};

alignas(::aie::vector_decl_align) uint16_t temp_cost_buffer[16 * 64] = {0};

alignas(::aie::vector_decl_align) int32_t temp_cost_buffer_32[16 * 64] = {0};

alignas(::aie::vector_decl_align) uint16_t texture_buffer[64] = {0};

alignas(::aie::vector_decl_align) int16_t tmp_buffer[16];

int ITER = 0;

::aie::vector<uint8_t, 64> METADATA;

namespace xf {

namespace cv {

namespace aie {

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

class SbmBaseImpl {
   public:
    void runImpl(adf::input_buffer<uint8_t>& img_in_1,

                 adf::input_buffer<uint8_t>& img_in_2,

                 adf::input_buffer<uint8_t>& img_in_3,

                 adf::input_buffer<uint8_t>& img_in_4,

                 adf::input_buffer<uint8_t>& img_in_5,

                 adf::output_buffer<int16_t>& out);

    void xf_sbm(uint8_t* restrict img_in_1,

                uint8_t* restrict img_in_2,

                uint8_t* restrict img_in_3,

                uint8_t* restrict img_in_4,

                int16_t* restrict out1);

    ::aie::vector<uint16_t, 32> xf_find_min(::aie::vector<uint16_t, 32> curr_cost,

                                            ::aie::vector<uint8_t, 32> curr_disparity,

                                            uint16_t* min_ptr,

                                            uint8_t* disp_ptr);

    void xf_interp(::aie::vector<uint16_t, 16> min_cost,

                   ::aie::vector<uint16_t, 16> d,

                   ::aie::vector<uint16_t, 16> p,

                   ::aie::vector<uint16_t, 16> n,

                   int16_t* interpolation_ptr);

    ::aie::vector<float, 16> xf_reciprocal(::aie::vector<int32_t, 16> x);

    ::aie::vector<float, 16> xf_reciprocal_bfloat16_lib(::aie::vector<bfloat16, 16> x);

    ::aie::mask<16> xf_find_uniqueness_threshold(::aie::vector<int32_t, 16> min1,

                                                 ::aie::vector<int32_t, 16> min2,

                                                 ::aie::vector<int32_t, 16> min3,

                                                 ::aie::vector<int32_t, 16> min4);

    ::aie::vector<uint16_t, 64> xf_find_texture_threshold(uint16_t* texture_ptr, uint8_t* add_ptr, uint8_t* sub_ptr);

    ::aie::vector<uint16_t, 64> xf_find_col_sum(::aie::vector<uint16_t, 64> v_row_sum);

    void print_all(::aie::vector<int32_t, 16> V1_min1_3,

                   ::aie::vector<int32_t, 16> V1_min2_3,

                   ::aie::vector<int32_t, 16> V1_min3_3,

                   ::aie::vector<int32_t, 16> V1_min4_3,

                   ::aie::vector<int32_t, 16> V1_right_3,

                   ::aie::vector<int32_t, 16> V1_left_3);
};

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((noinline)) void SbmBaseImpl<TILE_OUT_HEIGHT,

                                           NO_DISPARITIES,

                                           TILE_WINSIZE,

                                           UNIQUENESS_RATIO,

                                           TEXTURE_THRESHOLD,

                                           FILTERED,

                                           TILE_IN_HEIGHT>::print_all(::aie::vector<int32_t, 16> V1_min1_3,

                                                                      ::aie::vector<int32_t, 16> V1_min2_3,

                                                                      ::aie::vector<int32_t, 16> V1_min3_3,

                                                                      ::aie::vector<int32_t, 16> V1_min4_3,

                                                                      ::aie::vector<int32_t, 16> V1_right_3,

                                                                      ::aie::vector<int32_t, 16> V1_left_3) {
    ::aie::accum<acc32, 16> acc1, acc_right, acc_left;

    acc1.template from_vector(V1_min1_3);

    auto min_vec = acc1.template to_vector<uint16_t>(8);

    ::aie::print(min_vec, true, "  V1_min1_3= ");

    acc1.template from_vector(V1_min2_3);

    min_vec = acc1.template to_vector<uint16_t>(8);

    ::aie::print(min_vec, true, "  V1_min2_3= ");

    acc1.template from_vector(V1_min3_3);

    min_vec = acc1.template to_vector<uint16_t>(8);

    ::aie::print(min_vec, true, "  V1_min3_3= ");

    acc1.template from_vector(V1_min4_3);

    min_vec = acc1.template to_vector<uint16_t>(8);

    ::aie::print(min_vec, true, "  V1_min4_3= ");

    auto D1 = ::aie::sub(NO_DISPARITIES - 1, ::aie::bit_and(0xFF, V1_min1_3));

    auto D2 = ::aie::sub(NO_DISPARITIES - 1, ::aie::bit_and(0xFF, V1_min2_3));

    auto D3 = ::aie::sub(NO_DISPARITIES - 1, ::aie::bit_and(0xFF, V1_min3_3));

    ::aie::print(D1, true, "  D1= ");

    ::aie::print(D2, true, "  D2= ");

    ::aie::print(D3, true, "  D3= ");

    acc_right = ::aie::vector_cast<acc32>(V1_right_3);

    acc_left = ::aie::vector_cast<acc32>(V1_left_3);

    ::aie::print(acc_left.to_vector<uint16_t>(8), true, "  V1_left_3= ");

    ::aie::print(acc_right.to_vector<uint16_t>(8), true, "  V1_right_3= ");
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((always_inline))::aie::mask<16>

SbmBaseImpl<TILE_OUT_HEIGHT,

            NO_DISPARITIES,

            TILE_WINSIZE,

            UNIQUENESS_RATIO,

            TEXTURE_THRESHOLD,

            FILTERED,

            TILE_IN_HEIGHT>::xf_find_uniqueness_threshold(::aie::vector<int32_t, 16> min1,

                                                          ::aie::vector<int32_t, 16> min2,

                                                          ::aie::vector<int32_t, 16> min3,

                                                          ::aie::vector<int32_t, 16> min4) {
    ::aie::accum<acc32, 16> acc1, acc2;

    acc1.template from_vector(min1);

    ::aie::vector<uint16_t, 16> first_min = acc1.template to_vector<uint16_t>(8);

    auto D1 = ::aie::sub(NO_DISPARITIES - 1, ::aie::bit_and(0xFF, min1));

    auto D2 = ::aie::sub(NO_DISPARITIES - 1, ::aie::bit_and(0xFF, min2));

    auto D3 = ::aie::sub(NO_DISPARITIES - 1, ::aie::bit_and(0xFF, min3));

    D2 = ::aie::abs(::aie::sub(D1, D2));

    auto m1 = ::aie::eq(1, D2);

    D3 = ::aie::abs(::aie::sub(D1, D3));

    auto m2 = ::aie::eq(1, D3) & m1;

    min2 = ::aie::select(min2, min3, m1);

    min2 = ::aie::select(min2, min4, m2);

    acc1.template from_vector(min2);

    ::aie::vector<uint16_t, 16> second_min = acc1.template to_vector<uint16_t>(8);

    uint8_t hundred = 100;

    acc1 = ::aie::mul(second_min, hundred);

    acc2 = ::aie::mul(first_min, hundred);

    acc2 = ::aie::mac(acc2, first_min, (uint16_t)UNIQUENESS_RATIO);

    auto bcond = ::aie::le(acc1.template to_vector<int32_t>(0), acc2.template to_vector<int32_t>(0));

    return bcond;
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((always_inline))::aie::vector<uint16_t, 64>

SbmBaseImpl<TILE_OUT_HEIGHT,

            NO_DISPARITIES,

            TILE_WINSIZE,

            UNIQUENESS_RATIO,

            TEXTURE_THRESHOLD,

            FILTERED,

            TILE_IN_HEIGHT>::xf_find_col_sum(::aie::vector<uint16_t, 64> V_row_sum) {
    constexpr int TILE_WINSIZE_IN = TILE_WINSIZE;

    ::aie::vector<uint16_t, 64> zero_vec = ::aie::broadcast<uint16_t, 64>(0);

    ::aie::vector<uint16_t, 64> SUM1 = ::aie::add(::aie::shuffle_down_fill(V_row_sum, zero_vec, 1), V_row_sum); // c1+c2

    ::aie::vector<uint16_t, 64> SUM2 = ::aie::add(SUM1, ::aie::shuffle_down_fill(SUM1, zero_vec, 2)); // c1+c2+c3+c4

    ::aie::vector<uint16_t, 64> SUM3 = ::aie::add(SUM2, ::aie::shuffle_down_fill(SUM2, zero_vec, 4)); // c1+...+c8

    if (TILE_WINSIZE_IN == 5) { // c1+...+c4 + (c5)

        SUM3 = ::aie::add(SUM2, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 4));
    }

    if (TILE_WINSIZE_IN == 7) { // c1+...+c8 - (c8)

        SUM3 = ::aie::sub(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 7));
    }

    if (TILE_WINSIZE_IN == 9) { // c1+...+c8 + (c9)

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 8));
    }

    if (TILE_WINSIZE_IN == 11) { // c1+...+c8 + (c9, (c10+c11))

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 8));

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM1, zero_vec, 9));
    }

    if (TILE_WINSIZE_IN == 13) { // c1+...+c8 + (c9+...+c16) - (c16,(c15+c14))

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM3, zero_vec, 8));

        SUM3 = ::aie::sub(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 15));

        SUM3 = ::aie::sub(SUM3, ::aie::shuffle_down_fill(SUM1, zero_vec, 13));
    }

    if (TILE_WINSIZE_IN == 15) { // c1+...+c8 + (c9+...+c16) - (c16)

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM3, zero_vec, 8));

        SUM3 = ::aie::sub(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 15));
    }

    if (TILE_WINSIZE_IN == 17) { // c1+...+c8 + (c9+...+c16) + (c17)

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM3, zero_vec, 8));

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 16));
    }

    if (TILE_WINSIZE_IN == 19) { // c1+...+c8 + (c9+...+c16) + (c17,(c18+c19))

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM3, zero_vec, 8));

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 16));

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM1, zero_vec, 17));
    }

    if (TILE_WINSIZE_IN == 21) { // c1+...+c8 + (c9+...+c16) + (c17,(c18+c19+c820+c21))

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM3, zero_vec, 8));

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(V_row_sum, zero_vec, 16));

        SUM3 = ::aie::add(SUM3, ::aie::shuffle_down_fill(SUM2, zero_vec, 17));
    }

    return SUM3;
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((always_inline))::aie::vector<uint16_t, 64>

SbmBaseImpl<TILE_OUT_HEIGHT,

            NO_DISPARITIES,

            TILE_WINSIZE,

            UNIQUENESS_RATIO,

            TEXTURE_THRESHOLD,

            FILTERED,

            TILE_IN_HEIGHT>::xf_find_texture_threshold(uint16_t* texture_buffer_ptr,

                                                       uint8_t* ladd_ptr,

                                                       uint8_t* lsub_ptr) {
    ::aie::accum<acc32, 64> acc1;

    ::aie::vector<uint16_t, 64> V_texture_sum = ::aie::load_v<64>(texture_buffer_ptr);

    auto v11 = ::aie::broadcast<uint8_t, 64>(31);

    ::aie::vector<uint8_t, 64> V_left_sub = ::aie::load_v<64>(lsub_ptr);

    ::aie::vector<uint8_t, 64> V_left_add = ::aie::load_v<64>(ladd_ptr);

    acc1.template from_vector(V_left_sub);

    acc1 = ::aie::sub(acc1, v11);

    ::aie::vector<int16_t, 64> V_diff = ::aie::abs(acc1.template to_vector<int16_t>(0));

    V_texture_sum = ::aie::sub(V_texture_sum, V_diff.cast_to<uint16_t>());

    acc1.template from_vector(V_left_add);

    acc1 = ::aie::sub(acc1, v11);

    V_diff = ::aie::abs(acc1.template to_vector<int16_t>(0));

    V_texture_sum = ::aie::add(V_texture_sum, V_diff.cast_to<uint16_t>());

    ::aie::store_v(texture_buffer_ptr, V_texture_sum);

    auto SUM = xf_find_col_sum(V_texture_sum);

    return SUM;
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((always_inline))::aie::vector<float, 16>

SbmBaseImpl<TILE_OUT_HEIGHT,

            NO_DISPARITIES,

            TILE_WINSIZE,

            UNIQUENESS_RATIO,

            TEXTURE_THRESHOLD,

            FILTERED,

            TILE_IN_HEIGHT>::xf_reciprocal_bfloat16_lib(::aie::vector<bfloat16, 16> x) {
    const ::aie::vector<int16, 16> magic = ::aie::broadcast<int16, 16>(0x7eb5);

    const ::aie::vector<bfloat16, 16> k1 = ::aie::broadcast<bfloat16, 16>(1.9395974f);

    const ::aie::vector<bfloat16, 16> k2 = ::aie::broadcast<bfloat16, 16>(1.436142f);

    const ::aie::vector<bfloat16, 16> ones = ::aie::broadcast<bfloat16, 16>(1.0f);

    // init acc

    ::aie::accum<accfloat, 16> acc;

    // int16 i = *(int16*)&x; i = 0x7eb5 - i; bfloat16 y = *(bfloat16*)&i;

    ::aie::vector<bfloat16, 16> y = ::aie::sub(magic, x.cast_to<int16>()).cast_to<bfloat16>();

    // y = k1*y*((-x)*y + k2);

    acc = ::aie::negmul(x, y);

    acc = ::aie::add(acc, k2);

    acc = ::aie::mul(acc.to_vector<bfloat16>(), y);

    acc = ::aie::mul(acc.to_vector<bfloat16>(), k1);

    y = acc.to_vector<bfloat16>();

    // bfloat16 r = y*(-x) + 1.0f;

    acc = ::aie::negmul(x, acc.to_vector<bfloat16>());

    acc = ::aie::add(acc, ones);

    acc = ::aie::mul(y, acc.to_vector<bfloat16>());

    acc = ::aie::add(acc, y);

    ::aie::vector<float, 16> y_float = acc.to_vector<float>();

    return y_float;
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((always_inline))::aie::vector<float, 16>

SbmBaseImpl<TILE_OUT_HEIGHT,

            NO_DISPARITIES,

            TILE_WINSIZE,

            UNIQUENESS_RATIO,

            TEXTURE_THRESHOLD,

            FILTERED,

            TILE_IN_HEIGHT>::xf_reciprocal(::aie::vector<int32_t, 16> x_int) {
    const ::aie::vector<int32, 16> magic = ::aie::broadcast<int32, 16>(0x7eb53567);

    const ::aie::vector<float, 16> k1 = ::aie::broadcast<float, 16>(1.9395974f);

    const ::aie::vector<float, 16> k2 = ::aie::broadcast<float, 16>(1.436142f);

    const ::aie::vector<float, 16> ones = ::aie::broadcast<float, 16>(1.0f);

    ::aie::vector<float, 16> x = ::aie::to_float(x_int);

    // int16 i = *(int16*)&x;

    // i = 0x7eb5 - i;

    // bfloat16 y = *(bfloat16*)&i;

    ::aie::vector<float, 16> y = ::aie::sub(magic, x.cast_to<int32>()).cast_to<float>();

    // y = k1*y*((-x)*y + k2);

    ::aie::accum<accfloat, 16> acc = ::aie::negmul(x, y);

    acc = ::aie::add(acc, k2);

    acc = ::aie::mul(acc.to_vector<float>(), y);

    acc = ::aie::mul(acc.to_vector<float>(), k1);

    y = acc.to_vector<float>();

    // bfloat16 r = y*(-x) + 1.0f;

    acc = ::aie::negmul(x, acc.to_vector<float>());

    acc = ::aie::add(acc, ones);

    // y = y*r + y;

    acc = ::aie::mul(y, acc.to_vector<float>());

    acc = ::aie::add(acc, y);

    y = acc.to_vector<float>();

    return y;
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((always_inline)) void SbmBaseImpl<TILE_OUT_HEIGHT,

                                                NO_DISPARITIES,

                                                TILE_WINSIZE,

                                                UNIQUENESS_RATIO,

                                                TEXTURE_THRESHOLD,

                                                FILTERED,

                                                TILE_IN_HEIGHT>::xf_interp(::aie::vector<uint16_t, 16> min_cost,

                                                                           ::aie::vector<uint16_t, 16> d,

                                                                           ::aie::vector<uint16_t, 16> p,

                                                                           ::aie::vector<uint16_t, 16> n,

                                                                           int16_t* interpolation_ptr) {
    ::aie::vector<int16_t, 16> temp_vec_int16, temp;

    ::aie::vector<int32_t, 16> num;

    ::aie::accum<acc32, 16> acc, acc1;

    ::aie::accum<acc64, 16> acc64;

    ::aie::accum<accfloat, 16> accf;

    const ::aie::vector<int16, 16> two_vec = ::aie::broadcast<int16, 16>(2);

    const ::aie::vector<int16, 16> two_five_six_vec = ::aie::broadcast<int16, 16>(256);

    uint16_t const1 = 256;

    set_rnd(2);

    uint16_t zero1 = 0;

    //  if (min_d == 0){p = n;}

    ::aie::mask<16> m = ::aie::eq(zero1, d);

    p = ::aie::select(p, n, m);

    //  if (min_d == NO_OF_DISPARITIES-1){n=p;}

    uint16_t DISP_VAL = NO_DISPARITIES - 1;

    ::aie::mask<16> m1 = ::aie::eq(DISP_VAL, d);

    n = ::aie::select(n, p, m1);

    //  int k = p + n - 2 * min_cost +  (abs(p-n));

    ::aie::vector<uint16_t, 16> temp_vec_u16 = ::aie::add(p, n);

    acc = ::aie::negmul(min_cost, two_vec);

    acc = ::aie::add(acc, temp_vec_u16);

    acc1.template from_vector(p, 0);

    acc1 = ::aie::sub(acc1, n);

    auto temp1 = ::aie::abs(acc1.template to_vector<int16_t>(0));

    acc = ::aie::add(acc, temp1);

    acc1.template from_vector(p, 0);

    acc1 = ::aie::sub(acc1, n);

    acc1 = ::aie::mul(acc1.template to_vector<int16_t>(0), two_five_six_vec);

    ::aie::vector<float, 16> x_float = ::aie::to_float(acc.template to_vector<int32_t>(0));

    accf.template from_vector(x_float);

    ::aie::vector<bfloat16, 16> x1 = to_v16bfloat16(accf);

    ::aie::vector<float, 16> one_over_k = xf_reciprocal_bfloat16_lib(x1);

    auto delta = ::aie::to_fixed<int32_t>(one_over_k, 16);

    acc64 = ::aie::mul(acc1.template to_vector<int32_t>(0), delta);

    delta = acc64.template to_vector<int32_t>(16);

    acc = ::aie::add(::aie::add(::aie::mul(d, const1), 15), delta);

    temp_vec_int16 = acc.template to_vector<int16_t>(4);

    ::aie::store_v(interpolation_ptr, temp_vec_int16);
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

__attribute__((noinline)) void SbmBaseImpl<TILE_OUT_HEIGHT,

                                           NO_DISPARITIES,

                                           TILE_WINSIZE,

                                           UNIQUENESS_RATIO,

                                           TEXTURE_THRESHOLD,

                                           FILTERED,

                                           TILE_IN_HEIGHT>::xf_sbm(uint8_t* restrict lsub_ptr,

                                                                   uint8_t* restrict ladd_ptr,

                                                                   uint8_t* restrict rsub_ptr,

                                                                   uint8_t* restrict radd_ptr,

                                                                   int16_t* restrict out) {
    uint16_t* restrict vert_row_sum_ptr = (uint16_t*)vert_row_sum_buffer;

    uint16_t* restrict texture_buffer_ptr = (uint16_t*)texture_buffer;

    uint16_t* restrict temp_cost_buffer_ptr = (uint16_t*)temp_cost_buffer;

    int32_t* restrict temp_cost_buffer_32_ptr = (int32_t*)temp_cost_buffer_32;

    int16_t* restrict out_ptr_bckup = (int16_t*)out;

    constexpr int TILE_WINSIZE_IN = TILE_WINSIZE;

    {
        ::aie::accum<acc16, 64> acc1, acc2, acc3, acc4;

        ::aie::vector<uint8_t, 64> V_left_sub = ::aie::load_v<64>(lsub_ptr);

        ::aie::vector<uint8_t, 64> V_left_add = ::aie::load_v<64>(ladd_ptr);

        int LOOP_CNT = (NO_DISPARITIES % 64) == 0 ? NO_DISPARITIES / 64 : NO_DISPARITIES / 64 + 1;

        int DISP_CNT = NO_DISPARITIES - 1;

        int ctr = 1;

#ifdef KERNEL_DEBUG

        if (1) {
            ::aie::print(V_left_sub, true, "V_left_sub");

            ::aie::print(V_left_add, true, "V_left_sub");
        }

#endif

        for (int loop1_cnt = 0; loop1_cnt < LOOP_CNT; loop1_cnt++) {
            ::aie::vector<uint8_t, 64> V_right_sub1 = ::aie::load_v<64>(rsub_ptr + (loop1_cnt * 64));

            ::aie::vector<uint8_t, 64> V_right_sub2 = ::aie::load_v<64>(rsub_ptr + 64 + (loop1_cnt * 64));

            ::aie::vector<uint8_t, 64> V_right_add1 = ::aie::load_v<64>(radd_ptr + (loop1_cnt * 64));

            ::aie::vector<uint8_t, 64> V_right_add2 = ::aie::load_v<64>(radd_ptr + 64 + (loop1_cnt * 64));

#ifdef KERNEL_DEBUG

            if (1) {
                ::aie::print(V_right_sub1, true, "V_right_sub1");

                ::aie::print(V_right_sub2, true, "V_right_sub2");

                ::aie::print(V_right_add1, true, "V_right_add1");

                ::aie::print(V_right_add2, true, "V_right_add2");
            }

#endif

            acc1.template from_vector(V_left_sub);

            acc2.template from_vector(V_left_add);

            ::aie::vector<uint16_t, 64> V_row_sum = ::aie::load_v<64>(vert_row_sum_ptr);

            ::aie::vector<uint8_t, 64> V_right_sub = ::aie::shuffle_down_fill(V_right_sub1, V_right_sub2, ctr);

            ::aie::vector<uint8_t, 64> V_right_add = ::aie::shuffle_down_fill(V_right_add1, V_right_add2, ctr);

            int tmp = (DISP_CNT - 63 < 0) ? 0 : DISP_CNT - 63;

            // LOOP1

            for (int d = DISP_CNT; d > tmp; d--) chess_prepare_for_pipelining {
                    // find abs-diff between bottom rows. Then, subtract from row_sum

                    acc3 = ::aie::sub(acc1, V_right_sub);

                    ::aie::vector<int16_t, 64> V_diff = ::aie::abs(acc3.template to_vector<int16_t>(0));

                    V_row_sum = ::aie::sub(V_row_sum, V_diff.cast_to<uint16_t>());

                    // find abs-diff between top rows. Then, add to row_sum

                    ctr = ctr + 1;

                    acc4 = ::aie::sub(acc2, V_right_add);

                    V_diff = ::aie::abs(acc4.template to_vector<int16_t>(0));

                    V_row_sum = ::aie::add(V_row_sum, V_diff.cast_to<uint16_t>());

                    //  store current row_sum

                    ::aie::store_v(vert_row_sum_ptr, V_row_sum);

#ifdef KERNEL_DEBUG1

                    if (1) {
                        ::aie::print(V_row_sum, true, "V_row_sum");
                    }

#endif

                    V_right_sub = ::aie::shuffle_down_fill(V_right_sub1, V_right_sub2, ctr);

                    vert_row_sum_ptr = vert_row_sum_ptr + 64;

                    V_row_sum = ::aie::load_v<64>(vert_row_sum_ptr);

                    V_right_add = ::aie::shuffle_down_fill(V_right_add1, V_right_add2, ctr);
                }

            // find abs-diff between bottom rows. Then, subtract from row_sum

            acc3 = ::aie::sub(acc1, V_right_sub);

            ::aie::vector<int16_t, 64> V_diff = ::aie::abs(acc3.template to_vector<int16_t>(0));

            V_row_sum = ::aie::sub(V_row_sum, V_diff.cast_to<uint16_t>());

            // find abs-diff between top rows. Then, add to row_sum

            acc4 = ::aie::sub(acc2, V_right_add);

            V_diff = ::aie::abs(acc4.template to_vector<int16_t>(0));

            V_row_sum = ::aie::add(V_row_sum, V_diff.cast_to<uint16_t>());

            //  store current row_sum

            ::aie::store_v(vert_row_sum_ptr, V_row_sum);

#ifdef KERNEL_DEBUG1

            if (1) {
                ::aie::print(V_row_sum, true, "V_row_sum");
            }

#endif

            vert_row_sum_ptr = vert_row_sum_ptr + 64;

            ctr = 1;

            DISP_CNT = DISP_CNT - 64;
        }
    }

    ::aie::vector<int32_t, 64 / 4> V1_min1_0 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min2_0 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min3_0 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min4_0 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min1_1 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min2_1 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min3_1 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min4_1 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min1_2 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min2_2 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min3_2 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min4_2 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min1_3 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min2_3 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min3_3 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_min4_3 = ::aie::broadcast<int32_t, 64 / 4>(0x7fffffff);

    ::aie::vector<int32_t, 64 / 4> V1_right_0, V1_right_1, V1_right_2, V1_right_3;

    ::aie::vector<int32_t, 64 / 4> V1_left_0, V1_left_1, V1_left_2, V1_left_3;

    {
        ::aie::vector<int32_t, 64 / 4> V1_prev_cost_1, V1_prev_cost_2, V1_prev_cost_3, V1_prev_cost_4;

        ::aie::mask<16> M1, M2, M3, M4;

        int init_disp = NO_DISPARITIES - 1;

        for (int d2 = 0; d2 < NO_DISPARITIES / 16; d2++) {
            vert_row_sum_ptr = (uint16_t*)vert_row_sum_buffer + (d2 * 16 * 64);

            temp_cost_buffer_ptr = (uint16_t*)temp_cost_buffer;

            for (int tmp_ctr = 0; tmp_ctr < 16; tmp_ctr++) chess_prepare_for_pipelining {
                    ::aie::vector<uint16_t, 64> V_row_sum = ::aie::load_v<64>(vert_row_sum_ptr);

                    vert_row_sum_ptr += 64;

                    auto SUM3 = xf_find_col_sum(V_row_sum);

                    ::aie::store_v(temp_cost_buffer_ptr, SUM3);

                    temp_cost_buffer_ptr += 64;
                }

            temp_cost_buffer_32_ptr = (int32_t*)temp_cost_buffer_32;

            temp_cost_buffer_ptr = (uint16_t*)temp_cost_buffer;

            ::aie::accum<acc32, 16> acc_32;

            for (int curr_disp = init_disp; curr_disp >= init_disp - 15; curr_disp--) {
                ::aie::vector<uint8_t, 16> V_disp_index =

                    ::aie::broadcast<uint8_t, 16>(NO_DISPARITIES - 1 - curr_disp);

                for (int d1 = 0; d1 < 4; d1++) chess_prepare_for_pipelining {
                        ::aie::vector<uint16_t, 16> V_curr_cost_16bit = ::aie::load_v<16>(temp_cost_buffer_ptr);

                        temp_cost_buffer_ptr += 16;

                        acc_32.template from_vector(V_curr_cost_16bit, 8);

                        acc_32 = ::aie::add(acc_32, V_disp_index);

                        ::aie::store_v(temp_cost_buffer_32_ptr, acc_32.template to_vector<int32_t>(0));

                        temp_cost_buffer_32_ptr += 16;
                    }
            }

            temp_cost_buffer_32_ptr = (int32_t*)temp_cost_buffer_32;

            for (int tmp_ctr = 0; tmp_ctr < 16; tmp_ctr++) chess_prepare_for_pipelining {
                    ::aie::vector<int32_t, 16> V_curr_cost_part = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    V1_left_0 = ::aie::select(V1_left_0, V_curr_cost_part, M1);

                    M1 = ::aie::lt(V_curr_cost_part, V1_min1_0);

                    auto temp = ::aie::select(V1_min1_0, V_curr_cost_part, M1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min1_0, M1);

                    V1_min1_0 = temp;

                    V1_right_0 = ::aie::select(V1_right_0, V1_prev_cost_1, M1);

                    V1_prev_cost_1 = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    temp_cost_buffer_32_ptr += 64;

                    ::aie::mask<16> m1 = ::aie::lt(V_curr_cost_part, V1_min2_0);

                    temp = ::aie::select(V1_min2_0, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min2_0, m1);

                    V1_min2_0 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min3_0);

                    temp = ::aie::select(V1_min3_0, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min3_0, m1);

                    V1_min3_0 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min4_0);

                    V1_min4_0 = ::aie::select(V1_min4_0, V_curr_cost_part, m1);
                }

            temp_cost_buffer_32_ptr = (int32_t*)temp_cost_buffer_32 + 16;

            for (int tmp_ctr = 0; tmp_ctr < 16; tmp_ctr++) chess_prepare_for_pipelining {
                    ::aie::vector<int32_t, 16> V_curr_cost_part = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    V1_left_1 = ::aie::select(V1_left_1, V_curr_cost_part, M2);

                    M2 = ::aie::lt(V_curr_cost_part, V1_min1_1);

                    auto temp = ::aie::select(V1_min1_1, V_curr_cost_part, M2);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min1_1, M2);

                    V1_min1_1 = temp;

                    V1_right_1 = ::aie::select(V1_right_1, V1_prev_cost_2, M2);

                    V1_prev_cost_2 = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    temp_cost_buffer_32_ptr += 64;

                    ::aie::mask<16> m1 = ::aie::lt(V_curr_cost_part, V1_min2_1);

                    temp = ::aie::select(V1_min2_1, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min2_1, m1);

                    V1_min2_1 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min3_1);

                    temp = ::aie::select(V1_min3_1, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min3_1, m1);

                    V1_min3_1 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min4_1);

                    V1_min4_1 = ::aie::select(V1_min4_1, V_curr_cost_part, m1);
                }

            temp_cost_buffer_32_ptr = (int32_t*)temp_cost_buffer_32 + 32;

            for (int tmp_ctr = 0; tmp_ctr < 16; tmp_ctr++) chess_prepare_for_pipelining {
                    ::aie::vector<int32_t, 16> V_curr_cost_part = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    V1_left_2 = ::aie::select(V1_left_2, V_curr_cost_part, M3);

                    M3 = ::aie::lt(V_curr_cost_part, V1_min1_2);

                    auto temp = ::aie::select(V1_min1_2, V_curr_cost_part, M3);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min1_2, M3);

                    V1_min1_2 = temp;

                    V1_right_2 = ::aie::select(V1_right_2, V1_prev_cost_3, M3);

                    V1_prev_cost_3 = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    temp_cost_buffer_32_ptr += 64;

                    ::aie::mask<16> m1 = ::aie::lt(V_curr_cost_part, V1_min2_2);

                    temp = ::aie::select(V1_min2_2, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min2_2, m1);

                    V1_min2_2 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min3_2);

                    temp = ::aie::select(V1_min3_2, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min3_2, m1);

                    V1_min3_2 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min4_2);

                    V1_min4_2 = ::aie::select(V1_min4_2, V_curr_cost_part, m1);
                }

            temp_cost_buffer_32_ptr = (int32_t*)temp_cost_buffer_32 + 48;

            for (int tmp_ctr = 0; tmp_ctr < 16; tmp_ctr++) chess_prepare_for_pipelining {
                    ::aie::vector<int32_t, 16> V_curr_cost_part = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    V1_left_3 = ::aie::select(V1_left_3, V_curr_cost_part, M4);

                    M4 = ::aie::lt(V_curr_cost_part, V1_min1_3);

                    auto temp = ::aie::select(V1_min1_3, V_curr_cost_part, M4);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min1_3, M4);

                    V1_min1_3 = temp;

                    V1_right_3 = ::aie::select(V1_right_3, V1_prev_cost_4, M4);

                    V1_prev_cost_4 = ::aie::load_v<16>(temp_cost_buffer_32_ptr);

                    temp_cost_buffer_32_ptr += 64;

                    ::aie::mask<16> m1 = ::aie::lt(V_curr_cost_part, V1_min2_3);

                    temp = ::aie::select(V1_min2_3, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min2_3, m1);

                    V1_min2_3 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min3_3);

                    temp = ::aie::select(V1_min3_3, V_curr_cost_part, m1);

                    V_curr_cost_part = ::aie::select(V_curr_cost_part, V1_min3_3, m1);

                    V1_min3_3 = temp;

                    m1 = ::aie::lt(V_curr_cost_part, V1_min4_3);

                    V1_min4_3 = ::aie::select(V1_min4_3, V_curr_cost_part, m1);
                }

            init_disp = init_disp - 16;
        }
    }

#ifdef KERNEL_DEBUG1

    if (ITER == 14) {
        print_all(V1_min1_0, V1_min2_0, V1_min3_0, V1_min4_0, V1_right_0, V1_left_0);

        print_all(V1_min1_1, V1_min2_1, V1_min3_1, V1_min4_1, V1_right_1, V1_left_1);

        print_all(V1_min1_2, V1_min2_2, V1_min3_2, V1_min4_2, V1_right_2, V1_left_2);

        print_all(V1_min1_3, V1_min2_3, V1_min3_3, V1_min4_3, V1_right_3, V1_left_3);
    }

#endif

    {
        // Uniqueness threshold

        auto bcond_0 = xf_find_uniqueness_threshold(V1_min1_0, V1_min2_0, V1_min3_0, V1_min4_0);

        auto bcond_1 = xf_find_uniqueness_threshold(V1_min1_1, V1_min2_1, V1_min3_1, V1_min4_1);

        auto bcond_2 = xf_find_uniqueness_threshold(V1_min1_2, V1_min2_2, V1_min3_2, V1_min4_2);

        auto bcond_3 = xf_find_uniqueness_threshold(V1_min1_3, V1_min2_3, V1_min3_3, V1_min4_3);

        // Texture threshold

        uint16_t t_thr = TEXTURE_THRESHOLD;

        ::aie::vector<uint16_t, 64> threshold_sum = xf_find_texture_threshold(texture_buffer_ptr, ladd_ptr, lsub_ptr);

        ::aie::vector<uint16_t, 16> text_0 = ::extract_v16uint16(threshold_sum, 0);

        ::aie::mask<16> bcond_00 = ::aie::lt(text_0, t_thr);

        text_0 = ::extract_v16uint16(threshold_sum, 1);

        ::aie::mask<16> bcond_11 = ::aie::lt(text_0, t_thr);

        text_0 = ::extract_v16uint16(threshold_sum, 2);

        ::aie::mask<16> bcond_22 = ::aie::lt(text_0, t_thr);

        text_0 = ::extract_v16uint16(threshold_sum, 3);

        ::aie::mask<16> bcond_33 = ::aie::lt(text_0, t_thr);

        // Filter out unreliable disparities based on the threhold values

        ::aie::vector<uint16_t, 16> V_filter = ::aie::broadcast<uint16_t, 16>(FILTERED);

        ::aie::vector<int32_t, 16> V_disps = ::aie::broadcast<int32_t, 16>(NO_DISPARITIES - 1);

        ::aie::accum<acc32, 16> acc = ::aie::vector_cast<acc32>(::aie::sub(V_disps, ::aie::bit_and(0xFF, V1_min1_0)));

        ::aie::vector<uint16_t, 16> D_0 = acc.to_vector<uint16_t>(0);

        D_0 = ::aie::select(D_0, V_filter, bcond_0 | bcond_00);

        acc = ::aie::vector_cast<acc32>(::aie::sub(V_disps, ::aie::bit_and(0xFF, V1_min1_1)));

        ::aie::vector<uint16_t, 16> D_1 = acc.to_vector<uint16_t>(0);

        D_1 = ::aie::select(D_1, V_filter, bcond_1 | bcond_11);

        acc = ::aie::vector_cast<acc32>(::aie::sub(V_disps, ::aie::bit_and(0xFF, V1_min1_2)));

        ::aie::vector<uint16_t, 16> D_2 = acc.to_vector<uint16_t>(0);

        D_2 = ::aie::select(D_2, V_filter, bcond_2 | bcond_22);

        acc = ::aie::vector_cast<acc32>(::aie::sub(V_disps, ::aie::bit_and(0xFF, V1_min1_3)));

        ::aie::vector<uint16_t, 16> D_3 = acc.to_vector<uint16_t>(0);

        D_3 = ::aie::select(D_3, V_filter, bcond_3 | bcond_33);

        // Interpolation

        int COLS_COMBINE1 = 64 - TILE_WINSIZE_IN + 1;

        if (ITER == (TILE_WINSIZE - 2)) {
            out += COLS_COMBINE1 - 32; // (32 int16 meta data)

            uint8_t* meta_out_ptr = (uint8_t*)out;

            METADATA[4] = COLS_COMBINE1;

            METADATA[5] = 0;

            int16_t t1 = TILE_OUT_HEIGHT;

            METADATA[6] = (int8_t)(t1 & 0x00FF);

            METADATA[7] = (int8_t)((t1 & 0xFF00) >> 8);

            METADATA[20] = COLS_COMBINE1;

            METADATA[21] = 0;

            METADATA[22] = (int8_t)(t1 & 0x00FF);

            METADATA[23] = (int8_t)((t1 & 0xFF00) >> 8);

            METADATA[8] = 0; // ovlapL

            METADATA[9] = 0;

            METADATA[10] = 0; // ovlapR

            METADATA[11] = 0;

            METADATA[12] = 0; // ovlapT

            METADATA[13] = 0;

            METADATA[14] = 0; // ovlapB

            METADATA[15] = 0;

            uint8_t pos_h_in_l = METADATA[30]; // pos h

            uint8_t pos_h_in_h = METADATA[31]; // pos h

            uint8_t pos_v_in_l = METADATA[32]; // pos v

            uint8_t pos_v_in_h = METADATA[33]; // pos v

            METADATA[26] = pos_h_in_l; // pos h

            METADATA[27] = pos_h_in_h; // pos h

            METADATA[28] = pos_v_in_l; // pos v

            METADATA[29] = pos_v_in_h; // pos v

            // set offset for PLIO

            int16_t posh = (static_cast<int16_t>(pos_h_in_h) << 8) | pos_h_in_l;

            int16_t posv = (static_cast<int16_t>(pos_v_in_h) << 8) | pos_v_in_l;

            const int32_t offset = (posv)*IMAGE_WIDTH + (posh);

            METADATA[16] = (offset & 0x000000ff);

            METADATA[17] = ((offset >> 8) * (0x000000ff));

            METADATA[18] = ((offset >> 16) * (0x000000ff));

            METADATA[19] = ((offset >> 24) * (0x000000ff));

            ::aie::store_unaligned_v(meta_out_ptr, METADATA);

        } else {
            // Interpolation
            int16_t* tmp_ptr1 = (int16_t*)tmp_buffer;
            ::aie::accum<acc32, 16> acc_left = ::aie::vector_cast<acc32>(V1_left_0);
            ::aie::accum<acc32, 16> acc_right = ::aie::vector_cast<acc32>(V1_right_0);
            ::aie::accum<acc32, 16> acc_min = ::aie::vector_cast<acc32>(V1_min1_0);
            xf_interp(acc_min.template to_vector<uint16_t>(8), D_0, acc_left.to_vector<uint16_t>(8),
                      acc_right.to_vector<uint16_t>(8), out);
            out = out + 16;

            acc_left = ::aie::vector_cast<acc32>(V1_left_1);
            acc_right = ::aie::vector_cast<acc32>(V1_right_1);
            acc_min = ::aie::vector_cast<acc32>(V1_min1_1);
            xf_interp(acc_min.template to_vector<uint16_t>(8), D_1, acc_left.to_vector<uint16_t>(8),
                      acc_right.to_vector<uint16_t>(8), out);
            out = out + 16;
            acc_left = ::aie::vector_cast<acc32>(V1_left_2);
            acc_right = ::aie::vector_cast<acc32>(V1_right_2);
            acc_min = ::aie::vector_cast<acc32>(V1_min1_2);
            int COLS_COMBINE1 = 64 - TILE_WINSIZE_IN + 1;
            if (TILE_WINSIZE_IN > 17) {
                xf_interp(acc_min.template to_vector<uint16_t>(8), D_2, acc_left.to_vector<uint16_t>(8),
                          acc_right.to_vector<uint16_t>(8), tmp_ptr1);
                for (int i = 0; i < COLS_COMBINE1 % 16; i = i + 1) {
                    *out++ = tmp_ptr1[i];
                }
            } else {
                xf_interp(acc_min.template to_vector<uint16_t>(8), D_2, acc_left.to_vector<uint16_t>(8),
                          acc_right.to_vector<uint16_t>(8), out);
                out = out + 16;
                acc_left = ::aie::vector_cast<acc32>(V1_left_3);
                acc_right = ::aie::vector_cast<acc32>(V1_right_3);
                acc_min = ::aie::vector_cast<acc32>(V1_min1_3);
                xf_interp(acc_min.template to_vector<uint16_t>(8), D_3, acc_left.to_vector<uint16_t>(8),
                          acc_right.to_vector<uint16_t>(8), tmp_ptr1);
                for (int i = 0; i < COLS_COMBINE1 % 16; i = i + 1) {
                    *out++ = tmp_ptr1[i];
                }
            }
        }

        // Buffer zeroing

        // chess_separator();

        ITER = (ITER + 1) % (TILE_OUT_HEIGHT + TILE_WINSIZE - 1);

        if (ITER == 0) {
            auto vec = ::aie::broadcast<uint16_t, 64>(0);

            vert_row_sum_ptr = (uint16_t*)vert_row_sum_buffer;

            texture_buffer_ptr = (uint16_t*)texture_buffer;

            for (int i = 0; i < COST_BUF_SIZE; i++) {
                ::aie::store_v(vert_row_sum_ptr, vec);

                vert_row_sum_ptr += 64;
            }

            ::aie::store_v(texture_buffer_ptr, vec);
        }
    }
}

template <int TILE_OUT_HEIGHT,

          int NO_DISPARITIES,

          int TILE_WINSIZE,

          int UNIQUENESS_RATIO,

          int TEXTURE_THRESHOLD,

          int FILTERED,

          int TILE_IN_HEIGHT>

void SbmBaseImpl<TILE_OUT_HEIGHT,

                 NO_DISPARITIES,

                 TILE_WINSIZE,

                 UNIQUENESS_RATIO,

                 TEXTURE_THRESHOLD,

                 FILTERED,

                 TILE_IN_HEIGHT>::runImpl(adf::input_buffer<uint8_t>& img_in_1,

                                          adf::input_buffer<uint8_t>& img_in_2,

                                          adf::input_buffer<uint8_t>& img_in_3,

                                          adf::input_buffer<uint8_t>& img_in_4,

                                          adf::input_buffer<uint8_t>& img_in_5,

                                          adf::output_buffer<int16_t>& out) {
    uint8_t* restrict img_in1_ptr = (uint8_t*)::aie::begin(img_in_1);

    uint8_t* restrict img_in2_ptr = (uint8_t*)::aie::begin(img_in_2);

    uint8_t* restrict img_in3_ptr = (uint8_t*)::aie::begin(img_in_3);

    uint8_t* restrict img_in4_ptr = (uint8_t*)::aie::begin(img_in_4);

    uint8_t* restrict meta_data_ptr = (uint8_t*)::aie::begin(img_in_5);

    int16_t* restrict img_out_ptr = (int16_t*)::aie::begin(out);
    // printf(" ITER = %d ", ITER);

    if (ITER == 0) {
        METADATA = ::aie::load_v<64>(meta_data_ptr);
    }

    xf_sbm(img_in1_ptr, img_in2_ptr, img_in3_ptr, img_in4_ptr, img_out_ptr);
}

} // namespace aie

} // namespace cv

} // namespace xf

#endif
