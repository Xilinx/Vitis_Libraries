/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef DATAMOVER_TYPES_HPP
#define DATAMOVER_TYPES_HPP

#ifndef WARN_DEPR_UTILS_MOVER
#define WARN_DEPR_UTILS_MOVER
#warning Data Mover APIs in Utility Library have been moved to top-level Data Mover Library in 2023.1. Its files would be removed from Utility Library in future release.
#endif

#include <ap_int.h>
#include <stdint.h>

namespace xf {
namespace datamover {

enum MoverMode { MODE_PRELOAD = 0, MODE_RUN };

/// Data type for loading constant into RAM.
struct ConstData {
    enum { Port_Width = 32 };
    typedef ap_uint<Port_Width> type;
};

/// Data type for writing check result into DDR.
struct CheckResult {
    enum { Port_Width = 8 };
    typedef ap_uint<Port_Width> type;
};

} /* datamover */
} /* xf */
#endif
