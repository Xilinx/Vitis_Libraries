/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
//================================== End Lic =================================================
#ifndef _DATA_PATH_H_
#define _DATA_PATH_H_

#include <ap_fixed.h>
#include <complex>
#include "vt_fft.hpp"
#include "vt_fft_L2.hpp"
using namespace xf::dsp::fft;

// Define Numfer of FFTs, FFT Size, and Super Sample Rate
#define N_FFT 1024
#define FFT_LEN 1024
#define SSR 8

typedef complex_wrapper<float> T_in;
#define IID 0

// Define parameter structure for FFT
struct fftParams : ssr_fft_default_params {
    static const int NUM_FFT_MAX = N_FFT;
    static const int N = FFT_LEN;
    static const int R = SSR;
};

// typedef ssr_fft_output_type<fftParams,T_in>::t_ssr_fft_out T_out;
typedef T_in T_out;

#endif // _DATA_PATH_H_
