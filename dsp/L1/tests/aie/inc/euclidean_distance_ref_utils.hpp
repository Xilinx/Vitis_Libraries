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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_REF_UTILS_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_REF_UTILS_HPP_

/*
    EUCLIDEAN_DISTANCE reference model utility helpers
*/

#include <adf.h>
#include "fir_ref_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

static constexpr unsigned int kFixedDimOfED = 4; // Fixed spatial dimension: 4 coordinates per point (AOS layout)

} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_REF_UTILS_HPP_
