/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
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

#include "graph.h"
#include <common/xf_aie_utils.hpp>

// instantiate adf dataflow graph
YCrCb2RGBGraph ycrcb2rgb[NO_COLS] = {{6, 2, 0}}; //, {8, 2, 1}};

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    for (int col = 0; col < NO_COLS; col++) {
        ycrcb2rgb[col].init();
        ycrcb2rgb[col].run(1);
        ycrcb2rgb[col].wait();
    }
    return 0;
}
#endif
