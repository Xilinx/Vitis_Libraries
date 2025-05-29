/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/

// TODO: remove after service request
//#define bfloat16 float

#include "main.cpp"
#include "common_defines.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>

// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

#define N_ITER 1
#define SAMPLES_IN LEN* N_ITER
#define SAMPLES_IN_M LEN* M_COLUMNS* N_ITER
#define SAMPLES_OUT LEN* N_ITER

#define TYPE int

static std::vector<char> load_xclbin(xrtDeviceHandle device, const std::string& fnm) {
    if (fnm.empty()) throw std::runtime_error("No xclbin specified");

    // load bit stream
    std::ifstream stream(fnm);
    stream.seekg(0, stream.end);
    size_t size = stream.tellg();
    stream.seekg(0, stream.beg);

    std::vector<char> header(size);
    stream.read(header.data(), size);

    auto top = reinterpret_cast<const axlf*>(header.data());
    if (xrtDeviceLoadXclbin(device, top)) throw std::runtime_error("Xclbin loading failed");

    return header;
}

int main(int argc, char** argv) {
    //////////////////////////////////////////
    // Open xclbin
    //////////////////////////////////////////
    printf("Opening xclbin...\n");
    auto dhdl = xrtDeviceOpen(0); // Open Device the local device
    if (dhdl == nullptr) throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
    auto xclbin = load_xclbin(dhdl, "binary_container_1.xclbin");
    auto top = reinterpret_cast<const axlf*>(xclbin.data());
    adf::registerXRT(dhdl, top->m_header.uuid);

    int sizeIn = SAMPLES_IN;
    int sizeInM = SAMPLES_IN_M;
    int sizeOut = SAMPLES_OUT;

    //////////////////////////////////////////
    // input memory
    // Allocating the input size of sizeIn to MM2S
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////
    printf("Allocating memory...\n");
    xrtBufferHandle in_bohdl = xrtBOAlloc(dhdl, sizeInM * sizeof(TYPE), 0, 0);
    xrtBufferHandle in_bohdl2 = xrtBOAlloc(dhdl, sizeInM * sizeof(TYPE), 0, 0);
    xrtBufferHandle in_bohdl3 = xrtBOAlloc(dhdl, sizeInM * sizeof(TYPE), 0, 0);
    xrtBufferHandle in_bohdl4 = xrtBOAlloc(dhdl, sizeIn * sizeof(TYPE), 0, 0);
    xrtBufferHandle in_bohdl5 = xrtBOAlloc(dhdl, sizeIn * sizeof(TYPE), 0, 0);
    xrtBufferHandle in_bohdl6 = xrtBOAlloc(dhdl, sizeInM * sizeof(TYPE), 0, 0);
    xrtBufferHandle in_bohdl7 = xrtBOAlloc(dhdl, sizeIn * sizeof(TYPE), 0, 0);
    auto in_bomapped = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl));
    auto in_bomapped2 = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl2));
    auto in_bomapped3 = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl3));
    auto in_bomapped4 = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl4));
    auto in_bomapped5 = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl5));
    auto in_bomapped6 = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl6));
    auto in_bomapped7 = reinterpret_cast<TYPE*>(xrtBOMap(in_bohdl7));
    for (int i = 0; i < sizeInM; i++) {
        in_bomapped[i] = 100;
        in_bomapped2[i] = 300;
        in_bomapped3[i] = 500;
        in_bomapped6[i] = 700;
    }
    for (int i = 0; i < sizeIn; i++) {
        in_bomapped4[i] = 200;
        in_bomapped5[i] = 400;
        in_bomapped7[i] = 600;
    }

#if defined(__SYNCBO_ENABLE__)
    xrtBOSync(in_bohdl, XCL_BO_SYNC_BO_TO_DEVICE, sizeIn * sizeof(TYPE), 0);
