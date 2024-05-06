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

#ifndef _AIE_ACCUMULATEWEIGHTED_AIE_8_16_H_
#define _AIE_ACCUMULATEWEIGHTED_AIE_8_16__H_

#define PARALLEL_FACTOR_16b 16

#define SRS_SHIFT 8

namespace xf {
namespace cv {
namespace aie {

/**
 * ----------------------------------------------------------------------------
 * HLI accumulate weighted
 * ----------------------------------------------------------------------------
*/
template <typename IN_T, typename T, unsigned int N>
__attribute__((noinline)) void accumulateweighted(const IN_T* restrict img_in1,
                                                  const IN_T* restrict img_in2,
                                                  T* restrict img_out,
                                                  const T& img_width,
                                                  const T& img_height,
                                                  float& scale) {
    //    int16_t fix_scale=float2fix(scale,SRS_SHIFT) ;//(float)scale*(1<<8);
    //    int16_t fix_scale1=float2fix((1-scale),SRS_SHIFT) ;// (1-scale)*(1<<8);
    int16_t fix_scale = ::aie::to_fixed<int16_t>(scale, SRS_SHIFT);        //(float)scale*(1<<8);
    int16_t fix_scale1 = ::aie::to_fixed<int16_t>((1 - scale), SRS_SHIFT); //(float)scale*(1<<8);

    ::aie::vector<int16_t, N> weight(fix_scale, fix_scale1);

    for (int j = 0; j < (img_height * img_width); j += N) // 16x samples per loop
        chess_prepare_for_pipelining chess_loop_range(14, ) {
            ::aie::vector<IN_T, N> data_buf1 = ::aie::load_v<N>(img_in1);
            img_in1 += N;
            ::aie::vector<IN_T, N> data_buf2 = ::aie::load_v<N>(img_in2);
            img_in2 += N;

            ::aie::accum<acc32, N> acc =
                ::aie::accumulate<N>(weight, data_buf1, data_buf2); // weight[0] * data_buf1 + weight[1] * data_buf2

            ::aie::store_v(img_out, acc.template to_vector<T>(SRS_SHIFT));

            img_out += N;
        }
}

/**
 * ----------------------------------------------------------------------------
 * 16-bit Accumulate
 * ----------------------------------------------------------------------------
 */
template <typename IN_T, typename T, unsigned int N>
__attribute__((noinline)) void accumulateweighted_api(adf::input_buffer<IN_T>& img_in1,
                                                      adf::input_buffer<IN_T>& img_in2,
                                                      adf::output_buffer<T>& img_out,
                                                      float& alpha) {
    IN_T* restrict img_in_ptr = (IN_T*)::aie::begin(img_in1);
    IN_T* restrict img_in_ptr1 = (IN_T*)::aie::begin(img_in2);
    T* restrict img_out_ptr = (T*)::aie::begin(img_out);

    const int16_t img_width = xfGetTileWidth(img_in_ptr);
    const int16_t img_height = xfGetTileHeight(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);
    xfUnsignedSaturation(img_out_ptr);

    IN_T* restrict ptr0 = (IN_T*)xfGetImgDataPtr(img_in_ptr);
    IN_T* restrict ptr1 = (IN_T*)xfGetImgDataPtr(img_in_ptr1);
    T* restrict ptr_out = (T*)xfGetImgDataPtr(img_out_ptr);

    accumulateweighted<IN_T, T, PARALLEL_FACTOR_16b>(ptr0, ptr1, ptr_out, img_width, img_height, alpha);
}

} // aie
} // cv
} // xf
#endif
