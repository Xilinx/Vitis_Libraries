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
resizeGraph resize[NO_COLS] = {{6, 2, 0}, {8, 2, 1}};

void read_file_uint8(FILE* fpout, uint8_t* in_buffer, int n) {
    for (int i = 0; i < n; i++) {
        fscanf(fpout, "%hhu \n", (in_buffer));
        in_buffer += 1;
    }
}

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    uint32_t scale_x_fix = compute_scalefactor<16>(TILE_WIDTH_IN, TILE_WIDTH_OUT);
    uint32_t scale_y_fix = compute_scalefactor<16>(TILE_HEIGHT_IN, TILE_HEIGHT_OUT);

    for (int col = 0; col < NO_COLS; col++) {
        resize[col].init();
        for (int i = 0; i < NO_CORES_PER_COL; i++) {
            resize[col].update(resize[col].scalex[i], scale_x_fix);
            resize[col].update(resize[col].scaley[i], scale_y_fix);
        }
        resize[col].run(1);
        resize[col].wait();
        resize[col].end();
    }
    return 0;
}
#endif
