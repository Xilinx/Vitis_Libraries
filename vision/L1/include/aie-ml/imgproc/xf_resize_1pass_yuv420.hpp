/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
#include <common/xf_aie_device_traits.hpp>
#endif

namespace xf {
namespace cv {
namespace aie {

class Resizeyuv {
   private:
#if __AIE_ARCH__ == 22 || __AIE_ARCH__ == 21 // AIE2P/S
    static constexpr int N = 64;
#else
    static constexpr int N = 32;
#endif
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

    void init_pos_wt_accum(uint32_t scale_x, ::aie::vector<int32_t, 8>& pos_vec, ::aie::accum<acc32, 32>& wtx_accum) {
        int32_t p = (scale_x / 2) - ToFixed<int32_t, 16>(0.5);
        for (int i = 0; i < 8; i++) {
            pos_vec[i] = p;
            p += scale_x;
        }

        ::aie::vector<int32_t, 32> twx;
        auto[w1, w2] = ::aie::interleave_zip(pos_vec, pos_vec, 1);
        auto[w11, w12] = ::aie::interleave_zip(w1, w1, 2);
        auto[w21, w22] = ::aie::interleave_zip(w2, w2, 2);
        twx.insert(0, w11);
        twx.insert(1, w12);
        twx.insert(2, w21);
        twx.insert(3, w22);
        wtx_accum = ::aie::from_vector<acc32>(twx, 0);
    }

    void update_pos_wt_accum(uint32_t scale_x, ::aie::vector<int32_t, 8>& pos_vec, ::aie::accum<acc32, 32>& wtx_accum) {
        pos_vec = ::aie::add(pos_vec, ::aie::broadcast<int32_t, 8>(8 * scale_x));
        wtx_accum = ::aie::add(wtx_accum, ::aie::broadcast<int32_t, 32>(8 * scale_x));
    }

