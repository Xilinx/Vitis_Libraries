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

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>

namespace us{
namespace L1{

template <typename T, const unsigned T_LEN, const unsigned T_INCREMENT, const unsigned T_SIMD_DEPTH, unsigned T_SCALAR>
void LessOrEqualThanS(adf::input_buffer<T>& input_vector, adf::output_buffer<T>& output_vector);

template <typename T, const unsigned T_LEN, const unsigned T_INCREMENT, const unsigned T_SIMD_DEPTH, unsigned T_SCALAR>
void LessOrEqualThanSInternalBuffer(adf::input_buffer<T>& input_vector, adf::output_buffer<T>& output_vector);


// nested called

template <typename T, const unsigned T_LEN, const unsigned T_INCREMENT, const unsigned T_SIMD_DEPTH, unsigned T_SCALAR>
void m_LessOrEqualThanS(T *input_vector, T *output_vector);


// retrocompatibility

template <typename T, const unsigned T_LEN, const unsigned T_INCREMENT, const unsigned T_SIMD_DEPTH, unsigned T_SCALAR>
void lessOrEqualThanS(adf::input_buffer<T>& input_vector, adf::output_buffer<T>& __restrict output_vector);

}
}
