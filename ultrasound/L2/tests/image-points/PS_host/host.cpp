/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>
#include <cstring>

// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

#define INPUT_RANGE 3
#define OUTPUT_RANGE 1

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

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

template <typename T>
T* data_loading(std::string filename, int size) {
    T* buffer;

    std::ifstream infile(filename, std::ios::in);

    if (infile.is_open()) {
        // data loading
        buffer = new T[size];

        for (int i = 0; i < size; i++) {
            infile >> buffer[i];
        }
    } else {
        std::cout << "input file is empty!" << std::endl;
    }

    infile.close();

    std::cout << "file:" << filename << " size:" << size << std::endl;
    return buffer;
}

int main(int argc, const char** argv) {
    //////////////////////////////////////////
    // input cmd parser
    //////////////////////////////////////////
    ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:[-xclbin] xclbin path is not set!\n";
        return 1;
    }

    std::string data_path;
    if (!parser.getCmdOption("-data", data_path)) {
        std::cout << "ERROR:[-data] data path is not set!\n";
        return 1;
    }

    //////////////////////////////////////////
    // Open xclbin
    //////////////////////////////////////////
    auto dhdl = xrtDeviceOpen(0); // Open Device the local device
    if (dhdl == nullptr) throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
    auto xclbin = load_xclbin(dhdl, xclbin_path);
    auto top = reinterpret_cast<const axlf*>(xclbin.data());
    adf::registerXRT(dhdl, top->m_header.uuid);

    //////////////////////////////////////////
    // data loading
    //////////////////////////////////////////
    // global
    int sizeIn[INPUT_RANGE];
    int sizeOut[OUTPUT_RANGE];
    float* buf_mem[INPUT_RANGE];
    std::string file_set[INPUT_RANGE];

    // Input: image-points
    file_set[0] = data_path + "start_positions.txt";
    file_set[1] = data_path + "directions.txt";
    file_set[2] = data_path + "samples_arange.txt";

    // mem size
    sizeIn[0] = 4;    // start_positions
    sizeIn[1] = 4;    // directions
    sizeIn[2] = 32;   // samples_arrange
    sizeOut[0] = 128; // image_points

    // mem data loading
    for (int i = 0; i < INPUT_RANGE; i++) {
        buf_mem[i] = data_loading<float>(file_set[i], sizeIn[i]);
    }

    //////////////////////////////////////////
    // input memory
    // Allocating the input size of sizeIn to MM2S
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    xrtBufferHandle in_bohdl_set[INPUT_RANGE];
    float* in_bomapped_set[INPUT_RANGE];

    for (int i = 0; i < INPUT_RANGE; i++) {
        in_bohdl_set[i] = xrtBOAlloc(dhdl, sizeIn[i] * sizeof(float), 0, 0);
        in_bomapped_set[i] = reinterpret_cast<float*>(xrtBOMap(in_bohdl_set[i]));
        memcpy(in_bomapped_set[i], buf_mem[i], sizeIn[i] * sizeof(float));
        std::cout << "Input memory" << i << " virtual addr 0x" << in_bomapped_set[i] << std::endl;
    }

    for (int i = 0; i < INPUT_RANGE; i++) {
        xrtBOSync(in_bohdl_set[i], XCL_BO_SYNC_BO_TO_DEVICE, sizeIn[i] * sizeof(float), 0);
    }

    //////////////////////////////////////////
    // output memory
    // Allocating the output size of sizeOut to S2MM
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    xrtBufferHandle out_bohdl_set[OUTPUT_RANGE];
    float* out_bomapped_set[OUTPUT_RANGE];

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        out_bohdl_set[i] = xrtBOAlloc(dhdl, sizeOut[i] * sizeof(float), 0, 0);
        out_bomapped_set[i] = reinterpret_cast<float*>(xrtBOMap(out_bohdl_set[i]));
        memset(out_bomapped_set[i], 0xABCDEF00, sizeOut[i] * sizeof(float));
        std::cout << "Output memory" << i << " virtual addr 0x " << out_bomapped_set[i] << std::endl;
    }

    //////////////////////////////////////////
    // mm2s ips
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    // set xrt kernels & run kernels for PL mm2s
    xrtKernelHandle mm2s_khdl_set[INPUT_RANGE];
    xrtRunHandle mm2s_rhdl_set[INPUT_RANGE];

    for (int i = 0; i < INPUT_RANGE; i++) {
        // get kl name
        char kl_name[256];
        sprintf(kl_name, "mm2s%d", i + 1);

        // Open mm2s PL kernels and Set arguments for run
        mm2s_khdl_set[i] = xrtPLKernelOpen(dhdl, top->m_header.uuid, kl_name);
        mm2s_rhdl_set[i] = xrtRunOpen(mm2s_khdl_set[i]);
        xrtRunSetArg(mm2s_rhdl_set[i], 0, in_bohdl_set[i]);
        xrtRunSetArg(mm2s_rhdl_set[i], 2, sizeIn[i]);
        xrtRunStart(mm2s_rhdl_set[i]);
    }
    std::cout << "input kernel complete" << std::endl;

    //////////////////////////////////////////
    // s2mm ips
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    // set xrt kernels & run kernels for PL s2mm
    xrtKernelHandle s2mm_khdl_set[OUTPUT_RANGE];
    xrtRunHandle s2mm_rhdl_set[OUTPUT_RANGE];

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        // gen kernel name
        char kl_name[256];
        sprintf(kl_name, "s2mm%d", i + 1);

        // Open s2mm PL kernels and Set arguments for run
        s2mm_khdl_set[i] = xrtPLKernelOpen(dhdl, top->m_header.uuid, kl_name);
        s2mm_rhdl_set[i] = xrtRunOpen(s2mm_khdl_set[i]);
        xrtRunSetArg(s2mm_rhdl_set[i], 0, out_bohdl_set[i]);
        xrtRunSetArg(s2mm_rhdl_set[i], 2, sizeOut[i]);
        xrtRunStart(s2mm_rhdl_set[i]);
    }
    std::cout << "output kernel complete" << std::endl;

    //////////////////////////////////////////
    // graph execution for AIE
    //////////////////////////////////////////

    printf("xrtGraphOpen\n");
    auto ghdl = xrtGraphOpen(dhdl, top->m_header.uuid, "d");
    printf("xrtGraphRun\n");
    xrtGraphRun(ghdl, 1);

    //////////////////////////////////////////
    // wait for mm2s1 & mm2s2 done
    //////////////////////////////////////////

    for (int i = 0; i < INPUT_RANGE; i++) {
        auto state = xrtRunWait(mm2s_rhdl_set[i]);
        std::cout << "mm2s" << i + 1 << " completed with status(" << state << ")\n";
        xrtRunClose(mm2s_rhdl_set[i]);
        xrtKernelClose(mm2s_khdl_set[i]);
    }
    std::cout << "mm2s wait complete" << std::endl;

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        auto state = xrtRunWait(s2mm_rhdl_set[i]);
        std::cout << "s2mm" << i + 1 << "completed with status(" << state << ")\n";
        xrtRunClose(s2mm_rhdl_set[i]);
        xrtKernelClose(s2mm_khdl_set[i]);
    }
    std::cout << "s2mm wait compete" << std::endl;

    xrtGraphEnd(ghdl, 0);
    printf("xrtGraphEnd..\n");
    xrtGraphClose(ghdl);

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        xrtBOSync(out_bohdl_set[i], XCL_BO_SYNC_BO_FROM_DEVICE, sizeOut[i] * sizeof(float), 0);
    }

    //////////////////////////////////////////
    // Comparing the execution data to the golden data
    //////////////////////////////////////////

    int err_cnt = 0;
    std::string golden_file_set[OUTPUT_RANGE];
    golden_file_set[0] = data_path + "image_points_golden.txt";

    // loading golden data
    float* golden_buf_mem[OUTPUT_RANGE];
    int golden_sizeOut[OUTPUT_RANGE];
    golden_sizeOut[0] = 128;

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        golden_buf_mem[i] = data_loading<float>(golden_file_set[i], golden_sizeOut[i]);
    }

    for (int k = 0; k < OUTPUT_RANGE; k++) {
        for (int i = 0; i < sizeOut[k]; i++) {
            float result_tmp = (float)out_bomapped_set[k][i];
            float result_golden = golden_buf_mem[k][i];
            if (result_tmp != result_golden) {
                printf("i:%d, golden:%f result:%f\n", i, result_golden, result_tmp);
                err_cnt++;
            }
        }
    }

    //////////////////////////////////////////
    // clean up XRT
    //////////////////////////////////////////

    std::cout << "Releasing remaining XRT objects...\n";

    // intput handle and buffer clean
    for (int i = 0; i < INPUT_RANGE; i++) {
        xrtBOFree(in_bohdl_set[i]);
        delete[] buf_mem[i];
    }

    // output handle clean
    for (int i = 0; i < OUTPUT_RANGE; i++) {
        xrtBOFree(out_bohdl_set[i]);
    }

    // device close
    xrtDeviceClose(dhdl);

    //////////////////////////////////////////
    //  End
    //////////////////////////////////////////
    printf("Test Done, err_cnt:%d\n", err_cnt);

    return err_cnt;
}