#endif

    //////////////////////////////////////////
    // output memory
    // Allocating the output size of sizeOut to S2MM
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    xrtBufferHandle out_bohdl = xrtBOAlloc(dhdl, sizeOut * sizeof(TYPE), 0, 0);
    auto out_bomapped = reinterpret_cast<TYPE*>(xrtBOMap(out_bohdl));

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayImagePoints1");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl = xrtKernelRun(mm2s_khdl, in_bohdl, nullptr, sizeInM);
    printf("run mm2s1\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl2 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayImagePoints2");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl2 = xrtKernelRun(mm2s_khdl2, in_bohdl2, nullptr, sizeInM);
    printf("run mm2s2\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl3 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayRefPoint");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl3 = xrtKernelRun(mm2s_khdl3, in_bohdl3, nullptr, sizeInM);
    printf("run mm2s3\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl4 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayDistance1");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl4 = xrtKernelRun(mm2s_khdl4, in_bohdl4, nullptr, sizeIn);
    printf("run mm2s4\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl5 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayDistance2");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl5 = xrtKernelRun(mm2s_khdl5, in_bohdl5, nullptr, sizeIn);
    printf("run mm2s5\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl6 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayFocalPoint");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl6 = xrtKernelRun(mm2s_khdl6, in_bohdl6, nullptr, sizeInM);
    printf("run mm2s6\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl7 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayTStart");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl7 = xrtKernelRun(mm2s_khdl7, in_bohdl7, nullptr, sizeIn);
    printf("run mm2s7\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl8 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayDirection");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl8 = xrtKernelRun(mm2s_khdl8, nullptr, SIMD_DEPTH);
    printf("run mm2s8\n");

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle mm2s_khdl9 = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelaySpeedOfSound");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle mm2s_rhdl9 = xrtKernelRun(mm2s_khdl9, nullptr, sizeIn);
    printf("run mm2s9\n");

    //////////////////////////////////////////
    // s2mm ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle s2mm_khdl = xrtPLKernelOpen(dhdl, top->m_header.uuid, "PLDelayOutput");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the out_bohdl is the output buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle s2mm_rhdl = xrtKernelRun(s2mm_khdl, out_bohdl, nullptr, sizeOut);
    printf("run s2mm\n");

    //////////////////////////////////////////
    // graph execution for AIE
    //////////////////////////////////////////

    printf("graph init. This does nothing because CDO in boot PDI already configures AIE.\n");
    delay_graph.init();

    printf("graph run\n");
    delay_graph.run(N_ITER);

    delay_graph.end();
    printf("graph end\n");

    //////////////////////////////////////////
    // wait for mm2s done
    //////////////////////////////////////////

    auto state = xrtRunWait(mm2s_rhdl);
    std::cout << "mm2s completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl);
    xrtKernelClose(mm2s_khdl);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl2);
    std::cout << "mm2s2 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl2);
    xrtKernelClose(mm2s_khdl2);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl3);
    std::cout << "mm2s3 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl3);
    xrtKernelClose(mm2s_khdl3);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl4);
    std::cout << "mm2s4 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl4);
    xrtKernelClose(mm2s_khdl4);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl5);
    std::cout << "mm2s5 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl5);
    xrtKernelClose(mm2s_khdl5);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl6);
    std::cout << "mm2s6 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl6);
    xrtKernelClose(mm2s_khdl6);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl7);
    std::cout << "mm2s7 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl7);
    xrtKernelClose(mm2s_khdl7);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl8);
    std::cout << "mm2s8 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl8);
    xrtKernelClose(mm2s_khdl8);

    //////////////////////////////////////////
    // wait for mm2s2 done
    //////////////////////////////////////////

    state = xrtRunWait(mm2s_rhdl9);
    std::cout << "mm2s9 completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl9);
    xrtKernelClose(mm2s_khdl9);

    //////////////////////////////////////////
    // wait for s2mm done
    //////////////////////////////////////////

    state = xrtRunWait(s2mm_rhdl);
    std::cout << "s2mm completed with status(" << state << ")\n";
    xrtRunClose(s2mm_rhdl);
    xrtKernelClose(s2mm_khdl);

#if defined(__SYNCBO_ENABLE__)
    xrtBOSync(out_bohdl, XCL_BO_SYNC_BO_FROM_DEVICE, sizeOut * sizeof(TYPE), 0);
#endif

    //////////////////////////////////////////
    // Comparing the execution data to the golden data
    //////////////////////////////////////////

    std::cout << std::endl << "Result" << std::endl;

    int error_count = 0;
    for (int i = 0; i < sizeOut; i++) {
        std::cout << out_bomapped[i] << std::endl;
    }

    std::cout << std::endl << "End of Result" << std::endl;

    //////////////////////////////////////////
    // clean up XRT
    //////////////////////////////////////////

    std::cout << "Releasing remaining XRT objects...\n";
    // xrtBOUnmap(dhdl, in_bohdl, in_bomapped);
    // xrtBOUnmap(dhdl, out_bohdl, out_bomapped);
    xrtBOFree(in_bohdl);
    xrtBOFree(in_bohdl2);
    xrtBOFree(in_bohdl3);
    xrtBOFree(in_bohdl4);
    xrtBOFree(out_bohdl);
    xrtDeviceClose(dhdl);

    return error_count * (-1);
}
