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

#ifndef __XF_NMS_HPP__
#define __XF_NMS_HPP__

#include <adf.h>
#include <algorithm>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#define THRESH_SHIFT 12

namespace xf {
namespace cv {
namespace aie {

using uint8 = uint8_t;
using uint16 = uint16_t;

alignas(::aie::vector_decl_align) static uint8 xor_8[64] = {
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0};

alignas(::aie::vector_decl_align) static uint8 xor_4[64] = {
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};

alignas(::aie::vector_decl_align) static uint8 xor_2[64] = {
    0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0};

v32uint16 shuffle_xor(v32uint16 v, unsigned order) {
    v32uint16 orig;
    v64uint8 v_xor;

    if (order == 1)
        v_xor = *(v64uint8*)xor_2;
    else if (order == 2)
        v_xor = *(v64uint8*)xor_4;
    else if (order == 3)
        v_xor = *(v64uint8*)xor_8;
    else if (order == 4 || order == 5) {
        v_xor = *(v64uint8*)xor_8;
        orig = v;
        v = ::shiftx(v, v, 3, 48);
    }

    v32uint16 v2;

    if (order == 5) {
        auto acc = ::mul_2x8_8x8(v, v_xor);
        v2 = ::set_v32uint16(1, ::ussrs(acc, 0));
        v = ::shift(orig, orig, 32);
        v = ::shiftx(v, v, 3, 48);
        acc = ::mul_2x8_8x8(v, v_xor);
        v2 = ::insert(v2, 0, ::ussrs(acc, 0));
    } else if (order == 1) {
        v2 = ::shuffle(v, v, T16_1x2_flip);
    } else {
        auto acc = ::mul_2x8_8x8(v, v_xor);
        v2 = ::set_v32uint16(0, ::ussrs(acc, 0));

        if (order == 4) {
            v = ::shift(orig, orig, 32);
            v = ::shiftx(v, v, 3, 48);
        } else {
            v = ::shift(v, v, 32);
        }

        acc = ::mul_2x8_8x8(v, v_xor);
        v2 = ::insert(v2, 1, ::ussrs(acc, 0));
    }

    return v2;
}

std::pair<v32uint16, v32uint16> minmax_roll(v32uint16 a, v32uint16 b, unsigned rolls) {
    v32uint16 tmp1, tmp2, tmp3, tmp4;
    tmp1 = a;
    tmp2 = b;

    for (unsigned k = 0; k < rolls; ++k) {
        tmp3 = ::min(tmp1, tmp2);
        tmp4 = ::max(tmp1, tmp2);

        // e.g.: S4>1 D8 S2<1 --> S2>1 D8 S4<1
        tmp1 = ::shuffle(tmp3, tmp4, T16_2x32_lo);
        tmp2 = ::shuffle(tmp3, tmp4, T16_2x32_hi);
    }

    return std::make_pair(tmp1, tmp2);
}

std::pair<v32uint16, v32uint16> maxmin_roll(v32uint16 a, v32uint16 b, unsigned rolls) {
    v32uint16 tmp1, tmp2, tmp3, tmp4;
    tmp1 = a;
    tmp2 = b;

    for (unsigned k = 0; k < rolls; ++k) {
        tmp3 = ::max(tmp1, tmp2);
        tmp4 = ::min(tmp1, tmp2);

        // e.g.: S4>1 D8 S2<1 --> S2>1 D8 S4<1
        tmp1 = ::shuffle(tmp3, tmp4, T16_2x32_lo);
        tmp2 = ::shuffle(tmp3, tmp4, T16_2x32_hi);
    }

    return std::make_pair(tmp1, tmp2);
}

template <unsigned Stage>
__attribute__((always_inline)) std::pair<v32uint16, v32uint16> bitonic_sort_stage(v32uint16 v1, v32uint16 v2) {
    v32uint16 tmp1, tmp2, tmp3, tmp4;

    if
        constexpr(Stage == 0) {
            tmp1 = ::shuffle(v1, v2, DINTLV_lo_16o32);
            tmp2 = ::shuffle(v1, v2, DINTLV_hi_16o32);

            tmp3 = ::max(tmp1, tmp2);
            tmp4 = ::min(tmp1, tmp2);

            tmp1 = ::shuffle(tmp3, tmp4, INTLV_lo_16o32);
            tmp2 = ::shuffle(tmp3, tmp4, INTLV_hi_16o32);

            return std::make_pair(tmp1, tmp2);
        }
    else if
        constexpr(Stage >= 1 && Stage < 5) {
            const int modeA_lo[] = {T16_32x2_lo, T32_16x2_lo, T64_8x2_lo, T128_4x2_lo, T256_2x2_lo};
            const int modeA_hi[] = {T16_32x2_hi, T32_16x2_hi, T64_8x2_hi, T128_4x2_hi, T256_2x2_hi};
            const int modeB[] = {T512_1x2_lo, T16_16x2, T16_8x4, T16_4x8, T16_2x16};

            tmp1 = ::shuffle(v1, v2, modeA_lo[Stage]);
            tmp2 = ::shuffle(v1, v2, modeA_hi[Stage]);

            tmp2 = shuffle_xor(tmp2, Stage);

            tmp3 = ::max(tmp1, tmp2);
            tmp4 = ::min(tmp1, tmp2);

            tmp4 = shuffle_xor(tmp4, Stage);

            // e.g.: S2 D8 S4 --> S8 D8
            tmp3 = ::shuffle(tmp3, modeB[Stage]);
            tmp4 = ::shuffle(tmp4, modeB[Stage]);

            // e.g.: S8 D8 --> S4>1 D8 S2<1
            tmp1 = ::shuffle(tmp3, tmp4, T16_2x32_lo);
            tmp2 = ::shuffle(tmp3, tmp4, T16_2x32_hi);

            return maxmin_roll(tmp1, tmp2, Stage);
        }
    else if
        constexpr(Stage == 5) {
            tmp1 = v1;
            tmp2 = v2;

            tmp2 = shuffle_xor(tmp2, Stage);

            tmp3 = ::max(tmp1, tmp2);
            tmp4 = ::min(tmp1, tmp2);

            tmp4 = shuffle_xor(tmp4, Stage);

            // e.g.: S8 D8 --> S4>1 D8 S2<1
            tmp1 = ::shuffle(tmp3, tmp4, T16_2x32_lo);
            tmp2 = ::shuffle(tmp3, tmp4, T16_2x32_hi);

            return maxmin_roll(tmp1, tmp2, Stage);
        }
}

// Sort a single 64 element vector
auto test_bitonic_sort_v64(::aie::vector<uint16, 64> in) {
    auto[v1_sorted0, v2_sorted0] = bitonic_sort_stage<0>(in.template extract<32>(0), in.template extract<32>(1));
    auto[v1_sorted1, v2_sorted1] = bitonic_sort_stage<1>(v1_sorted0, v2_sorted0);
    auto[v1_sorted2, v2_sorted2] = bitonic_sort_stage<2>(v1_sorted1, v2_sorted1);
    auto[v1_sorted3, v2_sorted3] = bitonic_sort_stage<3>(v1_sorted2, v2_sorted2);
    auto[v1_sorted4, v2_sorted4] = bitonic_sort_stage<4>(v1_sorted3, v2_sorted3);
    auto[v1_sorted5, v2_sorted5] = bitonic_sort_stage<5>(v1_sorted4, v2_sorted4);

    return ::concat(v1_sorted5, v2_sorted5);
}

// Sort a single 32 element vector
// NOTE: this can be optimized by doing a 32 element only version of bitonic_sort_stage
::aie::vector<uint16, 32> test_bitonic_sort_v32(::aie::vector<uint16, 32> in) {
    ::aie::vector<uint16, 32> tmp;
    auto[v1_sorted0, v2_sorted0] = bitonic_sort_stage<0>(in, tmp);
    auto[v1_sorted1, v2_sorted1] = bitonic_sort_stage<1>(v1_sorted0, v2_sorted0);
    auto[v1_sorted2, v2_sorted2] = bitonic_sort_stage<2>(v1_sorted1, v2_sorted1);
    auto[v1_sorted3, v2_sorted3] = bitonic_sort_stage<3>(v1_sorted2, v2_sorted2);
    auto[v1_sorted4, v2_sorted4] = bitonic_sort_stage<4>(v1_sorted3, v2_sorted3);

    return v1_sorted4;
}

template <typename T, unsigned N, unsigned NVE>
uint16 iou(const T* restrict in_xmin,
           const T* restrict in_ymin,
           const T* restrict in_xmax,
           const T* restrict in_ymax,
           const T* restrict box_idx,
           T* restrict out_iou,
           T thresh,
           T tot_valid,
           T max_det) {
    const T* in_xmin_ptr = in_xmin;
    const T* in_ymin_ptr = in_ymin;
    const T* in_xmax_ptr = in_xmax;
    const T* in_ymax_ptr = in_ymax;

    ::aie::vector<uint8_t, N> flag_E =
        ::aie::shuffle_down_fill(::aie::broadcast<uint8_t, N>(1), ::aie::broadcast<uint8_t, N>(0), (N - tot_valid));
    ::aie::vector<uint8_t, N> flag_S = ::aie::broadcast<uint8_t, N>(0);

    for (int i = 0; i < tot_valid; i++) {
        T in_ref_xmin = in_xmin_ptr[i];
        T in_ref_ymin = in_ymin_ptr[i];
        T in_ref_xmax = in_xmax_ptr[i];
        T in_ref_ymax = in_ymax_ptr[i];

        const T* in_xmin_ptr_loc = in_xmin;
        const T* in_ymin_ptr_loc = in_ymin;
        const T* in_xmax_ptr_loc = in_xmax;
        const T* in_ymax_ptr_loc = in_ymax;

        int16_t tmp1 = in_ref_ymax - in_ref_ymin;
        int16_t tmp2 = in_ref_xmax - in_ref_xmin;
        int32_t area_i = tmp1 * tmp2;

        flag_S[i] = 1;
        bool bDontIgnore = flag_E[i];
        for (int j = 0; j < tot_valid; j += NVE) { // 16x samples per loop
            ::aie::mask<NVE> m_flag_E = ::aie::eq(flag_E, (uint8_t)1);
            ::aie::mask<NVE> m_flag_S = ::aie::eq(flag_S, (uint8_t)1);

            ::aie::vector<T, NVE> vec_xmin = ::aie::load_unaligned_v<NVE>(in_xmin_ptr_loc); // Q0.9
            ::aie::vector<T, NVE> vec_ymin = ::aie::load_unaligned_v<NVE>(in_ymin_ptr_loc); // Q0.9
            ::aie::vector<T, NVE> vec_xmax = ::aie::load_unaligned_v<NVE>(in_xmax_ptr_loc); // Q0.9
            ::aie::vector<T, NVE> vec_ymax = ::aie::load_unaligned_v<NVE>(in_ymax_ptr_loc); // Q0.9
            in_xmin_ptr_loc += NVE;
            in_ymin_ptr_loc += NVE;
            in_xmax_ptr_loc += NVE;
            in_ymax_ptr_loc += NVE;

            ::aie::vector<int16_t, NVE> tmp3 = ::aie::vector_cast<int16_t>(::aie::sub(vec_ymax, vec_ymin)); // Q0.9
            ::aie::vector<int16_t, NVE> tmp4 = ::aie::vector_cast<int16_t>(::aie::sub(vec_xmax, vec_xmin)); // Q0.9

            ::aie::accum<acc32, NVE> area_j = ::aie::mul(tmp3, tmp4); // Q0.18

            ::aie::vector<T, NVE> intersec_xmin = ::aie::max(in_ref_xmin, vec_xmin); // Q0.9
            ::aie::vector<T, NVE> intersec_ymin = ::aie::max(in_ref_ymin, vec_ymin); // Q0.9
            ::aie::vector<T, NVE> intersec_xmax = ::aie::min(in_ref_xmax, vec_xmax); // Q0.9
            ::aie::vector<T, NVE> intersec_ymax = ::aie::min(in_ref_ymax, vec_ymax); // Q0.9

            ::aie::vector<int16_t, NVE> tmp5 =
                ::aie::vector_cast<int16_t>(::aie::sub(intersec_xmax, intersec_xmin)); // Q0.9
            ::aie::vector<int16_t, NVE> tmp6 =
                ::aie::vector_cast<int16_t>(::aie::sub(intersec_ymax, intersec_ymin)); // Q0.9

            ::aie::vector<int16_t, NVE> v_zero = ::aie::zeros<int16_t, NVE>();

            ::aie::vector<T, NVE> tmp7 = ::aie::vector_cast<T>(::aie::max(v_zero, tmp5)); // Q0.9
            ::aie::vector<T, NVE> tmp8 = ::aie::vector_cast<T>(::aie::max(v_zero, tmp6)); // Q0.9

            ::aie::accum<acc32, NVE> intersec_area = ::aie::mul(tmp7, tmp8); // Q0.18

            ::aie::vector<int32_t, NVE> area_j_vec = area_j.template to_vector<int32_t>(0);
            ::aie::vector<int32_t, NVE> tmp9 = ::aie::add(area_j_vec, area_i); // Q0.18

            ::aie::vector<int32_t, NVE> intersec_area_vec = intersec_area.template to_vector<int32_t>(0); // Q0.18
            ::aie::vector<int32_t, NVE> tmp10 = ::aie::sub(tmp9, intersec_area_vec);                      // Q0.18
            auto thresh_new = ::aie::mul(thresh, tmp10); // thresh Q0.12, tmp10 Q0.18, thresh_new Q0.30

            ::aie::mask<NVE> m_flag_C =
                ::aie::lt(intersec_area_vec, (thresh_new.template to_vector<int32_t>(THRESH_SHIFT)));

            ::aie::mask<NVE> m_flag_CS = m_flag_C | m_flag_S;
            ::aie::mask<NVE> m_flag_E_store = m_flag_E & m_flag_CS;

            flag_E = bDontIgnore ? ::aie::select((uint8_t)0, (uint8_t)1, m_flag_E_store) : flag_E;
        }
    }

    uint16 det_boxes = 0;
    for (int i = 0; i < tot_valid; i++) chess_prepare_for_pipelining {
            int incr = ((flag_E[i] == 1) && (i < max_det));
            out_iou[det_boxes] = box_idx[i];
            det_boxes += incr;
        }

    return det_boxes;
}

template <int N>
class NMSBaseImpl {
    static constexpr int tot = 32; // setting the MAX_VALID to 32

