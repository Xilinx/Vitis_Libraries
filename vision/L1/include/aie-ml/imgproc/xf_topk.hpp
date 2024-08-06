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
#ifndef __XF_TOPK_HPP__
#define __XF_TOPK_HPP__

#include <adf.h>
#include <algorithm>
#include <aie_api/utils.hpp>
#include <aie_api/aie.hpp>
#include <stdio.h>

#include <common/xf_aie_hw_utils.hpp>

#define IN_T uint16
#define OUT_T uint16

namespace xf {
namespace cv {
namespace aie {

class TopK {
   public:
    void runImpl(IN_T* restrict _input,
                 IN_T* restrict _output1,
                 OUT_T* restrict _output2,
                 const int& num_elems,
                 const int& k,
                 const int& start_idx);
    void xf_topk(IN_T* restrict _input,
                 IN_T* restrict _output1,
                 OUT_T* restrict _output2,
                 const int& num_elems,
                 const int& k,
                 const int& start_idx);

    void runImpl(adf::input_buffer<IN_T>& input1,
                 adf::output_buffer<IN_T>& output1,
                 adf::output_buffer<OUT_T>& output2,
                 const int& num_elem,
                 const int& k,
                 const int& start_idx);
};
__attribute__((noinline)) void TopK::xf_topk(IN_T* restrict ptr_in_,
                                             IN_T* restrict ptr_out1_,
                                             OUT_T* restrict ptr_out2,
                                             const int& num_elems,
                                             const int& k,
                                             const int& start_idx) {
    bfloat16* ptr_in = (bfloat16*)ptr_in_;
    bfloat16* ptr_out1 = (bfloat16*)ptr_out1_;
    const int16_t total_input_elem = num_elems;
    const int topK = k;
    constexpr int VECF = 32;

    ::aie::vector<uint16_t, VECF> init_pos;
    for (int i = 0; i < VECF; i++) {
        init_pos[i] = i + start_idx;
    }

    for (int i = 0; i < topK; i++) {
        auto data = ::aie::load_v<VECF>(ptr_in);
        auto pos = init_pos;

        auto res = data;
        auto res_pos = pos;

        for (int j = VECF; j < total_input_elem; j += VECF) chess_prepare_for_pipelining chess_loop_range(, ) {
                data = ::aie::load_v<VECF>(ptr_in + j);
                pos = ::aie::add((uint16_t)VECF, pos);
                auto m = ::aie::gt(res, data);
                res = ::aie::select(data, res, m);
                res_pos = ::aie::select(pos, res_pos, m);
            }

        bfloat16 max_val = res[0];
        uint16_t max_pos = res_pos[0];
        for (int j = 0; j < VECF; j++) {
            if (res[j] > max_val) {
                max_val = res[j];
                max_pos = res_pos[j];
            }
        }

        // Storing result (5 cycles)
        ptr_in[max_pos - start_idx] = 0;
        ptr_out1[i] = max_val;
        ptr_out2[i] = max_pos;
    }
}

void TopK::runImpl(IN_T* restrict _input,
                   IN_T* restrict _output1,
                   OUT_T* restrict _output2,
                   const int& num_elems,
                   const int& k,
                   const int& start_idx) {
    xf_topk(_input, _output1, _output2, num_elems, k, start_idx);
}

void TopK::runImpl(adf::input_buffer<IN_T>& input1,
                   adf::output_buffer<IN_T>& output1,
                   adf::output_buffer<OUT_T>& output2,
                   const int& num_elems,
                   const int& k,
                   const int& start_idx) {
    IN_T* restrict in_ptr = (IN_T*)::aie::begin(input1);

    IN_T* restrict out_ptr1 = (IN_T*)::aie::begin(output1);
    OUT_T* restrict out_ptr2 = (OUT_T*)::aie::begin(output2);

    xfCopyMetaData(in_ptr, out_ptr1);
    xfCopyMetaData(in_ptr, out_ptr2);

    const int out_elem = k;
    int row = xf::cv::aie::xfGetTilePosV(out_ptr1);
    int outOffset = row * out_elem;
    xf::cv::aie::xfSetTileOutTWidth(out_ptr1, k);
    xf::cv::aie::xfSetTileOutTHeight(out_ptr1, 1);
    xf::cv::aie::xfSetTileOutOffset_L(out_ptr1, (outOffset & 0x0000ffff));
    xf::cv::aie::xfSetTileOutOffset_U(out_ptr1, (outOffset >> 16));

    int row1 = xf::cv::aie::xfGetTilePosV(out_ptr2);
    int outOffset1 = row1 * out_elem;
    xf::cv::aie::xfSetTileOutTWidth(out_ptr2, k);
    xf::cv::aie::xfSetTileOutTHeight(out_ptr2, 1);
    xf::cv::aie::xfSetTileOutOffset_L(out_ptr2, (outOffset1 & 0x0000ffff));
    xf::cv::aie::xfSetTileOutOffset_U(out_ptr2, (outOffset1 >> 16));

    IN_T* restrict ptr_in = (IN_T*)xfGetImgDataPtr(in_ptr);

    IN_T* restrict ptr_out1 = (IN_T*)xfGetImgDataPtr(out_ptr1);
    OUT_T* restrict ptr_out2 = (OUT_T*)xfGetImgDataPtr(out_ptr2);

    runImpl(ptr_in, ptr_out1, ptr_out2, num_elems, k, start_idx);
}

} // namespace aie
} // namespace cv
} // namespace xf

#endif
