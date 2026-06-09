/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

/**
 * HLS top: Stockham DIT radix-2 FFT (vitis_fft_variant).
 * FFT size = NUM_FFT (power of 2), set at compile time via -DNUM_FFT.
 */
#include "fft_1d_variant_types.hpp"
#include "fft1d_variant_r2_stages.hpp"

using namespace xf::dsp::fft;

#ifndef NUM_FFT
#define NUM_FFT 4096
#endif

void fft1d_variant_r2_stages_top(hls::stream<struct_fft_ssr2>& strm_in,
                                 hls::stream<struct_fft_ssr2>& strm_out,
                                 hls::stream<ap_uint<2> >& strm_token_in,
                                 hls::stream<ap_uint<2> >& strm_token_out) {
#pragma HLS INTERFACE axis port = strm_in
#pragma HLS INTERFACE axis port = strm_out
#pragma HLS INTERFACE axis port = strm_token_in
#pragma HLS INTERFACE axis port = strm_token_out
#pragma HLS INTERFACE mode = s_axilite port = return bundle = control

    stockham_fft_top<NUM_FFT, struct_fft_ssr2>(strm_in, strm_out, strm_token_in, strm_token_out);
}
