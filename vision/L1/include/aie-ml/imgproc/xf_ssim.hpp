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
#include <algorithm>
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>

#include <common/xf_aie_hw_utils.hpp>

namespace xf {
namespace cv {
namespace aie {

template <int TILE_WIDTH, int TILE_HEIGHT, int N>
class ssimBaseImpl {
    uint16_t (&mwei)[176];

   public:
    ssimBaseImpl(uint16_t (&wei)[176]) : mwei(wei) {}
    void runImpl(uint8* img_in1, uint8* img_in2, int64& deno_sum, int64& numo_sum);

    void xf_ssim(const uint8* ptr1, const uint8* ptr2, const uint16* coeff, int64& deno_sum, int64& numo_sum);
};

inline __attribute__((always_inline)) v16acc64 mul_32bit(v16uint32 a0, v16uint32 b0) {
    v16uint32 a1; //=*coeff;
    v16uint32 b1; //=*coeff;
    for (int i = 0; i < 16; i++) {
        a1 = upd_elem(a1, i, 0);
        b1 = upd_elem(b1, i, 0);
    }

    v32uint16 a_lo = (v32uint16)shuffle(a0, a1, 2);
    v32uint16 a_hi = (v32uint16)shuffle(a0, a1, 3);
    v32uint16 b_lo = (v32uint16)shuffle(b0, b1, 2);
    v32uint16 b_hi = (v32uint16)shuffle(b0, b1, 3);
    v16acc64 acc = mul_elem_16_2(a_hi, b_hi);
    acc = mac_elem_16_2_conf(a_hi, 0, b_lo, false, acc, 0, 1, 0, 0);
    acc = mac_elem_16_2_conf(a_lo, false, b_hi, 0, acc, 0, 0, 0, 0);
    acc = mac_elem_16_2_conf(a_lo, false, b_lo, false, acc, 0, 1, 0, 0);
    return acc;
}

inline __attribute__((always_inline)) v16acc64 mul_32bit(v16uint32 a0, v16int32 b0) {
    v16uint32 a1; //=*coeff;
    v16int32 b1;  //=*coeff;
    for (int i = 0; i < 16; i++) {
        a1 = upd_elem(a1, i, 0);
        b1 = upd_elem(b1, i, 0);
    }

    v32uint16 a_lo = (v32uint16)shuffle(a0, a1, 2);
    v32uint16 a_hi = (v32uint16)shuffle(a0, a1, 3);
    v32uint16 b_lo = (v32uint16)shuffle(b0, b1, 2);
    v32int16 b_hi = (v32int16)shuffle(b0, b1, 3);
    v16acc64 acc = mul_elem_16_2(a_hi, b_hi);
    acc = mac_elem_16_2_conf(a_hi, 0, b_lo, false, acc, 0, 1, 0, 0);
    acc = mac_elem_16_2_conf(a_lo, false, b_hi, 1, acc, 0, 0, 0, 0);
    acc = mac_elem_16_2_conf(a_lo, false, b_lo, false, acc, 0, 1, 0, 0);
    return acc;
}

int64 reduceadd(::aie::accum<acc64, 16> acc) {
    acc = add(acc, ::aie::vector_cast<acc64>(::aie::shuffle_down(::aie::vector_cast<cint32>(acc), 8)));
    acc = add(acc, ::aie::vector_cast<acc64>(::aie::shuffle_down(::aie::vector_cast<cint32>(acc), 4)));
    acc = add(acc, ::aie::vector_cast<acc64>(::aie::shuffle_down(::aie::vector_cast<cint32>(acc), 2)));
    acc = add(acc, ::aie::vector_cast<acc64>(::aie::shuffle_down(::aie::vector_cast<cint32>(acc), 1)));

    return __builtin_bit_cast(int64, ::aie::vector_cast<cint32>(acc).get(0));
}

alignas(::aie::vector_decl_align) const uint8_t zero_vec[64] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

template <int TILE_WIDTH, int TILE_HEIGHT, int N>
__attribute__((noinline)) void ssimBaseImpl<TILE_WIDTH, TILE_HEIGHT, N>::xf_ssim(
    const uint8* ptr1, const uint8* ptr2, const uint16* coeff, int64& deno_sum, int64& numo_sum) {
    const uint16_t img_width = 64;
    const uint16_t img_height = 64;
    uint8* img_in_ptr = (uint8*)ptr1;
    uint8* img_in1_ptr = (uint8*)ptr2;
    uint16* kernel_vec = (uint16*)(coeff);
    uint8* zero_ptr = (uint8*)zero_vec;

    ::aie::tile::current().set_saturation(::aie::saturation_mode::truncate);

    ::aie::vector<uint32_t, 16> c1_fixval(429497, 429497, 429497, 429497, 429497, 429497, 429497, 429497, 429497,
                                          429497, 429497, 429497, 429497, 429497, 429497, 429497);
    ::aie::vector<uint32_t, 16> c2_fixval(3865471, 3865471, 3865471, 3865471, 3865471, 3865471, 3865471, 3865471,
                                          3865471, 3865471, 3865471, 3865471, 3865471, 3865471, 3865471, 3865471);
    ::aie::vector<uint8_t, 32> vec_x1, vec_x2, vec_y1, vec_y2;
    ::aie::vector<uint16_t, 32> vec_x, vec_y, vec_temp, temp, vec_tempy, temp_y, weights;
    ::aie::vector<uint16_t, 32> vec_xy;

    ::aie::accum<acc64, 16> acc_x1 = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> acc_x = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> acc_y = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> acc_y1 = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> acc_xx = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> acc_yy = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> acc_xy = ::aie::zeros<acc64, 16>();

    ::aie::vector<uint32_t, 16> out_x, out_y;
    ::aie::vector<int32_t, 16> out_new;
    ::aie::vector<uint32_t, 16> out_new1;

    ::aie::accum<acc64, 16> sigmaacc_xx, sigmaacc_yy, sigmaacc_xy, acc_muxx, acc_muyy, acc_muxy;

    ::aie::accum<acc64, 16> acc_c1;
    ::aie::accum<acc64, 16> acc_c2;

    acc_c1.from_vector<uint32_t>(c1_fixval);
    acc_c2.from_vector<uint32_t>(c2_fixval);

    ::aie::accum<acc64, 16> deno_acc = ::aie::zeros<acc64, 16>();
    ::aie::accum<acc64, 16> numo_acc = ::aie::zeros<acc64, 16>();

    for (int i = 0; i < (64); i++) //////////////////rows loop
    {
        uint8* ptr[11];
        uint8* ptr_y[11];
        uint16* coeff_row[11];

        ptr[0] = (i < 5) ? zero_ptr : img_in_ptr + (i - 5) * img_width;
        ptr[1] = (i < 4) ? zero_ptr : img_in_ptr + (i - 4) * img_width;
        ptr[2] = (i < 3) ? zero_ptr : img_in_ptr + (i - 3) * img_width;
        ptr[3] = (i < 2) ? zero_ptr : img_in_ptr + (i - 2) * img_width;
        ptr[4] = (i < 1) ? zero_ptr : img_in_ptr + (i - 1) * img_width;

        ptr[5] = img_in_ptr + (i)*img_width;

        ptr[6] = (i < 63) ? img_in_ptr + (i + 1) * img_width : zero_ptr;
        ptr[7] = (i < 62) ? img_in_ptr + (i + 2) * img_width : zero_ptr;
        ptr[8] = (i < 61) ? img_in_ptr + (i + 3) * img_width : zero_ptr;
        ptr[9] = (i < 60) ? img_in_ptr + (i + 4) * img_width : zero_ptr;
        ptr[10] = (i < 59) ? img_in_ptr + (i + 5) * img_width : zero_ptr;

        ptr_y[0] = (i < 5) ? zero_ptr : img_in1_ptr + (i - 5) * img_width;
        ptr_y[1] = (i < 4) ? zero_ptr : img_in1_ptr + (i - 4) * img_width;
        ptr_y[2] = (i < 3) ? zero_ptr : img_in1_ptr + (i - 3) * img_width;
        ptr_y[3] = (i < 2) ? zero_ptr : img_in1_ptr + (i - 2) * img_width;
        ptr_y[4] = (i < 1) ? zero_ptr : img_in1_ptr + (i - 1) * img_width;

        ptr_y[5] = img_in1_ptr + (i)*img_width;

        ptr_y[6] = (i < 63) ? img_in1_ptr + (i + 1) * img_width : zero_ptr;
        ptr_y[7] = (i < 62) ? img_in1_ptr + (i + 2) * img_width : zero_ptr;
        ptr_y[8] = (i < 61) ? img_in1_ptr + (i + 3) * img_width : zero_ptr;
        ptr_y[9] = (i < 60) ? img_in1_ptr + (i + 4) * img_width : zero_ptr;
        ptr_y[10] = (i < 59) ? img_in1_ptr + (i + 5) * img_width : zero_ptr;

        ::aie::accum<acc32, 32> acc_sq1, acc_sq2, acc_sq3;
        /**********************left region*****************************/
        {
            for (int k = 0; k < 11; k++) chess_prepare_for_pipelining chess_loop_range(1, ) {
                    // const uint16_t* ptr_coeff = coeff+(k*16);
                    coeff_row[k] = kernel_vec + (k * 16);
                    weights.insert(0, ::aie::load_v<16>(coeff_row[k])); // load weights

                    vec_x1 = ::aie::load_v<32>(ptr[k]);   // load 32 elem of x
                    vec_y1 = ::aie::load_v<32>(ptr_y[k]); // load 32 elem of x

                    vec_x = unpack(vec_x1); // convt to 16b type
                    vec_y = unpack(vec_y1); // convt to 16b type

                    // compute mu_x & mu_y //
                    temp = ::aie::shuffle_up_fill(vec_x, ::aie::broadcast<uint16, 32>(0),
                                                  5); // padding with 5 zeros on left
                    acc_x = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x, weights, 0, temp, 0);
                    acc_x = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x, weights, 4, temp, 4);
                    acc_x = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x, weights, 8, temp, 8);
                    temp_y = ::aie::shuffle_up_fill(vec_y, ::aie::broadcast<uint16, 32>(0),
                                                    5); // padding with 5 zeros on left
                    acc_y = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y, weights, 0, temp_y, 0);
                    acc_y = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y, weights, 4, temp_y, 4);
                    acc_y = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y, weights, 8, temp_y, 8);
                    // x*2, y*2, x*y //

