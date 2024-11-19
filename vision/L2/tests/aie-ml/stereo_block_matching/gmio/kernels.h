/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SBM_RUNNER_H
#define _SBM_RUNNER_H

#include <adf/window/types.h>
#include <adf/stream/types.h>
#include "adf.h"
#include "config.h"

template <int TILE_HEIGHT,
          int NO_DISPARITIES,
          int TILE_WINSIZE,
          int UNIQUENESS_RATIO,
          int TEXTURE_THRESHOLD,
          int FILTERED,
          int TILE_IN_HEIGHT>
class SbmRunner {
   public:
    void run(adf::input_buffer<uint8_t>& in_1,
             adf::input_buffer<uint8_t>& in_2,
             adf::input_buffer<uint8_t>& in_3,
             adf::input_buffer<uint8_t>& in_4,
             adf::input_buffer<uint8_t>& in_5,
             adf::output_buffer<int16_t>& out);

    static void registerKernelClass() { REGISTER_FUNCTION(SbmRunner::run); }
};

template <int TILE_HEIGHT, int NO_DISPARITIES, int TILE_WINSIZE>
class SobelRunner {
   public:
    void run(adf::input_buffer<uint8_t>& in,
             adf::output_buffer<uint8_t>& metadata_out,
             adf::output_buffer<uint8_t>& out);
    static void registerKernelClass() { REGISTER_FUNCTION(SobelRunner::run); }
};
#endif
