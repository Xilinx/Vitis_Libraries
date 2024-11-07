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
/*
Function Approximation reference model
*/

#include "func_approx_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {

float chosenFunc(float dataIn, unsigned int funcEnum) {
    switch (funcEnum) {
        case SQRT_FUNC:
            return std::sqrt(dataIn);
        case INVSQRT_FUNC:
            return (1 / std::sqrt(dataIn));
        case LOG_FUNC:
            return std::log(dataIn);
        case EXP_FUNC:
            return std::exp(dataIn);
        case INV_FUNC:
            return (1 / dataIn);
        default:
            printf("Invalid FUNC_CHOICE, defaulting to sqrt()");
            return std::sqrt(dataIn);
    }
}
// Function Approximation Ref Base specialization
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_FUNC_CHOICE>
void func_approx_ref<TT_DATA,
                     TP_COARSE_BITS,
                     TP_FINE_BITS,
                     TP_DOMAIN_MODE,
                     TP_WINDOW_VSIZE,
                     TP_SHIFT,
                     TP_RND,
                     TP_SAT,
                     TP_FUNC_CHOICE>::approx_main(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow) {
    TT_DATA* inPtr = (TT_DATA*)inWindow.data();
    TT_DATA* outPtr = (TT_DATA*)outWindow.data();
    TT_DATA d_in;
    TT_DATA d_out;
    float d_in_float, temp;
    T_accRef<TT_DATA> accum;
    float domainBias = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    int maxDomain;
    if (std::is_same<TT_DATA, int16>::value || std::is_same<TT_DATA, int32>::value) {
        maxDomain = 1 << (TP_FINE_BITS + TP_COARSE_BITS - TP_DOMAIN_MODE);
    } else {
        maxDomain = 1;
    }

    for (unsigned int i = 0; i < TP_WINDOW_VSIZE; i++) {
        d_in = *inPtr++;
        d_in_float = ((float)d_in / maxDomain) + domainBias;
        temp = chosenFunc(d_in_float, TP_FUNC_CHOICE);
        accum = (T_accRef<TT_DATA>)(temp * maxDomain); // covert back to TT_DATA
        roundAcc(TP_RND, TP_SHIFT, accum);
        saturateAcc(accum, TP_SAT);
        d_out = castAcc(accum);
        *outPtr++ = d_out;
    }
};
}
}
}
}
