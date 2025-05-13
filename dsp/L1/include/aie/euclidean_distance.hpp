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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_HPP_

/*
EUCLIDEAN_DISTANCE
This file exists to capture the definition of the EUCLIDEAN_DISTANCE kernel class.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which can't be
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
#include "fir_utils.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "ed_sqrt_luts_floats.h"
#include "euclidean_distance_traits.hpp"

using namespace adf;

#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_HPP_DEBUG_
//#define _DSPLIB_EUCLIDEAN_DISTANCE_HPP_DEBUG_
#endif //_DSPLIB_EUCLIDEAN_DISTANCE_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {
//-----------------------------------------------------------------------------------------------------
// Single kernel specialization for io bufer

template <typename TT_DATA,
          typename TT_DATA_OUT,
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_IS_OUTPUT_SQUARED>
class euclidean_distance_squared {
   private:
    // constants derived from configuration parameters

    // number of multiplications per lane for main intrinsic
    static constexpr unsigned int m_kMuls = fnGetNumofMULs<TT_DATA>();

    // number of lanes that intrinsic would operate
    static constexpr unsigned int m_kLanes = fnGetNumofLanes<TT_DATA>();

    // load max possible elements each time based on sample size from memory that aie would operate
    static constexpr unsigned int m_kVecLoad = (kMaxBitsLoadOnAie / (fnSampleSize<TT_DATA>()));

   public:
    // Constructor of euclidean_distance class
    euclidean_distance_squared() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(euclidean_distance_squared::euclideanDistMain); }
    // Euclidean_Distance
    void euclideanDistMain(input_buffer<TT_DATA>& __restrict inWindowP,
                           input_buffer<TT_DATA>& __restrict inWindowQ,
                           output_buffer<TT_DATA_OUT>& __restrict squaredOut);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel specialization for io bufer

template <typename TT_DATA, typename TT_DATA_OUT, unsigned int TP_LEN, unsigned int TP_IS_OUTPUT_SQUARED>
class euclidean_distance {
   private:
    // constants derived from configuration parameters

    // number of lanes that intrinsic would operate
    static constexpr unsigned int m_kLanes = fnGetNumofLanes<TT_DATA_OUT>();

    // load max possible elements each time based on sample size from memory that aie would operate
    static constexpr unsigned int m_kVecLoad = (kMaxBitsLoadOnAie / (fnSampleSize<TT_DATA>()));

    // number of samples as per data type.
    static constexpr unsigned int m_ksampleSize = (fnSampleSize<TT_DATA_OUT>());

   public:
    // Constructor of euclidean_distance class
    euclidean_distance() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(euclidean_distance::euclideanDistMain); }
    // Euclidean_Distance
    void euclideanDistMain(input_buffer<TT_DATA>& __restrict squaredIn, output_buffer<TT_DATA_OUT>& __restrict sqrtOut);
};

} // namespace euclidean_distance
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_HPP_
