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
#ifndef _DSPLIB_DFT_FNS_HPP_
#define _DSPLIB_DFT_FNS_HPP_

#include <adf.h>
#include <vector>

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

#define TRUNC(x, y) (((x) / y) * y)

/**
 * @endcond
 */

/**
 * @ingroup dft_utils
 * @brief getWindowSize is utility to obtain the value for the window size for multiple frames
 * @param[out] windowSize size of the window that the users needs to input
 * @param[in] pointSize point size wanted for the dft
 * @param[in] frame number of frames
 * @param[in] TT_TWIDDLE twiddle type
 *
 */

template <typename T_T, unsigned int TP_POINT_SIZE, unsigned int TP_NUM_FRAMES>
constexpr unsigned int getWindowSize() {
    constexpr int kSamplesInVect = 256 / 8 / sizeof(T_T);
    constexpr int kPaddedSize = CEIL(TP_POINT_SIZE, kSamplesInVect);
    return TP_NUM_FRAMES * kPaddedSize;
}

/**
 * @endcond
 */

/**
 * @}
 */
}
}
}
}
}

#endif
