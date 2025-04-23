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
#include <type_traits>

namespace us{
namespace L1{

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void CosV(adf::input_buffer< T >& input_vector, adf::output_buffer< T >& output_vector);


// nested called

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
inline void m_CosV(adf::input_buffer< int32 >& input_vector, adf::output_buffer< int32 >& output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
inline void m_CosV(adf::input_buffer< int16 >& input_vector, adf::output_buffer< int16 >& output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
inline void m_CosV(adf::input_buffer< float >& input_vector, adf::output_buffer< float >& output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
inline void m_CosV(adf::input_buffer< bfloat16 >& input_vector, adf::output_buffer< bfloat16 >& output_vector);

//----------------------------------------------

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void CosVInternalBuffer(adf::input_buffer< T >& input_vector, adf::output_buffer< T >& output_vector);


// nested called

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_CosV(T *input_vector, T *output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_CosV(int32 *input_vector, int32 *output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_CosV(int16 *input_vector, int16 *output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_CosV(float *input_vector, float *output_vector);

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_CosV(bfloat16 *input_vector, bfloat16 *output_vector);

//----------------------------------------------

// retrocompatibility

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void cosV(adf::input_buffer< T >& input_vector, adf::output_buffer< T >& output_vector);

}
}
