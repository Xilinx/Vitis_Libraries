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
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_IS_OUTPUT_SQUARED>
void euclidean_distance_ref<TT_DATA, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED>::
    euclidean_distanceMain(input_buffer<TT_DATA>& inWindowP,
                           input_buffer<TT_DATA>& inWindowQ,
                           output_buffer<TT_DATA>& outWindow) {
    constexpr unsigned int kDim = kFixedDimOfED; // Fixed spatial dimension (4 coords per point)

    TT_DATA outData;         // output euclidean distance result
    TT_DATA resOfSub[kDim];  // per-dimension (Q - P) differences
    T_accRef<TT_DATA> accum; // accumulator for summing squared differences

    // Pointers to fetch P and Q data
    TT_DATA* inPtrP = (TT_DATA*)inWindowP.data(); // pointer to P input data
    TT_DATA* inPtrQ = (TT_DATA*)inWindowQ.data(); // pointer to Q input data
    TT_DATA* outPtr = (TT_DATA*)outWindow.data(); // pointer to euclidean distance output

    // Computation of Euclidean Distance between P and Q
    for (unsigned int i = 0; i < TP_LEN; i++) {
        accum = null_accRef<TT_DATA>(); // Initialize accumulator

        // Compute (Q - P) for each of the kDim spatial dimensions
        for (unsigned int dimIndx = 0; dimIndx < kDim; dimIndx++) {
            TT_DATA inDataP = *inPtrP++; // Fetch P sample
            TT_DATA inDataQ = *inPtrQ++; // Fetch Q sample
            resOfSub[dimIndx] = (inDataQ - inDataP);
        }

        // Accumulate squared differences over TP_DIM active dimensions
        for (unsigned int dataIndx = 0; dataIndx < TP_DIM; dataIndx++) {
            accum.real += (float)(resOfSub[dataIndx] * resOfSub[dataIndx]);
        }

        // Cast accumulator result to output type
        outData = castAcc<TT_DATA>(accum);

        // If output is not squared, take the square root of the accumulated value.
        if (TP_IS_OUTPUT_SQUARED == 0) {
            outData = sqrtf(outData);
        }

        // Store the result to the output io buffer
        *outPtr++ = outData;

    } // End of Loop
};    // End of euclidean_distanceMain()

} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {