/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef _AIE_NMS_AA_H_
#define _AIE_NMS_AA_H_

namespace xf {
namespace cv {
namespace aie {

/**
 * ----------------------------------------------------------------------------
 * HLI iou function
 * ----------------------------------------------------------------------------
*/
// Maximum boxes, used for buffer allocs
static constexpr int _max_val = 128;
static constexpr int THRESH_SHIFT = 12;

alignas(::aie::vector_decl_align) uint8_t flag_E_ptr[_max_val];
alignas(::aie::vector_decl_align) uint8_t flag_S_ptr[_max_val];

template <typename T, unsigned N, unsigned NVE>
uint16 iou(const T* restrict in_ymin,
           const T* restrict in_xmin,
           const T* restrict in_ymax,
           const T* restrict in_xmax,
           T* restrict out_iou,
           T thresh,
           T tot_valid,
           T max_det) {
    const T* in_xmin_ptr = in_xmin;
    const T* in_ymin_ptr = in_ymin;
    const T* in_xmax_ptr = in_xmax;
    const T* in_ymax_ptr = in_ymax;

    // fill the flag data based on valids
    for (int i = 0; i < N; i += NVE) chess_prepare_for_pipelining {
            int fill_factor = 0;
            if ((i + NVE) > tot_valid) {
                fill_factor = (i + NVE) - tot_valid;
                fill_factor = fill_factor > NVE ? NVE : fill_factor;
            }
            ::aie::vector<uint8_t, NVE> flag_E = ::aie::shuffle_down_fill(
                ::aie::broadcast<uint8_t, NVE>(1), ::aie::broadcast<uint8_t, NVE>(0), fill_factor);
            ::aie::vector<uint8_t, NVE> flag_S = ::aie::broadcast<uint8_t, NVE>(0);

            ::aie::store_v(flag_E_ptr + i, flag_E);
            ::aie::store_v(flag_S_ptr + i, flag_S);
        }

    // compute IOU across all the boxes
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

        flag_S_ptr[i] = 1;
        bool bDontIgnore = flag_E_ptr[i];
        for (int j = 0; j < tot_valid; j += NVE) { // 16x samples per loop
            ::aie::vector<uint8_t, NVE> flag_E = ::aie::load_v<NVE>(flag_E_ptr + j);
            ::aie::vector<uint8_t, NVE> flag_S = ::aie::load_v<NVE>(flag_S_ptr + j);

            ::aie::mask<NVE> m_flag_E = ::aie::eq(flag_E, (uint8_t)1);
            ::aie::mask<NVE> m_flag_S = ::aie::eq(flag_S, (uint8_t)1);

            ::aie::vector<T, NVE> vec_xmin = ::aie::load_v<NVE>(in_xmin_ptr_loc); // Q0.9
            ::aie::vector<T, NVE> vec_ymin = ::aie::load_v<NVE>(in_ymin_ptr_loc); // Q0.9
            ::aie::vector<T, NVE> vec_xmax = ::aie::load_v<NVE>(in_xmax_ptr_loc); // Q0.9
            ::aie::vector<T, NVE> vec_ymax = ::aie::load_v<NVE>(in_ymax_ptr_loc); // Q0.9
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
            ::aie::store_v(flag_E_ptr + j, flag_E);
        }
    }

    int idx = 0;
    uint16 det_boxes = 0;
    for (int i = 0; i < tot_valid; i++) chess_prepare_for_pipelining {
            int incr = ((flag_E_ptr[i] == 1) && (i < max_det));
            det_boxes += incr;
            out_iou[idx] = in_ymin_ptr[i];
            out_iou[idx + 1] = in_xmin_ptr[i];
            out_iou[idx + 2] = in_ymax_ptr[i];
            out_iou[idx + 3] = in_xmax_ptr[i];
            idx += (4 * incr);
        }

    return det_boxes;
}

/**
 * -----------------------------------------------------------------------------------
 * NMS AA: (non-maximal supression axis-aligned)
 *   The boxes must be pre-arranged such that all the valids are followed with
 *   invalid data, and the number of valids must be provided by the total_valid_boxes
 *   parameter.
 * _ymin: ymin ('y' of top left co-ordinate) adf input buffer
 * _xmin: ymin ('x' of top left co-ordinate) adf input buffer
 * _ymax: ymax ('y' of bottom right co-ordinate) adf input buffer
 * _ymax: ymax ('x' of bottom right co-ordinate) adf input buffer
 * iou_thresh: IOU threshold in Q0.12 format
 * max_detections: Maximum detections, if the total valids exceeds this,
 *                 truncate the list to max det
 * total_valid_boxes: Total valid boxes in the list of all boxes.
 * -----------------------------------------------------------------------------------
*/

void nms_aa_api(adf::input_buffer<int16>& _ymin,
                adf::input_buffer<int16>& _xmin,
                adf::input_buffer<int16>& _ymax,
                adf::input_buffer<int16>& _xmax,
                adf::output_buffer<int16>& _out,
                const int16_t& iou_threshold,
                const int16_t& max_detections,
                const int16_t& total_valid_boxes) {
    int16* restrict _ymin_ptr = (int16*)::aie::begin(_ymin);
    int16* restrict _xmin_ptr = (int16*)::aie::begin(_xmin);
    int16* restrict _ymax_ptr = (int16*)::aie::begin(_ymax);
    int16* restrict _xmax_ptr = (int16*)::aie::begin(_xmax);
    int16* restrict _out_ptr = (int16*)::aie::begin(_out);

    constexpr int vec_fact = 32;
    uint16 detected_boxes = iou<int16, _max_val, vec_fact>(_ymin_ptr, _xmin_ptr, _ymax_ptr, _xmax_ptr, _out_ptr + 1,
                                                           iou_threshold, total_valid_boxes, max_detections);
    _out_ptr[0] = detected_boxes;
}

} // aie
} // cv
} // xf
#endif
