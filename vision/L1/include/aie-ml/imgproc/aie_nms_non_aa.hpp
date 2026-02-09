/***********************************************************************************************************
© Copyright 2023 Advanced Micro Devices, Inc. All rights reserved.
This file contains confidential and proprietary information of Advanced Micro Devices, Inc. (AMD) and is
protected under U.S. and international copyright and other intellectual property laws.
DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials distributed herewith.
Except as otherwise provided in a valid license issued to you by AMD, and to the maximum extent permitted
by applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND AMD HEREBY
DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO
WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and (2) AMD shall
not be liable (whether in contract or tort, including negligence, or under any other theory of liability)
for any loss or damage of any kind or nature related to, arising under or in connection with these materials,
including for any direct, or any indirect, special, incidental, or consequential loss or damage (including
loss of data, profits, goodwill, or any type of loss or damage suffered as a result of any action brought by
a third party) even if such damage or loss was reasonably foreseeable or AMD had been advised of the possibility
of the same.
CRITICAL APPLICATIONS
AMD products are not designed or intended to be fail-safe, or for use in any application requiring fail-
safe performance, such as life-support or safety devices or systems, Class III medical devices, nuclear
facilities, applications related to the deployment of airbags, or any other applications that could lead to
death, personal injury, or severe property or environmental damage (individually and collectively,
"Critical Applications"). Customer assumes the sole risk and liability of any use of AMD products in
Critical Applications, subject only to applicable laws and regulations governing limitations on product
liability.
THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES
***********************************************************************************************************/

#ifndef __AIE_NMS_NON_AA_
#define __AIE_NMS_NON_AA_

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>

