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

#ifndef __XF_PIXELWISE_SELECT_
#define __XF_PIXELWISE_SELECT_

#include <adf.h>
#include <algorithm>
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>

#include <common/xf_aie_hw_utils.hpp>

namespace xf {
namespace cv {
namespace aie {

class PixelwiseSelect {
   public:
    void runImpl(adf::input_buffer<uint8_t>& frame,
                 adf::input_buffer<uint8_t>& mask,
                 adf::output_buffer<uint8_t>& output);
    void runImpl(adf::input_buffer<uint8_t>& in_frame,
                 adf::input_buffer<uint8_t>& mask,
                 adf::input_buffer<uint8_t>& bg_frame,
                 adf::output_buffer<uint8_t>& output);
    void xf_pixel_wise_select(uint8_t* frame, uint8_t* mask, int16 height, int16 width, uint8_t* output);
    void xf_pixel_wise_select(
        uint8_t* in_frame, uint8_t* mask, uint8_t* bg_frame, int16 height, int16 width, uint8_t* output);
};

__attribute__((noinline)) void PixelwiseSelect::xf_pixel_wise_select(
    uint8_t* frame, uint8_t* mask, int16 height, int16 width, uint8_t* output) {
    const int16 image_width = width;
    const int16 image_height = height;

    uint8_t* restrict _frame = (uint8_t*)(frame);
    uint8_t* restrict _mask = (uint8_t*)(mask);
    uint8_t* restrict _output = (uint8_t*)(output);
    int16_t num_vectors = image_width >> 5;

    ::aie::vector<uint8_t, 32> vec_x;
    ::aie::vector<uint8_t, 32> vec_x1;
    ::aie::vector<uint8_t, 32> ones = ::aie::broadcast<uint8, 32>(1);
    ::aie::vector<uint8_t, 32> t1;

    ::aie::accum<acc32, 32> acc_x;

    for (int i = 0; i < image_height * num_vectors; i++) chess_prepare_for_pipelining chess_loop_range(1, ) {
            vec_x = ::aie::load_v<32>(_frame);
            vec_x1 = ::aie::load_v<32>(_mask);
            acc_x = ::aie::mul(vec_x, vec_x1);
            ::aie::store_v(_output, acc_x.template to_vector<uint8>(0));
            _frame += 32;
            _mask += 32;
            _output += 32;
        }
}

__attribute__((noinline)) void PixelwiseSelect::xf_pixel_wise_select(
    uint8_t* in_frame, uint8_t* mask, uint8_t* bg_frame, int16 height, int16 width, uint8_t* output) {
    const int16 image_width = width;
    const int16 image_height = height;

    uint8_t* restrict _in_frame = (uint8_t*)(in_frame);
    uint8_t* restrict _bg_frame = (uint8_t*)(bg_frame);
    uint8_t* restrict _mask = (uint8_t*)(mask);
    uint8_t* restrict _output = (uint8_t*)(output);
    int16_t num_vectors = image_width >> 5;

    ::aie::vector<uint8_t, 32> vec_in;
    ::aie::vector<uint8_t, 32> vec_bg;
    ::aie::vector<uint8_t, 32> vec_m;
    ::aie::vector<uint8_t, 32> vec_out;

    for (int i = 0; i < image_height * num_vectors; i++) chess_prepare_for_pipelining chess_loop_range(1, ) {
            vec_in = ::aie::load_v<32>(_in_frame);
            vec_bg = ::aie::load_v<32>(_bg_frame);
            vec_m = ::aie::load_v<32>(_mask);
            auto mask_val = ::aie::gt(vec_m, (uint8_t)0);
            vec_out = ::aie::select(vec_bg, vec_in, mask_val);
            ::aie::store_v(_output, vec_out);
            _in_frame += 32;
            _bg_frame += 32;
            _mask += 32;
            _output += 32;
        }
}

void PixelwiseSelect::runImpl(adf::input_buffer<uint8_t>& frame,
                              adf::input_buffer<uint8_t>& mask,
                              adf::output_buffer<uint8_t>& output) {
    uint8_t* f = (uint8_t*)::aie::begin(frame);
    uint8_t* m = (uint8_t*)::aie::begin(mask);
    uint8_t* o = (uint8_t*)::aie::begin(output);

    int height = xfGetTileHeight(f);
    int width = xfGetTileWidth(f);

    xfCopyMetaData(f, o);

    uint8_t* f_ptr = (uint8_t*)xfGetImgDataPtr(f);
    uint8_t* m_ptr = (uint8_t*)xfGetImgDataPtr(m);
    uint8_t* o_ptr = (uint8_t*)xfGetImgDataPtr(o);

    ::aie::vector<int16, 16> vv = ::aie::broadcast<int16, 16>(width);
    ::aie::print(vv, true, "width:");

    vv = ::aie::broadcast<int16, 16>(height);
    ::aie::print(vv, true, "height:");
    xf_pixel_wise_select(f_ptr, m_ptr, height, width, o_ptr);
}

void PixelwiseSelect::runImpl(adf::input_buffer<uint8_t>& in_frame,
                              adf::input_buffer<uint8_t>& mask,
                              adf::input_buffer<uint8_t>& bg_frame,
                              adf::output_buffer<uint8_t>& output) {
    uint8_t* f = (uint8_t*)::aie::begin(in_frame);
    uint8_t* m = (uint8_t*)::aie::begin(mask);
    uint8_t* b = (uint8_t*)::aie::begin(bg_frame);
    uint8_t* o = (uint8_t*)::aie::begin(output);

    int height = xfGetTileHeight(f);
    int width = xfGetTileWidth(f);

    xfCopyMetaData(f, o);

    uint8_t* f_ptr = (uint8_t*)xfGetImgDataPtr(f);
    uint8_t* m_ptr = (uint8_t*)xfGetImgDataPtr(m);
    uint8_t* b_ptr = (uint8_t*)xfGetImgDataPtr(b);
    uint8_t* o_ptr = (uint8_t*)xfGetImgDataPtr(o);

    xf_pixel_wise_select(f_ptr, m_ptr, b_ptr, height, width, o_ptr);
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif