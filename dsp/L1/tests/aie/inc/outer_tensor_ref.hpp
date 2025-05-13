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
#ifndef _DSPLIB_OUTER_TENSOR_REF_HPP_
#define _DSPLIB_OUTER_TENSOR_REF_HPP_

/*
Outer Tensor Product reference model
*/

#include <adf.h>
#include <limits>
#include <array>
#include "device_defs.h"
#include "fir_ref_utils.hpp"
#include "outer_tensor_ref_utils.hpp"

using namespace adf;

//#define _DSPLIB_OUTER_TENSOR_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class outer_tensor_ref_base { // base class with all features except interface which is left to inherited
   private:
   public:
    // Constructor
    outer_tensor_ref_base() {}
};

//-----------------------------------------------------------------------------------------------------
// Outer Tensor - default/base 'specialization'
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class outer_tensor_ref : public outer_tensor_ref_base<TT_DATA_A,
                                                      TT_DATA_B,
                                                      TP_DIM_A,
                                                      TP_DIM_B,
                                                      TP_NUM_FRAMES,
                                                      TP_SHIFT,
                                                      TP_API,
                                                      TP_RND,
                                                      TP_SAT> {
   private:
   public:
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;

    // Constructor
    outer_tensor_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(outer_tensor_ref::outer_tensor_main); }

    void outer_tensor_main(input_buffer<TT_DATA_A>& inWindow0,
                           input_buffer<TT_DATA_B>& inWindow1,
                           output_buffer<out_t>& outWindow0);
};
}
}
}
} // closing namespace xf::dsp::aie::outer_tensor

#endif // _DSPLIB_OUTER_TENSOR_REF_HPP_