                    acc_sq1 = ::aie::mul_square(vec_x1);
                    vec_x = acc_sq1.template to_vector<uint16_t>(0);
                    temp = ::aie::shuffle_up_fill(vec_x, ::aie::broadcast<uint16, 32>(0),
                                                  5); // padding with 5 zeros on left
                    acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 0, temp, 0);
                    acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 4, temp, 4);
                    acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 8, temp, 8);

                    acc_sq2 = ::aie::mul_square(vec_y1);
                    vec_y = acc_sq2.template to_vector<uint16_t>(0);
                    temp_y = ::aie::shuffle_up_fill(vec_y, ::aie::broadcast<uint16, 32>(0),
                                                    5); // padding with 5 zeros on left
                    acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 0, temp_y, 0);
                    acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 4, temp_y, 4);
                    acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 8, temp_y, 8);

                    acc_sq3 = ::aie::mul(vec_x1, vec_y1);
                    vec_xy = acc_sq3.template to_vector<uint16_t>(0);
                    temp = ::aie::shuffle_up_fill(vec_xy, ::aie::broadcast<uint16, 32>(0),
                                                  5); // padding with 5 zeros on left
                    acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 0, temp, 0);
                    acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 4, temp, 4);
                    acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 8, temp, 8);
                }
            ///////mu_x**2 mu_y**2///////////////////////
            // ::aie::vector<uint16_t,16> mu_x = acc_x.template to_vector<uint16_t>(0);

            // ::aie::vector<uint16_t,16> mu_y = acc_y.template to_vector<uint16_t>(16);
            //::aie::print(mu_x,false,"\nmu_x:");
            //::aie::print(mu_y,false,"\nmu_y:");

            ::aie::vector<uint32_t, 16> out_x1, out_y1;
            ::aie::vector<uint32_t, 16> x2_out;

            out_x1 = acc_x.template to_vector<uint32_t>(0);
            acc_muxx = mul_32bit(out_x1, out_x1);

            x2_out = acc_xx.template to_vector<uint32_t>(0);
            acc_xx = ups_to_v16acc64(x2_out, 16);
            sigmaacc_xx = sub(acc_xx, acc_muxx);

            out_y1 = acc_y.template to_vector<uint32_t>(0);
            //::aie::store_v( out_ptr+=16, out_y1 );
            acc_muyy = mul_32bit(out_y1, out_y1);
            x2_out = acc_yy.template to_vector<uint32_t>(0);
            acc_yy = ups_to_v16acc64(x2_out, 16);
            sigmaacc_yy = sub(acc_yy, acc_muyy);

            acc_muxy = mul_32bit(out_x1, out_y1);
            x2_out = acc_xy.template to_vector<uint32_t>(0);

            acc_xy = ups_to_v16acc64(x2_out, 16);
            sigmaacc_xy = sub(acc_xy, acc_muxy);

            ::aie::accum<acc64, 16> cs_de = add(sigmaacc_xx, sigmaacc_yy);
            cs_de = add(cs_de, acc_c2);
            ::aie::accum<acc64, 16> cs_nu = add(sigmaacc_xy, sigmaacc_xy);
            cs_nu = add(cs_nu, acc_c2);

            ::aie::accum<acc64, 16> ss_de = add(acc_muxx, acc_muyy);
            ss_de = add(ss_de, acc_c1);
            ::aie::accum<acc64, 16> ss_nu = add(acc_muxy, acc_muxy);
            ss_nu = add(ss_nu, acc_c1);

            out_x = cs_de.template to_vector<uint32_t>(16);
            out_y = ss_de.template to_vector<uint32_t>(16);
            ::aie::accum<acc64, 16> deno = mul_32bit(out_x, out_y);
            ::aie::vector<int32_t, 16> final_deno = deno.template to_vector<int32_t>(32);

            out_new = cs_nu.template to_vector<int32_t>(16);
            out_new1 = ss_nu.template to_vector<uint32_t>(16);
            ::aie::accum<acc64, 16> numo = mul_32bit(out_new1, out_new);
            ::aie::vector<int32_t, 16> final_numo = numo.template to_vector<int32_t>(32);

            deno_acc = ::aie::add(deno_acc, final_deno);
            numo_acc = ::aie::add(numo_acc, final_numo);

            acc_x = ::aie::zeros<acc64, 16>();
            acc_y = ::aie::zeros<acc64, 16>();
            acc_xx = ::aie::zeros<acc64, 16>();
            acc_yy = ::aie::zeros<acc64, 16>();
            acc_xy = ::aie::zeros<acc64, 16>();
        }

        /*********************middle region**************************/
        {
            for (int r = 0; r < 2; r++) chess_prepare_for_pipelining chess_loop_range(2, ) {
                    for (int k = 0; k < 11; k++) chess_prepare_for_pipelining chess_loop_range(1, ) {
                            // const uint16_t* ptr_coeff = coeff+(k*16);
                            coeff_row[k] = kernel_vec + (k * 16);
                            weights.insert(0, ::aie::load_v<16>(coeff_row[k])); // load weights

                            vec_x1.insert(0, ::aie::load_v<16>(ptr[k]));
                            ptr[k] += 16; // load 32 elem of x -> 8 to 40
                            vec_x1.insert(1, ::aie::load_v<16>(ptr[k]));
                            ptr[k] += 16;
                            vec_x = unpack(vec_x1); // convt to 16b type
                            vec_y1.insert(0, ::aie::load_v<16>(ptr_y[k]));
                            ptr_y[k] += 16; // load 32 elem of x -> 8 to 40
                            vec_y1.insert(1, ::aie::load_v<16>(ptr_y[k]));
                            ptr_y[k] += 16;
                            vec_y = unpack(vec_y1); // convt to 16b type

                            // compute mu_x & mu_y//
                            acc_x1 = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x1, weights, 0, vec_x, 11); // k0--k3
                            vec_x2.insert(0, ::aie::load_v<16>(ptr[k]));
                            vec_temp = unpack(vec_x2);
                            temp = ::aie::shuffle_down_fill(vec_x, vec_temp, 15);
                            acc_x1 = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x1, weights, 4, temp, 0); // k4-k7
                            acc_x1 = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x1, weights, 8, temp, 4); // k8-k10
                            ptr[k] = ptr[k] - 16;
                            acc_y1 = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y1, weights, 0, vec_y, 11); // k0--k3
                            vec_y2.insert(0, ::aie::load_v<16>(ptr_y[k]));
                            vec_tempy = unpack(vec_y2);
                            temp_y = ::aie::shuffle_down_fill(vec_y, vec_tempy, 15);
                            acc_y1 = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y1, weights, 4, temp_y, 0); // k4-k7
                            acc_y1 = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y1, weights, 8, temp_y, 4); // k8-k10
                            ptr_y[k] = ptr_y[k] - 16;
                            // x*2, y*2, x*y //
                            acc_sq1 = ::aie::mul_square(vec_x1);
                            vec_x = acc_sq1.template to_vector<uint16_t>(0);
                            acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 0, vec_x, 11); // k0--k3
                            acc_sq1 = ::aie::mul_square(vec_x2);
                            vec_temp = acc_sq1.template to_vector<uint16_t>(0);
                            temp = ::aie::shuffle_down_fill(vec_x, vec_temp, 15);
                            acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 4, temp, 0); // k4-k7
                            acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 8, temp, 4); // k8-k10

                            acc_sq2 = ::aie::mul_square(vec_y1);
                            vec_y = acc_sq2.template to_vector<uint16_t>(0);
                            acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 0, vec_y, 11); // k0--k3
                            acc_sq2 = ::aie::mul_square(vec_y2);
                            vec_tempy = acc_sq2.template to_vector<uint16_t>(0);
                            temp_y = ::aie::shuffle_down_fill(vec_y, vec_tempy, 15);
                            acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 4, temp_y, 0); // k4-k7
                            acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 8, temp_y, 4); // k8-k10

                            acc_sq3 = ::aie::mul(vec_x1, vec_y1);
                            vec_x = acc_sq3.template to_vector<uint16_t>(0);
                            acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 0, vec_x, 11); // k0--k3
                            acc_sq3 = ::aie::mul(vec_x2, vec_y2);
                            vec_temp = acc_sq3.template to_vector<uint16_t>(0);
                            temp = ::aie::shuffle_down_fill(vec_x, vec_temp, 15);
                            acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 4, temp, 0); // k4-k7
                            acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 8, temp, 4); // k8-k10
                        }

                    ///////mu_x**2 mu_y**2///////////////////////

                    ::aie::vector<uint32_t, 16> out_x1, out_y1;
                    ::aie::vector<uint32_t, 16> x2_out;
                    out_x1 = acc_x1.template to_vector<uint32_t>(0);

                    acc_muxx = mul_32bit(out_x1, out_x1);
                    x2_out = acc_xx.template to_vector<uint32_t>(0);

                    acc_xx = ups_to_v16acc64(x2_out, 16);
                    sigmaacc_xx = sub(acc_xx, acc_muxx);

                    out_y1 = acc_y1.template to_vector<uint32_t>(0);
                    //::aie::store_v( out_ptr+=16, out_y1 );

                    acc_muyy = mul_32bit(out_y1, out_y1);
                    x2_out = acc_yy.template to_vector<uint32_t>(0);
                    acc_yy = ups_to_v16acc64(x2_out, 16);
                    sigmaacc_yy = sub(acc_yy, acc_muyy);

                    acc_muxy = mul_32bit(out_x1, out_y1);
                    x2_out = acc_xy.template to_vector<uint32_t>(0);
                    acc_xy = ups_to_v16acc64(x2_out, 16);
                    sigmaacc_xy = sub(acc_xy, acc_muxy);

                    ::aie::accum<acc64, 16> cs_de = add(sigmaacc_xx, sigmaacc_yy);
                    cs_de = add(cs_de, acc_c2);
                    ::aie::accum<acc64, 16> cs_nu = add(sigmaacc_xy, sigmaacc_xy);
                    cs_nu = add(cs_nu, acc_c2);

                    ::aie::accum<acc64, 16> ss_de = add(acc_muxx, acc_muyy);
                    ss_de = add(ss_de, acc_c1);
                    ::aie::accum<acc64, 16> ss_nu = add(acc_muxy, acc_muxy);
                    ss_nu = add(ss_nu, acc_c1);

                    out_x = cs_de.template to_vector<uint32_t>(16);
                    out_y = ss_de.template to_vector<uint32_t>(16);
                    ::aie::accum<acc64, 16> deno = mul_32bit(out_x, out_y);
                    ::aie::vector<int32_t, 16> final_deno = deno.template to_vector<int32_t>(32);

                    out_new = cs_nu.template to_vector<int32_t>(16);
                    out_new1 = ss_nu.template to_vector<uint32_t>(16);
                    ::aie::accum<acc64, 16> numo = mul_32bit(out_new1, out_new);
                    ::aie::vector<int32_t, 16> final_numo = numo.template to_vector<int32_t>(32);

                    deno_acc = ::aie::add(deno_acc, final_deno);
                    numo_acc = ::aie::add(numo_acc, final_numo);

                    acc_x1 = ::aie::zeros<acc64, 16>();
                    acc_y1 = ::aie::zeros<acc64, 16>();
                    acc_xx = ::aie::zeros<acc64, 16>();
                    acc_yy = ::aie::zeros<acc64, 16>();
                    acc_xy = ::aie::zeros<acc64, 16>();
                }
        }
        ////////////////////////rigtht region///////////////////////////////////
        {
            for (int k = 0; k < 11; k++) chess_prepare_for_pipelining chess_loop_range(1, ) {
                    coeff_row[k] = kernel_vec + (k * 16);
                    //  const uint16_t* ptr_coeff = coeff+(k*16);
                    weights.insert(0, ::aie::load_v<16>(coeff_row[k])); // load weights

                    vec_x1 = ::aie::load_v<32>(ptr[k]);   // load 32 elem of x
                    vec_x = unpack(vec_x1);               // convt to 16b type
                    vec_y1 = ::aie::load_v<32>(ptr_y[k]); // load 32 elem of x
                    vec_y = unpack(vec_y1);               // convt to 16b type

                    // compute mu_x & mu_y//
                    acc_x = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x, weights, 0, vec_x, 11);
                    temp = ::aie::shuffle_down_fill(vec_x, ::aie::broadcast<uint16, 32>(0),
                                                    5); // padding with 5 zeros on left
                    acc_x = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x, weights, 4, temp, 10);
                    acc_x = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_x, weights, 8, temp, 14);

                    acc_y = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y, weights, 0, vec_y, 11);
                    temp_y = ::aie::shuffle_down_fill(vec_y, ::aie::broadcast<uint16, 32>(0),
                                                      5); // padding with 5 zeros on left
                    acc_y = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y, weights, 4, temp_y, 10);
                    acc_y = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_y, weights, 8, temp_y, 14);
                    // x*2, y*2, x*y //

                    acc_sq1 = ::aie::mul_square(vec_x1);
                    vec_x = acc_sq1.template to_vector<uint16_t>(0);
                    acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 0, vec_x, 11);
                    temp = ::aie::shuffle_down_fill(vec_x, ::aie::broadcast<uint16, 32>(0),
                                                    5); // padding with 5 zeros on left
                    acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 4, temp, 10);
                    acc_xx = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xx, weights, 8, temp, 14);

                    acc_sq2 = ::aie::mul_square(vec_y1);
                    vec_y = acc_sq2.template to_vector<uint16_t>(0);
                    acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 0, vec_y, 11);
                    temp_y = ::aie::shuffle_down_fill(vec_y, ::aie::broadcast<uint16, 32>(0),
                                                      5); // padding with 5 zeros on left
                    acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 4, temp_y, 10);
                    acc_yy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_yy, weights, 8, temp_y, 14);

                    acc_sq3 = ::aie::mul(vec_x1, vec_y1);
                    vec_x = acc_sq3.template to_vector<uint16_t>(0);
                    acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 0, vec_x, 11);
                    temp = ::aie::shuffle_down_fill(vec_x, ::aie::broadcast<uint16, 32>(0),
                                                    5); // padding with 5 zeros on left
                    acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 4, temp, 10);
                    acc_xy = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc_xy, weights, 8, temp, 14);
                }
            ///////mu_x**2 mu_y**2///////////////////////

            ::aie::vector<uint32_t, 16> out_x1, out_y1;
            ::aie::vector<uint32_t, 16> x2_out, y2_out;

            out_x1 = acc_x.template to_vector<uint32_t>(0);
            acc_muxx = mul_32bit(out_x1, out_x1);

            x2_out = acc_xx.template to_vector<uint32_t>(0);
            acc_xx = ups_to_v16acc64(x2_out, 16);
            sigmaacc_xx = sub(acc_xx, acc_muxx);

            out_y1 = acc_y.template to_vector<uint32_t>(0);
            //::aie::store_v( out_ptr+=16, out_y1 );
            acc_muyy = mul_32bit(out_y1, out_y1);
            y2_out = acc_yy.template to_vector<uint32_t>(0);
            acc_yy = ups_to_v16acc64(y2_out, 16);
            sigmaacc_yy = sub(acc_yy, acc_muyy);

            acc_muxy = mul_32bit(out_x1, out_y1);
            x2_out = acc_xy.template to_vector<uint32_t>(0);
            acc_xy = ups_to_v16acc64(x2_out, 16);
            sigmaacc_xy = sub(acc_xy, acc_muxy);

            ::aie::accum<acc64, 16> cs_de = add(sigmaacc_xx, sigmaacc_yy);
            cs_de = add(cs_de, acc_c2);
            ::aie::accum<acc64, 16> cs_nu = add(sigmaacc_xy, sigmaacc_xy);
            cs_nu = add(cs_nu, acc_c2);

            ::aie::accum<acc64, 16> ss_de = add(acc_muxx, acc_muyy);
            ss_de = add(ss_de, acc_c1);
            ::aie::accum<acc64, 16> ss_nu = add(acc_muxy, acc_muxy);
            ss_nu = add(ss_nu, acc_c1);

            out_x = cs_de.template to_vector<uint32_t>(16);
            out_y = ss_de.template to_vector<uint32_t>(16);
            ::aie::accum<acc64, 16> deno = mul_32bit(out_x, out_y);
            ::aie::vector<int32_t, 16> final_deno = deno.template to_vector<int32_t>(32);

            out_new = cs_nu.template to_vector<int32_t>(16);
            out_new1 = ss_nu.template to_vector<uint32_t>(16);
            ::aie::accum<acc64, 16> numo = mul_32bit(out_new1, out_new);
            ::aie::vector<int32_t, 16> final_numo = numo.template to_vector<int32_t>(32);

            deno_acc = ::aie::add(deno_acc, final_deno);
            numo_acc = ::aie::add(numo_acc, final_numo);

            acc_x = ::aie::zeros<acc64, 16>();
            acc_y = ::aie::zeros<acc64, 16>();
            acc_xx = ::aie::zeros<acc64, 16>();
            acc_yy = ::aie::zeros<acc64, 16>();
            acc_xy = ::aie::zeros<acc64, 16>();
        }

    } // rowloop

    deno_sum = reduceadd(deno_acc);
    numo_sum = reduceadd(numo_acc);

    /*int64_t temp_deno=deno_sum;
int64_t temp_numo=numo_sum;

printf("*****************************************************************************deno***************************************************
is %lld\n",temp_deno);
printf("*****************************************************************************numo***************************************************
is %lld\n",temp_numo);
     */

} // xf_ssim

template <int TILE_WIDTH, int TILE_HEIGHT, int N>
void ssimBaseImpl<TILE_WIDTH, TILE_HEIGHT, N>::runImpl(uint8* img_in1,
                                                       uint8* img_in2,
                                                       int64& deno_sum,
                                                       int64& numo_sum) {
    xf_ssim(img_in1, img_in2, mwei, deno_sum, numo_sum);
}

} // aie
} // cv
} // xf
