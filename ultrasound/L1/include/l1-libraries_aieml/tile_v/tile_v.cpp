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
#include "tile_v.hpp"

namespace us{
namespace L1{

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void TileV(adf::input_buffer< T >& input_vector, adf::output_buffer< T >& output_matrix){

	aie::vector< T, T_SIMD_DEPTH > op = aie::zeros< T, T_SIMD_DEPTH >();

	auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_vector);
	auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_matrix);

	op = *iter_in;

	for(unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
		chess_prepare_for_pipelining {

//		op = *iter_in;

		*iter_out = op;

		iter_out++;
//		iter_in++;
	}


}

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void TileVInternalBuffer(adf::input_buffer< T >& input_vector, adf::output_buffer< T >& output_matrix){

	T* buffer_in = (T*)input_vector.data();
	T* buffer_out = (T*)output_matrix.data();
	m_TileV< T, T_LEN, T_INCREMENT, T_SIMD_DEPTH >(buffer_in, buffer_out);

}


// nested called

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_TileV(T *input_vector, T *output_matrix){

	aie::vector< T, T_SIMD_DEPTH > op = aie::zeros< T, T_SIMD_DEPTH >();

	auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_vector);
	auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_matrix);

	op = *iter_in;

	for(unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
		chess_prepare_for_pipelining {
//		op = *iter_in;

		*iter_out = op;

		iter_out++;
//		iter_in++;
	}

}

//retrocompatibility

template< typename T, unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void tileV(adf::input_buffer< T >& input_vector, adf::output_buffer< T >& output_matrix){

	T* buffer_in = (T*)input_vector.data();
	T* buffer_out = (T*)output_matrix.data();
	m_TileV< T, T_LEN, T_INCREMENT, T_SIMD_DEPTH >(buffer_in, buffer_out);

}

}
}

