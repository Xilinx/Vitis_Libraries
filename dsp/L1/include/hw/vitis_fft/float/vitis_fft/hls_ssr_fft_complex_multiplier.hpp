/*
 * Copyright 2019 Xilinx, Inc.
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

// File Name  : hls_ssr_fft_complex_multiplier.hpp
#ifndef _HLS_SSR_FFT_COMPLEX_MULTIPLIER_H_
#define _HLS_SSR_FFT_COMPLEX_MULTIPLIER_H_
//#include <complex>

#include "vitis_fft/fft_complex.hpp"

namespace vitis {
namespace dsp {
namespace fft {
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template <typename T_op1, typename T_op2, typename T_prd>
void complexMultiply(complex_wrapper<T_op1> p_complexOp1,
                     complex_wrapper<T_op2> p_complexOp2,
                     complex_wrapper<T_prd>& p_product) {
#pragma HLS INLINE // recursive
    p_product.real(p_complexOp1.real() * p_complexOp2.real() - p_complexOp1.imag() * p_complexOp2.imag());
    p_product.imag(p_complexOp1.real() * p_complexOp2.imag() + p_complexOp1.imag() * p_complexOp2.real());
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
} // end namespace fft
} // end namespace dsp
} // namespace vitis
#endif //_HLS_SSR_FFT_COMPLEX_MULTIPLIER_H_