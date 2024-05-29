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

#ifndef __XF_MASK_GEN_H__
#define __XF_MASK_GEN_H__

#include <adf.h>
#include <algorithm>
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>

#include <common/xf_aie_hw_utils.hpp>

#define __MAX_VAL_8__ 255

#define T uint8
#define T_OUT uint8
#define N 32

namespace xf {
namespace cv {
namespace aie {

class MaskGenTrack {
   public:
    static uint32_t non_zero_count;
    static uint32_t sum;

    void runImplTracking(T* restrict _pred_depth,
                         T* restrict _pred_seg,
                         const uint8 pred_seg_thresh,
                         const uint8 depth_min,
                         const uint8 depth_max,
                         const uint16 tile_height,
                         const uint16 tile_width);
};

uint32_t MaskGenTrack::non_zero_count = 0;
uint32_t MaskGenTrack::sum = 0;

__attribute__((noinline)) void MaskGenTrack::runImplTracking(T* restrict _pred_depth,
                                                             T* restrict _pred_seg,
                                                             const uint8 pred_seg_thresh,
                                                             const uint8 depth_min,
                                                             const uint8 depth_max,
                                                             const uint16 tile_height,
                                                             const uint16 tile_width) {
    T pd_min = depth_min;
    T pd_max = depth_max;
    T pd_max_min_diff = (pd_max - pd_min);

    ::aie::vector<uint16_t, N> zeros = ::aie::zeros<uint16_t, N>();
    ::aie::vector<uint16_t, N> ones = ::aie::broadcast<uint16_t, N>((T)1);
    ::aie::vector<uint16_t, N> non_zero_loc_v = zeros;      // updateable vec
    ::aie::accum<acc32, N> acc1 = ::aie::zeros<acc32, N>(); // updateable acc

    uint16 non_zero_loc = 0;
    uint32 sum_loc = 0;

    for (int i = 0; i < tile_width * tile_height; i += N) // 32x samples per loop
        chess_prepare_for_pipelining chess_loop_range(16, ) {
            ::aie::vector<T, N> vec = ::aie::load_v<N>(_pred_depth + i);
            ::aie::vector<T, N> tmp_v1 = ::aie::sub(vec, pd_min);
            ::aie::accum<acc32, N> acc0 = ::aie::mul(tmp_v1, (T)__MAX_VAL_8__);
            ::aie::vector<uint16_t, N> tmp_v2 = acc0.template to_vector<uint16_t>(0);

            ::aie::vector<T, N> vec_seg = ::aie::load_v<N>(_pred_seg + i);
            ::aie::mask<N> flag_pred_seg = ::aie::gt(vec_seg, (T)pred_seg_thresh);
            ::aie::vector<uint16_t, N> person_depth_mask = ::aie::select(zeros, tmp_v2, flag_pred_seg);
            ::aie::mask<N> mask_count_zero = ::aie::gt(person_depth_mask, (uint16_t)0);
            ::aie::vector<uint16_t, N> tmp_v3 = ::aie::select(zeros, ones, mask_count_zero);
            non_zero_loc_v = ::aie::add(non_zero_loc_v, tmp_v3);
            acc1 = ::aie::add(acc1, person_depth_mask);
        }
    non_zero_loc = ::aie::reduce_add(non_zero_loc_v);
    ::aie::vector<int32_t, N> sum_loc_v = acc1.template to_vector<int32_t>(0);
    sum_loc = ::aie::reduce_add(sum_loc_v);
    MaskGenTrack::sum += (sum_loc / pd_max_min_diff);
    MaskGenTrack::non_zero_count += non_zero_loc;
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
