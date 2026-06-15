/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#include <adf/window/types.h>
#include <adf/stream/types.h>
#include "adf.h"

template <int code>
void gaincontrol(adf::input_buffer<uint8>& input,
                 adf::output_buffer<uint8>& output,
                 const uint8_t& rgain,
                 const uint8_t& bgain,
                 const uint8_t& ggain);