namespace xf {
namespace cv {
namespace aie {

bfloat16 iou_threshold = 0.6;

// 0: Ref, 1: Target
// 0: X, 1: Y
// 0: Bottom left, 1 Bottom right, 2 Top right, 3 Top left
// 116 Number of boxes
alignas(::aie::vector_decl_align) bfloat16 norm_bf[2][2][4][116];

// Q.21 format
// 0: X, 1: Y
// 116 Number of boxes
alignas(::aie::vector_decl_align) int32 centre_i32[2][116];

// Area for all boxes
alignas(::aie::vector_decl_align) bfloat16 area[116];

// Q.21 format
// 0: X, 1: Y
// 116 Number of boxes
alignas(::aie::vector_decl_align) int32 rotated_coordinates_i32[2][4][116];

// sin cos co-eff
alignas(::aie::vector_decl_align) int32_t sin_coeffs[16] = {67105182, -11178610, 556320, -12750, 136};
alignas(::aie::vector_decl_align) int32_t cos_coeffs[16] = {67108754, -33553524, 2795239, -92852, 1606, -14};
alignas(::aie::vector_decl_align) bfloat16 tan_coeffs[16] = {1, 0.00043487548828125, -0.3671875, 0.15234375};

// total number of polygon vertex per box
alignas(::aie::vector_decl_align) uint8_t num_valid_polygon_coord[116] = {0};

// polygon vertex, append the vertex accordingly
alignas(::aie::vector_decl_align) bfloat16 polygon_vertex[2][8][116];

// Array for inverse operations, min of 1/1 to 1/8
alignas(::aie::vector_decl_align) bfloat16 inv_array[9] = {0, 1, 0.5, 0.3333, 0.25, 0.2, 0.16667, 0.142857, 0.125};

// Array for order, this array will be used for sorting
alignas(::aie::vector_decl_align) int32 order_arr[8] = {0, 1, 2, 3, 4, 5, 6, 7};

// loop checker value for pipelining, computed as,
// total elem count: 100
// maximum vectorizable data 16*
static unsigned char LOOP_CHECK_VAL = 4; // (100-96)

/**********************************************************
 * init the num Valid array
 * ********************************************************/
template <int _N, int VEC_FACTOR>
__attribute__((noinline)) void initMem() {
    for (auto i = 0u; i < 116; i++) num_valid_polygon_coord[i] = 0;
}

/**********************************************************
 * computePolynomial
 * ********************************************************/
template <int NO_COEFFS, int VEC_FACTOR>
::aie::vector<int32, VEC_FACTOR> computePolynomial(::aie::vector<int32, VEC_FACTOR> x1, int32* coeffs) {
    ::aie::vector<int32_t, VEC_FACTOR> x1_square, vacc, vcoeff;
    ::aie::accum<acc64, VEC_FACTOR> acc;

    vacc = ::aie::broadcast<int32_t, VEC_FACTOR>(coeffs[NO_COEFFS - 2]);
    vcoeff = ::aie::broadcast<int32_t, VEC_FACTOR>(coeffs[NO_COEFFS - 1]);

    acc = ::aie::mul(x1, x1);
    x1_square = acc.template to_vector<int32_t>(26);
    acc.from_vector(vacc, 26);

    acc = ::aie::mac(acc, x1_square, vcoeff);

    for (int i = NO_COEFFS - 3; i >= 0; i--) chess_prepare_for_pipelining chess_unroll_loop() {
            vacc = acc.template to_vector<int32_t>(26);
            vcoeff = ::aie::broadcast<int32_t, VEC_FACTOR>(coeffs[i]);
            acc.from_vector(vcoeff, 26);
            acc = ::aie::mac(acc, x1_square, vacc);
        }
    vacc = acc.template to_vector<int32_t>(26);
    return vacc;
}

/**********************************************************
 * Rotate Coordinates
 * ********************************************************/
template <int IN_TENSOR_LENGTH, int SUB_TENSOR_LENGTH, int VEC_FACTOR>
__attribute__((noinline)) void rotateCoordImpl(
    adf::input_buffer<float, adf::extents<IN_TENSOR_LENGTH> >& __restrict input) {
    auto c_x_in = ::aie::begin_vector<VEC_FACTOR>(input);
    auto c_y_in = c_x_in + SUB_TENSOR_LENGTH / VEC_FACTOR;
    auto wvec_in = c_x_in + 2 * SUB_TENSOR_LENGTH / VEC_FACTOR;
    auto hvec_in = c_x_in + 3 * SUB_TENSOR_LENGTH / VEC_FACTOR;
    auto avec_in = c_x_in + 4 * SUB_TENSOR_LENGTH / VEC_FACTOR;

    for (auto i = 0u; i < (SUB_TENSOR_LENGTH / VEC_FACTOR); i++) chess_prepare_for_pipelining {
            auto aveci32 = ::aie::to_fixed<int32, VEC_FACTOR>(*avec_in++, 26);
            auto ovecs = computePolynomial<5, VEC_FACTOR>(aveci32, sin_coeffs);
            ovecs = ::aie::mul(ovecs, aveci32).template to_vector<int32>(26);
            auto ovecc = computePolynomial<6, VEC_FACTOR>(aveci32, cos_coeffs);

            auto hveci32 = ::aie::to_fixed<int32, VEC_FACTOR>(*hvec_in++, 21);
            auto wveci32 = ::aie::to_fixed<int32, VEC_FACTOR>(*wvec_in++, 21);
            auto c_x_i32 = ::aie::to_fixed<int32, VEC_FACTOR>(*c_x_in++, 21);
            auto c_y_i32 = ::aie::to_fixed<int32, VEC_FACTOR>(*c_y_in++, 21);

            auto area_hw = ::aie::mul<acc64>(hveci32, wveci32).template to_vector<int32>(26); // A15.16
            auto area_bf16 = ::aie::to_float<bfloat16>(area_hw, 16);
            ::aie::store_unaligned_v(area + i * VEC_FACTOR, area_bf16);

            int32* restrict c_x_ptr = (int32*)centre_i32[0];
            int32* restrict c_y_ptr = (int32*)centre_i32[1];

            int32* restrict rc_ptr[2][4];

            for (auto j = 0u; j < 2; j++) {
                for (auto k = 0u; k < 4; k++) {
                    rc_ptr[j][k] = (int32*)rotated_coordinates_i32[j][k];
                }
            }

            ::aie::store_unaligned_v(c_x_ptr + i * VEC_FACTOR, c_x_i32);
            ::aie::store_unaligned_v(c_y_ptr + i * VEC_FACTOR, c_y_i32);

            // x offset for height,width aie::sub(x_cos, y_sin)
            // x offset for height,-width aie::sub(-1*x_cos, y_sin)
            // x offset for -height,-width aie::sub(y_sin, x_cos)
            // x offset for -height,width aie::sub(x_cos, -1*y_sin)
            auto x_temp = ::aie::mul<acc64>(wveci32, ovecc).template to_vector<int32>(27);
            auto y_temp = ::aie::mul<acc64>(hveci32, ovecs).template to_vector<int32>(27);

            ::aie::vector<int32, VEC_FACTOR> output_vec;
            output_vec = ::aie::add(c_x_i32, ::aie::sub(y_temp, x_temp));
            ::aie::store_unaligned_v(rc_ptr[0][0] + i * VEC_FACTOR, output_vec);

            output_vec = ::aie::add(c_x_i32, ::aie::add(x_temp, y_temp));
            ::aie::store_unaligned_v(rc_ptr[0][1] + i * VEC_FACTOR, output_vec);

            output_vec = ::aie::add(c_x_i32, ::aie::sub(x_temp, y_temp));
            ::aie::store_unaligned_v(rc_ptr[0][2] + i * VEC_FACTOR, output_vec);

            output_vec = ::aie::add(c_x_i32, ::aie::sub(::aie::neg(x_temp), y_temp));
            ::aie::store_unaligned_v(rc_ptr[0][3] + i * VEC_FACTOR, output_vec);

            // y offset for height,width aie::add(x_sin, y_cos)
            // y offset for height,-width aie::add(-1*x_sin, y_cos)
            // y offset for -height,-width aie::add(-1*x_sin, -1*y_cos)
            // y offset for -height,width aie::add(x_sin, -1*y_cos)
            x_temp = ::aie::mul<acc64>(wveci32, ovecs).template to_vector<int32>(27);
            y_temp = ::aie::mul<acc64>(hveci32, ovecc).template to_vector<int32>(27);

            output_vec = ::aie::add(c_y_i32, ::aie::neg(::aie::add(x_temp, y_temp)));
            ::aie::store_unaligned_v(rc_ptr[1][0] + i * VEC_FACTOR, output_vec);

            output_vec = ::aie::add(c_y_i32, ::aie::sub(x_temp, y_temp));
            ::aie::store_unaligned_v(rc_ptr[1][1] + i * VEC_FACTOR, output_vec);

            output_vec = ::aie::add(c_y_i32, ::aie::add(x_temp, y_temp));
            ::aie::store_unaligned_v(rc_ptr[1][2] + i * VEC_FACTOR, output_vec);

            output_vec = ::aie::add(c_y_i32, ::aie::sub(y_temp, x_temp));
            ::aie::store_unaligned_v(rc_ptr[1][3] + i * VEC_FACTOR, output_vec);
        }
}

/**********************************************************
 * NormalizeCoordinates
 * ********************************************************/
template <int _N, int VEC_FACTOR>
__attribute__((noinline)) void normCoordImpl(uint8_t start_index) {
    int32 curr_centre[2];
    ::aie::vector<int32, VEC_FACTOR> c_vec[2];
    ::aie::vector<int32, VEC_FACTOR> target_coordinates[2][4];
    bfloat16* restrict store_ptr_ref[2][4];
    bfloat16* restrict store_ptr_target[2][4];

    curr_centre[0] = centre_i32[0][start_index];
    curr_centre[1] = centre_i32[1][start_index];

    int32 ref_coordinates[2][4];
    for (auto j = 0u; j < 2; j++) chess_prepare_for_pipelining {
            for (auto k = 0u; k < 4; k++) {
                ref_coordinates[j][k] = rotated_coordinates_i32[j][k][start_index];
            }
        }
    int load_idx = start_index + 1;
    for (auto i = 0; i < _N; i += VEC_FACTOR) chess_prepare_for_pipelining {
            if (load_idx < (i + LOOP_CHECK_VAL)) {
                for (auto j = 0u; j < 2; j++) {
                    c_vec[j] = ::aie::load_unaligned_v((int32*)centre_i32[j] + load_idx);
                    c_vec[j] = ::aie::min(c_vec[j], curr_centre[j]);

                    for (auto k = 0u; k < 4; k++) {
                        target_coordinates[j][k] =
                            ::aie::load_unaligned_v((int32*)rotated_coordinates_i32[j][k] + load_idx);
                        store_ptr_ref[j][k] = (bfloat16*)norm_bf[0][j][k] + load_idx;
                        store_ptr_target[j][k] = (bfloat16*)norm_bf[1][j][k] + load_idx;
                        ::aie::store_unaligned_v(
                            store_ptr_ref[j][k],
                            ::aie::to_float<bfloat16>(::aie::sub(ref_coordinates[j][k], c_vec[j]), 21));
                        ::aie::store_unaligned_v(
                            store_ptr_target[j][k],
                            ::aie::to_float<bfloat16>(::aie::sub(target_coordinates[j][k], c_vec[j]), 21));
                    }
                }
                load_idx += VEC_FACTOR;
            }
        }
}

/**********************************************************
 * CrossProduct
 * ********************************************************/
template <int VEC_FACTOR>
__attribute__((noinline))::aie::vector<bfloat16, VEC_FACTOR> crossProduct(
    ::aie::vector<bfloat16, VEC_FACTOR> refBox_a_x,
    ::aie::vector<bfloat16, VEC_FACTOR> refBox_a_y,
    ::aie::vector<bfloat16, VEC_FACTOR> refBox_b_x,
    ::aie::vector<bfloat16, VEC_FACTOR> refBox_b_y,
    ::aie::vector<bfloat16, VEC_FACTOR> tarBox_x,
    ::aie::vector<bfloat16, VEC_FACTOR> tarBox_y) {
    ::aie::vector<bfloat16, VEC_FACTOR> term1 = ::aie::sub(refBox_b_x, refBox_a_x);
    ::aie::vector<bfloat16, VEC_FACTOR> term2 = ::aie::sub(tarBox_y, refBox_a_y);
    ::aie::vector<bfloat16, VEC_FACTOR> term3 = ::aie::sub(refBox_b_y, refBox_a_y);
    ::aie::vector<bfloat16, VEC_FACTOR> term4 = ::aie::sub(tarBox_x, refBox_a_x);

    ::aie::accum<accfloat, VEC_FACTOR> acc_term1 = ::aie::mul(term2, term1);
    ::aie::accum<accfloat, VEC_FACTOR> acc_term2 = ::aie::mul(term4, term3);
    ::aie::accum<accfloat, VEC_FACTOR> acc_term = ::aie::sub(acc_term1, acc_term2);

    ::aie::vector<bfloat16, VEC_FACTOR> vec_ret = acc_term.template to_vector<bfloat16>(0);
    return vec_ret;
}

/**********************************************************
 * findInness
 * ********************************************************/
template <int _N, int VEC_FACTOR>
__attribute__((noinline)) void findInnessImpl(bfloat16 refBox_x[4][116],
                                              bfloat16 refBox_y[4][116],
                                              bfloat16 tarBox_x[4][116],
                                              bfloat16 tarBox_y[4][116],
                                              uint8_t start_index) { // TODO: keep this 100 or 132, for loading vectors

    for (auto i = 0u; i < 4; i++) {
        int load_idx = start_index + 1;
        for (auto iter = 0; iter < _N; iter += VEC_FACTOR) chess_prepare_for_pipelining {
                if (load_idx < (iter + LOOP_CHECK_VAL)) {
                    ::aie::vector<bfloat16, VEC_FACTOR> ref_vec_x[4];
                    ::aie::vector<bfloat16, VEC_FACTOR> ref_vec_y[4];
                    for (auto j = 0u; j < 4; j++) {
                        ref_vec_x[j] = ::aie::load_unaligned_v<VEC_FACTOR>(&refBox_x[j][load_idx]);
                        ref_vec_y[j] = ::aie::load_unaligned_v<VEC_FACTOR>(&refBox_y[j][load_idx]);
                    }
                    ::aie::vector<bfloat16, VEC_FACTOR> tar_vec_x =
                        ::aie::load_unaligned_v<VEC_FACTOR>(&tarBox_x[i][load_idx]);
                    ::aie::vector<bfloat16, VEC_FACTOR> tar_vec_y =
                        ::aie::load_unaligned_v<VEC_FACTOR>(&tarBox_y[i][load_idx]);

                    ::aie::vector<bfloat16, VEC_FACTOR> cp1 = crossProduct<VEC_FACTOR>(
                        ref_vec_x[0], ref_vec_y[0], ref_vec_x[1], ref_vec_y[1], tar_vec_x, tar_vec_y);
                    ::aie::vector<bfloat16, VEC_FACTOR> cp2 = crossProduct<VEC_FACTOR>(
                        ref_vec_x[1], ref_vec_y[1], ref_vec_x[2], ref_vec_y[2], tar_vec_x, tar_vec_y);
                    ::aie::vector<bfloat16, VEC_FACTOR> cp3 = crossProduct<VEC_FACTOR>(
                        ref_vec_x[2], ref_vec_y[2], ref_vec_x[3], ref_vec_y[3], tar_vec_x, tar_vec_y);
                    ::aie::vector<bfloat16, VEC_FACTOR> cp4 = crossProduct<VEC_FACTOR>(
                        ref_vec_x[3], ref_vec_y[3], ref_vec_x[0], ref_vec_y[0], tar_vec_x, tar_vec_y);

                    bfloat16 zero_val = 0;

                    ::aie::mask<VEC_FACTOR> le_mask1 = ::aie::le(cp1, zero_val);
                    ::aie::mask<VEC_FACTOR> le_mask2 = ::aie::le(cp2, zero_val);
                    ::aie::mask<VEC_FACTOR> le_mask3 = ::aie::le(cp3, zero_val);
                    ::aie::mask<VEC_FACTOR> le_mask4 = ::aie::le(cp4, zero_val);

                    ::aie::mask<VEC_FACTOR> ge_mask1 = ::aie::ge(cp1, zero_val);
                    ::aie::mask<VEC_FACTOR> ge_mask2 = ::aie::ge(cp2, zero_val);
                    ::aie::mask<VEC_FACTOR> ge_mask3 = ::aie::ge(cp3, zero_val);
                    ::aie::mask<VEC_FACTOR> ge_mask4 = ::aie::ge(cp4, zero_val);

                    ::aie::mask<VEC_FACTOR> le_mask = le_mask1 & le_mask2 & le_mask3 & le_mask4;
                    ::aie::mask<VEC_FACTOR> ge_mask = ge_mask1 & ge_mask2 & ge_mask3 & ge_mask4;

                    ::aie::mask<VEC_FACTOR> in_mask = le_mask | ge_mask;

                    for (int j = 0; j < VEC_FACTOR; j++) {
                        int idx_pv = load_idx + j;
                        if (in_mask.test(j)) {
                            polygon_vertex[0][num_valid_polygon_coord[idx_pv]][idx_pv] = tar_vec_x[j];
                            polygon_vertex[1][num_valid_polygon_coord[idx_pv]][idx_pv] = tar_vec_y[j];
                            num_valid_polygon_coord[idx_pv]++;
                        }
                    }
                    load_idx += VEC_FACTOR;
                }
            }
    }
}

/**********************************************************
 * findCross
 * ********************************************************/
template <int _N, int VEC_FACTOR>
__attribute__((noinline)) void findCrossImpl(bfloat16 refBox_x[4][116],
                                             bfloat16 refBox_y[4][116],
                                             bfloat16 tarBox_x[4][116],
                                             bfloat16 tarBox_y[4][116],
                                             uint8_t start_index) { // TODO: keep this 100 or 132, for loading vectors

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int load_idx = start_index + 1;
            for (auto iter = 0; iter < _N; iter += VEC_FACTOR) chess_prepare_for_pipelining {
                    if (load_idx < (iter + LOOP_CHECK_VAL)) {
                        ::aie::vector<bfloat16, VEC_FACTOR> ref_vec_x[2];
                        ::aie::vector<bfloat16, VEC_FACTOR> ref_vec_y[2];
                        ::aie::vector<bfloat16, VEC_FACTOR> tar_vec_x[2];
                        ::aie::vector<bfloat16, VEC_FACTOR> tar_vec_y[2];

                        int im1 = (i == 0) ? 3 : (i - 1);
                        int jm1 = (j == 0) ? 3 : (j - 1);
                        ref_vec_x[0] = ::aie::load_unaligned_v<VEC_FACTOR>(&refBox_x[jm1][load_idx]);
                        ref_vec_x[1] = ::aie::load_unaligned_v<VEC_FACTOR>(&refBox_x[j][load_idx]);
                        ref_vec_y[0] = ::aie::load_unaligned_v<VEC_FACTOR>(&refBox_y[jm1][load_idx]);
                        ref_vec_y[1] = ::aie::load_unaligned_v<VEC_FACTOR>(&refBox_y[j][load_idx]);

                        tar_vec_x[0] = ::aie::load_unaligned_v<VEC_FACTOR>(&tarBox_x[im1][load_idx]);
                        tar_vec_x[1] = ::aie::load_unaligned_v<VEC_FACTOR>(&tarBox_x[i][load_idx]);
                        tar_vec_y[0] = ::aie::load_unaligned_v<VEC_FACTOR>(&tarBox_y[im1][load_idx]);
                        tar_vec_y[1] = ::aie::load_unaligned_v<VEC_FACTOR>(&tarBox_y[i][load_idx]);

                        ::aie::vector<bfloat16, VEC_FACTOR> det_term1 = ::aie::sub(ref_vec_x[1], ref_vec_x[0]);
                        ::aie::vector<bfloat16, VEC_FACTOR> det_term2 = ::aie::sub(tar_vec_y[1], tar_vec_y[0]);
                        ::aie::vector<bfloat16, VEC_FACTOR> det_term3 = ::aie::sub(ref_vec_y[1], ref_vec_y[0]);
                        ::aie::vector<bfloat16, VEC_FACTOR> det_term4 = ::aie::sub(tar_vec_x[1], tar_vec_x[0]);

                        ::aie::accum<accfloat, VEC_FACTOR> acc_term1 = ::aie::mul(det_term1, det_term2);
                        ::aie::accum<accfloat, VEC_FACTOR> acc_term2 = ::aie::mul(det_term3, det_term4);
                        ::aie::accum<accfloat, VEC_FACTOR> acc_term = ::aie::sub(acc_term1, acc_term2);
                        ::aie::vector<bfloat16, VEC_FACTOR> det_vec = acc_term.template to_vector<bfloat16>(0);

                        ::aie::vector<bfloat16, VEC_FACTOR> inv_det_vec = ::aie::inv(det_vec);

                        ::aie::vector<bfloat16, VEC_FACTOR> cp1 = crossProduct<VEC_FACTOR>(
                            tar_vec_x[0], tar_vec_y[0], tar_vec_x[1], tar_vec_y[1], ref_vec_x[0], ref_vec_y[0]);
                        ::aie::vector<bfloat16, VEC_FACTOR> cp2 = crossProduct<VEC_FACTOR>(
                            ref_vec_x[0], ref_vec_y[0], ref_vec_x[1], ref_vec_y[1], tar_vec_x[0], tar_vec_y[0]);

                        ::aie::accum<accfloat, VEC_FACTOR> ua = ::aie::mul(cp1, inv_det_vec);
                        ::aie::accum<accfloat, VEC_FACTOR> ub = ::aie::mul(cp2, inv_det_vec);
                        ::aie::vector<bfloat16, VEC_FACTOR> ua_vec = ua.template to_vector<bfloat16>(0);
                        ::aie::vector<bfloat16, VEC_FACTOR> ub_vec = ub.template to_vector<bfloat16>(0);
                        ::aie::vector<bfloat16, VEC_FACTOR> sub_term_x = ::aie::sub(ref_vec_x[1], ref_vec_x[0]);
                        ::aie::vector<bfloat16, VEC_FACTOR> sub_term_y = ::aie::sub(ref_vec_y[1], ref_vec_y[0]);

                        // TODO: implement this as mac operation
                        ::aie::accum<accfloat, VEC_FACTOR> acc_term_x = ::aie::mul(ua_vec, sub_term_x);
                        ::aie::accum<accfloat, VEC_FACTOR> acc_term_y = ::aie::mul(ua_vec, sub_term_y);
                        ::aie::vector<bfloat16, VEC_FACTOR> mul_term_x = acc_term_x.template to_vector<bfloat16>(0);
                        ::aie::vector<bfloat16, VEC_FACTOR> mul_term_y = acc_term_y.template to_vector<bfloat16>(0);
                        ::aie::vector<bfloat16, VEC_FACTOR> intersec_x = ::aie::add(mul_term_x, ref_vec_x[0]);
                        ::aie::vector<bfloat16, VEC_FACTOR> intersec_y = ::aie::add(mul_term_y, ref_vec_y[0]);

                        ::aie::mask<VEC_FACTOR> mask1 = ::aie::ge(ua_vec, (bfloat16)0);
                        ::aie::mask<VEC_FACTOR> mask2 = ::aie::le(ua_vec, (bfloat16)1);
                        ::aie::mask<VEC_FACTOR> mask3 = ::aie::le(ub_vec, (bfloat16)0);
                        ::aie::mask<VEC_FACTOR> mask4 = ::aie::ge(ub_vec, (bfloat16)(-1));
                        ::aie::mask<VEC_FACTOR> mask_det = ::aie::neq(det_vec, (bfloat16)(0)); // parallel or coinc

                        ::aie::mask<VEC_FACTOR> in_mask = mask1 & mask2 & mask3 & mask4 & mask_det;
                        for (int vec_j = 0; vec_j < VEC_FACTOR; vec_j++) {
                            int idx_pv = load_idx + vec_j;
                            if (in_mask.test(vec_j)) {
                                polygon_vertex[0][num_valid_polygon_coord[idx_pv]][idx_pv] = intersec_x[vec_j];
                                polygon_vertex[1][num_valid_polygon_coord[idx_pv]][idx_pv] = intersec_y[vec_j];
                                num_valid_polygon_coord[idx_pv]++;
                            }
                        }
                        load_idx += VEC_FACTOR;
                    }
                }
        }
    }
}

/**********************************************************
 * atan2_runImpl
 * ********************************************************/
template <int NO_COEFFS, int VEC_FAC_SORT>
__attribute__((always_inline))::aie::vector<bfloat16, VEC_FAC_SORT> atan2Impl(
    ::aie::vector<bfloat16, VEC_FAC_SORT> x1, ::aie::vector<bfloat16, VEC_FAC_SORT> y1) {
    // Load input
    ::aie::vector<bfloat16, VEC_FAC_SORT> z1, z1_sq, z1_inv, z1_inv_sq, vacc, vacc_inv, vcoeff;
    ::aie::accum<accfloat, VEC_FAC_SORT> acc, acc_inv;
    ::aie::vector<float, VEC_FAC_SORT> buff;

    const bfloat16 one_bf = 1;
    const bfloat16 neg_one_bf = -1;
    const bfloat16 zero_bf = 0;
    const bfloat16 pi_by_2_bf = 1.57079633;
    const bfloat16 pi_bf = 3.14159265;
    const bfloat16 neg_pi_bf = -3.14159265;

    z1 = ::aie::mul<accfloat>(y1, ::aie::inv(x1)).template to_vector<bfloat16>();
    z1_inv = ::aie::mul<accfloat>(x1, ::aie::inv(y1)).template to_vector<bfloat16>();
    z1 = ::aie::abs(z1);
    z1_inv = ::aie::abs(z1_inv);

    vacc = ::aie::broadcast<bfloat16, VEC_FAC_SORT>(tan_coeffs[NO_COEFFS - 2]);
    vacc_inv = ::aie::broadcast<bfloat16, VEC_FAC_SORT>(tan_coeffs[NO_COEFFS - 2]);
    vcoeff = ::aie::broadcast<bfloat16, VEC_FAC_SORT>(tan_coeffs[NO_COEFFS - 1]);
    acc.from_vector(vacc);
    acc_inv.from_vector(vacc_inv);

    acc = ::aie::mac(acc, z1, vcoeff);
    acc_inv = ::aie::mac(acc_inv, z1_inv, vcoeff);
    for (int i = NO_COEFFS - 3; i >= 0; i--) chess_prepare_for_pipelining chess_unroll_loop() {
            vacc = acc.template to_vector<bfloat16>();
            vacc_inv = acc_inv.template to_vector<bfloat16>();
            vcoeff = ::aie::broadcast<bfloat16, VEC_FAC_SORT>(tan_coeffs[i]);
            acc.from_vector(vcoeff);
            acc_inv.from_vector(vcoeff);
            acc = ::aie::mac(acc, z1, vacc);
            acc_inv = ::aie::mac(acc_inv, z1_inv, vacc_inv);
        }
    vacc = acc.template to_vector<bfloat16>();
    vacc_inv = acc_inv.template to_vector<bfloat16>();

    acc = ::aie::mul<accfloat>(vacc, z1);
    acc_inv = ::aie::mul<accfloat>(vacc_inv, z1_inv);
    vacc = acc.template to_vector<bfloat16>();
    vacc_inv = acc_inv.template to_vector<bfloat16>();
    vacc_inv = ::aie::sub(pi_by_2_bf, vacc_inv);

    ::aie::mask<VEC_FAC_SORT> angle_mask = ::aie::gt(z1, one_bf);
    vacc = ::aie::select(vacc, vacc_inv, angle_mask);

    angle_mask = ::aie::lt(x1, zero_bf);
    vacc_inv = ::aie::sub(pi_bf, vacc);
    vacc = ::aie::select(vacc, vacc_inv, angle_mask);

    angle_mask = ::aie::lt(y1, zero_bf);
    vacc_inv = ::aie::sub(zero_bf, vacc);
    vacc = ::aie::select(vacc, vacc_inv, angle_mask);

    return vacc;
}

/**********************************************************
 * SortVertex
 * ********************************************************/
template <int VEC_FAC_SORT>
__attribute__((noinline)) void sortVertexImpl(int start_index) {
    for (int iter = 0; iter < 100; iter++) chess_prepare_for_pipelining {
            int8_t numPoints = num_valid_polygon_coord[iter];
            if (iter >= (start_index + 1) && numPoints > 2) {
                auto order = ::aie::load_v<VEC_FAC_SORT>(order_arr);
                ::aie::vector<bfloat16, 8> vec_x, vec_y;
                bfloat16 centroid_x = 0, centroid_y = 0;
                for (auto i = 0; i < numPoints; i++) {
                    vec_x[i] = polygon_vertex[0][i][iter];
                    vec_y[i] = polygon_vertex[1][i][iter];
                    centroid_x += vec_x[i];
                    centroid_y += vec_y[i];
                }

                // TODO: check for acc loss here, need more prec?
                bfloat16 inv_val = inv_array[numPoints];
                centroid_x *= inv_val;
                centroid_y *= inv_val;

                auto vec_x1 = ::aie::sub(vec_x, centroid_x);
                auto vec_y1 = ::aie::sub(vec_y, centroid_y);
                auto vec_angle = atan2Impl<4, VEC_FAC_SORT>(vec_x, vec_y);

                auto vec_32 = ::aie::to_fixed<int32, VEC_FAC_SORT>(vec_angle, 18);
                int32 mask_val = 0xFFFFFFF8;
                vec_32 = ::aie::bit_and(mask_val, vec_32);
                vec_32 = ::aie::add(vec_32, order);

                for (int i = 0; i < numPoints - 1; i++) {
                    for (int j = 0; j < numPoints - i - 1; j++) {
                        if (vec_32[j] > vec_32[j + 1]) {
                            int32 temp = vec_32[j];
                            vec_32[j] = vec_32[j + 1];
                            vec_32[j + 1] = temp;
                        }
                    }
                }

                for (auto i = 0; i < numPoints; i++) {
                    polygon_vertex[0][i][iter] = vec_x[vec_32[i] & 0x7];
                    polygon_vertex[1][i][iter] = vec_y[vec_32[i] & 0x7];
                }
                for (auto i = numPoints; i < VEC_FAC_SORT; i++) {
                    polygon_vertex[0][i][iter] = vec_x[vec_32[0] & 0x7];
                    polygon_vertex[1][i][iter] = vec_y[vec_32[0] & 0x7];
                }
            }
        }
}

/**********************************************************
 * AopAndIouImpl
 * ********************************************************/
template <int _N, int VEC_FACTOR>
__attribute__((noinline)) void AopAndIouImpl(int start_index, uint8_t* box_valid) {
    bfloat16 area1 = area[start_index];

    int load_idx = start_index + 1;
    for (auto i = 0; i < _N; i += VEC_FACTOR) chess_prepare_for_pipelining {
            if (load_idx < (i + LOOP_CHECK_VAL)) {
                // Intersection area computation / area of Polygon
                ::aie::vector<bfloat16, VEC_FACTOR> prev_x, curr_x;
                ::aie::vector<bfloat16, VEC_FACTOR> prev_y, curr_y;
                ::aie::accum<accfloat, VEC_FACTOR> acc_area;
                acc_area = ::aie::sub(acc_area, acc_area); // TODO: init accumulator to zero

                prev_x = ::aie::load_unaligned_v<VEC_FACTOR>((bfloat16*)polygon_vertex[0][7] + load_idx);
                prev_y = ::aie::load_unaligned_v<VEC_FACTOR>((bfloat16*)polygon_vertex[1][7] + load_idx);

                for (auto j = 0u; j < 8; j++) {
                    curr_x = ::aie::load_unaligned_v<VEC_FACTOR>((bfloat16*)polygon_vertex[0][j] + load_idx);
                    curr_y = ::aie::load_unaligned_v<VEC_FACTOR>((bfloat16*)polygon_vertex[1][j] + load_idx);

                    prev_x = ::aie::sub(curr_x, prev_x);
                    prev_y = ::aie::add(curr_y, prev_y);

                    acc_area = ::aie::add(acc_area, ::aie::mul<accfloat>(prev_x, prev_y));

                    prev_x = curr_x;
                    prev_y = curr_y;
                }

                ::aie::vector<bfloat16, VEC_FACTOR> area_sum = acc_area.template to_vector<bfloat16>();
                acc_area = ::aie::mul<accfloat>(area_sum, (bfloat16)0.5);
                area_sum = acc_area.template to_vector<bfloat16>();
                area_sum = ::aie::abs(area_sum);

                // Compute den for IOU and multiply with iou thresh
                ::aie::vector<bfloat16, VEC_FACTOR> area2 =
                    ::aie::load_unaligned_v<VEC_FACTOR>((bfloat16*)area + load_idx);
                area2 = ::aie::add(area2, area1);
                area2 = ::aie::sub(area2, area_sum);
                auto iou_updated = ::aie::mul<accfloat>(area2, iou_threshold).template to_vector<bfloat16>();

                auto iou_mask = ::aie::le(
                    area_sum,
                    iou_updated); // if the area is less than that of the threshold, there is no enough overlap
                ::aie::vector<uint8_t, VEC_FACTOR> num_vertex_vec =
                    ::aie::load_unaligned_v<VEC_FACTOR>((uint8_t*)num_valid_polygon_coord + load_idx);
                auto vertex_mask = ::aie::le(num_vertex_vec,
                                             (uint8_t)2); // any polygon with vertex of less than or eq to 2 will be '1'
                auto comp_mask = iou_mask | vertex_mask;
                ::aie::vector<uint8_t, VEC_FACTOR> mask_to_vec = ::aie::select((uint8_t)0, (uint8_t)1, comp_mask);

                ::aie::vector<uint8_t, VEC_FACTOR> flags =
                    ::aie::load_unaligned_v<VEC_FACTOR>((uint8_t*)box_valid + load_idx);
                flags = ::aie::bit_and(flags, mask_to_vec);
                ::aie::store_unaligned_v((uint8_t*)box_valid + load_idx, flags);
                load_idx += VEC_FACTOR;
            }
        }
}

/**********************************************************
 * nmsNonAA
 * ********************************************************/
template <int _N,
          int IN_TENSOR_LENGTH,
          int SUB_TENSOR_LENGTH,
          int IN_VALID_LENGTH,
          int OUT_VALID_LENGTH,
          int VEC_FACTOR>
void nmsNonAA(adf::input_buffer<float, adf::extents<IN_TENSOR_LENGTH> >& __restrict input,
              adf::input_buffer<uint8_t, adf::extents<IN_VALID_LENGTH> >& __restrict input_flag,
              adf::output_buffer<uint8_t, adf::extents<OUT_VALID_LENGTH> >& __restrict output_flag) {
    rotateCoordImpl<IN_TENSOR_LENGTH, SUB_TENSOR_LENGTH, VEC_FACTOR>(input);
    uint8_t* restrict box_valid = (uint8_t*)::aie::begin(input_flag);
    uint8_t* restrict out_box_valid = (uint8_t*)::aie::begin(output_flag);
    for (auto ref_idx = 0u; ref_idx < (_N - 1); ref_idx++) { // 100-1
        if (box_valid[ref_idx] == 1) {
            initMem<_N, VEC_FACTOR>();
            normCoordImpl<_N, VEC_FACTOR>(ref_idx);
            findInnessImpl<_N, VEC_FACTOR>(norm_bf[0][0], norm_bf[0][1], norm_bf[1][0], norm_bf[1][1], ref_idx);
            findInnessImpl<_N, VEC_FACTOR>(norm_bf[1][0], norm_bf[1][1], norm_bf[0][0], norm_bf[0][1], ref_idx);
            findCrossImpl<_N, VEC_FACTOR>(norm_bf[0][0], norm_bf[0][1], norm_bf[1][0], norm_bf[1][1], ref_idx);
            sortVertexImpl<8>(ref_idx);
            AopAndIouImpl<_N, VEC_FACTOR>(ref_idx, box_valid);
        }
    }

    for (auto i = 0; i < _N; i += VEC_FACTOR)
        ::aie::store_unaligned_v(out_box_valid + i, ::aie::load_unaligned_v<VEC_FACTOR>(box_valid + i));
}

} // aie
} // cv
} // xf

#endif //__AIE_NMS_NON_AA_
