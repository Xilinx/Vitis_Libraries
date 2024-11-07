/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#pragma once

#ifndef _DSPLIB_FIR_PARAMS_DEFAULTS_HPP_
#define _DSPLIB_FIR_PARAMS_DEFAULTS_HPP_

#include <stdio.h>
#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {

struct fir_params_defaults {
   public:
    using BTT_DATA = cint16;
    using BTT_OUT_DATA = BTT_DATA;
    using BTT_COEFF = int16;
    static constexpr unsigned int Bdim = 0;
    static constexpr unsigned int BTP_FIR_LEN = 16;
    static constexpr unsigned int BTP_FIR_RANGE_LEN = 4;
    static constexpr unsigned int BTP_SHIFT = 0;
    static constexpr unsigned int BTP_RND = 0;
    static constexpr unsigned int BTP_TDM_CHANNELS = 1;
    static constexpr unsigned int BTP_INTERPOLATE_FACTOR = 1;
    static constexpr unsigned int BTP_DECIMATE_FACTOR = 1;
    static constexpr unsigned int BTP_INPUT_WINDOW_VSIZE = 256;
    static constexpr unsigned int BTP_CASC_LEN = 1;
    static constexpr unsigned int BTP_USE_COEFF_RELOAD = 0;
    static constexpr unsigned int BTP_NUM_OUTPUTS = 1;
    static constexpr unsigned int BTP_DUAL_IP = 0;
    static constexpr unsigned int BTP_API = 0;
    static constexpr unsigned int BTP_SSR = 1;
    static constexpr unsigned int BTP_COEFF_PHASE = 0;
    static constexpr unsigned int BTP_COEFF_PHASE_OFFSET = 0;
    static constexpr unsigned int BTP_COEFF_PHASES = 1;
    static constexpr unsigned int BTP_COEFF_PHASES_LEN = BTP_FIR_RANGE_LEN * BTP_COEFF_PHASES;
    static constexpr unsigned int BTP_PARA_INTERP_POLY = 1;
    static constexpr unsigned int BTP_PARA_DECI_POLY = 1;
    static constexpr unsigned int BTP_PARA_INTERP_INDEX = 0;
    static constexpr unsigned int BTP_PARA_DECI_INDEX = 0;
    static constexpr bool BTP_CASC_IN = false;
    static constexpr bool BTP_CASC_OUT = false;
    static constexpr int BTP_MODIFY_MARGIN_OFFSET = 0;
    static constexpr unsigned int BTP_KERNEL_POSITION = 0;
    static constexpr unsigned int BTP_SAT = 1;
    static constexpr unsigned int BTP_SSR_MODE = 0; // 0 - default: decompose to array of TP_SSR^2; 1 - decompose to a
                                                    // vector of TP_SSR, where kernels form independent paths.
};
template <typename fp = fir_params_defaults>
void printParams() {
    printf("FIR Params: \n");
    printf("Bdim                      = %d.\n", fp::Bdim);
    printf("BTP_FIR_LEN               = %d.\n", fp::BTP_FIR_LEN);
    printf("BTP_FIR_RANGE_LEN         = %d.\n", fp::BTP_FIR_RANGE_LEN);
    printf("BTP_SHIFT                 = %d.\n", fp::BTP_SHIFT);
    printf("BTP_RND                   = %d.\n", fp::BTP_RND);
    printf("BTP_TDM_CHANNELS          = %d.\n", fp::BTP_TDM_CHANNELS);
    printf("BTP_INTERPOLATE_FACTOR    = %d.\n", fp::BTP_INTERPOLATE_FACTOR);
    printf("BTP_DECIMATE_FACTOR       = %d.\n", fp::BTP_DECIMATE_FACTOR);
    printf("BTP_INPUT_WINDOW_VSIZE    = %d.\n", fp::BTP_INPUT_WINDOW_VSIZE);
    printf("BTP_CASC_LEN              = %d.\n", fp::BTP_CASC_LEN);
    printf("BTP_USE_COEFF_RELOAD      = %d.\n", fp::BTP_USE_COEFF_RELOAD);
    printf("BTP_NUM_OUTPUTS           = %d.\n", fp::BTP_NUM_OUTPUTS);
    printf("BTP_DUAL_IP               = %d.\n", fp::BTP_DUAL_IP);
    printf("BTP_API                   = %d.\n", fp::BTP_API);
    printf("BTP_SSR                   = %d.\n", fp::BTP_SSR);
    printf("BTP_COEFF_PHASE           = %d.\n", fp::BTP_COEFF_PHASE);
    printf("BTP_COEFF_PHASE_OFFSET    = %d.\n", fp::BTP_COEFF_PHASE_OFFSET);
    printf("BTP_COEFF_PHASES          = %d.\n", fp::BTP_COEFF_PHASES);
    printf("BTP_COEFF_PHASES_LEN      = %d.\n", fp::BTP_COEFF_PHASES_LEN);
    printf("BTP_PARA_INTERP_INDEX     = %d.\n", fp::BTP_PARA_INTERP_INDEX);
    printf("BTP_PARA_DECI_INDEX       = %d.\n", fp::BTP_PARA_DECI_INDEX);
    printf("BTP_PARA_INTERP_POLY      = %d.\n", fp::BTP_PARA_INTERP_POLY);
    printf("BTP_PARA_DECI_POLY        = %d.\n", fp::BTP_PARA_DECI_POLY);
    printf("BTP_CASC_IN               = %d.\n", fp::BTP_CASC_IN);
    printf("BTP_CASC_OUT              = %d.\n", fp::BTP_CASC_OUT);
    printf("BTP_MODIFY_MARGIN_OFFSET  = %d.\n", fp::BTP_MODIFY_MARGIN_OFFSET);
    printf("BTP_KERNEL_POSITION       = %d.\n", fp::BTP_KERNEL_POSITION);
    printf("BTP_RND                   = %d.\n", fp::BTP_SAT);
    printf("BTP_SSR_MODE              = %d.\n", fp::BTP_SSR_MODE);
}
template <typename fp = fir_params_defaults>
class fir_type_default {
   public:
    static constexpr unsigned int getKernelFirRangeLen();

    static constexpr unsigned int getFirRangeLen();

    static constexpr unsigned int getTapLen();

    static constexpr unsigned int getDF();

    static constexpr unsigned int getSSRMargin();

    static constexpr unsigned int getFirType();
};
}
}
}
#endif // _DSPLIB_FIR_PARAMS_DEFAULTS_HPP_
