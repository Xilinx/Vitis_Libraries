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

#ifndef __XF_RESIZE_
#define __XF_RESIZE_

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>
#include <utility>

namespace xf {
namespace cv {
namespace aie {

class Resize {
   public:
    int last_blk_repeat(int tile_width_out) {
        int last_blk = ((((tile_width_out + 32 - 1) / 32) * 32) - tile_width_out);
        return last_blk;
    }

    auto compute_wtsy(int row, uint32_t scale_y, int img_height_in) {
        int32_t position = row * scale_y + (scale_y >> 1) - ToFixed<int32_t, 16>(0.5f);
        position = (position > 0) * position;
        uint16_t wt_16 = position;
        uint8_t weighty = wt_16 >> 8;
        position = (position >> 16);
        int p = (position < (img_height_in - 1)) * position + (position >= (img_height_in - 1)) * (img_height_in - 2);
        int w = (position < (img_height_in - 1)) * weighty + (position >= (img_height_in - 1)) * 255;
        return std::make_pair(p, w);
    }

    void init_pos_wt_accum(uint32_t scale_x, ::aie::vector<int32_t, 8>& pos_accum, ::aie::accum<acc32, 32>& wtx_accum) {
        int32_t p = (scale_x / 2) - ToFixed<int32_t, 16>(0.5);
        for (int i = 0; i < 8; i++) {
            pos_accum[i] = p;
            p += scale_x;
        }

        ::aie::vector<int32_t, 32> twx;
        auto[w1, w2] = ::aie::interleave_zip(pos_accum, pos_accum, 1);
        auto[w11, w12] = ::aie::interleave_zip(w1, w1, 2);
        auto[w21, w22] = ::aie::interleave_zip(w2, w2, 2);
        twx.insert(0, w11);
        twx.insert(1, w12);
        twx.insert(2, w21);
        twx.insert(3, w22);
        wtx_accum = ::aie::from_vector<acc32>(twx, 0);
    }

    void update_pos_wt_accum(uint32_t scale_x,
                             ::aie::vector<int32_t, 8>& pos_accum,
                             ::aie::accum<acc32, 32>& wtx_accum) {
        pos_accum = ::aie::add(pos_accum, ::aie::broadcast<int32_t, 8>(8 * scale_x));
        wtx_accum = ::aie::add(wtx_accum, ::aie::broadcast<int32_t, 32>(8 * scale_x));
    }

    void update_pos_wt_accum_last_blk(uint32_t scale_x,
                                      int last_blk,
                                      ::aie::vector<int32_t, 8>& pos_accum,
                                      ::aie::accum<acc32, 32>& wtx_accum) {
        pos_accum = ::aie::sub(pos_accum, ::aie::broadcast<int32_t, 8>(last_blk * scale_x));
        wtx_accum = ::aie::sub(wtx_accum, ::aie::broadcast<int32_t, 32>(last_blk * scale_x));
    }

    void xf_resize1DV(uint8_t* input1,
                      uint8_t* input2,
                      uint8_t* output,
                      int row,
                      uint32_t scale_y,
                      int img_height_in,
                      int tile_width_out,
                      int nChannels);

    void xf_resize1DH(
        uint8_t* input, uint8_t* output, int row, uint32_t scale_x, int tile_width_in, int tile_width_out);

    void xf_resize1DH1CH(
        uint8_t* input, uint8_t* output, int row, uint32_t scale_x, int tile_width_in, int tile_width_out);

    void xf_resize1DUpscaleGenericTile(uint8_t* input,
                                       uint8_t* output,
                                       int start_in_row,
                                       int start_out_row,
                                       uint32_t scale_y,
                                       int img_height_in,
                                       int tile_height_out,
                                       int tile_width_out);

    void runImpl(uint8_t* input,
                 uint8_t* output,
                 int row,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int tile_width_in,
                 int line_stride_in,
                 int tile_width_out,
                 int nChannels);

