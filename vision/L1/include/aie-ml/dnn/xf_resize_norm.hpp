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

#ifndef __XF_RESIZE_NORM_
#define __XF_RESIZE_NORM_

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>

namespace xf {
namespace cv {
namespace aie {

class ResizeNorm {
    int mnFBitsIn;
    int mnFBitsAlpha;
    int mnFBitsBeta;
    int mnFBitsOut;

   public:
    ResizeNorm() : mnFBitsIn(0), mnFBitsAlpha(0), mnFBitsBeta(0), mnFBitsOut(0) {}
    ResizeNorm(int nFBitsIn, int nFBitsAlpha, int nFBitsBeta, int nFBitsOut)
        : mnFBitsIn(nFBitsIn), mnFBitsAlpha(nFBitsAlpha), mnFBitsBeta(nFBitsBeta), mnFBitsOut(nFBitsOut) {}
    ResizeNorm(int nFBitsAlpha, int nFBitsBeta, int nFBitsOut)
        : mnFBitsIn(0), mnFBitsAlpha(nFBitsAlpha), mnFBitsBeta(nFBitsBeta), mnFBitsOut(nFBitsOut) {}

    int updateNormalizeParameters(int nFBitsIn, int nFBitsAlpha, int nFBitsBeta, int nFBitsOut) {
        mnFBitsIn = nFBitsIn;
        mnFBitsAlpha = nFBitsAlpha;
        mnFBitsBeta = nFBitsBeta;
        mnFBitsOut = nFBitsOut;
    }

    int last_blk_repeat(int tile_width_out) {
        int last_blk = ((((tile_width_out + 8 - 1) / 8) * 8) - tile_width_out);
        return last_blk;
    }

