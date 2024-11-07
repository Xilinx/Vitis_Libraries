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
#ifndef _DSPLIB_TPOSE_REF_CPP_
#define _DSPLIB_TPOSE_REF_CPP_

/*
DFT single channel reference model
*/

#ifndef _DSPLIB_DFT_REF_DEBUG_
//#define _DSPLIB_DFT_REF_DEBUG_
#endif //_DSPLIB_DFT_REF_DEBUG_
#include "device_defs.h"
#include <adf.h>
#include <limits>

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace tpose {

//-----------------------------------------------------------------------------------------------------
// DFT single channel reference model class
template <typename TT_DATA, // type of data input and output
          unsigned int TP_DIM1,
          unsigned int TP_DIM2>
class transpose_ref {
   public:
    // Constructor
    transpose_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(transpose_ref::transposeRefMain); }
    // FFT
    void transposeRefMain(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow);
};
}
}
}
} // namespace

#endif // _DSPLIB_TPOSE_REF_CPP_
