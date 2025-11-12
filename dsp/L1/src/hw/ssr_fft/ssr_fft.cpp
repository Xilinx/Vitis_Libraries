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
#ifndef SSR_FFT_CPP
#define SSR_FFT_CPP

#include "ssr_fft.h"
// #define __SSR_FFT_DEBUG__

void ssr_fft_wrapper(ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE, FFT_NIFFT>::TT_STREAM inData[POINT_SIZE],
                     ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE, FFT_NIFFT>::TT_STREAM outData[POINT_SIZE]) {
#pragma HLS interface mode = ap_ctrl_none port = return
#pragma HLS PIPELINE II = 1 style = flp
    static ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE, FFT_NIFFT> uut;
    uut.ssr_fft(inData, outData);
}
#endif