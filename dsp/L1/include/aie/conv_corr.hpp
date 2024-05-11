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
#ifndef _DSPLIB_CONV_CORR_HPP_
#define _DSPLIB_CONV_CORR_HPP_

/*
CONV_CORR
This file exists to capture the definition of the CONV_CORR kernel class.
The class definition holds defensive checks on parameter range and other legality.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which are not
included in aie graph level compilation.
*/

/* Coding conventions
TT_      template type suffix
TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include <vector>
#include <array>
#include "device_defs.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "conv_corr_traits.hpp"

using namespace adf;

#ifndef _DSPLIB_CONV_CORR_HPP_DEBUG_
//#define _DSPLIB_CONV_CORR_HPP_DEBUG_
#endif //_DSPLIB_CONV_CORR_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {
//-----------------------------------------------------------------------------------------------------
// Single kernel specialization for io bufer

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
class conv_corr {
   private:
    // constants derived from configuration parameters
    // number of multiplications per lane for main intrinsic
    static constexpr unsigned int m_kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>();

    // number of lanes that intrinsic would operate
    static constexpr unsigned int m_kLanes = getNumofLanes<TT_DATA_F, TT_DATA_G>();

    // number of points that intrinsic would operate
    static constexpr unsigned int m_kPoints = getNumofPoints<TT_DATA_F, TT_DATA_G>();

    // Loop count that aie would operate
    static constexpr unsigned int m_kPaddedLenData =
        getPaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

    // length of F vector that aie would operate
    static constexpr unsigned int m_kLoopCount = getLoopCount<TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

    // load max possible elements each time based on sample size from memory that aie would operate
    static constexpr unsigned int m_kVecLoadF = (MAX_BITS_LOAD_ON_AIE() / (aie_CC_Fsample_Size<TT_DATA_F>()));

    // load max possible elements each time based on sample size from memory that aie would operate
    static constexpr unsigned int m_kVecLoadG = (MAX_BITS_LOAD_ON_AIE() / (aie_CC_Gsample_Size<TT_DATA_G>()));

   public:
    // Constructor of con_corr class
    conv_corr() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr::conv_corrMain); }
    // Conv_Corr
    void conv_corrMain(input_buffer<TT_DATA_F>& __restrict inWindowF,
                       input_buffer<TT_DATA_G>& __restrict inWindowG,
                       output_buffer<TT_DATA_OUT>& __restrict outWindow);
};

} // namespace conv_corr
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_CONV_CORR_HPP_
