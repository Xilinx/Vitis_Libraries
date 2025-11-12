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
#ifndef _DSPLIB_CHOLESKY_REF_HPP_
#define _DSPLIB_CHOLESKY_REF_HPP_

/*
cholesky reference model
*/

#include <adf.h>
#include <limits>
#include <array>
#include "device_defs.h"
#include "fir_ref_utils.hpp"

using namespace adf;

// #define _DSPLIB_CHOLESKY_REF_DEBUG_

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES>
class cholesky_ref_base { // base class with all features except interface which is left to inherited
    private:
    public:
        // Constructor
        cholesky_ref_base() {}
};

//-----------------------------------------------------------------------------------------------------
// Cholesky - default/base 'specialization'
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES>
class cholesky_ref : public cholesky_ref_base<TT_DATA,
                                                TP_DIM,
                                                TP_NUM_FRAMES> {
    private:
    public:
        static constexpr unsigned int kVecSampleNum = __MAX_READ_WRITE__ / 8 / sizeof(TT_DATA);
        static constexpr unsigned int kNumVecsPerDim = TP_DIM / kVecSampleNum;

        // Constructor
        cholesky_ref() {}

        // Register Kernel Class
        static void registerKernelClass() { REGISTER_FUNCTION(cholesky_ref::cholesky_main); }

        void cholesky_main(input_buffer<TT_DATA>& inWindow0,
                            output_buffer<TT_DATA>& outWindow0);
};
}
}
}
} // closing namespace xf::dsp::aie::cholesky

#endif // _DSPLIB_CHOLESKY_REF_HPP_
