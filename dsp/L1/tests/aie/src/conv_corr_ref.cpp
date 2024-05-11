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
    Conv_Corr reference model implements Convolution/correlation of given 2 signals
*/
#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "conv_corr_ref_utils.hpp"
#include "conv_corr_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

// Example test for conv_corr of 2 signals
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT>
void conv_corr_ref<TT_DATA_F,
                   TT_DATA_G,
                   TT_DATA_OUT,
                   TP_FUNCT_TYPE,
                   TP_COMPUTE_MODE,
                   TP_F_LEN,
                   TP_G_LEN,
                   TP_SHIFT,
                   TP_API,
                   TP_RND,
                   TP_SAT>::conv_corrMain(input_buffer<TT_DATA_F>& inWindowF,
                                          input_buffer<TT_DATA_G>& inWindowG,
                                          output_buffer<TT_DATA_OUT>& outWindow) {
    unsigned int loopCount = CEIL(m_kLoopCount, m_kLanes); // loop count to iterate on each sample.

    TT_DATA_F inDataF;                  // input data of F Sig.
    TT_DATA_G inDataG, buffG[TP_G_LEN]; // input data of G Sig.
    TT_DATA_OUT outData;                // output data of conv/corr.
    T_accRef<TT_DATA_OUT> accum;        // declaration of accumulator

    // Pointers to fetch F and G data
    TT_DATA_F* inPtrF = (TT_DATA_F*)inWindowF.data();     // pointer to F data
    TT_DATA_G* inPtrG = (TT_DATA_G*)inWindowG.data();     // pointer to G data
    TT_DATA_OUT* outPtr = (TT_DATA_OUT*)outWindow.data(); // pointer to output of conv/corr.

    // Base pointers of F and G data
    TT_DATA_F* baseinPtrF = (TT_DATA_F*)inWindowF.data(); // base pointer which holds starting address of F data

    if (TP_FUNCT_TYPE == 1) {
        for (unsigned int i = 0; i < TP_G_LEN; i++) {
            buffG[(TP_G_LEN - 1) - i] = *inPtrG++; // for convolution, G signal has to be reversed
        }
    } else {
        for (unsigned int i = 0; i < TP_G_LEN; i++) {
            buffG[i] = *inPtrG++; // for correlation , G signal will be as it is
        }
    }

    // Convolution/correlation computation
    for (unsigned int i = 0; i < loopCount; i++) {
        accum =
            null_accRef<TT_DATA_OUT>(); // reset accumulator at the start of new multiply accumulation of each output
        inPtrF = baseinPtrF++;          // update pointer of F data
        inPtrG = &buffG[0];             // update pointer of G data

        // Inner Loop to do multiply and accum
        for (unsigned int j = 0; j < TP_G_LEN; j++) {
            inDataF = *inPtrF++; // Fetch F sample
            inDataG = *inPtrG++; // Fetch G Sample

            // Multiply and accumulate the result.
            multiplyAccum<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>(accum, inDataF, inDataG);
        }

        roundAcc(TP_RND, TP_SHIFT, accum);     // apply rounding if any bit errors
        saturateAcc(accum, TP_SAT);            // apply saturation if any overflow of data occurs.
        outData = castAcc<TT_DATA_OUT>(accum); // writing output results to vector from accumulator
        *outPtr++ = outData;                   // Store the results to io buffer of out.

    } // End of Loop {
};    // End of conv_corr_ref() {

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {