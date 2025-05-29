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

#include "common_defines.hpp"

#include "l1-libraries_aieml/mul/mul.hpp"
#include "l1-libraries_aieml/sum/sum.hpp"

namespace us {
namespace L2 {

template <typename T>
void CatmullRomA1A2(

    adf::input_buffer<T>& point_0,
    adf::input_buffer<T>& point_1,
    adf::input_buffer<T>& point_2,
    adf::input_buffer<T>& t1t,
    adf::input_buffer<T>& tt0,
    adf::input_buffer<T>& t2t,
    adf::input_buffer<T>& tt1,

    adf::output_buffer<T>& output_a1,
    adf::output_buffer<T>& output_a2);

template <typename T>
void CatmullRomA3B1(

    adf::input_buffer<T>& output_a1,
    adf::input_buffer<T>& output_a2,
    adf::input_buffer<T>& point_2,
    adf::input_buffer<T>& point_3,
    adf::input_buffer<T>& t3t,
    adf::input_buffer<T>& tt2,
    adf::input_buffer<T>& t2t,
    adf::input_buffer<T>& tt0,

    adf::output_buffer<T>& output_a3,
    adf::output_buffer<T>& output_b1);

template <typename T>
void CatmullRomB2C(

    adf::input_buffer<T>& output_a2,
    adf::input_buffer<T>& output_a3,
    adf::input_buffer<T>& output_b1,
    adf::input_buffer<T>& t3t,
    adf::input_buffer<T>& tt1,
    adf::input_buffer<T>& t2t,

    adf::output_buffer<T>& output_catmull_rom);
}
}
