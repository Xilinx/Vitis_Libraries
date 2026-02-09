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
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <common/xf_aie_utils.hpp>
#include <stdio.h>

#include <algorithm>
#include <common/xf_aie_hw_utils.hpp>
#include "xf_filter2d_16b_aieml.hpp"

#ifndef _AIE_YUY2FILTER2D_AIEML_H_
#define _AIE_YUY2FILTER2D_AIEML_H_

#define PARALLEL_FACTOR_16b 16 // Parallelization factor for 16b operations (16x mults)
#define SRS_SHIFT 10           // SRS shift used can be increased if input data likewise adjusted)
alignas(::aie::vector_decl_align) int16_t y_buff[2048];
alignas(::aie::vector_decl_align) int16_t uv_buff[2048];
alignas(::aie::vector_decl_align) int16_t y_filtered_buff[2048];

namespace xf {
namespace cv {
namespace aie {

__attribute__((noinline)) void yuy2_filter2D(adf::input_buffer<int16_t>& img_in,
                                             const int16_t (&coeff)[16],
                                             adf::output_buffer<int16_t>& img_out) {
    ::aie::vector<int16_t, 8> v;
    v = ::aie::broadcast<int16_t, 8>(1);

    int16_t* restrict img_in_ptr = (int16_t*)::aie::begin(img_in);
    int16_t* restrict img_out_ptr = (int16_t*)::aie::begin(img_out);

    const int16_t image_width = xfGetTileWidth(img_in_ptr) * 2; // inclusive of channels
    const int16_t image_height = xfGetTileHeight(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    int16_t* in_img_ptr = (int16_t*)xfGetImgDataPtr(img_in_ptr);
    int16_t* out_img_ptr = (int16_t*)xfGetImgDataPtr(img_out_ptr);

    int16_t* restrict y_filtered_ptr = (int16_t*)y_filtered_buff;
    int16_t* restrict y_ptr = (int16_t*)y_buff;
    int16_t* restrict uv_ptr = (int16_t*)uv_buff;

    // unzip
    {
        for (int i = 0; i < image_height; i++) chess_unroll_loop(4) {
                int posn;
                for (int j = 0; j < image_width; j += 32) chess_prepare_for_pipelining {
                        posn = image_width * i + j; // posn in img
                        ::aie::vector<int16_t, 16> y_temp, uv_temp;
                        std::tie(y_temp, uv_temp) = ::aie::interleave_unzip(
                            ::aie::load_v<16>(in_img_ptr + posn), ::aie::load_v<16>(in_img_ptr + posn + 16), 1);
                        ::aie::store_v(y_ptr + (posn / 2), y_temp);
                        ::aie::store_v(uv_ptr + (posn / 2), uv_temp);
                    }
            }
    }

    // filter
    filter2D_aieml((int16_t*)y_ptr, coeff, (int16_t*)y_filtered_ptr, image_width / 2, image_width / 2, image_height);

    // zip
    {
        int N = 32;
        for (int i = 0; i < image_height; i++) chess_prepare_for_pipelining {
                for (int j = 0; j < image_width; j += 64) chess_prepare_for_pipelining {
                        int posn = image_width * i + j;
                        ::aie::vector<int16_t, 32> y_temp, uv_temp;
                        y_temp = ::aie::load_v<32>(y_filtered_ptr + posn / 2);
                        uv_temp = ::aie::load_v<32>(uv_ptr + posn / 2);
                        std::tie(y_temp, uv_temp) = ::aie::interleave_zip(y_temp, uv_temp, 1);

                        ::aie::store_v(out_img_ptr + posn, y_temp);
                        ::aie::store_v(out_img_ptr + posn + 32, uv_temp);
                    }
            }
    }

    // free(y_buff);
    // free(y_filtered_buff);
    // free(uv_buff);
    // std::cout<<"Freed memory."<<std::endl;
}
}
}
}
#endif
