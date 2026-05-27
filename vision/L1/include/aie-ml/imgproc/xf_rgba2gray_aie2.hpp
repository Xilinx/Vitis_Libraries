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

#include <adf.h>
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>
#include <common/xf_aie_hw_utils.hpp>
//#include <common/xf_aie_hw_utils.hpp>
// #include <stdio.h>
// #include <iostream>

#ifndef __XF_RGBA2GRAY__HPP__
#define __XF_RGBA2GRAY__HPP__

namespace xf {
namespace cv {
namespace aie {

class Rgba2Gray {
   private:
    static constexpr int VECTORIZATION_FACTOR = 32;

   public:
    void runImpl(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out);
    void xf_rgba2gray(uint8_t* ptr1, uint8_t* out_ptr, uint16_t tile_width, uint16_t tile_height);
};

__attribute__((noinline)) void Rgba2Gray::xf_rgba2gray(uint8_t* restrict ptr1,
                                                       uint8_t* restrict ptr_out,
                                                       uint16_t tile_width,
                                                       uint16_t tile_height) {
    ::aie::vector<uint8_t, 16> wt(77, 150, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ::aie::vector<uint8_t, 32> wt_blue = ::aie::broadcast<uint8_t, 32>(29);
    ::aie::vector<uint8_t, 32> rgba_channel0, rgba_channel1, rgba_channel3, rgba_channel2;
    ::aie::vector<uint8_t, 32> r, g, b, gray;
    ::aie::accum<acc32, VECTORIZATION_FACTOR> acc;
    uint16_t more_pixels = 0, loop_count;
    loop_count = (tile_height * tile_width) >> 5; // Divide by VECTORIZATION-FACTOR - ASSUMING VEC-FACT = 32*

    for (int j = 0; j < loop_count; j += 1) {
        // READ 32-bit RGBA channels of 32 pixels. Total 1024 bits.
        rgba_channel0 = ::aie::load_v<32>(ptr1);
        ptr1 += 32;
        rgba_channel1 = ::aie::load_v<32>(ptr1);
        ptr1 += 32;
        rgba_channel2 = ::aie::load_v<32>(ptr1);
        ptr1 += 32;
        rgba_channel3 = ::aie::load_v<32>(ptr1);
        ptr1 += 32;

        // Unzip the interleaved channels
        auto[rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(rgba_channel0, rgba_channel1),
                                                         ::aie::concat(rgba_channel2, rgba_channel3), 2);
        r = ::aie::filter_even(rg_temp, 1);
        g = ::aie::filter_odd(rg_temp, 1);
        b = ::aie::filter_even(ba_temp, 1);

        // MAC operations and store
        acc = ::aie::mul(b, wt_blue);
        acc = ::aie::accumulate<VECTORIZATION_FACTOR>(acc, wt, 0, r, g);
        gray = acc.template to_vector<uint8_t>(8);
        ::aie::store_v((uint8_t*)ptr_out, gray);
        ptr_out = ptr_out + VECTORIZATION_FACTOR;
    }

    // Check if more pixels to be processed? // No. of more pixels to be processed
    more_pixels = (tile_height * tile_width) - (loop_count * VECTORIZATION_FACTOR);

    // If more pixels to be processed, then move the pointers back so that we have 32 pixels to process
    if (more_pixels != 0) {
        // Find the pixel-shift requried to process 32 pixels at once
        more_pixels = VECTORIZATION_FACTOR - more_pixels;

        // Each input pixel is 32 bit (4 uint8_t). So pointer moved back with (pixel-shift*4)
        ptr1 = ptr1 - (more_pixels << 2);

        // Each output pixel is 8 bit (1 uint8_t). So pointer moved back with (pixel-shift*1)
        ptr_out = ptr_out - more_pixels;

        // Repeat as above loop
        rgba_channel0 = ::aie::load_unaligned_v<32>(ptr1);
        ptr1 += 32;
        rgba_channel1 = ::aie::load_unaligned_v<32>(ptr1);
        ptr1 += 32;
        rgba_channel2 = ::aie::load_unaligned_v<32>(ptr1);
        ptr1 += 32;
        rgba_channel3 = ::aie::load_unaligned_v<32>(ptr1);
        auto[rg_temp, ba_temp] = ::aie::interleave_unzip(::aie::concat(rgba_channel0, rgba_channel1),
                                                         ::aie::concat(rgba_channel2, rgba_channel3), 2);
        r = ::aie::filter_even(rg_temp, 1);
        g = ::aie::filter_odd(rg_temp, 1);
        b = ::aie::filter_even(ba_temp, 1);

        acc = ::aie::mul(b, wt_blue);
        acc = ::aie::accumulate<VECTORIZATION_FACTOR>(acc, wt, 0, r, g);
        gray = acc.template to_vector<uint8_t>(8);
        ::aie::store_unaligned_v((uint8_t*)ptr_out, gray);
    }
}

void Rgba2Gray::runImpl(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out) {
    uint8_t* img_in = (uint8_t*)::aie::begin(in);
    uint8_t* img_out = (uint8_t*)::aie::begin(out);

    int16_t tile_width = xfGetTileWidth(img_in);
    int16_t tile_height = xfGetTileHeight(img_in);

    if (tile_width == 0 || tile_height == 0) return;

    xfCopyMetaData(img_in, img_out);
    xfSetTileWidth(img_out, tile_width);

    xfUnsignedSaturation(img_out);

    uint8_t* in_ptr = (uint8_t*)xfGetImgDataPtr(img_in);
    uint8_t* out_ptr = (uint8_t*)xfGetImgDataPtr(img_out);

    xf_rgba2gray(in_ptr, out_ptr, tile_width, tile_height);
}
} // aie
} // cv
} // xf
#endif
