/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef KERNEL_IMG_HPP
#define KERNEL_IMG_HPP

namespace us {
namespace L1 {

using namespace adf;

template <class T>
struct para_img_t {
    T step;
    int32 iter_line;
    int32 iter_element;
    int32 iter_seg;
    int32 num_line;
    int32 num_element;
    int32 num_seg;
    para_img_t(T stp, int32 line, int32 element, int32 seg) {
        step = stp;
        iter_line = 0;
        iter_element = 0;
        iter_seg = 0;
        num_line = line;
        num_element = element;
        num_seg = seg;
    }
    void print() {
        printf("img: <%d/%d lines, %d/%d elements, %d/%d segments>\n", iter_line, num_line, iter_element, num_element,
               iter_seg, num_seg);
    }
    void update() {
        if (iter_seg != num_seg - 1)
            iter_seg++;
        else {
            iter_seg = 0;
            if (iter_element != num_element - 1)
                iter_element++;
            else {
                iter_element = 0;
                iter_line++;
            }
        }
    }
};

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_img_t,
          int LEN_OUT_img_t,
          int LEN32b_PARA_img_t>
void kfun_img_1d(const T (&para_const)[LEN32b_PARA_img_t],
                 const T (&para_start)[NUM_LINE_t],
                 output_buffer<T, adf::extents<LEN_OUT_img_t> >& out);

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_img_t,
          int LEN_OUT_img_t,
          int LEN32b_PARA_img_t>
void kfun_img_1d_shell(const T (&para_const)[LEN32b_PARA_img_t],
                       const T (&para_start)[NUM_LINE_t],
                       output_buffer<T, adf::extents<LEN_OUT_img_t> >& out);
}
}

#endif