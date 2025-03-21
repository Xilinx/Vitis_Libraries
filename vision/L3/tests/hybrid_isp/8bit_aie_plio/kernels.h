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

#ifndef _DEMOSAIC_RUNNER_H
#define _DEMOSAIC_RUNNER_H

#include <adf/window/types.h>
#include <adf/stream/types.h>
#include "adf.h"
#include "config.h"
#include "imgproc/xf_demosaicing_aie2.hpp"

template <typename T, unsigned int N, int code>
void comb(adf::input_buffer<uint8_t>& input,
          const int16_t (&coeff)[25],
          const uint8_t& black_level,
          const uint16_t& mul_fact,
          const uint8_t& rgain,
          const uint8_t& bgain,
          const uint8_t& ggain,
          adf::output_buffer<uint8_t>& outputDem,
          adf::output_buffer<uint8_t>& output);

#endif
