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

#include "graph.h"

// instantiate adf dataflow graph
blacklevelGraph bl;

// initialize and run the dataflow graph
#if defined(__AIESIM__) || defined(__X86SIM__)

int main(int argc, char** argv) {
    int16_t BlackLevel = 32;
    const int MaxLevel = 255;
    float MulValue1 = (float)((float)MaxLevel / (MaxLevel - BlackLevel));
    int32_t MulValue = 37470; // Q(1.15)

    bl.init();
    bl.update(bl.blacklevel, BlackLevel);
    bl.update(bl.mulfact, MulValue);
    bl.run(1);
    bl.end();
    return 0;
}
#endif
