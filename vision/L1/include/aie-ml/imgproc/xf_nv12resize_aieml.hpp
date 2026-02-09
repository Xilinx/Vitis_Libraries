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

#ifndef _XF_NV12resize_AIEML_H_
#define _XF_NV12resize_AIEML_H_

#define PARALLEL_FACTOR_16b 16 // Parallelization factor for 16b operations (16x mults)
#define SRS_SHIFT 10           // SRS shift used can be increased if input data likewise adjusted)

alignas(::aie::vector_decl_align) uint8_t u_buf[4800];
alignas(::aie::vector_decl_align) uint8_t v_buf[4800];
alignas(::aie::vector_decl_align) uint8_t outu_buf[192];
alignas(::aie::vector_decl_align) uint8_t outv_buf[192];

namespace xf {
namespace cv {
namespace aie {

__attribute__((noinline)) void resize_area(uint8_t* y, uint8_t* y_out, int16_t tile_width, int16_t tile_height) {
    ::aie::vector<uint8_t, 32> in_vec, in_vec1;
    ::aie::vector<uint16_t, 16> sum = ::aie::zeros<uint16_t, 16>();
    ::aie::vector<uint16_t, 16> const_val = ::aie::broadcast<uint16_t, 16>(2621);

    uint8_t* r0 = (uint8_t*)(y);
    uint8_t* r1 = (uint8_t*)(y + tile_width);
    uint8_t* r2 = (uint8_t*)(y + 2 * tile_width);
    uint8_t* r3 = (uint8_t*)(y + 3 * tile_width);
    uint8_t* r4 = (uint8_t*)(y + 4 * tile_width);

    for (int i = 0; i < tile_width; i += 30) chess_prepare_for_pipelining {
            in_vec = ::aie::load_unaligned_v<32>(r0);
            r0 += 30;
            in_vec1 = ::aie::load_unaligned_v<32>(r1);
            r1 += 30;
            ::aie::vector<uint16_t, 32> out = ::aie::add(in_vec.unpack(), in_vec1.unpack());
            in_vec = ::aie::load_unaligned_v<32>(r2);
            r2 += 30;
            in_vec1 = ::aie::load_unaligned_v<32>(r3);
            r3 += 30;
            ::aie::vector<uint16_t, 32> out1 = ::aie::add(in_vec.unpack(), in_vec1.unpack());
            ::aie::vector<uint16_t, 32> finla_out = ::aie::add(out, out1);
            in_vec = ::aie::load_unaligned_v<32>(r4);
            r4 += 30;
            out = ::aie::add(finla_out, in_vec.unpack());

            sum[0] = out[0] + out[1] + out[2] + out[3] + out[4];
            sum[1] = out[5] + out[6] + out[7] + out[8] + out[9];
            sum[2] = out[10] + out[11] + out[12] + out[13] + out[14];
            sum[3] = out[15] + out[16] + out[17] + out[18] + out[19];
            sum[4] = out[20] + out[21] + out[22] + out[23] + out[24];
            sum[5] = out[25] + out[26] + out[27] + out[28] + out[29];

            ::aie::accum<acc32, 16> acc = ::aie::mul(sum, const_val);
            ::aie::set_rounding(::aie::rounding_mode::positive_inf);
            ::aie::vector<uint8_t, 16> out_vec = acc.template to_vector<uint8_t>(16);
            for (int j = 0; j < 6; j++) {
                *y_out++ = out_vec[j];
            }
        }
}
__attribute__((noinline)) void uv_interleave(
    uint8_t* uv, uint8_t* u_out, uint8_t* v_out, int16_t tile_width, int16_t tile_height) {
    ::aie::vector<uint8_t, 64> uv_vec;
    for (int i = 0; i < (tile_height); i++) {
        for (int j = 0; j < (tile_width); j += 64) chess_prepare_for_pipelining {
                uv_vec.insert(0, ::aie::load_v<32>(uv));
                uv += 32;
                uv_vec.insert(1, ::aie::load_v<32>(uv));
                uv += 32;
                auto[u, v] = ::aie::interleave_unzip(uv_vec, uv_vec, 1);
                ::aie::store_v(u_out, u);
                ::aie::store_v(v_out, v);
                u_out += 32;
                v_out += 32;
            }
    }
}
__attribute__((noinline)) void uv_merge(
    uint8_t* u, uint8_t* v, uint8_t* uv_out, int16_t tile_width, int16_t tile_height) {
    ::aie::vector<uint8_t, 32> u_vec;
    ::aie::vector<uint8_t, 32> v_vec;

    for (int j = 0; j < (tile_width); j += 32) chess_prepare_for_pipelining {
            u_vec = ::aie::load_v<32>(u);
            u += 32;
            v_vec = ::aie::load_v<32>(v);
            v += 32;
            auto[u, v] = ::aie::interleave_zip(u_vec, v_vec, 1);
            ::aie::store_v(uv_out, u);
            uv_out += 32;
            ::aie::store_v(uv_out, v);
            uv_out += 32;
        }
}
__attribute__((noinline)) void nv12resize_y(adf::input_buffer<uint8_t>& img_in, adf::output_buffer<uint8_t>& img_out) {
    uint8_t* restrict y_in_ptr = (uint8_t*)::aie::begin(img_in);
    uint8_t* restrict y_out_ptr = (uint8_t*)::aie::begin(img_out);

    const int16_t image_width = xfGetTileWidth(y_in_ptr);
    const int16_t image_height = xfGetTileHeight(y_in_ptr);

    xfCopyMetaData(y_in_ptr, y_out_ptr);

    xfSetTileOutTWidth(y_out_ptr, xfGetTileWidth(y_out_ptr) / 5);
    xfSetTileOutTHeight(y_out_ptr, xfGetTileHeight(y_out_ptr) / 5);

    xfSetTileOutPosV(y_out_ptr, xfGetTilePosV(y_out_ptr) / 5);

    int out_offset = (xfGetTilePosV(y_out_ptr) / 5) * (image_width / 5);
    xfSetTileOutOffset_L(y_out_ptr, (out_offset & 0x0000ffff));
    xfSetTileOutOffset_U(y_out_ptr, (out_offset >> 16));

    uint8_t* yin_img_ptr = (uint8_t*)xfGetImgDataPtr(y_in_ptr);
    uint8_t* yout_img_ptr = (uint8_t*)xfGetImgDataPtr(y_out_ptr);

    // y-resize
    resize_area((uint8_t*)yin_img_ptr, (uint8_t*)yout_img_ptr, image_width, image_height);
}
__attribute__((noinline)) void nv12resize_uv(adf::input_buffer<uint8_t>& img_uv,
                                             adf::output_buffer<uint8_t>& uvimg_out) {
    uint8_t* restrict uv_in_ptr = (uint8_t*)::aie::begin(img_uv);
    uint8_t* restrict uv_out_ptr = (uint8_t*)::aie::begin(uvimg_out);

    const int16_t uvimage_width = xfGetTileWidth(uv_in_ptr);
    const int16_t uvimage_height = xfGetTileHeight(uv_in_ptr);

    xfCopyMetaData(uv_in_ptr, uv_out_ptr);

    xfSetTileOutTWidth(uv_out_ptr, xfGetTileOutTWidth(uv_in_ptr) / 5);
    xfSetTileOutTHeight(uv_out_ptr, xfGetTileOutTHeight(uv_in_ptr) / 5);

    xfSetTileOutPosV(uv_out_ptr, xfGetTilePosV(uv_out_ptr) / 5);

    int out_offset = (xfGetTilePosV(uv_out_ptr) / 5) * (uvimage_width / 5);
    xfSetTileOutOffset_L(uv_out_ptr, (out_offset & 0x0000ffff));
    xfSetTileOutOffset_U(uv_out_ptr, (out_offset >> 16));

    uint8_t* uvin_img_ptr = (uint8_t*)xfGetImgDataPtr(uv_in_ptr);
    uint8_t* uvout_img_ptr = (uint8_t*)xfGetImgDataPtr(uv_out_ptr);

    // UV-resize
    uint8_t* restrict u_ptr = (uint8_t*)u_buf;
    uint8_t* restrict v_ptr = (uint8_t*)v_buf;
    uint8_t* restrict outu_ptr = (uint8_t*)outu_buf;
    uint8_t* restrict outv_ptr = (uint8_t*)outv_buf;

    uv_interleave((uint8_t*)uvin_img_ptr, u_ptr, v_ptr, (uvimage_width * 2), uvimage_height);

    resize_area((uint8_t*)u_ptr, (uint8_t*)outu_ptr, (uvimage_width), uvimage_height);
    resize_area((uint8_t*)v_ptr, (uint8_t*)outv_ptr, (uvimage_width), uvimage_height);

    uv_merge(outu_ptr, outv_ptr, uvout_img_ptr, (uvimage_width / 5), (uvimage_height / 5));
}

/* __attribute__((noinline)) void nv12resize_api(adf::input_buffer<uint8_t>& img_in, adf::input_buffer<uint8_t>& img_uv,
                                             adf::output_buffer<uint8_t>& img_out, adf::output_buffer<uint8_t>&
uvimg_out) {

    uint8_t* restrict y_in_ptr = (uint8_t*)::aie::begin(img_in);
    uint8_t* restrict y_out_ptr = (uint8_t*)::aie::begin(img_out);
    uint8_t* restrict uv_in_ptr = (uint8_t*)::aie::begin(img_uv);
    uint8_t* restrict uv_out_ptr = (uint8_t*)::aie::begin(uvimg_out);

    const int16_t image_width = xfGetTileWidth(y_in_ptr);
    const int16_t image_height = xfGetTileHeight(y_in_ptr);
    const int16_t uvimage_width = xfGetTileWidth(uv_in_ptr);
    const int16_t uvimage_height = xfGetTileHeight(uv_in_ptr);


    xfCopyMetaData(y_in_ptr, y_out_ptr);
    xfCopyMetaData(uv_in_ptr, uv_out_ptr);

    xfSetTileOutTWidth(y_out_ptr, xfGetTileWidth(y_out_ptr) / 5);
    xfSetTileOutTHeight(y_out_ptr, xfGetTileHeight(y_out_ptr) / 5);
    xfSetTileOutTWidth(uv_out_ptr, xfGetTileOutTWidth(uv_in_ptr)  /5);
    xfSetTileOutTHeight(uv_out_ptr, xfGetTileOutTHeight(uv_in_ptr) / 5);

     xfSetTileOutPosV(y_out_ptr, xfGetTilePosV(y_out_ptr) / 5);
     xfSetTileOutPosV(uv_out_ptr, xfGetTilePosV(uv_out_ptr) / 5);

    uint8_t* yin_img_ptr = (uint8_t*)xfGetImgDataPtr(y_in_ptr);
    uint8_t* yout_img_ptr = (uint8_t*)xfGetImgDataPtr(y_out_ptr);
    uint8_t* uvin_img_ptr = (uint8_t*)xfGetImgDataPtr(uv_in_ptr);
    uint8_t* uvout_img_ptr = (uint8_t*)xfGetImgDataPtr(uv_out_ptr);

    // y-resize
    resize_area((uint8_t*)yin_img_ptr,  (uint8_t*)yout_img_ptr, image_width, image_height);

    // UV-resize
    uint8_t* restrict u_ptr = (uint8_t*)u_buf;
    uint8_t* restrict v_ptr = (uint8_t*)v_buf;
    uint8_t* restrict outu_ptr = (uint8_t*)outu_buf;
    uint8_t* restrict outv_ptr = (uint8_t*)outv_buf;

    uv_interleave((uint8_t*)uvin_img_ptr, u_ptr, v_ptr, (uvimage_width*2), uvimage_height);

    resize_area((uint8_t*)u_ptr,  (uint8_t*)outu_ptr, (uvimage_width), uvimage_height);
    resize_area((uint8_t*)v_ptr,  (uint8_t*)outv_ptr, (uvimage_width), uvimage_height);

    uv_merge(outu_ptr, outv_ptr, uvout_img_ptr, (uvimage_width/5), (uvimage_height/5));
} */
} // aie
} // cv
} // xf
#endif