   public:
    void runImpl(int16_t* box_dec,
                 uint16_t* score_dec,
                 uint16_t* out_boxes,
                 const uint16_t& iou_threshold,
                 const uint16_t& max_detections,
                 const uint16_t& score_threshold);

    void xf_validate(int16_t* box_dec,
                     uint16_t* score_dec,
                     int16_t* box_v,
                     int16_t* score_v,
                     const uint16_t& score_threshold,
                     const uint16_t& max_detections);
};

template <int N>
void NMSBaseImpl<N>::runImpl(int16_t* box_dec,
                             uint16_t* score_dec,
                             uint16_t* out_boxes,
                             const uint16_t& iou_threshold,
                             const uint16_t& max_detections,
                             const uint16_t& score_threshold) {
    alignas(::aie::vector_decl_align) int16_t score_vd[32];
    alignas(::aie::vector_decl_align) int16_t box_vd[32 * 5];

    int16_t* score_v = (int16_t*)score_vd;
    int16_t* box_v = (int16_t*)box_vd;

    // init score array to zero
    ::aie::vector<int16_t, tot> tmp = ::aie::zeros<int16_t, tot>();
    ::aie::store_v(score_v, tmp);

    // Decode box
    xf_validate(box_dec, score_dec, box_v, score_v, score_threshold, max_detections);
    // SORT 32-element
    ::aie::vector<uint16, tot> in_v32;
    for (unsigned i = 0; i < tot; ++i) chess_prepare_for_pipelining chess_loop_range(8, ) {
            in_v32[i] = (uint16)score_v[i];
        }

    ::aie::vector<uint16, tot> res32;
    res32 = test_bitonic_sort_v32(in_v32);

    // re-arrange
    ::aie::vector<uint8_t, tot* 2> split_v = ::aie::vector_cast<uint8>(res32);

    uint16_t in_xmin[tot], in_ymin[tot], in_xmax[tot], in_ymax[tot], box_idx[tot];

    uint16 total_valid_boxes = 0;
    for (unsigned i = 0; i < tot; ++i) chess_prepare_for_pipelining chess_loop_range(8, ) {
            int idx = (split_v[i * 2]);

            int idx_2 = idx * 5;
            in_ymin[i] = (uint16)box_v[idx_2];
            in_xmin[i] = (uint16)box_v[idx_2 + 1];
            in_ymax[i] = (uint16)box_v[idx_2 + 2];
            in_xmax[i] = (uint16)box_v[idx_2 + 3];
            box_idx[i] = (uint16)box_v[idx_2 + 4];

            total_valid_boxes += (split_v[(i * 2) + 1] > 0);
        }

    // IOU
    constexpr int vec_fact = 32;
    uint16 detected_boxes = iou<uint16, tot, vec_fact>(in_xmin, in_ymin, in_xmax, in_ymax, box_idx, out_boxes + 1,
                                                       iou_threshold, total_valid_boxes, max_detections);
    out_boxes[0] = detected_boxes;

    return;
}

template <int N>
__attribute__((noinline)) void NMSBaseImpl<N>::xf_validate(int16_t* box_dec,
                                                           uint16_t* score_dec,
                                                           int16_t* box_v,
                                                           int16_t* score_v,
                                                           const uint16_t& score_threshold,
                                                           const uint16_t& max_detections)

{
    ::aie::vector<int16_t, 32> ymin, ymax, xmin, xmax;
    ::aie::vector<uint16_t, 32> scores;
    int16_t ctr = 0;
    uint16_t valid_decode = 0;
    uint16_t temp_11;
    int16_t min_val = 0;
    int16_t max_val = 512;
    uint16_t idx = 0;
    int iter = 0;

    for (int i = 0; i < 47; i++) chess_prepare_for_pipelining {
            for (int jj = 0; jj < 32; jj++) {
                xmin[jj] = box_dec[idx];
                ymin[jj] = box_dec[idx + 1];
                xmax[jj] = box_dec[idx + 2];
                ymax[jj] = box_dec[idx + 3];
                idx += 4;
            }

            // SCORE IMPLEMENTATION
            scores = ::aie::load_unaligned_v<(N)>(score_dec);
            score_dec += (N);

            // scores_in > threshold
            auto bcond = ::aie::ge(ymin, min_val) & ::aie::le(ymin, max_val) & ::aie::ge(ymax, min_val) &
                         ::aie::le(ymax, max_val) & ::aie::ge(xmin, min_val) & ::aie::le(xmin, max_val) &
                         ::aie::ge(xmax, min_val) & ::aie::le(xmax, max_val) & ::aie::ge(scores, score_threshold);

            ::aie::vector<uint16_t, 32> incr = ::aie::select((uint16_t)0, (uint16_t)1, bcond);

            for (int j1 = 0; j1 < 32; j1++) {
                temp_11 = incr[j1];

                if ((temp_11 == 1) && valid_decode < max_detections) {
                    box_v[valid_decode * 5 + 0] = ymin[j1];
                    box_v[valid_decode * 5 + 1] = xmin[j1];
                    box_v[valid_decode * 5 + 2] = ymax[j1];
                    box_v[valid_decode * 5 + 3] = xmax[j1];
                    if (i < 46) {
                        box_v[valid_decode * 5 + 4] = i * 32 + j1;
                    } else {
                        box_v[valid_decode * 5 + 4] = i * 32 + j1 - 1;
                    }
                    score_v[valid_decode] = (uint16_t(scores[j1]) << 8) + valid_decode;
                    valid_decode += 1;
                    ctr += (i == 45 && j1 > 0);
                }
                iter++;
            }

            if (i == 45) {
                idx = idx - 4;
                score_dec = score_dec - 1;
                score_v = score_v - ctr;
                box_v = box_v - (4 * ctr);
                valid_decode = valid_decode - ctr;
                iter = iter - 1;
            }
        }
    score_dec = score_dec + 25;

    return;
}

} // aie
} // cv
} // xf
#endif
