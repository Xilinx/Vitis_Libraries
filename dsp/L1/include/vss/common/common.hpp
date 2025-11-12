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
#ifndef _DSPLIB_COMMON_HPP_
#define _DSPLIB_COMMON_HPP_

namespace xf {
namespace dsp {
namespace vss {
namespace common {

template <unsigned int powerOf2>
constexpr unsigned int fnLog2() {
    if
        constexpr(powerOf2 == 32768) { return 15; }
    else if
        constexpr(powerOf2 == 16384) { return 14; }
    else if
        constexpr(powerOf2 == 8192) { return 13; }
    else if
        constexpr(powerOf2 == 4096) { return 12; }
    else if
        constexpr(powerOf2 == 2048) { return 11; }
    else if
        constexpr(powerOf2 == 1024) { return 10; }
    else if
        constexpr(powerOf2 == 512) { return 9; }
    else if
        constexpr(powerOf2 == 256) { return 8; }
    else if
        constexpr(powerOf2 == 128) { return 7; }
    else if
        constexpr(powerOf2 == 64) { return 6; }
    else if
        constexpr(powerOf2 == 32) { return 5; }
    else if
        constexpr(powerOf2 == 16) { return 4; }
    else if
        constexpr(powerOf2 == 8) { return 3; }
    else if
        constexpr(powerOf2 == 4) { return 2; }
    else if
        constexpr(powerOf2 == 2) { return 1; }
    else if
        constexpr(powerOf2 == 1) { return 0; }
    else {
        return -1;
    }
};

template<unsigned int len, unsigned int rnd>
static constexpr unsigned int fnCeil(){
    return (len + rnd - 1)/rnd * rnd;
}

}}}}
#endif