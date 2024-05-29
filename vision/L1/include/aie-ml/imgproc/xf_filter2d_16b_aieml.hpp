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
#include <algorithm>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>

#define VECTORIZATION_FACTOR 16 // Parallelization factor for 16b operations (16x mults)
#define SRS_SHIFT 10            // SRS shift used can be increased if input data likewise adjusted)

namespace xf {
namespace cv {
namespace aie {

/**
 * 16-bit filter2D (3x3) with border effect handling
coeff arangement: ko, k1, k2, 0, k3, k4, k5, 0, k6, k7, k8, 0
 */
__attribute__((noinline)) void filter2D_aieml(int16_t* restrict _input,
                                              const int16_t (&coeff)[16],
                                              int16_t* restrict ptr_img_out,
                                              const int16_t img_width,
                                              const int16_t stride,
                                              const int16_t img_height) {
    ::aie::vector<int16_t, 32> row_buf0, row_buf1, row_buf2;
    ::aie::vector<int16_t, 32> temp;
    int16_t* coeff_ptr = (int16_t*)(coeff);
    ::aie::vector<int16_t, 16> vcoeff = *(v16int16*)(&coeff[0]);
    ::aie::accum<acc64, 16> acc;

    int16_t* _input_r[3];
    int16_t* out = (int16_t*)ptr_img_out;
    for (int _res_s0_y = 0; _res_s0_y < img_height; _res_s0_y++) {
        // chess_report(_res_s0_y);

        _input_r[0] = _input + std::max((_res_s0_y - 1), 0) * img_width;
        _input_r[1] = _input + (_res_s0_y)*img_width;
        _input_r[2] = _input + std::min((_res_s0_y + 1), (img_height - 1)) * img_width;
        out = ptr_img_out + (_res_s0_y)*img_width;

        //@Left border {
        {
            row_buf0 = ::aie::load_v<32>(_input_r[0]);
            temp = ::aie::shuffle_up_replicate(row_buf0, 1);
            acc = ::aie::sliding_mul<16, 4, 1, 1, 1, acc64>(vcoeff, 0, temp, 0);
            row_buf1 = ::aie::load_v<32>(_input_r[1]);
            temp = ::aie::shuffle_up_replicate(row_buf1, 1);
            acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 4, temp, 0);
            row_buf2 = ::aie::load_v<32>(_input_r[2]);
            temp = ::aie::shuffle_up_replicate(row_buf2, 1);
            acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 8, temp, 0);
            ::aie::store_v(out, acc.to_vector<int16_t>(10));

            _input_r[0] += VECTORIZATION_FACTOR;
            _input_r[1] += VECTORIZATION_FACTOR;
            _input_r[2] += VECTORIZATION_FACTOR;
            out += VECTORIZATION_FACTOR;
        }
        //@}

        //@Middle reigon {
        {
            for (int _res_s0_x = VECTORIZATION_FACTOR; _res_s0_x < (img_width - VECTORIZATION_FACTOR);
                 _res_s0_x += VECTORIZATION_FACTOR)
                chess_prepare_for_pipelining {
                    acc = ::aie::zeros<acc64, 16>();
                    // y-1
                    row_buf0 = ::aie::load_v<32>(_input_r[0]);
                    temp = ::aie::shuffle_up(row_buf0, 1);
                    temp[0] = *(_input_r[0] - 1);
                    acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 0, temp, 0);

                    // y
                    row_buf1 = ::aie::load_v<32>(_input_r[1]);
                    temp = ::aie::shuffle_up(row_buf1, 1);
                    temp[0] = *(_input_r[1] - 1);
                    acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 4, temp, 0);

                    // y+1
                    row_buf2 = ::aie::load_v<32>(_input_r[2]);
                    temp = ::aie::shuffle_up(row_buf2, 1);
                    temp[0] = *(_input_r[2] - 1);
                    acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 8, temp, 0);

                    ::aie::store_v(out, acc.to_vector<int16_t>(10));

                    _input_r[0] += VECTORIZATION_FACTOR;
                    _input_r[1] += VECTORIZATION_FACTOR;
                    _input_r[2] += VECTORIZATION_FACTOR;

                    out += VECTORIZATION_FACTOR;
                }
        }
        //@}

        //@Right border {
        {
            acc = ::aie::zeros<acc64, 16>();
            row_buf0 = ::aie::load_v<32>(_input_r[0] - VECTORIZATION_FACTOR);
            temp = ::aie::shuffle_down_replicate(row_buf0, 15);
            acc = ::aie::sliding_mul<16, 4, 1, 1, 1, acc64>(vcoeff, 0, temp, 0);
            row_buf1 = ::aie::load_v<32>(_input_r[1] - VECTORIZATION_FACTOR);
            temp = ::aie::shuffle_down_replicate(row_buf1, 15);
            acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 4, temp, 0);
            row_buf2 = ::aie::load_v<32>(_input_r[2] - VECTORIZATION_FACTOR);
            temp = ::aie::shuffle_down_replicate(row_buf2, 15);
            acc = ::aie::sliding_mac<16, 4, 1, 1, 1>(acc, vcoeff, 8, temp, 0);
            ::aie::store_v(out, acc.to_vector<int16_t>(10));
        }
        //@}
    } // for _res_s0_y
}

__attribute__((noinline)) void filter2D(adf::input_buffer<int16_t>& img_in,
                                        const int16_t (&coeff)[16],
                                        adf::output_buffer<int16_t>& img_out) {
    int16_t* restrict img_in_ptr = (int16_t*)::aie::begin(img_in);
    int16_t* restrict img_out_ptr = (int16_t*)::aie::begin(img_out);

    const int16_t image_width = xfGetTileWidth(img_in_ptr);
    const int16_t image_height = xfGetTileHeight(img_in_ptr);
    const int16_t stride = image_width;

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    int16_t* restrict ptr_img_buffer = (int16_t*)xfGetImgDataPtr(img_in_ptr);
    int16_t* restrict ptr_img_out = (int16_t*)xfGetImgDataPtr(img_out_ptr);

    filter2D_aieml(ptr_img_buffer, coeff, ptr_img_out, image_width, stride, image_height);
}

} // aie
} // cv
} // xf