    void runImpl(adf::input_buffer<uint8_t>& input,
                 adf::output_buffer<uint8_t>& output,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int nChannels);
};

__attribute__((noinline)) void Resize::xf_resize1DV(uint8_t* input1,
                                                    uint8_t* input2,
                                                    uint8_t* output,
                                                    int row,
                                                    uint32_t scale_y,
                                                    int img_height_in,
                                                    int tile_width_out,
                                                    int nChannels) {
    auto r = compute_wtsy(row, scale_y, img_height_in);
    const uint8_t weight = r.second;
    ::aie::vector<uint8_t, 64> wy =
        ::aie::concat(::aie::broadcast<uint8_t, 32>(255 - weight), ::aie::broadcast<uint8_t, 32>(weight));

    ::aie::vector<uint8_t, 64> data_vec;
    ::aie::accum<acc32, 32> acc;
    data_vec.insert(0, ::aie::load_v<32>(input1));
    data_vec.insert(1, ::aie::load_v<32>(input2));
    uint8_t* restrict img_in_ptr1 = (uint8_t*)input1 + 32;
    uint8_t* restrict img_in_ptr2 = (uint8_t*)input2 + 32;
    uint8_t* restrict img_out_ptr = (uint8_t*)output;
    int pixel_width = tile_width_out * nChannels;
    for (int i = 0; i < pixel_width / 32; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
            acc = mul_elem_32_2(data_vec, wy);
            acc = ::aie::add(acc, data_vec.template extract<32>(0));
            ::aie::store_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
            data_vec.insert(0, ::aie::load_v<32>(img_in_ptr1));
            data_vec.insert(1, ::aie::load_v<32>(img_in_ptr2));
            img_in_ptr1 += 32;
            img_in_ptr2 += 32;
            img_out_ptr += 32;
        }

    if ((pixel_width % 32) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(pixel_width);
        img_in_ptr1 -= (32 + LAST_BLK_REPEAT);
        img_in_ptr2 -= (32 + LAST_BLK_REPEAT);
        img_out_ptr -= LAST_BLK_REPEAT;

        data_vec.insert(0, ::aie::load_unaligned_v<32>(img_in_ptr1));
        data_vec.insert(1, ::aie::load_unaligned_v<32>(img_in_ptr2));
        acc = mul_elem_32_2(data_vec, wy);
        acc = ::aie::add(acc, data_vec.template extract<32>(0));

        auto last_vec = acc.template to_vector<uint8_t>(8);
        ::aie::store_unaligned_v(img_out_ptr,
                                 ::aie::select(::aie::load_unaligned_v<32>(img_out_ptr), last_vec,
                                               ::aie::mask<32>::from_uint32((0xFFFFFFFF << LAST_BLK_REPEAT))));
    }
}

__attribute__((noinline)) void Resize::xf_resize1DH(
    uint8_t* input, uint8_t* output, int row, uint32_t scale_x, int tile_width_in, int tile_width_out) {
    uint32_t* restrict img_in_ptr = (uint32_t*)input;
    uint8_t* restrict img_out_ptr = (uint8_t*)output;

    ::aie::vector<int32_t, 8> pos_accum;
    ::aie::accum<acc32, 32> wtx_accum;
    init_pos_wt_accum(scale_x, pos_accum, wtx_accum);

    auto posi = ::aie::max(pos_accum, ::aie::broadcast<int32_t, 8>(0));
    auto posx = ::aie::filter_odd(posi.template cast_to<int16_t>());
    set_sat();
    auto m = ::aie::lt(wtx_accum.template to_vector<int16_t>(8), (int16_t)0);
    clr_sat();

    auto wtxs = ::aie::select(wtx_accum.template to_vector<uint8_t>(8), ::aie::broadcast<uint8_t, 32>(0), m);
    ::aie::vector<uint8_t, 32> wx_inv = ::aie::sub((uint8_t)255, wtxs);

    for (int i = 0; i < (tile_width_out - 8); i += 8) chess_prepare_for_pipelining chess_loop_range(14, ) {
            ::aie::vector<uint32_t, 16> input_vector(
                img_in_ptr[posx[0]], img_in_ptr[posx[1]], img_in_ptr[posx[2]], img_in_ptr[posx[3]], img_in_ptr[posx[4]],
                img_in_ptr[posx[5]], img_in_ptr[posx[6]], img_in_ptr[posx[7]], img_in_ptr[posx[0] + 1],
                img_in_ptr[posx[1] + 1], img_in_ptr[posx[2] + 1], img_in_ptr[posx[3] + 1], img_in_ptr[posx[4] + 1],
                img_in_ptr[posx[5] + 1], img_in_ptr[posx[6] + 1], img_in_ptr[posx[7] + 1]);

            auto data_vec = input_vector.template cast_to<uint8_t>();

            ::aie::accum<acc32, 32> acc = mul_elem_32_2(data_vec, ::aie::concat(wx_inv, wtxs));
            acc = ::aie::add(acc, data_vec.template extract<32>(0));

            ::aie::store_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
            img_out_ptr += 32;

            update_pos_wt_accum(scale_x, pos_accum, wtx_accum);
            posx = ::aie::filter_odd(pos_accum.template cast_to<int16_t>());
            wtxs = wtx_accum.template to_vector<uint8_t>(8);
            wx_inv = ::aie::sub((uint8_t)255, wtxs);
        }

    {
        int LAST_BLK_REPEAT = last_blk_repeat(tile_width_out * 4);
        img_out_ptr -= LAST_BLK_REPEAT;

        update_pos_wt_accum_last_blk(scale_x, (LAST_BLK_REPEAT >> 2), pos_accum, wtx_accum);
        posx = ::aie::filter_odd(pos_accum.template cast_to<int16_t>());
        posx = ::aie::min(posx, (int16_t)(tile_width_in - 1));

        set_rnd(rnd_floor);
        auto m = ::aie::lt(wtx_accum.template to_vector<uint16_t>(16), (uint16_t)(tile_width_in - 1));
        set_rnd(rnd_conv_even);
        wtxs = ::aie::select(::aie::broadcast<uint8_t, 32>(0), wtx_accum.template to_vector<uint8_t>(8), m);
        wx_inv = ::aie::sub((uint8_t)255, wtxs);

        ::aie::vector<uint32_t, 16> input_vector(
            img_in_ptr[posx[0]], img_in_ptr[posx[1]], img_in_ptr[posx[2]], img_in_ptr[posx[3]], img_in_ptr[posx[4]],
            img_in_ptr[posx[5]], img_in_ptr[posx[6]], img_in_ptr[posx[7]], img_in_ptr[posx[0] + 1],
            img_in_ptr[posx[1] + 1], img_in_ptr[posx[2] + 1], img_in_ptr[posx[3] + 1], img_in_ptr[posx[4] + 1],
            img_in_ptr[posx[5] + 1], img_in_ptr[posx[6] + 1], img_in_ptr[posx[7] + 1]);

        auto data_vec = input_vector.template cast_to<uint8_t>();

        ::aie::accum<acc32, 32> acc = mul_elem_32_2(data_vec, ::aie::concat(wx_inv, wtxs));
        acc = ::aie::add(acc, data_vec.template extract<32>(0));

        ::aie::store_unaligned_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
    }
}

__attribute__((noinline)) void Resize::xf_resize1DH1CH(
    uint8_t* input, uint8_t* output, int row, uint32_t scale_x, int tile_width_in, int tile_width_out) {
    uint8_t* restrict img_in_ptr = (uint8_t*)input;
    uint8_t* restrict img_out_ptr = (uint8_t*)output;

    ::aie::vector<int32_t, 32> pos_accum;
    ::aie::accum<acc32, 32> wtx_accum;
    {
        int32_t p = (scale_x / 2) - ToFixed<int32_t, 16>(0.5);
        for (int i = 0; i < 32; i++) chess_prepare_for_pipelining {
                pos_accum[i] = p;
                p += scale_x;
            }
        wtx_accum = ::aie::from_vector<acc32>(pos_accum, 0);
    }

    auto posi = ::aie::max(pos_accum, ::aie::broadcast<int32_t, 32>(0));
    auto posx = ::aie::filter_odd(posi.template cast_to<int16_t>());
    set_sat();
    auto m = ::aie::lt(wtx_accum.template to_vector<int16_t>(8), (int16_t)0);
    clr_sat();

    auto wtxs = ::aie::select(wtx_accum.template to_vector<uint8_t>(8), ::aie::broadcast<uint8_t, 32>(0), m);
    ::aie::vector<uint8_t, 32> wx_inv = ::aie::sub((uint8_t)255, wtxs);

    for (int i = 0; i < (tile_width_out - 32); i += 32) chess_prepare_for_pipelining {
            ::aie::vector<uint8_t, 64> data_vec_hf;
            ::aie::vector<uint8_t, 64> data_vec_lf;
            for (int j = 0; j < 32; j++) chess_prepare_for_pipelining {
                    uint8_t* ptr = img_in_ptr + posx[j];
                    data_vec_hf = ::shiftl_elem(data_vec_hf, ptr[0]);
                    data_vec_lf = ::shiftl_elem(data_vec_lf, ptr[1]);
                }

            auto data_vec_h = data_vec_hf.template extract<32>(1);
            auto data_vec_l = data_vec_lf.template extract<32>(1);

            ::aie::accum<acc32, 32> acc =
                mul_elem_32_2(::aie::concat(data_vec_h, data_vec_l), ::aie::concat(wx_inv, wtxs));
            acc = ::aie::add(acc, data_vec_h);

            ::aie::store_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
            img_out_ptr += 32;

            pos_accum = ::aie::add(pos_accum, ::aie::broadcast<int32_t, 32>(32 * scale_x));
            wtx_accum = ::aie::from_vector<acc32>(pos_accum, 0);
            posx = ::aie::filter_odd(pos_accum.template cast_to<int16_t>());
            wtxs = wtx_accum.template to_vector<uint8_t>(8);
            wx_inv = ::aie::sub((uint8_t)255, wtxs);
        }

    {
        int LAST_BLK_REPEAT = ((((tile_width_out + 32 - 1) / 32) * 32) - tile_width_out);
        img_out_ptr -= LAST_BLK_REPEAT;

        pos_accum = ::aie::sub(pos_accum, ::aie::broadcast<int32_t, 32>(LAST_BLK_REPEAT * scale_x));
        wtx_accum = ::aie::from_vector<acc32>(pos_accum, 0);
        posx = ::aie::filter_odd(pos_accum.template cast_to<int16_t>());
        posx = ::aie::min(posx, (int16_t)(tile_width_in - 1));

        set_rnd(rnd_floor);
        auto m = ::aie::lt(wtx_accum.template to_vector<uint16_t>(16), (uint16_t)(tile_width_in - 1));
        set_rnd(rnd_conv_even);
        wtxs = ::aie::select(::aie::broadcast<uint8_t, 32>(0), wtx_accum.template to_vector<uint8_t>(8), m);
        wx_inv = ::aie::sub((uint8_t)255, wtxs);

        ::aie::vector<uint8_t, 64> data_vec_hf;
        ::aie::vector<uint8_t, 64> data_vec_lf;
        for (int j = 0; j < 32; j++) chess_prepare_for_pipelining {
                uint8_t* ptr = img_in_ptr + posx[j];
                data_vec_hf = ::shiftl_elem(data_vec_hf, ptr[0]);
                data_vec_lf = ::shiftl_elem(data_vec_lf, ptr[1]);
            }

        auto data_vec_h = data_vec_hf.template extract<32>(1);
        auto data_vec_l = data_vec_lf.template extract<32>(1);

        ::aie::accum<acc32, 32> acc = mul_elem_32_2(::aie::concat(data_vec_h, data_vec_l), ::aie::concat(wx_inv, wtxs));
        acc = ::aie::add(acc, data_vec_h);

        ::aie::store_unaligned_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
    }
}

// Specialized 1D resize pass for upscale
// All widths / strides assume 1 channel
__attribute__((noinline)) void Resize::xf_resize1DUpscaleGenericTile(uint8_t* input,
                                                                     uint8_t* output,
                                                                     int start_in_row,
                                                                     int start_out_row,
                                                                     uint32_t scale_y,
                                                                     int img_height_in,
                                                                     int tile_height_out,
                                                                     int tile_width_out) {
    set_rnd(rnd_conv_even);
    ::aie::vector<uint8_t, 64> data_vec;
    ::aie::accum<acc32, 32> acc;
    // Tile width should always be multiple of 32
    for (int j = 0; j < tile_width_out; j += 32) {
        uint8_t* restrict img_out_ptr = (uint8_t*)(output + j);
        for (int i = 0; i < tile_height_out; i++) chess_prepare_for_pipelining {
                auto r = compute_wtsy(start_out_row + i, scale_y, img_height_in);
                int y_idx = (r.first - start_in_row) * tile_width_out;
                uint8_t* restrict img_in_ptr1 = (uint8_t*)(input + y_idx + j);
                uint8_t* restrict img_in_ptr2 = (uint8_t*)(input + y_idx + tile_width_out + j);
                ::aie::vector<uint8_t, 64> wy = ::aie::concat(::aie::broadcast<uint8_t, 32>(255 - r.second),
                                                              ::aie::broadcast<uint8_t, 32>(r.second));

                data_vec.insert(0, ::aie::load_v<32>(img_in_ptr1));
                data_vec.insert(1, ::aie::load_v<32>(img_in_ptr2));
                acc = mul_elem_32_2(data_vec, wy);
                acc = ::aie::add(acc, data_vec.template extract<32>(0));
                ::aie::store_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
                img_out_ptr += tile_width_out;
            }
    }
    set_rnd(rnd_floor);
}

__attribute__((noinline)) void Resize::runImpl(uint8_t* input,
                                               uint8_t* output,
                                               int row,
                                               uint32_t scale_x,
                                               uint32_t scale_y,
                                               int img_height_in,
                                               int tile_width_in,
                                               int line_stride_in,
                                               int tile_width_out,
                                               int nChannels) {
    set_rnd(rnd_conv_even);
    uint8_t* img_in_ptr_1 = (uint8_t*)input;
    uint8_t* img_in_ptr_2 = img_in_ptr_1 + line_stride_in * nChannels;
    xf_resize1DV(img_in_ptr_1, img_in_ptr_2, img_in_ptr_1, row, scale_y, img_height_in, tile_width_in, nChannels);
    if (nChannels == 1)
        xf_resize1DH1CH(img_in_ptr_1, output, row, scale_x, tile_width_in, tile_width_out);
    else
        xf_resize1DH(img_in_ptr_1, output, row, scale_x, tile_width_in, tile_width_out);
    set_rnd(rnd_floor);
}

void Resize::runImpl(adf::input_buffer<uint8_t>& input,
                     adf::output_buffer<uint8_t>& output,
                     uint32_t scale_x,
                     uint32_t scale_y,
                     int img_height_in,
                     int nChannels) {
    uint8* img_in_ptr = (uint8*)::aie::begin(input);
    uint8* img_out_ptr = (uint8*)::aie::begin(output);

    const int16_t tile_width_in = xfGetTileWidth(img_in_ptr);
    const int16_t tile_width_out = xfGetTileOutTWidth(img_in_ptr);

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    uint8* restrict ptr_in = (uint8*)xfGetImgDataPtr(img_in_ptr);
    uint8* restrict ptr_out = (uint8*)xfGetImgDataPtr(img_out_ptr);
    int row = xfGetTileOutPosV(img_in_ptr);
    runImpl(ptr_in, ptr_out, row, scale_x, scale_y, img_height_in, tile_width_in, tile_width_in, tile_width_out,
            nChannels);
}

} // aie
} // cv
} // xf

#endif