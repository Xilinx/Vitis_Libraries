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

class MaskGen {
   public:
    static uint32_t non_zero_count;
    static uint32_t sum;

    void runImplMaskGen(T* restrict _pred_depth,
                        T_OUT* restrict _mask_for_editing,
                        const T depth_min,
                        const T depth_max,
                        const uint16 thresh_f_new,
                        const uint16 thresh_b_new,
                        const uint16 tile_height,
                        const uint16 tile_width);

    void MaskGenUtility(T* restrict _pred_depth,
                        T_OUT* restrict _mask_for_edit,
                        T pd_min,
                        uint16_t thresh_f_new,
                        uint16_t thresh_b_new,
                        ::aie::vector<T, N> zeros,
                        ::aie::vector<T, N> ones,
                        int i);
};

void MaskGen::MaskGenUtility(T* restrict _pred_depth,
                             T_OUT* restrict _mask_for_edit,
                             T pd_min,
                             uint16_t thresh_f_new,
                             uint16_t thresh_b_new,
                             ::aie::vector<T, N> zeros,
                             ::aie::vector<T, N> ones,
                             int i) {
    ::aie::vector<T, N> vec = ::aie::load_v<N>(_pred_depth + i);
    ::aie::vector<T, N> tmp_v1 = ::aie::sub(vec, pd_min);
    ::aie::accum<acc32, N> acc0 = ::aie::mul(tmp_v1, (T)__MAX_VAL_8__);
    ::aie::vector<uint16_t, N> tmp_v2 = acc0.template to_vector<uint16_t>(0);
    ::aie::mask<N> f_flag = ::aie::gt(tmp_v2, thresh_f_new);
    ::aie::mask<N> b_flag = ::aie::lt(tmp_v2, thresh_b_new);
    ::aie::mask<N> mask_flag = f_flag | b_flag;
    ::aie::vector<T, N> v1 = ::aie::select(zeros, ones, mask_flag);

    // 1 channel - writing out 1 channel
    ::aie::store_v(_mask_for_edit + i, v1);
}

__attribute__((noinline)) void MaskGen::runImplMaskGen(T* restrict _pred_depth,
                                                       T_OUT* restrict _mask_for_editing,
                                                       const T depth_min,
                                                       const T depth_max,
                                                       const uint16 thresh_f_new,
                                                       const uint16 thresh_b_new,
                                                       const uint16 tile_height,
                                                       const uint16 tile_width) {
    T pd_min = depth_min;
    T pd_max = depth_max;

    ::aie::vector<T, N> zeros = ::aie::zeros<T, N>();
    ::aie::vector<T, N> ones = ::aie::broadcast<T, N>((T)1);

    for (int i = 0; i < tile_width * tile_height; i += N) // 32x samples per loop
        chess_prepare_for_pipelining chess_loop_range(16, ) {
            MaskGenUtility(_pred_depth, _mask_for_editing, pd_min, thresh_f_new, thresh_b_new, zeros, ones, i);
        }
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
