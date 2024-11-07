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
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "transpose_ref.hpp"
#include "fir_ref_utils.hpp" // for saturateAcc
#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace tpose {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_DIM1, unsigned int TP_DIM2>
void transpose_ref<TT_DATA, TP_DIM1, TP_DIM2>::transposeRefMain(input_buffer<TT_DATA>& inWindow,
                                                                output_buffer<TT_DATA>& outWindow) {
    TT_DATA* inPtr = inWindow.data();
    TT_DATA* outPtr = outWindow.data();
    for (unsigned int op = 0; op < TP_DIM1 * TP_DIM2; op++) {
        outPtr[op] = inPtr[op / TP_DIM2 + (op % TP_DIM2) * TP_DIM1];
    }
};
}
}
}
}
