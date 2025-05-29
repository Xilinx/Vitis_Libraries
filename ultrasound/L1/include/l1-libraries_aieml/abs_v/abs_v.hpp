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
#include "common_defines.hpp"

namespace us {
namespace L1 {

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void AbsV(adf::input_buffer<T_IN>& input_vector, adf::output_buffer<T_OUT>& output);

// nested called

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<int32>& input_vector, adf::output_buffer<int32>& output);

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<int16>& input_vector, adf::output_buffer<int16>& output);

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<float>& input_vector, adf::output_buffer<float>& output);

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(adf::input_buffer<cint16>& input_vector, adf::output_buffer<int32>& output);

//-----------------------------

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void AbsVInternalBuffer(adf::input_buffer<T_IN>& input_vector, adf::output_buffer<T_OUT>& output);

// nested called

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(int32* input_vector, int32* output);

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(int16* input_vector, int16* output);

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(float* input_vector, float* output);

template <typename T_IN, typename T_OUT, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH>
void m_AbsV(cint16* input_vector, int32* output);
}
}
