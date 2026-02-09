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

stddevGraph stddev[NO_COLS] = {{6, 2, 0}}; //, {8, 2, 1}};//, {8, 2, 1}};//, {10, 2, 2}, {12, 2, 3}};

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    uint32_t reset = 1;
    float m_r, m_g, m_b;
    m_r = 0.5;
    m_g = 0.5;
    m_b = 0.5;
    for (int col = 0; col < NO_COLS; col++) {
        stddev[col].init();
        for (int i = 0; i < NO_CORES_PER_COL; i++) {
            stddev[col].update(stddev[col].reset[i], reset);
            stddev[col].update(stddev[col].m_r[i], m_r);
            stddev[col].update(stddev[col].m_g[i], m_g);
            stddev[col].update(stddev[col].m_b[i], m_b);
        }

        stddev[col].run(1);
        stddev[col].wait();
    }

    return 0;
}
#endif
