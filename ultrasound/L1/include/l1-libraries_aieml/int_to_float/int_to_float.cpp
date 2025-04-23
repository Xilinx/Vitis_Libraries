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
#include <int_to_float/int_to_float.hpp>

namespace us{
namespace L1{

template< unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void IntToFloat(adf::input_buffer< int32 >& input_vector, adf::output_buffer< float >& output_vector){

	aie::vector< int32, T_SIMD_DEPTH > op = aie::zeros< int32, T_SIMD_DEPTH >();
	aie::vector< float, T_SIMD_DEPTH > res = aie::zeros< float, T_SIMD_DEPTH >();

	auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_vector);
	auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);

	for(unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
		chess_prepare_for_pipelining{

		op = *iter_in;
		res = aie::to_float(op);
		*iter_out = res;

		iter_in++;
		iter_out++;

	}

}

template< unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void Int16ToBfloat(adf::input_buffer< int16 >& input_vector, adf::output_buffer< bfloat16 >& output_vector){

	aie::vector< int16, T_SIMD_DEPTH > op = aie::zeros< int16, T_SIMD_DEPTH >();
	aie::vector< bfloat16, T_SIMD_DEPTH > res = aie::zeros< bfloat16, T_SIMD_DEPTH >();

	auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_vector);
	auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);

	for(unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
		chess_prepare_for_pipelining{

		op = *iter_in;
		res = aie::to_float(op);
		*iter_out = res;

		iter_in++;
		iter_out++;

	}

}

//----------------------------------------------------------

template< unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void IntToFloatInternalBuffer(adf::input_buffer< int32 >& input_vector, adf::output_buffer< float >& output_vector){
	int32* buffer_in = (int32*)input_vector.data();
	float* buffer_out = (float*)output_vector.data();
	m_IntToFloat< T_LEN, T_INCREMENT, T_SIMD_DEPTH >(buffer_in, buffer_out);
}

template< unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void Int16ToBfloatInternalBuffer(adf::input_buffer< int16 >& input_vector, adf::output_buffer< bfloat16 >& output_vector){
	int16* buffer_in = (int16*)input_vector.data();
	bfloat16* buffer_out = (bfloat16*)output_vector.data();
	m_Int16ToBfloat< T_LEN, T_INCREMENT, T_SIMD_DEPTH >(buffer_in, buffer_out);
}

template< unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_IntToFloat(int32 *input_vector, float *output_vector){

	aie::vector< int32, T_SIMD_DEPTH > op = aie::zeros< int32, T_SIMD_DEPTH >();
	aie::vector< float, T_SIMD_DEPTH > res = aie::zeros< float, T_SIMD_DEPTH >();

	auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_vector);
	auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);

	for(unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
		chess_prepare_for_pipelining{

		op = *iter_in;
		res = aie::to_float(op);
		*iter_out = res;

		iter_in++;
		iter_out++;

	}

}

template< unsigned int T_LEN, unsigned int T_INCREMENT, unsigned int T_SIMD_DEPTH >
void m_Int16ToBfloat(int16 *input_vector, bfloat16 *output_vector){

	aie::vector< int16, T_SIMD_DEPTH > op = aie::zeros< int16, T_SIMD_DEPTH >();
	aie::vector< bfloat16, T_SIMD_DEPTH > res = aie::zeros< bfloat16, T_SIMD_DEPTH >();

	auto iter_in = aie::begin_vector< T_SIMD_DEPTH >(input_vector);
	auto iter_out = aie::begin_vector< T_SIMD_DEPTH >(output_vector);

	for(unsigned int i = 0; i < T_LEN; i += T_INCREMENT)
		chess_prepare_for_pipelining{

		op = *iter_in;
		res = aie::to_float(op);
		*iter_out = res;

		iter_in++;
		iter_out++;

	}

}


}
}



