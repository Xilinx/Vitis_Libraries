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
#ifndef _SOLVERLIB_QRD_HH_REF_TRAITS_HPP_
#define _SOLVERLIB_QRD_HH_REF_TRAITS_HPP_

#include "fir_ref_utils.hpp"
#include "single_mul_ref_out_types.hpp"
#include "single_mul_ref_acc_types.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {

template <typename T_DATA>
struct constVals {
    static constexpr T_DATA c0 = T_DATA(0.0);
    static constexpr T_DATA c1 = T_DATA(1.0);
    static constexpr T_DATA c2 = T_DATA(2.0);
};

template <>
struct constVals <cfloat> {
    static constexpr cfloat c0 = {0.0, 0.0};
    static constexpr cfloat c1 = {1.0, 0.0};
    static constexpr cfloat c2 = {2.0, 0.0};
};

}
}
}
}

#endif
