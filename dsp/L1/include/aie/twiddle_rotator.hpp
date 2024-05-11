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
#ifndef _DSPLIB_TWIDDLE_ROTATOR_HPP_
#define _DSPLIB_TWIDDLE_ROTATOR_HPP_

#include <adf.h>
#include <vector>
#include "fir_utils.hpp"
using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace ifft_2d_aie_pl {

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr unsigned int fnSqrt() {
    unsigned int sqrtVal =
        TP_POINT_SIZE == 65536
            ? 256
            : TP_POINT_SIZE == 16384
                  ? 128
                  : TP_POINT_SIZE == 4096
                        ? 64
                        : TP_POINT_SIZE == 1024
                              ? 32
                              : TP_POINT_SIZE == 256 ? 16 : TP_POINT_SIZE == 64 ? 8 : TP_POINT_SIZE == 16 ? 4 : 0;
    return sqrtVal;
}

template <unsigned int TP_POINT_SIZE>
constexpr unsigned int fnLog2() {
    if
        constexpr(TP_POINT_SIZE == 65536) { return 16; }
    else if
        constexpr(TP_POINT_SIZE == 32768) { return 15; }
    else if
        constexpr(TP_POINT_SIZE == 16384) { return 14; }
    else if
        constexpr(TP_POINT_SIZE == 8192) { return 13; }
    else if
        constexpr(TP_POINT_SIZE == 4096) { return 12; }
    else if
        constexpr(TP_POINT_SIZE == 2048) { return 11; }
    else if
        constexpr(TP_POINT_SIZE == 1024) { return 10; }
    else if
        constexpr(TP_POINT_SIZE == 512) { return 9; }
    else if
        constexpr(TP_POINT_SIZE == 256) { return 8; }
    else if
        constexpr(TP_POINT_SIZE == 128) { return 7; }
    else if
        constexpr(TP_POINT_SIZE == 64) { return 6; }
    else if
        constexpr(TP_POINT_SIZE == 32) { return 5; }
    else if
        constexpr(TP_POINT_SIZE == 16) { return 4; }
    else {
        return 0;
    }
}

template <unsigned int TP_POINT_SIZE>
constexpr unsigned int fnPtSizeD1() {
    unsigned int sqrtVal =
        TP_POINT_SIZE == 65536
            ? 256
            : TP_POINT_SIZE == 32768
                  ? 256
                  : TP_POINT_SIZE == 16384
                        ? 128
                        : TP_POINT_SIZE == 8192
                              ? 128
                              : TP_POINT_SIZE == 4096
                                    ? 64
                                    : TP_POINT_SIZE == 2048
                                          ? 64
                                          : TP_POINT_SIZE == 1024
                                                ? 32
                                                : TP_POINT_SIZE == 512
                                                      ? 32
                                                      : TP_POINT_SIZE == 256
                                                            ? 16
                                                            : TP_POINT_SIZE == 128
                                                                  ? 16
                                                                  : TP_POINT_SIZE == 64
                                                                        ? 8
                                                                        : TP_POINT_SIZE == 32
                                                                              ? 8
                                                                              : TP_POINT_SIZE == 16 ? 4 : 0;
    return sqrtVal;
}

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_PT_SIZE_D1,
          unsigned int TP_PT_SIZE_D2,
          unsigned int TP_SSR,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_PHASE>
class twiddleRotator {
    int count = 0;

   public:
    // Constructor
    twiddleRotator() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(twiddleRotator::twiddleRotation); }

    // Main function
    void twiddleRotation(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
}
}
}
}
}
#endif