    uint8_t compute_wtsy(int row, uint32_t scale_y, int img_height_in) {
        int32_t position = row * scale_y + (scale_y >> 1) - ToFixed<int32_t, 16>(0.5f);
        position = std::max(position, 0);
        uint16_t wt_16 = position & 0x0000FFFF;
        uint8_t weighty = wt_16 >> 8;
        position = (position >> 16);
        return (position < (img_height_in - 1)) ? weighty : (uint8_t)255;
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

    void xf_normalize(uint8_t* input, int8_t* output, int tile_width_out, const int16_t* coeff);

    void xf_denormalize(
        int8_t* input1, int8_t* input2, uint8_t* output1, uint8_t* output2, int tile_width_in, const int16_t* coeff);

    void xf_resize1DV(uint8_t* input1,
                      uint8_t* input2,
                      uint8_t* output,
                      int row,
                      uint32_t scale_y,
                      int img_height_in,
                      int tile_width_out);

    void xf_resize1DH(
        uint8_t* input, uint8_t* output, int row, uint32_t scale_x, int tile_width_in, int tile_width_out);

    void runImpl(uint8_t* input,
                 uint8_t* output,
                 int row,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int tile_width_in,
                 int line_stride_in,
                 int tile_width_out);

    void runImpl(uint8_t* input,
                 int8_t* output,
                 int row,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int tile_width_in,
                 int line_stride_in,
                 int tile_width_out,
                 const int16_t* coeff);

    void runImpl(int8_t* input,
                 uint8_t* output,
                 int row,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int tile_width_in,
                 int line_stride_in,
                 int tile_width_out,
                 const int16_t* coeff);
};

__attribute__((noinline)) void ResizeNorm::xf_normalize(uint8_t* input,
                                                        int8_t* output,
                                                        int tile_width_out,
                                                        const int16_t* coeff) {
    uint8_t alpha[4] = {(uint8_t)coeff[0], (uint8_t)coeff[1], (uint8_t)coeff[2], (uint8_t)coeff[3]};
    int8_t beta[4] = {(int8_t)coeff[4], (int8_t)coeff[5], (int8_t)coeff[6], (int8_t)coeff[7]};

    uint32_t alpha_i = *((uint32_t*)(alpha));
    int32_t beta_i = *((int32_t*)(beta));

    auto a_reg = ::aie::broadcast<uint32_t, 8>(alpha_i).template cast_to<uint8_t>();
    auto b_tmp = ::aie::broadcast<int32_t, 8>(beta_i).template cast_to<int8_t>();
    ::aie::vector<int8_t, 64> b_reg = ::aie::concat(b_tmp, ::aie::neg(b_tmp));

    ::aie::vector<uint8_t, 64> data_vec;
    ::aie::accum<acc32, 32> acc;
    data_vec.insert(0, ::aie::load_v<32>(input));
    data_vec.insert(1, a_reg);
    uint8_t* restrict img_in_ptr = (uint8_t*)input + 32;
    int8_t* restrict img_out_ptr = (int8_t*)output;
    int nFBits = mnFBitsAlpha + mnFBitsBeta - mnFBitsOut;
    for (int i = 0; i < tile_width_out / 8; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
            acc = mul_elem_32_2(data_vec, b_reg);
            ::aie::store_v(img_out_ptr, acc.template to_vector<int8_t>(nFBits));
            data_vec.insert(0, ::aie::load_v<32>(img_in_ptr));
            img_in_ptr += 32;
            img_out_ptr += 32;
        }

    if ((tile_width_out % 8) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(tile_width_out);
        img_in_ptr -= (32 + (LAST_BLK_REPEAT << 2));
        data_vec.insert(0, ::aie::load_unaligned_v<8>(((uint32_t*)img_in_ptr)).template cast_to<uint8_t>());
        acc = mul_elem_32_2(data_vec, b_reg);

        auto last_vec = acc.template to_vector<int8_t>(nFBits).template cast_to<uint32_t>();
        uint32_t* lastoutp = (uint32_t*)img_out_ptr;
        for (int i = LAST_BLK_REPEAT; i < 8; i++) {
            *lastoutp = last_vec[i];
            lastoutp++;
        }
    }
}

__attribute__((noinline)) void ResizeNorm::xf_denormalize(
    int8_t* input1, int8_t* input2, uint8_t* output1, uint8_t* output2, int tile_width_in, const int16_t* coeff) {
    uint8_t alpha[4] = {(uint8_t)coeff[0], (uint8_t)coeff[1], (uint8_t)coeff[2], (uint8_t)coeff[3]};
    int8_t beta[4] = {(int8_t)coeff[4], (int8_t)coeff[5], (int8_t)coeff[6], (int8_t)coeff[7]};

    uint32_t alpha_i = *((uint32_t*)(alpha));
    int32_t beta_i = *((int32_t*)(beta));

    auto a_reg = ::aie::broadcast<uint32_t, 8>(alpha_i).template cast_to<uint8_t>();
    auto b_reg = ::aie::broadcast<int32_t, 8>(beta_i).template cast_to<int8_t>();

    ::aie::accum<acc32, 32> acc_init;
    uint8_t nFBits = mnFBitsIn + mnFBitsBeta;
    uint8_t mFBits = nFBits - mnFBitsAlpha;
    uint8_t nFBits_x256 = nFBits - 8;
    uint8_t a_scale = (1 << mFBits);
    acc_init = ::aie::mul(a_reg, a_scale);

    ::aie::vector<int8_t, 32> data_vec1 = ::aie::load_v<32>(input1);
    ::aie::vector<int8_t, 32> data_vec2 = ::aie::load_v<32>(input2);
    int8_t* restrict img_in_ptr1 = (int8_t*)input1 + 32;
    int8_t* restrict img_in_ptr2 = (int8_t*)input2 + 32;
    uint8_t* restrict img_out_ptr1 = (uint8_t*)output1;
    uint8_t* restrict img_out_ptr2 = (uint8_t*)output2;
    set_rnd(rnd_floor);
    for (int i = 0; i < tile_width_in / 8; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
            ::aie::accum<acc32, 32> acc1 = ::aie::mac(acc_init, data_vec1, b_reg);
            ::aie::accum<acc32, 32> acc2 = ::aie::mac(acc_init, data_vec2, b_reg);
            ::aie::store_v(img_out_ptr1, acc1.template to_vector<uint8_t>(nFBits_x256));
            ::aie::store_v(img_out_ptr2, acc2.template to_vector<uint8_t>(nFBits_x256));
            data_vec1 = ::aie::load_v<32>(img_in_ptr1);
            data_vec2 = ::aie::load_v<32>(img_in_ptr2);
            img_in_ptr1 += 32;
            img_out_ptr1 += 32;
            img_in_ptr2 += 32;
            img_out_ptr2 += 32;
        }

    if ((tile_width_in % 8) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(tile_width_in);
        img_in_ptr1 -= (32 + (LAST_BLK_REPEAT << 2));
        img_in_ptr2 -= (32 + (LAST_BLK_REPEAT << 2));
        data_vec1 = ::aie::load_unaligned_v<8>(((uint32_t*)img_in_ptr1)).template cast_to<int8_t>();
        data_vec2 = ::aie::load_unaligned_v<8>(((uint32_t*)img_in_ptr2)).template cast_to<int8_t>();
        ::aie::accum<acc32, 32> acc1 = ::aie::mac(acc_init, data_vec1, b_reg);
        ::aie::accum<acc32, 32> acc2 = ::aie::mac(acc_init, data_vec2, b_reg);

        auto last_vec1 = acc1.template to_vector<uint8_t>(nFBits_x256).template cast_to<uint32_t>();
        auto last_vec2 = acc2.template to_vector<uint8_t>(nFBits_x256).template cast_to<uint32_t>();
        uint32_t* lastoutp1 = (uint32_t*)img_out_ptr1;
        uint32_t* lastoutp2 = (uint32_t*)img_out_ptr2;
        for (int i = LAST_BLK_REPEAT; i < 8; i++) {
            *lastoutp1 = last_vec1[i];
            *lastoutp2 = last_vec2[i];
            lastoutp1++;
            lastoutp2++;
        }
    }
    set_rnd(rnd_conv_even);
}

__attribute__((noinline)) void ResizeNorm::xf_resize1DV(uint8_t* input1,
                                                        uint8_t* input2,
                                                        uint8_t* output,
                                                        int row,
                                                        uint32_t scale_y,
                                                        int img_height_in,
                                                        int tile_width_out) {
    const uint8_t weight = compute_wtsy(row, scale_y, img_height_in);
    ::aie::vector<uint8_t, 64> wy =
        ::aie::concat(::aie::broadcast<uint8_t, 32>(255 - weight), ::aie::broadcast<uint8_t, 32>(weight));

    ::aie::vector<uint8_t, 64> data_vec;
    ::aie::accum<acc32, 32> acc;
    data_vec.insert(0, ::aie::load_v<32>(input1));
    data_vec.insert(1, ::aie::load_v<32>(input2));
    uint8_t* restrict img_in_ptr1 = (uint8_t*)input1 + 32;
    uint8_t* restrict img_in_ptr2 = (uint8_t*)input2 + 32;
    uint8_t* restrict img_out_ptr = (uint8_t*)output;
    for (int i = 0; i < tile_width_out / 8; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
            acc = mul_elem_32_2(data_vec, wy);
            acc = ::aie::add(acc, data_vec.template extract<32>(0));
            ::aie::store_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
            data_vec.insert(0, ::aie::load_v<32>(img_in_ptr1));
            data_vec.insert(1, ::aie::load_v<32>(img_in_ptr2));
            img_in_ptr1 += 32;
            img_in_ptr2 += 32;
            img_out_ptr += 32;
        }

    if ((tile_width_out % 8) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(tile_width_out);
        img_in_ptr1 -= (32 + (LAST_BLK_REPEAT << 2));
        img_in_ptr2 -= (32 + (LAST_BLK_REPEAT << 2));
        data_vec.insert(0, ::aie::load_unaligned_v<8>(((uint32_t*)img_in_ptr1)).template cast_to<uint8_t>());
        data_vec.insert(1, ::aie::load_unaligned_v<8>(((uint32_t*)img_in_ptr2)).template cast_to<uint8_t>());
        acc = mul_elem_32_2(data_vec, wy);
        acc = ::aie::add(acc, data_vec.template extract<32>(0));

        auto last_vec = acc.template to_vector<uint8_t>(8).template cast_to<uint32_t>();
        uint32_t* lastoutp = (uint32_t*)img_out_ptr;
        for (int i = LAST_BLK_REPEAT; i < 8; i++) {
            *lastoutp = last_vec[i];
            lastoutp++;
        }
    }
}

__attribute__((noinline)) void ResizeNorm::xf_resize1DH(
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
        int LAST_BLK_REPEAT = last_blk_repeat(tile_width_out);
        img_out_ptr -= (LAST_BLK_REPEAT << 2);

        update_pos_wt_accum_last_blk(scale_x, LAST_BLK_REPEAT, pos_accum, wtx_accum);
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

__attribute__((noinline)) void ResizeNorm::runImpl(uint8_t* input,
                                                   uint8_t* output,
                                                   int row,
                                                   uint32_t scale_x,
                                                   uint32_t scale_y,
                                                   int img_height_in,
                                                   int tile_width_in,
                                                   int line_stride_in,
                                                   int tile_width_out) {
    set_rnd(rnd_conv_even);
    uint32_t* img_in_ptr_1 = (uint32_t*)input;
    uint32_t* img_in_ptr_2 = img_in_ptr_1 + line_stride_in;
    xf_resize1DV((uint8_t*)img_in_ptr_1, (uint8_t*)img_in_ptr_2, (uint8_t*)img_in_ptr_1, row, scale_y, img_height_in,
                 tile_width_in);
    xf_resize1DH((uint8_t*)img_in_ptr_1, (uint8_t*)output, row, scale_x, tile_width_in, tile_width_out);
    set_rnd(rnd_floor);
}

__attribute__((noinline)) void ResizeNorm::runImpl(uint8_t* input,
                                                   int8_t* output,
                                                   int row,
                                                   uint32_t scale_x,
                                                   uint32_t scale_y,
                                                   int img_height_in,
                                                   int tile_width_in,
                                                   int line_stride_in,
                                                   int tile_width_out,
                                                   const int16_t* coeff) {
    set_rnd(rnd_conv_even);
    uint32_t* img_in_ptr_1 = (uint32_t*)input;
    uint32_t* img_in_ptr_2 = img_in_ptr_1 + line_stride_in;
    xf_resize1DV((uint8_t*)img_in_ptr_1, (uint8_t*)img_in_ptr_2, (uint8_t*)img_in_ptr_1, row, scale_y, img_height_in,
                 tile_width_in);
    xf_resize1DH((uint8_t*)img_in_ptr_1, (uint8_t*)output, row, scale_x, tile_width_in, tile_width_out);
    xf_normalize((uint8_t*)output, (int8_t*)output, tile_width_out, coeff);
    set_rnd(rnd_floor);
}

__attribute__((noinline)) void ResizeNorm::runImpl(int8_t* input,
                                                   uint8_t* output,
                                                   int row,
                                                   uint32_t scale_x,
                                                   uint32_t scale_y,
                                                   int img_height_in,
                                                   int tile_width_in,
                                                   int line_stride_in,
                                                   int tile_width_out,
                                                   const int16_t* coeff) {
    set_rnd(rnd_conv_even);
    uint32_t* img_in_ptr_1 = (uint32_t*)input;
    uint32_t* img_in_ptr_2 = img_in_ptr_1 + line_stride_in;
    xf_denormalize((int8_t*)img_in_ptr_1, (int8_t*)img_in_ptr_2, (uint8_t*)img_in_ptr_1, (uint8_t*)img_in_ptr_2,
                   tile_width_in, coeff);
    xf_resize1DV((uint8_t*)img_in_ptr_1, (uint8_t*)img_in_ptr_2, (uint8_t*)img_in_ptr_1, row, scale_y, img_height_in,
                 tile_width_in);
    xf_resize1DH((uint8_t*)img_in_ptr_1, (uint8_t*)output, row, scale_x, tile_width_in, tile_width_out);
    set_rnd(rnd_floor);
}

} // aie
} // cv
} // xf

#endif
