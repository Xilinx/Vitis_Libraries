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
#ifndef _DSPLIB_CONV_CORR_REF_HPP_
#define _DSPLIB_CONV_CORR_REF_HPP_

/*
    CONV_CORR reference model
*/

#include <adf.h>
#include <limits>
#include "conv_corr_ref_utils.hpp"

#ifndef _DSPLIB_CONV_CORR_REF_DEBUG_
//#define _DSPLIB_CONV_CORR_REF_DEBUG_
#endif //_DSPLIB_CONV_CORR_REF_DEBUG_

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

//-----------------------------------------------------------
// CONV_CORR reference model class
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
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES>
class conv_corr_ref {
   private:
    // constants derived from configuration parameters
    static constexpr unsigned int m_kLanes = ref_CC_NumLanes<TT_DATA_F, TT_DATA_G>();
    static constexpr unsigned int m_kPaddedDataLength =
        getRefPaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();
    static constexpr unsigned int m_kLoopCount = getLoopCount<TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

   public:
    // Default Constructor
    conv_corr_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr_ref::conv_corrMain); }

    // Conolution and Correlation
    void conv_corrMain(input_buffer<TT_DATA_F>& inWindowF,
                       input_buffer<TT_DATA_G>& inWindowG,
                       output_buffer<TT_DATA_OUT>& outWindow);
};

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_CONV_CORR_REF_HPP_