/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef _RESIZE_RUNNER_H
#define _RESIZE_RUNNER_H

#include <adf/window/types.h>
#include <adf/stream/types.h>
#include "adf.h"
#include "config.h"

class DenormResizeRunner {
    int mnFBitsIn;
    int mnFBitsAlpha;
    int mnFBitsBeta;
    int mnFBitsOut;

   public:
    DenormResizeRunner(int nFBitsIn, int nFBitsAlpha, int nFBitsBeta, int nFBitsOut)
        : mnFBitsIn(nFBitsIn), mnFBitsAlpha(nFBitsAlpha), mnFBitsBeta(nFBitsBeta), mnFBitsOut(nFBitsOut) {}

    void run(input_window<int8_t>* input,
             output_window<uint8_t>* output,
             int row,
             uint32_t scale_x,
             uint32_t scale_y,
             int img_height_in,
             int tile_width_in,
             int line_stride_in,
             int tile_width_out,
             const int16_t (&coeff)[8]);
    static void registerKernelClass() { REGISTER_FUNCTION(DenormResizeRunner::run); }
};

#endif
