/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef __XF_CVTCOLOR_IMPL_HPP__
#define __XF_CVTCOLOR_IMPL_HPP__

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>

namespace xf {
namespace cv {
namespace aie {

template <typename T, int FBITS>
const T toFixed(const float f) {
    return (T)(std::roundf(f * (1 << FBITS)));
}

class Yuv2Rgba {
   private:
    static constexpr int N = 32; // Vectorization factor

   public:
    void runImpl(uint8* img_in1, uint8* img_in2, uint8* img_out, uint16_t tile_width, uint16_t tile_height);

    void xf_yuv2rgba(const uint8_t* restrict ptr1,
                     const uint8_t* restrict ptr2,
                     uint8_t* restrict out_ptr,
                     uint16_t tile_width,
                     uint16_t tile_height,
                     uint8_t index);
};

__attribute__((noinline)) void Yuv2Rgba::xf_yuv2rgba(const uint8_t* restrict ptr1,
                                                     const uint8_t* restrict ptr2,
                                                     uint8_t* restrict ptr_out,
                                                     uint16_t tile_width,
                                                     uint16_t tile_height,
                                                     uint8_t index) {
    set_sat();
    const int offset = toFixed<int, 7>((0.0 - 1.164 * 16) + 0.5);
    ::aie::accum<acc32, N> acc_const;
    acc_const.template from_vector(::aie::broadcast<int, N>(offset));

    for (int i = 0; i < (tile_height); i++) {
        for (int j = 0; j < (tile_width); j += N) chess_prepare_for_pipelining {
                auto y = ::aie::load_v<N>(ptr1);
                auto uv =
                    ::aie::sub(::aie::load_v<N>(ptr2), ::aie::broadcast<uint8_t, N>(128)).template cast_to<int8_t>();
                ptr1 += N;
                ptr2 += N;
                auto[uu_temp, vv_temp] = ::aie::interleave_zip(uv, uv, 1);
                auto[uu, vv] = ::aie::interleave_unzip(uu_temp, vv_temp, 2);

                auto acc_Y = ::aie::mul(y, toFixed<uint8_t, 7>(1.164));
                ::aie::accum<acc32, N> acc;

                // R Channel {
                acc = ::addmac_elem_32_2(vv.template grow<64>(),
                                         ::aie::concat(::aie::broadcast<uint8_t, N>(toFixed<uint8_t, 7>(1.596)),
                                                       ::aie::broadcast<uint8_t, N>((uint8_t)0)),
                                         acc_Y, acc_const);
                auto r = acc.template to_vector<uint8_t>(7);
                //}

                // G channel {
                acc = ::addmsc_elem_32_2(::aie::concat(vv, uu),
                                         ::aie::concat(::aie::broadcast<uint8_t, N>(toFixed<uint8_t, 7>(0.813)),
                                                       ::aie::broadcast<uint8_t, N>(toFixed<uint8_t, 7>(0.391))),
                                         acc_Y, acc_const);
                auto g = acc.template to_vector<uint8_t>(7);
                //}

                std::tie(r, g) = ::aie::interleave_zip(r, g, 1);

                // B channel {
                acc = ::addmac_elem_32_2(::aie::concat(uu, uu),
                                         ::aie::concat(::aie::broadcast<uint8_t, N>(toFixed<uint8_t, 7>(1.018)),
                                                       ::aie::broadcast<uint8_t, N>(toFixed<uint8_t, 7>(1.0))),
                                         acc_Y, acc_const);
                auto b = acc.template to_vector<uint8_t>(7);
                //}

                auto a = ::aie::broadcast<uint8_t, N>((uint8_t)255);
                std::tie(b, a) = ::aie::interleave_zip(b, a, 1);

                auto[x1, y1] = ::aie::interleave_zip(::aie::concat(r, g), ::aie::concat(b, a), 2);

                // Store
                ::aie::store_v((uint8_t*)ptr_out, ::aie::concat(x1, y1));
                ptr_out = ptr_out + 4 * N;
            }
        // Check even/odd tile to modify UV pointer for next row
        // ptr2 = ptr2 - (1 - (i % 2)) * tile_width;
        ptr2 = ptr2 - (1 - (index % 2)) * tile_width;
    }
}

void Yuv2Rgba::runImpl(
    uint8* img_in1, uint8* img_in2, uint8* img_out, const uint16_t tile_width, const uint16_t tile_height) {
    xfCopyMetaData(img_in1, img_out);

    uint8_t* restrict ptr1 = (uint8_t*)xfGetImgDataPtr(img_in1);
    uint8_t* restrict ptr2 = (uint8_t*)xfGetImgDataPtr(img_in2);
    uint8_t* restrict ptr_out = (uint8_t*)xfGetImgDataPtr(img_out);

    int y_row = xfGetTilePosV(img_in1);
    int uv_row = xfGetTilePosV(img_in2);

    uint8_t index = (((y_row == 1) && (uv_row == 0)) ? 1 : (((y_row == 0) && (uv_row == 0)) ? 0 : (y_row % uv_row)));
    xf_yuv2rgba(ptr1, ptr2, ptr_out, tile_width, tile_height, index);
}
} // aie
} // cv
} // xf
#endif
