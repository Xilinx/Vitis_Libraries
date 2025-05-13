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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_REF_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_REF_HPP_

/*
    EUCLIDEAN_DISTANCE reference model
*/

#include <adf.h>
#include <limits>
#include "euclidean_distance_ref_utils.hpp"

#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_REF_DEBUG_
//#define _DSPLIB_EUCLIDEAN_DISTANCE_REF_DEBUG_
#endif //_DSPLIB_EUCLIDEAN_DISTANCE_REF_DEBUG_

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

//-----------------------------------------------------------
// EUCLIDEAN_DISTANCE reference model class
template <typename TT_DATA,
          typename TT_DATA_OUT,
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_IS_OUTPUT_SQUARED>
class euclidean_distance_ref {
   private:
   public:
    // Default Constructor
    euclidean_distance_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(euclidean_distance_ref::euclidean_distanceMain); }

    // Declaration of Euclidean Distance Main Function call.
    void euclidean_distanceMain(input_buffer<TT_DATA>& inWindowP,
                                input_buffer<TT_DATA>& inWindowQ,
                                output_buffer<TT_DATA_OUT>& outWindow);
};

} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_REF_HPP_