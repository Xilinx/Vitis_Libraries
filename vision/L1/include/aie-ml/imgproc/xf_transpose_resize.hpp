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

class Transpose1D {
   public:
    void runImpl(uint8_t* frame, int16 height, int16 width, uint8_t* output);
    void runImpl(adf::input_buffer<uint8_t>& input1,
                 adf::input_buffer<uint8_t>& input2,
                 adf::output_buffer<uint8_t>& output,
                 int outputStride,
                 int channels);
    void xf_transpose_1ch(uint8_t* frame, int16 height, int16 width, uint8_t* output);
    void xf_transpose_4ch(uint8_t* frame, int16 height, int16 width, uint8_t* output);
};

__attribute__((noinline)) void Transpose1D::xf_transpose_1ch(uint8_t* in_ptr,
                                                             int16 height,
                                                             int16 width,
                                                             uint8_t* out_ptr) {
    const int16 image_width = width;
    const int16 image_height = height;

    for (int wd = 0; wd < image_width / 4; wd++) chess_prepare_for_pipelining {
            ::aie::vector<uint8_t, 16> VEC0; // size of image height - ideally
            ::aie::vector<uint8_t, 16> VEC1;
            ::aie::vector<uint8_t, 16> VEC2;
            ::aie::vector<uint8_t, 16> VEC3;
            for (int ht = 0; ht < image_height / 16; ht++) chess_prepare_for_pipelining {
                    auto vec0 = ::aie::load_v<32>(in_ptr);
                    in_ptr += 32;
                    auto vec1 = ::aie::load_v<32>(in_ptr);
                    in_ptr += 32;
                    auto[v_01, v_23] = ::aie::interleave_unzip(vec0, vec1, 2);
                    VEC0.insert(ht, ::aie::filter_even(v_01, 1));
                    VEC1.insert(ht, ::aie::filter_odd(v_01, 1));
                    VEC2.insert(ht, ::aie::filter_even(v_23, 1));
                    VEC3.insert(ht, ::aie::filter_odd(v_23, 1));
                }
            ::aie::store_v(out_ptr, VEC0);
            out_ptr += image_height;
            ::aie::store_v(out_ptr, VEC1);
            out_ptr += image_height;
            ::aie::store_v(out_ptr, VEC2);
            out_ptr += image_height;
            ::aie::store_v(out_ptr, VEC3);
            out_ptr += image_height;
        }
}

__attribute__((noinline)) void Transpose1D::xf_transpose_4ch(uint8_t* in_ptr,
                                                             int16 height,
                                                             int16 width,
                                                             uint8_t* out_ptr) {
    const int16 image_width = width;
    const int16 image_height = height;

    for (int wd = 0; wd < image_width * image_height * 4 / 32; wd++) chess_prepare_for_pipelining {
            auto vec0 = ::aie::load_v<32>(in_ptr);
            in_ptr += 32;
            ::aie::store_v(out_ptr, vec0);
            out_ptr += 32;
        }
}

void Transpose1D::runImpl(uint8_t* frame, int16 height, int16 width, uint8_t* output) {
    xf_transpose_1ch(frame, height, width, output);
}

void Transpose1D::runImpl(adf::input_buffer<uint8_t>& input_metadata,
                          adf::input_buffer<uint8_t>& input_data,
                          adf::output_buffer<uint8_t>& output,
                          int outputStride,
                          int channels) {
    uint8_t* in_meta_ptr = (uint8_t*)::aie::begin(input_metadata);
    uint8_t* in_data_ptr = (uint8_t*)::aie::begin(input_data);

    uint8_t* out_ptr = (uint8_t*)::aie::begin(output);

    int height = xfGetTileOutTHeight(in_meta_ptr);
    int width = xfGetTileOutTWidth(in_meta_ptr);

    // printf("height=%d width=%d\n", height, width);

    xfCopyMetaData(in_meta_ptr, out_ptr);

    uint16_t mt_height = width; // xfGetTileWidth(out_ptr);
    uint16_t mt_width = height; // xfGetTileHeight(out_ptr);

    xfSetTileWidth(out_ptr, mt_width);
    xfSetTileHeight(out_ptr, mt_height);
    xfSetTileOutTWidth(out_ptr, mt_width);
    xfSetTileOutTHeight(out_ptr, mt_height);
    //    printf("xfGetTileOutPosV(out_ptr)=%d  xfGetTileOutPosH(out_ptr)=%d\n", xfGetTileOutPosV(out_ptr),
    //    xfGetTileOutPosH(out_ptr));
    uint16_t in_posv = xfGetTileOutPosV(out_ptr);
    uint16_t in_posh = xfGetTileOutPosH(out_ptr);
    xfSetTileOutPosH(out_ptr, in_posv);
    xfSetTileOutPosV(out_ptr, in_posh);
    // xfSetTileOVLP_HL(out_ptr, 0);
    // xfSetTileOVLP_HR(out_ptr, 0);
    // xfSetTileOVLP_VT(out_ptr, 0);
    // xfSetTileOVLP_VB(out_ptr, 0);
    //    printf("xfGetTileOutPosV(out_ptr)=%d  xfGetTileOutPosH(out_ptr)=%d\n", xfGetTileOutPosV(out_ptr),
    //    xfGetTileOutPosH(out_ptr));
    int outOffset = (xfGetTileOutPosV(out_ptr) * outputStride) + xfGetTileOutPosH(out_ptr);
    xfSetTileOutOffset_L(out_ptr, (uint16_t)(outOffset & 0x0000ffff));
    xfSetTileOutOffset_U(out_ptr, (uint16_t)(outOffset >> 16));
    uint8_t* ptr_out = (uint8_t*)xfGetImgDataPtr(out_ptr);
    //    printf("outOffset=%d \n", outOffset);

    if (channels == 1)
        xf_transpose_1ch(in_data_ptr, height, width, ptr_out);
    else
        xf_transpose_4ch(in_data_ptr, height, width, ptr_out);
}

} // namespace aie
} // namespace cv
} // namespace xf
