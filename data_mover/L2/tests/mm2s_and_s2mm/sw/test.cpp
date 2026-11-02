/*
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
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

#include "xrt/xrt_bo.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_kernel.h"

#include <iostream>
#include <cstring>
#include <string>

constexpr int NUM_PORTS = 1;
constexpr int DATA_COUNT = 1 << 10;

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cout << "test.ext XCLBIN_FILE [DEVICE_ID]" << std::endl;
    }
    std::string binaryFile = argv[1];
    int device_index = 0;
    if (argc > 2) device_index = std::stoi(argv[2]);

    std::cout << "Open the device" << device_index << std::endl;
    auto device = xrt::device(device_index);
    std::cout << "Load the xclbin " << binaryFile << std::endl;
    auto uuid = device.load_xclbin(binaryFile);

    auto krnl_mm2s = xrt::kernel(device, uuid, "mm2s");
    auto krnl_s2mm = xrt::kernel(device, uuid, "s2mm");

    std::cout << "Allocate Buffer in Global Memory\n";
    size_t vector_size_bytes = sizeof(int) * DATA_COUNT;
    xrt::bo bo_in[NUM_PORTS];
    xrt::bo bo_out[NUM_PORTS];
    for (int n = 0; n < NUM_PORTS; ++n) {
        bo_in[n] = xrt::bo(device, vector_size_bytes, krnl_mm2s.group_id(n * 3 + 0));
        bo_out[n] = xrt::bo(device, vector_size_bytes, krnl_s2mm.group_id(n * 3 + 1));

        // Map the contents of the buffer object into host memory
        auto bo_in_map = bo_in[n].map<int*>();
        for (int i = 0; i < DATA_COUNT; ++i) {
            bo_in_map[i] = i;
        }
    }

    // Synchronize buffer content with device side
    std::cout << "synchronize buffers to device global memory\n";
    for (int n = 0; n < NUM_PORTS; ++n) {
        bo_in[n].sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }

    std::cout << "Set arguments of MM2S kernel\n";
    auto run = xrt::run(krnl_mm2s);
    for (int n = 0; n < NUM_PORTS; ++n) {
        run.set_arg(n * 3 + 0, bo_in[n]);          // mm
        ;                                          // s
        run.set_arg(n * 3 + 2, vector_size_bytes); // size
    }

    std::cout << "Set arguments of S2MM kernel\n";
    auto run1 = xrt::run(krnl_s2mm);
    for (int n = 0; n < NUM_PORTS; ++n) {
        ;                                           // s
        run1.set_arg(n * 3 + 1, bo_out[n]);         // mm
        run1.set_arg(n * 3 + 2, vector_size_bytes); // size
    }

    std::cout << "Execution of the kernels\n";
    run.start();
    run1.start();

    run.wait();
    run1.wait();

    // Get the output;
    std::cout << "Get the output data from the device" << std::endl;
    for (int n = 0; n < NUM_PORTS; ++n) {
        bo_out[n].sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    }

    // Validate our results
    for (int n = 0; n < NUM_PORTS; ++n) {
        if (std::memcmp(bo_out[n].map<char*>(), bo_in[n].map<char*>(), vector_size_bytes)) {
            std::cout << "TEST FAILED" << std::endl;
            auto bo_in_map = bo_in[n].map<int*>();
            auto bo_out_map = bo_out[n].map<int*>();
            for (int i = 0; i < DATA_COUNT; ++i) {
                std::cout << bo_in_map[i] << ", " << bo_out_map[i] << std::endl;
            }
            std::cout << "TEST FAILED\n";
            return 1;
        }
    }

    std::cout << "TEST PASSED\n";
    return 0;
}
