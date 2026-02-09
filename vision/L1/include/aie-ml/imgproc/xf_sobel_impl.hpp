/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.
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

#ifndef __XF_SOBEL_IMPL_HPP__
#define __XF_SOBEL_IMPL_HPP__

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>

namespace xf {
namespace cv {
namespace aie {

template <int TILE_HEIGHT, int NO_DISPARITIES, int TILE_WINSIZE>
class SobelBaseImpl {
   public:
    void runImpl(adf::input_buffer<uint8_t>& in,
                 adf::output_buffer<uint8_t>& metadata_out,
                 adf::output_buffer<uint8_t>& out);
    void xf_sobel(uint8_t* restrict img_in_1, uint8_t* restrict ptr_out, int16_t posv);
};

template <int TILE_HEIGHT, int NO_DISPARITIES, int TILE_WINSIZE>
__attribute__((noinline)) void SobelBaseImpl<TILE_HEIGHT, NO_DISPARITIES, TILE_WINSIZE>::xf_sobel(
    uint8_t* restrict in_ptr, uint8_t* restrict ptr_out, int16_t posv) {
    ::aie::accum<acc32, 32> acc1;
    int16_t L_BORDER_VAL = -31;
    int16_t U_BORDER_VAL = 31;
    ::aie::vector<uint8_t, 32> V_31 = ::aie::broadcast<uint8_t, 32>((uint8_t)31);
    if (posv == 0) {
        for (int i = 0; i < TILE_WINSIZE + 1; i++) chess_prepare_for_pipelining {
                ::aie::store_v((uint8_t*)ptr_out, V_31);
                ptr_out += 32;
            }
    }

    for (int i = 0; i < TILE_HEIGHT - 2; i++) chess_prepare_for_pipelining {
            ::aie::vector<uint8_t, 32> V2 = ::aie::load_v<32>(in_ptr + 32);
            ::aie::vector<uint8_t, 32> V1 = ::aie::load_v<32>(in_ptr);
            ::aie::vector<uint8_t, 32> V3 = ::aie::load_v<32>(in_ptr + 32 + 32);
            in_ptr += 32;
            acc1 = ::mul_elem_32_2(::aie::concat(V2, V1), ::aie::concat(::aie::broadcast<uint8_t, 32>((uint8_t)2),
                                                                        ::aie::broadcast<uint8_t, 32>((uint8_t)1)));
            acc1 = ::aie::add(acc1, V3);
            ::aie::vector<int16_t, 32> V4 = acc1.template to_vector<int16_t>(0);
            ::aie::vector<int16_t, 32> V5 = ::aie::shuffle_down(V4, 2);
            ::aie::vector<int16_t, 32> V6 = ::aie::sub(V5, V4);
            auto m1 = ::aie::lt(V6, L_BORDER_VAL);
            auto m2 = ::aie::gt(V6, U_BORDER_VAL);
            V6 = ::aie::select(V6, ::aie::broadcast<int16_t, 32>((uint8_t)0), m1);
            V6 = ::aie::select(V6, ::aie::broadcast<int16_t, 32>((uint8_t)62), m2);
            auto V7 = ::aie::add(V6, U_BORDER_VAL);
            auto m3 = ~(m1 | m2);
            V6 = ::aie::select(V6, V7, m3);
            ::aie::store_v((uint8_t*)ptr_out, ::aie::pack(V6.cast_to<uint16_t>()));
            ptr_out += 32;
        }
}

template <int TILE_HEIGHT, int NO_DISPARITIES, int TILE_WINSIZE>
void SobelBaseImpl<TILE_HEIGHT, NO_DISPARITIES, TILE_WINSIZE>::runImpl(adf::input_buffer<uint8_t>& in,
                                                                       adf::output_buffer<uint8_t>& metadata_out,
                                                                       adf::output_buffer<uint8_t>& out) {
    uint8_t* restrict img_in_ptr = (uint8_t*)::aie::begin(in);
    uint8_t* restrict img_out_ptr = (uint8_t*)::aie::begin(out);
    uint8_t* restrict metadata_ptr = (uint8_t*)::aie::begin(metadata_out);
    xfCopyMetaData(img_in_ptr, metadata_ptr);
    const int16_t posh = xfGetTilePosH(img_in_ptr);
    const int16_t posv = xfGetTilePosV(img_in_ptr);
    int16_t posv_new = (posv == 0) ? 0 : posv + TILE_WINSIZE + 1;
    xfSetTileOutPosH(metadata_ptr, posh);
    xfSetTileOutPosV(metadata_ptr, posv_new);
    const int16_t otw = 32 + 240 + 32;
    xfSetTileOutTWidth(metadata_ptr, otw);
    xfSetTileWidth(metadata_ptr, otw);
    int16_t tile_ht_temp = TILE_HEIGHT - 2;
    const int16_t oth = tile_ht_temp + TILE_WINSIZE + 1;
    xfSetTileOutTHeight(metadata_ptr, oth);
    const int16_t OVLP = 0;
    xfSetTileOVLP_HL(metadata_ptr, OVLP);
    xfSetTileOVLP_HR(metadata_ptr, OVLP);
    xfSetTileOVLP_VT(metadata_ptr, OVLP);
    xfSetTileOVLP_VB(metadata_ptr, OVLP);
    uint8_t* restrict ptr0 = img_in_ptr + 64;
    xf_sobel(ptr0, img_out_ptr, posv);
}

} // namespace aie
} // namespace cv
} // namespace xf
#endif
