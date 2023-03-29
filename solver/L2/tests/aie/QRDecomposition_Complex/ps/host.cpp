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

//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <fstream>

#include "ocl.hpp"

#define DATA_SIZE 16
#define N_ITER DATA_SIZE / 4

int main(int argc, char** argv) {
    // Prepare data
    const int num = DATA_SIZE;
    const int size_in_byte = DATA_SIZE * sizeof(int);
    int dataInput[num], dataOutput[num], golden[num];
    for (int i = 0; i < num; i++) {
        dataInput[i] = rand() % 100;
        dataOutput[i] = 0;
        golden[i] = dataInput[i] * 2;
        std::cout << "Input: " << dataInput[i] << ", Output: " << dataOutput[i] << std::endl;
    }

    // Device and kernel
    /*
    auto d_hdl = xrtDeviceOpen(0); // Open Device the local device
    if (d_hdl == nullptr) throw std::runtime_error("No valid device handle found.");
    xrtDeviceLoadXclbinFile(d_hdl, "krnl_adder.xclbin");
    xuid_t uuid;
    xrtDeviceGetXclbinUUID(d_hdl, uuid);
    auto k_mm2s_hdl = xrtPLKernelOpen(d_hdl, uuid, "pl_mm2s:{pl_mm2s_1}");
    auto k_s2mm_hdl = xrtPLKernelOpen(d_hdl, uuid, "pl_s2mm:{pl_s2mm_1}");
    */
    fastXM xm(0, "krnl_adder.xclbin", {"pl_mm2s:{pl_mm2s_1}", "pl_s2mm:{pl_s2mm_1}"}, {"addergraph"});
    xm.runPL(0, {{0, true, size_in_byte, (char*)dataInput, 0}, {2, false, 0, nullptr, num}});
    xm.runPL(1, {{0, true, size_in_byte, (char*)dataOutput, 0}, {2, false, 0, nullptr, num}});
    xm.runGraph(0, N_ITER);
    xm.waitDone(100000);
    xm.fetchRes();

    /*
    // Bufer and host to device
    auto in_bo_hdl = xrtBOAlloc(d_hdl, size_in_byte, 0, xrtKernelArgGroupId(k_mm2s_hdl, 0));
    auto in_bo_ptr = reinterpret_cast<int*>(xrtBOMap(in_bo_hdl));
    auto out_bo_hdl = xrtBOAlloc(d_hdl, size_in_byte, 0, xrtKernelArgGroupId(k_s2mm_hdl, 0));
    auto out_bo_ptr = reinterpret_cast<int*>(xrtBOMap(out_bo_hdl));
    memcpy(in_bo_ptr, dataInput, size_in_byte);
    xrtBOSync(in_bo_hdl, XCL_BO_SYNC_BO_TO_DEVICE, size_in_byte, 0);

    // run PL kernel and AIE graph
    auto r_mm2s_hdl = xrtRunOpen(k_mm2s_hdl);
    auto r_s2mm_hdl = xrtRunOpen(k_s2mm_hdl);
    xrtRunSetArg(r_mm2s_hdl, 0, in_bo_hdl);
    xrtRunSetArg(r_mm2s_hdl, 2, num);
    xrtRunSetArg(r_s2mm_hdl, 0, out_bo_hdl);
    xrtRunSetArg(r_s2mm_hdl, 2, num);
    xrtRunStart(r_mm2s_hdl);
    xrtRunStart(r_s2mm_hdl);

    auto g_hdl = xrtGraphOpen(d_hdl, uuid, "addergraph");
    xrtGraphRun(g_hdl, N_ITER);
    */

    // wait finish
    /*
    xrtRunWait(r_mm2s_hdl);
    xrtRunWait(r_s2mm_hdl);
    xrtGraphWaitDone(g_hdl, 100000);
    xrtBOSync(out_bo_hdl, XCL_BO_SYNC_BO_FROM_DEVICE, size_in_byte, 0);
    memcpy(dataOutput, out_bo_ptr, size_in_byte);
    */
    // Verify
    bool pass = true;
    for (int i = 0; i < num; i++) {
        if (dataOutput[i] != golden[i]) {
            pass = false;
        }
        std::cout << "Output = " << dataOutput[i] << ", Golden = " << golden[i] << std::endl;
    }

    //
    /*
    xrtRunClose(r_mm2s_hdl);
    xrtRunClose(r_s2mm_hdl);
    xrtBOFree(in_bo_hdl);
    xrtBOFree(out_bo_hdl);
    xrtGraphClose(g_hdl);
    xrtKernelClose(k_mm2s_hdl);
    xrtKernelClose(k_s2mm_hdl);
    xrtDeviceClose(d_hdl);
    */

    if (pass) {
        std::cout << "test passed" << std::endl;
    } else {
        std::cout << "test failed" << std::endl;
    }
}
