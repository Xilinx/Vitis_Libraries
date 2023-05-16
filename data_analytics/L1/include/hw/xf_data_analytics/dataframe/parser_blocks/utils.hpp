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

/**
 * This file is part of Vitis Data Analytics Library.
 */
#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_UTILS_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_UTILS_HPP

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

inline double bitsToDouble(uint64_t in) {
    union {
        uint64_t __I;
        double __D;
    } __T;
    __T.__I = in;
    return __T.__D;
}

inline uint64_t doubleToBits(double __V) {
    union {
        uint64_t __I;
        double __D;
    } __T;
    __T.__D = __V;
    return __T.__I;
}
inline float bitsToFloat(uint32_t in) {
    union {
        uint32_t __I;
        double __F;
    } __T;
    __T.__I = in;
    return __T.__F;
}

inline uint32_t floatToBits(float __V) {
    union {
        uint32_t __I;
        float __F;
    } __T;
    __T.__F = __V;
    return __T.__I;
}
}
}
}
}
#endif
