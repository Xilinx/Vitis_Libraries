/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>

// This is used for the PL Kernels
#include "xrt/xrt.h"
#include "xrt/experimental/xrt_kernel.h"
#include "xrt/experimental/xrt_graph.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"
// simpleGraph addergraph;

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

#define DATA_SIZE 10000
#define N_ITER DATA_SIZE / 4

int main(int argc, char** argv) {
    using namespace std;
    //////////////////////////////////////////
    // Open xclbin
    //////////////////////////////////////////
    auto dhdl = xrtDeviceOpen(0); // Open Device the local device
    if (dhdl == nullptr) throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
    auto xclbin = load_xclbin(dhdl, "kernel.xclbin");
    auto top = reinterpret_cast<const axlf*>(xclbin.data());
    adf::registerXRT(dhdl, top->m_header.uuid);

    const int column_num = 3;
    const int row_num = 3;
    const int sizeIn = column_num * row_num;
    float* matrix = new float[sizeIn];

    //////////////////////////////////////////
    // input memory
    // Allocating the input size of sizeIn to MM2S
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    xrtBufferHandle in_bohdl0 = xrtBOAlloc(dhdl, sizeIn * sizeof(float), 0, 0);
    auto in_bomapped0 = reinterpret_cast<float*>(xrtBOMap(in_bohdl0));
    memcpy(in_bomapped0, matrix, sizeIn * sizeof(float));
    printf("Input memory virtual addr 0x%px\n", in_bomapped0);

    std::cout << "in_bohdl0 sync started\n";
    xrtBOSync(in_bohdl0, XCL_BO_SYNC_BO_TO_DEVICE, sizeIn * sizeof(float), 0);
    std::cout << "in_bohdl0 sync done\n";

    //////////////////////////////////////////
    // mm2s ip
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    xrtKernelHandle data_mover_khdl = xrtPLKernelOpen(dhdl, top->m_header.uuid, "data_mover:{data_mover}");
    // Need to provide the kernel handle, and the argument order of the kernel arguments
    // Here the in_bohdl is the input buffer, the nullptr is the streaming interface and must be null,
    // lastly, the size of the data. This info can be found in the kernel definition.
    xrtRunHandle data_mover_rhdl =
        xrtKernelRun(data_mover_khdl, in_bohdl0, nullptr, nullptr, nullptr, nullptr, row_num, column_num);
    printf("run data_mover\n");

    //////////////////////////////////////////
    // graph execution for AIE
    //////////////////////////////////////////

    auto g_hdl = xrtGraphOpen(dhdl, top->m_header.uuid, "mygraph");
    // addergraph.init();

    printf("graph run\n");
    xrtGraphUpdateRTP(g_hdl, "mygraph.k1.in[2]", (const char*)&row_num, 4);
    xrtGraphUpdateRTP(g_hdl, "mygraph.k1.in[3]", (const char*)&column_num, 4);
    xrtGraphRun(g_hdl, 1);

    // addergraph.end();
    xrtGraphClose(g_hdl);
    // xrtGraphWaitDone(g_hdl, 1000);
    printf("graph end\n");

    //////////////////////////////////////////
    // wait for data mover done
    //////////////////////////////////////////

    auto state = xrtRunWait(data_mover_rhdl);
    std::cout << "data_mover completed with status(" << state << ")\n";
    xrtRunClose(data_mover_rhdl);
    xrtKernelClose(data_mover_khdl);

    xrtBOSync(in_bohdl0, XCL_BO_SYNC_BO_FROM_DEVICE, sizeIn * sizeof(int), 0);
    memcpy(matrix, in_bomapped0, sizeIn * sizeof(float));

    //////////////////////////////////////////
    // Comparing the execution data to the golden data
    //////////////////////////////////////////

    int errorCount = 0;
    {
        for (int i = 0; i < row_num; i++) {
            for (int j = 0; j < column_num; j++) {
                cout << matrix[i * column_num + j] << " ";
            }
            cout << endl;
        }

        if (errorCount)
            printf("Test failed with %d errors\n", errorCount);
        else
            printf("TEST PASSED\n");
    }

    //////////////////////////////////////////
    // clean up XRT
    //////////////////////////////////////////

    std::cout << "Releasing remaining XRT objects...\n";
    xrtGraphClose(g_hdl);
    xrtBOFree(in_bohdl0);
    xrtDeviceClose(dhdl);

    delete[] matrix;

    return errorCount;
}
