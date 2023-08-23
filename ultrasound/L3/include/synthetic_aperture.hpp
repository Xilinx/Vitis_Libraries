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

#include "graph.cpp"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>

// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

#define INPUT_RANGE 28
#define OUTPUT_RANGE 6

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
        std::string line;
        for (size = 0; std::getline(infile, line); ++size)
            ;

        // back to the beginning of file
        infile.clear();
        infile.seekg(0, infile.beg);

        // data loading
        buffer = new T[size];

        for (int i = 0; i < size; i++) {
            infile >> buffer[i];
        }
    } else {
        std::cout << "direction input is empty!" << std::endl;
    }

    infile.close();

    std::cout << "file:" << filename << " size:" << size << std::endl;
    return buffer;
}

namespace us {
namespace L3 {

template <int TIER = 1>
void synthetic_aperture(std::string xclbin_path,
                        std::string data_path,
                        float* res_out[OUTPUT_RANGE],
                        float res_out_size[OUTPUT_RANGE],
                        int ITER) {
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

    file_set[0] = data_path + "start_positions.txt";
    file_set[1] = data_path + "directions.txt";
    file_set[2] = data_path + "samples_arange.txt";
    file_set[3] = data_path + "image_points.txt";
    file_set[4] = data_path + "image_points.txt";
    file_set[5] = data_path + "tx_def_delay_distance.txt";
    file_set[6] = data_path + "tx_def_delay_distance.txt";
    file_set[7] = data_path + "tx_def_ref_point.txt";
    file_set[8] = data_path + "tx_def_focal_point.txt";
    file_set[9] = data_path + "t_start.txt";
    file_set[10] = data_path + "apo_ref_0.txt";
    file_set[11] = data_path + "xdc_def_0.txt";
    file_set[12] = data_path + "apo_ref_1.txt";
    file_set[13] = data_path + "xdc_def_1.txt";
    file_set[14] = data_path + "image_points.txt";
    file_set[15] = data_path + "apodization_reference.txt";
    file_set[16] = data_path + "apo_distance_k.txt";
    file_set[17] = data_path + "F_number.txt";
    file_set[18] = data_path + "image_points.txt";
    file_set[19] = data_path + "delay_from_PL.txt";
    file_set[20] = data_path + "xdc_def_positions.txt";
    file_set[21] = data_path + "sampling_frequency.txt";
    file_set[22] = data_path + "P1.txt";
    file_set[23] = data_path + "P2.txt";
    file_set[24] = data_path + "P3.txt";
    file_set[25] = data_path + "P4.txt";
    file_set[26] = data_path + "P5.txt";
    file_set[27] = data_path + "P6.txt";

    sizeIn[0] = 4;    // start_positions.txt
    sizeIn[1] = 4;    // directions.txt
    sizeIn[2] = 32;   // samples_arange.txt
    sizeIn[3] = 128;  // image_points.txt
    sizeIn[4] = 128;  // image_points.txt
    sizeIn[5] = 4;    // tx_def_delay_distance.txt
    sizeIn[6] = 32;   // tx_def_delay_distance.txt
    sizeIn[7] = 4;    // tx_def_ref_point.txt
    sizeIn[8] = 4;    // tx_def_focal_point.txt
    sizeIn[9] = 4;    // t_start.txt
    sizeIn[10] = 32;  // apo_ref_0.txt
    sizeIn[11] = 32;  // xdc_def_0.txt
    sizeIn[12] = 32;  // apo_ref_1.txt
    sizeIn[13] = 32;  // xdc_def_1.txt
    sizeIn[14] = 128; // image_points.txt
    sizeIn[15] = 4;   // apodization_reference.txt
    sizeIn[16] = 32;  // apo_distance_k.txt
    sizeIn[17] = 4;   // F_number.txt
    sizeIn[18] = 128; // image_points.txt
    sizeIn[19] = 32;  // delay_from_PL.txt
    sizeIn[20] = 4;   // xdc_def_positions.txt
    sizeIn[21] = 32;  // sampling_frequency.txt
    sizeIn[22] = 128; // P1.txt
    sizeIn[23] = 128; // P2.txt
    sizeIn[24] = 128; // P3.txt
    sizeIn[25] = 128; // P4.txt
    sizeIn[26] = 128; // P5.txt
    sizeIn[27] = 128; // P6.txt

    // mem data loading
    for (int i = 0; i < INPUT_RANGE; i++) {
        buf_mem[i] = data_loading<float>(file_set[i], sizeIn[i]);
    }

    // mem data output length define
    sizeOut[0] = 128; // image-points output
    sizeOut[1] = 32;  // delay output
    sizeOut[2] = 32;  // focusing sa output
    sizeOut[3] = 32;  // apodization sa output
    sizeOut[4] = 32;  // samples
    sizeOut[5] = 128; // interpolator

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
    sa.init();
    std::cout << "graph init" << std::endl;

    sa.run(ITER);
    std::cout << "graph run" << std::endl;

    sa.end();
    std::cout << "graph end" << std::endl;

    //////////////////////////////////////////
    // wait for mm2s1 & mm2s2 done
    //////////////////////////////////////////

    for (int i = 0; i < INPUT_RANGE; i++) {
        // wait for run kernel done
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

    //////////////////////////////////////////
    // wait for s2mm done
    //////////////////////////////////////////

    for (int i = 0; i < OUTPUT_RANGE; i++) {
        xrtBOSync(out_bohdl_set[i], XCL_BO_SYNC_BO_FROM_DEVICE, sizeOut[i] * sizeof(float), 0);
    }

    //////////////////////////////////////////
    // sync output buffer
    //////////////////////////////////////////
    for (int k = 0; k < OUTPUT_RANGE; k++) {
        for (int i = 0; i < res_out_size[k]; i++) {
            float result_tmp = (float)out_bomapped_set[k][i];
            res_out[k][i] = result_tmp;
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
}
}
}
