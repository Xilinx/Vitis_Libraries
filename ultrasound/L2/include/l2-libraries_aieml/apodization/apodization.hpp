/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#pragma once

#include "l1-libraries_aieml/tile_v/tile_v.hpp"
#include "l1-libraries_aieml/diff/diff.hpp"
#include "l1-libraries_aieml/mul/mul.hpp"
#include "l1-libraries_aieml/sum_axis_1/sum_axis_1.hpp"
#include "l1-libraries_aieml/abs_v/abs_v.hpp"
#include "l1-libraries_aieml/equal_s/equal_s.hpp"
#include "l1-libraries_aieml/sum/sum.hpp"
#include "l1-libraries_aieml/div/div.hpp"
#include "l1-libraries_aieml/cos_v/cos_v.hpp"
#include "l1-libraries_aieml/less_or_equal_than_s/less_or_equal_than_s.hpp"

namespace us {
namespace L2 {

template <typename T>
void Apodization1(

    adf::input_buffer<T>& image_points,
    adf::input_buffer<T>& apodization_reference,
    adf::input_buffer<T>& apodization_direction,

    adf::output_buffer<T>& apo_depth_output

    );

template <typename T>
void Apodization2(

    adf::input_buffer<T>& apo_depth,
    adf::input_buffer<T>& two,
    adf::input_buffer<T>& apo_distance,
    adf::input_buffer<T>& f_number,
    adf::input_buffer<T>& one,

    adf::output_buffer<T>& index_output

    );

template <typename T>
void ApodizationOutput(

    adf::input_buffer<T>& index_from_apo_2,
    adf::input_buffer<T>& one,
    adf::input_buffer<T>& pi,
    adf::input_buffer<T>& two,

    adf::output_buffer<T>& output_hanning

    );
}
}
