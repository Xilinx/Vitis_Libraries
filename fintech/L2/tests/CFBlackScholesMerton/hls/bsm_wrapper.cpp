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
/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/**
 * @file bsm_wrapper.cpp
 * @brief HLS wrapper for BSM kernel which parallelizes the single closed-form
 * solver
 */

#include <cmath>
#include <iostream>
#include "ap_fixed.h"
#include "hls_math.h"
#include "xf_fintech/cf_bsm.hpp"

#define SQRT2 1.4142135623730950488016887242097f
#define WORDS_PER_DDR 16
#define BITS_PER_DATA_TYPE 32

extern "C" {

/// @brief Wraps closed-form solver to allow parallel processing by accessing
/// the wide bus in a parallel.
/// This models how the kernel can interface with the DDR bus in a real
/// solution.
///
/// @param[in]  s_in      Input parameters read as a vector bus type
/// @param[in]  v_in      Input parameters read as a vector bus type
/// @param[in]  r_in      Input parameters read as a vector bus type
/// @param[in]  t_in      Input parameters read as a vector bus type
/// @param[in]  k_in      Input parameters read as a vector bus type
/// @param[in]  call      Controls whether call or put is calculated
/// @param[in]  num       Total number of input data sets to process
/// @param[out] price_out Output parameters read as a vector bus type
/// @param[out] delta_out Output parameters read as a vector bus type
/// @param[out] gamma_out Output parameters read as a vector bus type
/// @param[out] vega_out  Output parameters read as a vector bus type
/// @param[out] theta_out Output parameters read as a vector bus type
/// @param[out] rho_out   Output parameters read as a vector bus type
void bsm_wrapper(ap_uint<512>* s_in,
                 ap_uint<512>* v_in,
                 ap_uint<512>* r_in,
                 ap_uint<512>* t_in,
                 ap_uint<512>* k_in,
                 ap_uint<512>* q_in,
                 unsigned int call,
                 unsigned int num,
                 ap_uint<512>* price_out,
                 ap_uint<512>* delta_out,
                 ap_uint<512>* gamma_out,
                 ap_uint<512>* vega_out,
                 ap_uint<512>* theta_out,
                 ap_uint<512>* rho_out) {
/// @brief Define the AXI parameters.  Each input/output parameter has a
/// separate port
#pragma HLS INTERFACE m_axi port = s_in offset = slave bundle = d0_port
#pragma HLS INTERFACE m_axi port = v_in offset = slave bundle = d1_port
#pragma HLS INTERFACE m_axi port = r_in offset = slave bundle = d2_port
#pragma HLS INTERFACE m_axi port = t_in offset = slave bundle = d3_port
#pragma HLS INTERFACE m_axi port = k_in offset = slave bundle = d4_port
#pragma HLS INTERFACE m_axi port = q_in offset = slave bundle = d5_port
#pragma HLS INTERFACE m_axi port = price_out offset = slave bundle = d6_port
#pragma HLS INTERFACE m_axi port = delta_out offset = slave bundle = d7_port
#pragma HLS INTERFACE m_axi port = gamma_out offset = slave bundle = d8_port
#pragma HLS INTERFACE m_axi port = vega_out offset = slave bundle = d9_port
#pragma HLS INTERFACE m_axi port = theta_out offset = slave bundle = d10_port
#pragma HLS INTERFACE m_axi port = rho_out offset = slave bundle = d11_port

#pragma HLS INTERFACE s_axilite port = s_in bundle = control
#pragma HLS INTERFACE s_axilite port = v_in bundle = control
#pragma HLS INTERFACE s_axilite port = r_in bundle = control
#pragma HLS INTERFACE s_axilite port = t_in bundle = control
#pragma HLS INTERFACE s_axilite port = k_in bundle = control
#pragma HLS INTERFACE s_axilite port = q_in bundle = control
#pragma HLS INTERFACE s_axilite port = price_out bundle = control
#pragma HLS INTERFACE s_axilite port = delta_out bundle = control
#pragma HLS INTERFACE s_axilite port = gamma_out bundle = control
#pragma HLS INTERFACE s_axilite port = vega_out bundle = control
#pragma HLS INTERFACE s_axilite port = theta_out bundle = control
#pragma HLS INTERFACE s_axilite port = rho_out bundle = control

#pragma HLS INTERFACE s_axilite port = call bundle = control
#pragma HLS INTERFACE s_axilite port = num bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // Working variables for the vector processing
    float s[WORDS_PER_DDR];
    float v[WORDS_PER_DDR];
    float r[WORDS_PER_DDR];
    float t[WORDS_PER_DDR];
    float k[WORDS_PER_DDR];
    float q[WORDS_PER_DDR];

    float price[WORDS_PER_DDR];
    float delta[WORDS_PER_DDR];
    float gamma[WORDS_PER_DDR];
    float vega[WORDS_PER_DDR];
    float theta[WORDS_PER_DDR];
    float rho[WORDS_PER_DDR];

    unsigned int i = 0;
    unsigned int out_count = 0;
// This is an example of fully unrolling the inner loop.  This causes all 16
// words in the vector
// bus to be processed in parallel
ddr_word_loop:
    for (unsigned n = 0; n < num / WORDS_PER_DDR; n++) {
#pragma HLS PIPELINE II = 1

        for (unsigned int j = 0; j < WORDS_PER_DDR; ++j) {
#pragma HLS UNROLL

            unsigned int s_temp = s_in[n].range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j);
            s[j] = *(float*)(&s_temp);

            unsigned int v_temp = v_in[n].range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j);
            v[j] = *(float*)(&v_temp);

            unsigned int r_temp = r_in[n].range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j);
            r[j] = *(float*)(&r_temp);

            unsigned int t_temp = t_in[n].range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j);
            t[j] = *(float*)(&t_temp);

            unsigned int k_temp = k_in[n].range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j);
            k[j] = *(float*)(&k_temp);

            unsigned int q_temp = q_in[n].range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j);
            q[j] = *(float*)(&q_temp);
        }

        // Call WORDS_PER_DDR sub functions
        for (unsigned int j = 0; j < WORDS_PER_DDR; ++j) {
#pragma HLS UNROLL
            xf::fintech::cfBSMEngine<float>(s[j], v[j], r[j], t[j], k[j], q[j], call, &price[j], &delta[j], &gamma[j],
                                            &vega[j], &theta[j], &rho[j]);
        }

        // Vectorize result back to DDR size
        ap_uint<512> price_temp_wide = 0;
        ap_uint<512> delta_temp_wide = 0;
        ap_uint<512> gamma_temp_wide = 0;
        ap_uint<512> vega_temp_wide = 0;
        ap_uint<512> theta_temp_wide = 0;
        ap_uint<512> rho_temp_wide = 0;

        for (unsigned int j = 0; j < WORDS_PER_DDR; ++j) {
#pragma HLS UNROLL

            float price_temp;
            float delta_temp;
            float gamma_temp;
            float vega_temp;
            float theta_temp;
            float rho_temp;

            price_temp = price[j];
            price_temp_wide.range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j) =
                *(unsigned int*)(&price_temp);

            delta_temp = delta[j];
            delta_temp_wide.range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j) =
                *(unsigned int*)(&delta_temp);

            gamma_temp = gamma[j];
            gamma_temp_wide.range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j) =
                *(unsigned int*)(&gamma_temp);

            vega_temp = vega[j];
            vega_temp_wide.range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j) =
                *(unsigned int*)(&vega_temp);

            theta_temp = theta[j];
            theta_temp_wide.range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j) =
                *(unsigned int*)(&theta_temp);

            rho_temp = rho[j];
            rho_temp_wide.range(BITS_PER_DATA_TYPE * (j + 1) - 1, BITS_PER_DATA_TYPE * j) = *(unsigned int*)(&rho_temp);
        }

        price_out[n] = price_temp_wide;
        delta_out[n] = delta_temp_wide;
        gamma_out[n] = gamma_temp_wide;
        vega_out[n] = vega_temp_wide;
        theta_out[n] = theta_temp_wide;
        rho_out[n] = rho_temp_wide;
    }
}
}
