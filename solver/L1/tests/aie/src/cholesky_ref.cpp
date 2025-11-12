/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
Cholesky reference model
*/
#include "device_defs.h"
#include "cholesky_ref.hpp"
#include "cholesky_ref_utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "fir_ref_utils.hpp"
#include <cassert>

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {

#define IDX(i, j)  (i)*TP_DIM + (j)

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES>
void cholesky_ref<TT_DATA,
                    TP_DIM,
                    TP_NUM_FRAMES>::cholesky_main(input_buffer<TT_DATA>& inWindow0,
                                                    output_buffer<TT_DATA>& outWindow0) {
    TT_DATA* inPtr = (TT_DATA*)inWindow0.data();
    TT_DATA* outPtr = (TT_DATA*)outWindow0.data();


    // Processing of one window
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {


        // Motivation: We're going to copy all the data across for comparison in our smoke tests.
        for (int i = 0; i < TP_DIM * TP_DIM; i++) {
            outPtr[i] = inPtr[i];
        }

        // The actual cholesky algorithm...
        for (int D = 0; D < TP_DIM; D++) {
            float diag = getReal<TT_DATA>( inPtr[IDX(D, D)] );
            float diag_inv_sqrt = 1.0 / std::sqrt(diag);

            for (int j = 0; j < TP_DIM; j++) {
                inPtr[IDX(D, j)] *= diag_inv_sqrt;
            }

            for (int i = D+1; i < TP_DIM; i++) {
                for (int j = D; j < TP_DIM; j++) {
                    TT_DATA subtractor = getConj<TT_DATA>(inPtr[IDX(D, i)]) * inPtr[IDX(D, j)];
                    inPtr[IDX(i, j)] -= subtractor;
                }
            }
        }


        // Flushing all of the copied input data out of the output buffer.
        for (int i = 0; i < TP_DIM * TP_DIM; i++) {
            outPtr[i] = zero<TT_DATA>();
        }

        // Copying over only the vectors which will be processed by the UUT...
        for (int i = 0; i < TP_DIM; i += kVecSampleNum) {
            for (int j = i; j < TP_DIM; j += kVecSampleNum) {

                // Populating in kVecSampleNum * kVecSampleNum blocks...
                for (int m = i; m < i+kVecSampleNum; m++) {
                    for(int n = j; n < j+kVecSampleNum; n++) {
                        outPtr[IDX(m, n)] = inPtr[IDX(m, n)];
                    }
                }
            }
        }


        inPtr += TP_DIM * TP_DIM;
        outPtr += TP_DIM * TP_DIM;
    }
}

};
}
}
} // closing namespace xf::dsp::aie::cholesky
