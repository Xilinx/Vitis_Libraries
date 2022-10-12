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

namespace xf {
namespace cv {
namespace aie {
template <int WIDTH_IN, int HEIGHT_IN, int WIDTH_OUT, int HEIGHT_OUT, int IMG_HEIGHT_OUT>
class Resize {
    uint16_t (&mPos)[WIDTH_OUT];
    uint8_t (&mwtsX)[WIDTH_OUT << 2];
    uint8_t (&mwtsY)[IMG_HEIGHT_OUT];

   public:
    Resize(uint16_t (&pos)[WIDTH_OUT], uint8_t (&wtsx)[WIDTH_OUT << 2], uint8_t (&wtsy)[IMG_HEIGHT_OUT])
        : mPos(pos), mwtsX(wtsx), mwtsY(wtsy) {}
    void runImpl(uint8* input, uint8* output, const uint16* pos, const uint8* weightx, const uint8 weighty);
    void runImpl(uint8* input, uint8* output, int row);
    void runImpl(input_window<uint8>* input, output_window<uint8>* output);
};

template <int WIDTH_IN, int HEIGHT_IN, int WIDTH_OUT, int HEIGHT_OUT, int IMG_HEIGHT_OUT>
__attribute__((noinline)) void Resize<WIDTH_IN, HEIGHT_IN, WIDTH_OUT, HEIGHT_OUT, IMG_HEIGHT_OUT>::runImpl(
    uint8* input, uint8* output, const uint16* pos, const uint8* weightx, const uint8 weighty) {
    int32* img_in_ptr = (int32*)input;
    uint8* img_out_ptr = (uint8*)output;

    ::aie::vector<int32, 16> input_vector;
    ::aie::vector<uint8, 32> wt_row = ::aie::zeros<uint8, 32>();
    ::aie::vector<uint8, 16> wy;

    for (int i = 0; i < 8; i++) chess_unroll_loop() {
            wy[i] = weighty;
            wy[i + 8] = (255 - weighty);
        }

    for (int i = 0; i < WIDTH_OUT; i += 4) chess_prepare_for_pipelining chess_loop_range(14, ) {
            for (int j = 0; j < 4; j += 1) chess_unroll_loop() {
                    input_vector[j] = img_in_ptr[pos[i + j]];
                    input_vector[j + 4] = img_in_ptr[pos[i + j] + 1];
                    input_vector[j + 8] = img_in_ptr[WIDTH_IN + pos[i + j]];
                    input_vector[j + 12] = img_in_ptr[WIDTH_IN + pos[i + j] + 1];
                }

            auto wx = ::aie::load_v<16>(weightx);

            auto acc_wt = ::aie::mul(wx, wy);
            wt_row.insert(0, acc_wt.template to_vector<uint8>(8));

            auto data_vec = input_vector.template cast_to<uint8>();

            ::aie::accum<acc32, 16> acc =
                ::mul16(data_vec, 0, 0x33323130, 32, 0x3120, wt_row, 0, 0x33221100, 8, 0x3210);
            ::aie::store_v(img_out_ptr, acc.template to_vector<uint8>(8));

            weightx += 16;
            img_out_ptr += 16;
        }
}

template <int WIDTH_IN, int HEIGHT_IN, int WIDTH_OUT, int HEIGHT_OUT, int IMG_HEIGHT_OUT>
void Resize<WIDTH_IN, HEIGHT_IN, WIDTH_OUT, HEIGHT_OUT, IMG_HEIGHT_OUT>::runImpl(uint8* input, uint8* output, int row) {
    runImpl(input, output, mPos, mwtsX, mwtsY[row]);
}

template <int WIDTH_IN, int HEIGHT_IN, int WIDTH_OUT, int HEIGHT_OUT, int IMG_HEIGHT_OUT>
void Resize<WIDTH_IN, HEIGHT_IN, WIDTH_OUT, HEIGHT_OUT, IMG_HEIGHT_OUT>::runImpl(input_window<uint8>* input,
                                                                                 output_window<uint8>* output) {
    uint8* img_in_ptr = (uint8*)input->ptr;
    uint8* img_out_ptr = (uint8*)output->ptr;

    xfCopyMetaData(img_in_ptr, img_out_ptr);

    uint8* restrict ptr_in = (uint8*)xfGetImgDataPtr(img_in_ptr);
    uint8* restrict ptr_out = (uint8*)xfGetImgDataPtr(img_out_ptr);
    int row = xfGetTileOutPosV(img_in_ptr);

    runImpl(ptr_in, ptr_out, row);
}

} // aie
} // cv
} // xf

#endif
