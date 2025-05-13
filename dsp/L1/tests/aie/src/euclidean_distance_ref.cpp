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
/*
    euclidean_distance reference model compute the euclidean distance of 2 points with n dimensions
*/
#include <adf.h>
#include "aie_api/aie_adf.hpp"
#include "euclidean_distance_ref_utils.hpp"
#include "euclidean_distance_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

// Example test for euclidean_distance of 2 points in n dimensions.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_DATA_OUT,
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_IS_OUTPUT_SQUARED>
void euclidean_distance_ref<TT_DATA, TT_DATA_OUT, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED>::
    euclidean_distanceMain(input_buffer<TT_DATA>& inWindowP,
                           input_buffer<TT_DATA>& inWindowQ,
                           output_buffer<TT_DATA_OUT>& outWindow) {
    unsigned int loopCount = TP_LEN;   // Number of iterations
    unsigned int nDim = kFixedDimOfED; // Number of dimensions

    TT_DATA inDataP;         // input data of P Sig.
    TT_DATA inDataQ;         // input data of Q Sig.
    TT_DATA_OUT outData;     // output data of eucliean distance.
    T_accRef<TT_DATA> accum; // declaration of accumulator

    // Pointers to fetch P and Q data
    TT_DATA* inPtrP = (TT_DATA*)inWindowP.data();         // pointer to P data
    TT_DATA* inPtrQ = (TT_DATA*)inWindowQ.data();         // pointer to Q data
    TT_DATA_OUT* outPtr = (TT_DATA_OUT*)outWindow.data(); // pointer to output of eucliean distance.

    // Computation of Euclidean Distance between P and Q
    for (unsigned int i = 0; i < loopCount; i++) {
        accum = null_accRef<TT_DATA>(); // Initialize accumulator
        for (unsigned int dimIndx = 0; dimIndx < nDim; dimIndx++) {
            inDataP = *inPtrP++; // Fetch P sample
            inDataQ = *inPtrQ++; // Fetch Q Sample
            TT_DATA diff = (inDataQ - inDataP);
            accum.real += (float)(diff * diff); // Accumulate the squared difference
        }
        outData = castAcc<TT_DATA>(accum); // writing output results to vector from accumulator
        if (TP_IS_OUTPUT_SQUARED == 0) {
            outData = sqrtf(outData);
        }
        // Store the results to io buffer of out.
        *outPtr++ = outData;
    } // End of Loop {
};    // End of euclidean_distance_ref() {

} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {