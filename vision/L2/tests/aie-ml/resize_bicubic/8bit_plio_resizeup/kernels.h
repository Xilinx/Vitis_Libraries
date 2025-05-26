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

class ResizeRunner {
    uint32_t (&mwtsY)[LUT_DEPTH];

   public:
    ResizeRunner(uint32_t (&wtsy)[LUT_DEPTH]) : mwtsY(wtsy) {}
    void run(adf::input_buffer<uint8_t>& input,
             adf::output_buffer<uint8_t>& metadata,
             adf::output_buffer<uint8_t>& output,
             int channels,
             uint32_t scale_y,
             int img_height_in,
             int img_height_out,
             float scale_y_f);
    static void registerKernelClass() {
        REGISTER_FUNCTION(ResizeRunner::run);
        REGISTER_PARAMETER(mwtsY);
    }
};
void transpose_api(adf::input_buffer<uint8_t>& input_metadata, adf::output_buffer<uint8_t>& output);
#endif