    void update_pos_wt_accum_last_blk(uint32_t scale_x,
                                      int last_blk,
                                      ::aie::vector<int32_t, 8>& pos_vec,
                                      ::aie::accum<acc32, 32>& wtx_accum) {
        pos_vec = ::aie::sub(pos_vec, ::aie::broadcast<int32_t, 8>(last_blk * scale_x));
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

    void xf_resize1DH1CH(
        uint8_t* input, uint8_t* output, int row, uint32_t scale_x, int tile_width_in, int tile_width_out);

    void runImpl(uint8_t* input1,
                 uint8_t* output,
                 int row,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int tile_width_in,
                 int line_stride_in,
                 int tile_width_out,
                 int nChannels,
                 int tile_height_out,
                 int tile_height_in);

    void runImply(adf::input_buffer<uint8_t>& input1,
                  adf::output_buffer<uint8_t>& output1,
                  uint32_t scale_x,
                  uint32_t scale_y,
                  int img_height_in,
                  int nChannels);

    void runImpl(adf::input_buffer<uint8_t>& input1,
                 adf::output_buffer<uint8_t>& output1,
                 uint32_t scale_x,
                 uint32_t scale_y,
                 int img_height_in,
                 int nChannels);

    template <int VEC_FACTOR>
    void xf_extract_yuv(uint8_t* ptr_yuyv,
                        uint8_t* ptr_y,
                        uint8_t* ptr_u,
                        uint8_t* ptr_v,
                        int16_t tile_width_in,
                        int16_t tile_height_in);
    template <int VEC_FACTOR>
    void xf_merge_yuv(uint8_t* ptr_yuyv,
                      uint8_t* ptr_y,
                      uint8_t* ptr_u,
                      uint8_t* ptr_v,
                      int16_t tile_width_out,
                      int16_t tile_height_out);
};

template <int VEC_FACTOR>
__attribute__((noinline)) void Resizeyuv::xf_extract_yuv(
    uint8_t* ptr_yuyv, uint8_t* ptr_y, uint8_t* ptr_u, uint8_t* ptr_v, int16_t tile_width_in, int16_t tile_height_in) {
    ::aie::vector<uint8_t, VEC_FACTOR> yuyv_channel0, yuyv_channel1, yuyv_channel3, yuyv_channel2;

    for (int i = 0; i < (tile_width_in * tile_height_in) / (4 * VEC_FACTOR); i++)
        chess_prepare_for_pipelining chess_loop_range(4, ) {
            yuyv_channel0 = ::aie::load_v<VEC_FACTOR>(ptr_yuyv);
            ptr_yuyv += VEC_FACTOR;
            yuyv_channel1 = ::aie::load_v<VEC_FACTOR>(ptr_yuyv);
            ptr_yuyv += VEC_FACTOR;
            yuyv_channel2 = ::aie::load_v<VEC_FACTOR>(ptr_yuyv);
            ptr_yuyv += VEC_FACTOR;
            yuyv_channel3 = ::aie::load_v<VEC_FACTOR>(ptr_yuyv);
            ptr_yuyv += VEC_FACTOR;
            auto[y, uv_temp] = ::aie::interleave_unzip(::aie::concat(yuyv_channel0, yuyv_channel1),
                                                       ::aie::concat(yuyv_channel2, yuyv_channel3), 1);
            auto u = ::aie::filter_even(uv_temp, 1);
            auto v = ::aie::filter_odd(uv_temp, 1);
            ::aie::store_v(ptr_y, y);
            ::aie::store_v(ptr_u, u);
            ::aie::store_v(ptr_v, v);

            ptr_y += 2 * VEC_FACTOR;
            ptr_u += VEC_FACTOR;
            ptr_v += VEC_FACTOR;
        }
}

template <int VEC_FACTOR>
__attribute__((noinline)) void Resizeyuv::xf_merge_yuv(
    uint8_t* ptr_out, uint8_t* ptr_y, uint8_t* ptr_u, uint8_t* ptr_v, int16_t tile_width_out, int16_t tile_height_out) {
    for (int i = 0; i < (tile_width_out * tile_height_out) / (2 * VEC_FACTOR); i++)
        chess_prepare_for_pipelining chess_loop_range(4, ) {
            auto y = ::aie::load_v<VEC_FACTOR>(ptr_y);
            ptr_y += VEC_FACTOR;
            auto y_even = ::aie::filter_even(y, 1);
            auto y_odd = ::aie::filter_odd(y, 1);
            auto u = ::aie::load_v<VEC_FACTOR / 2>(ptr_u);
            ptr_u += VEC_FACTOR / 2;
            auto v = ::aie::load_v<VEC_FACTOR / 2>(ptr_v);
            ptr_v += VEC_FACTOR / 2;
            std::tie(y_even, u) = ::aie::interleave_zip(y_even, u, 1);
            std::tie(y_odd, v) = ::aie::interleave_zip(y_odd, v, 1);
            auto[x1, y1] = ::aie::interleave_zip(::aie::concat(y_even, u), ::aie::concat(y_odd, v), 2);
            ::aie::store_v((uint8_t*)ptr_out, ::aie::concat(x1, y1));
            ptr_out = ptr_out + VEC_FACTOR * 2;
        }
}

__attribute__((noinline)) void Resizeyuv::xf_resize1DV(uint8_t* input1,
                                                       uint8_t* input2,
                                                       uint8_t* output,
                                                       int row,
                                                       uint32_t scale_y,
                                                       int img_height_in,
                                                       int tile_width_out,
                                                       int nChannels) {
    auto r = compute_wtsy(row, scale_y, img_height_in);
    const uint8_t weight = r.second;
    ::aie::vector<uint8_t, N* 2> wy =
        ::aie::concat(::aie::broadcast<uint8_t, N>(255 - weight), ::aie::broadcast<uint8_t, N>(weight));

    ::aie::vector<uint8_t, N * 2> data_vec;
    ::aie::accum<acc32, N> acc;
    data_vec.insert(0, ::aie::load_v<N>(input1));
    data_vec.insert(1, ::aie::load_v<N>(input2));
    uint8_t* restrict img_in_ptr1 = (uint8_t*)input1 + N;
    uint8_t* restrict img_in_ptr2 = (uint8_t*)input2 + N;
    uint8_t* restrict img_out_ptr = (uint8_t*)output;
    int pixel_width = tile_width_out * nChannels;
    for (int i = 0; i < pixel_width / N; i += 1) chess_prepare_for_pipelining chess_loop_range(14, ) {
#ifdef __SUPPORT_MUL64__
            acc = mul_elem_64_2(data_vec, wy);
#else
            acc = mul_elem_32_2(data_vec, wy);
#endif
            acc = ::aie::add(acc, data_vec.template extract<N>(0));
            ::aie::store_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
            data_vec.insert(0, ::aie::load_v<N>(img_in_ptr1));
            data_vec.insert(1, ::aie::load_v<N>(img_in_ptr2));
            img_in_ptr1 += N;
            img_in_ptr2 += N;
            img_out_ptr += N;
        }

    if ((pixel_width % N) != 0) {
        int LAST_BLK_REPEAT = last_blk_repeat(pixel_width);
        img_in_ptr1 -= (N + LAST_BLK_REPEAT);
        img_in_ptr2 -= (N + LAST_BLK_REPEAT);
        img_out_ptr -= LAST_BLK_REPEAT;

        data_vec.insert(0, ::aie::load_unaligned_v<N>(img_in_ptr1));
        data_vec.insert(1, ::aie::load_unaligned_v<N>(img_in_ptr2));
#ifdef __SUPPORT_MUL64__
        acc = mul_elem_64_2(data_vec, wy);
#else
        acc = mul_elem_32_2(data_vec, wy);
#endif
        acc = ::aie::add(acc, data_vec.template extract<N>(0));

        auto last_vec = acc.template to_vector<uint8_t>(8);
#ifdef __MASK_WITH64__
        ::aie::store_unaligned_v(img_out_ptr,
                                 ::aie::select(::aie::load_unaligned_v<N>(img_out_ptr), last_vec,
                                               ::aie::mask<N>::from_uint64((0xFFFFFFFF << LAST_BLK_REPEAT))));
#else
        ::aie::store_unaligned_v(img_out_ptr,
                                 ::aie::select(::aie::load_unaligned_v<N>(img_out_ptr), last_vec,
                                               ::aie::mask<N>::from_uint32((0xFFFFFFFF << LAST_BLK_REPEAT))));
#endif
    }
}

__attribute__((noinline)) void Resizeyuv::xf_resize1DH1CH(
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

#ifdef __SUPPORT_ACC64__
            ::aie::vector<uint16_t, 32> data_vec_h_16 = ::aie::unpack(data_vec_h);
            ::aie::vector<uint16_t, 32> data_vec_l_16 = ::aie::unpack(data_vec_l);
            ::aie::vector<uint16_t, 32> wx_inv_16 = ::aie::unpack(wx_inv);
            ::aie::vector<uint16_t, 32> wtxs_16 = ::aie::unpack(wtxs);
            ::aie::accum<acc64, 32> acc =
                mul_elem_32_2(::aie::concat(data_vec_h_16, data_vec_l_16), ::aie::concat(wx_inv_16, wtxs_16));
            acc = ::aie::add(acc, data_vec_h_16);
#else
            ::aie::accum<acc32, 32> acc =
                mul_elem_32_2(::aie::concat(data_vec_h, data_vec_l), ::aie::concat(wx_inv, wtxs));
            acc = ::aie::add(acc, data_vec_h);
#endif

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

#ifdef __SUPPORT_ACC64__
        ::aie::vector<uint16_t, 32> data_vec_h_16 = ::aie::unpack(data_vec_h);
        ::aie::vector<uint16_t, 32> data_vec_l_16 = ::aie::unpack(data_vec_l);
        ::aie::vector<uint16_t, 32> wx_inv_16 = ::aie::unpack(wx_inv);
        ::aie::vector<uint16_t, 32> wtxs_16 = ::aie::unpack(wtxs);
        ::aie::accum<acc64, 32> acc =
            mul_elem_32_2(::aie::concat(data_vec_h_16, data_vec_l_16), ::aie::concat(wx_inv_16, wtxs_16));
        acc = ::aie::add(acc, data_vec_h_16);
#else
        ::aie::accum<acc32, 32> acc = mul_elem_32_2(::aie::concat(data_vec_h, data_vec_l), ::aie::concat(wx_inv, wtxs));
        acc = ::aie::add(acc, data_vec_h);
#endif

        ::aie::store_unaligned_v(img_out_ptr, acc.template to_vector<uint8_t>(8));
    }
}

static constexpr int max_tile_width_in = 3840;
static constexpr int max_tile_height_in = 2;
static constexpr int max_tile_width_out = 3840;
static constexpr int max_tile_height_out = 1;
alignas(::aie::vector_decl_align) uint8_t U_buffer[max_tile_width_in * max_tile_height_in / 2] = {0};
alignas(::aie::vector_decl_align) uint8_t V_buffer[max_tile_width_in * max_tile_height_in / 2] = {0};

__attribute__((noinline)) void uv_separate(const uint8_t* restrict uv,
                                           uint8_t* restrict u_out,
                                           uint8_t* restrict v_out,
                                           int16_t tile_width,
                                           int16_t tile_height) {
    ::aie::vector<uint8_t, 64> uv_vec;
    ::aie::vector<uint8_t, 64> uv_vec1;
    int loop_cnt = (tile_height * tile_width);
    for (int j = 0; j < (loop_cnt); j += 128) chess_prepare_for_pipelining {
            uv_vec = ::aie::load_v<64>(uv);
            uv += 64;
            uv_vec1 = ::aie::load_v<64>(uv);
            uv += 64;
            auto[u, v] = ::aie::interleave_unzip(uv_vec, uv_vec1, 1);
            ::aie::store_v(u_out, u);
            u_out += 64;
            ::aie::store_v(v_out, v);
            v_out += 64;
        }
    //    }
}
__attribute__((noinline)) void uv_merge(
    uint8_t* restrict u, uint8_t* restrict v, uint8_t* restrict uv_out, int16_t tile_width, int16_t tile_height) {
    ::aie::vector<uint8_t, 64> u_vec;
    ::aie::vector<uint8_t, 64> v_vec;

    for (int j = 0; j < (tile_width); j += 64) chess_prepare_for_pipelining {
            u_vec = ::aie::load_v<64>(u);
            u += 64;
            v_vec = ::aie::load_v<64>(v);
            v += 64;
            auto[u, v] = ::aie::interleave_zip(u_vec, v_vec, 1);
            ::aie::store_v(uv_out, u);
            uv_out += 64;
            ::aie::store_v(uv_out, v);
            uv_out += 64;
        }
}

void Resizeyuv::runImply(adf::input_buffer<uint8_t>& input1,
                         adf::output_buffer<uint8_t>& output1,
                         uint32_t scale_x,
                         uint32_t scale_y,
                         int img_height_in,
                         int nChannels) {
    // *************  Y resize   *************
    uint8* img_in_ptr_y = (uint8*)::aie::begin(input1);
    uint8* img_out_ptr_y = (uint8*)::aie::begin(output1);
    xfCopyMetaData(img_in_ptr_y, img_out_ptr_y);

    const int16_t tile_width_in = xfGetTileWidth(img_in_ptr_y);
    const int16_t tile_width_out = xfGetTileOutTWidth(img_in_ptr_y);
    const int16_t tile_height_in = xfGetTileHeight(img_in_ptr_y);
    const int16_t tile_height_out = xfGetTileOutTHeight(img_in_ptr_y);
    int row = xfGetTileOutPosV(img_in_ptr_y);

    uint8* restrict Y_ptr = (uint8*)xfGetImgDataPtr(img_in_ptr_y);
    uint8* restrict Y_ptr_out = (uint8*)xfGetImgDataPtr(img_out_ptr_y);

    set_rnd(rnd_conv_even);
    int ch = 1;
    xf_resize1DV(Y_ptr, Y_ptr + tile_width_in, Y_ptr, row, scale_y, img_height_in, tile_width_in, ch);
    xf_resize1DH1CH(Y_ptr, Y_ptr_out, row, scale_x, tile_width_in, tile_width_out);
    set_rnd(rnd_floor);
}

void Resizeyuv::runImpl(adf::input_buffer<uint8_t>& input1,
                        adf::output_buffer<uint8_t>& output1,
                        uint32_t scale_x,
                        uint32_t scale_y,
                        int img_height_in,
                        int nChannels) {
    // *************  UV resize   *************

    uint8* img_in_ptr_uv = (uint8*)::aie::begin(input1);
    uint8* img_out_ptr_uv = (uint8*)::aie::begin(output1);
    xfCopyMetaData(img_in_ptr_uv, img_out_ptr_uv);

    const int16_t tile_width_in_uv = xfGetTileWidth(img_in_ptr_uv);
    const int16_t tile_width_out_uv = xfGetTileOutTWidth(img_in_ptr_uv);
    const int16_t tile_height_in_uv = xfGetTileHeight(img_in_ptr_uv);
    const int16_t tile_height_out_uv = xfGetTileOutTHeight(img_in_ptr_uv);
    int row_uv = xfGetTileOutPosV(img_in_ptr_uv);

    uint8* restrict UV_ptr = (uint8*)xfGetImgDataPtr(img_in_ptr_uv);
    uint8* restrict UV_ptr_out = (uint8*)xfGetImgDataPtr(img_out_ptr_uv);
    uint8_t* restrict U_ptr = (uint8_t*)U_buffer;
    uint8_t* restrict V_ptr = (uint8_t*)V_buffer;
    int ch = 1;

    uv_separate((uint8_t*)UV_ptr, U_ptr, V_ptr, (tile_width_in_uv * 2), tile_height_in_uv);

    set_rnd(rnd_conv_even);
    xf_resize1DV(U_ptr, U_ptr + tile_width_in_uv, U_ptr, row_uv, scale_y, img_height_in, tile_width_in_uv, ch);
    xf_resize1DH1CH(U_ptr, U_ptr, row_uv, scale_x, tile_width_in_uv, tile_width_out_uv);
    xf_resize1DV(V_ptr, V_ptr + tile_width_in_uv, V_ptr, row_uv, scale_y, img_height_in, tile_width_in_uv, ch);
    xf_resize1DH1CH(V_ptr, V_ptr, row_uv, scale_x, tile_width_in_uv, tile_width_out_uv);
    set_rnd(rnd_floor);

    uv_merge(U_ptr, V_ptr, UV_ptr_out, tile_width_out_uv, tile_height_out_uv);
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
