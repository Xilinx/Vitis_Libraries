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
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>

#include "us_models.hpp"
#include "scanline_ModuleByModule.hpp"
#include "scanline_LineByLine.hpp"
#include "scanline_ElementByElement.hpp"
#include "scanline_UnitByUnit.hpp"

#include <chrono>
// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

#include "host.hpp"
//#include "graph_scanline.hpp"
#include "scanline_AllinAIE.hpp"

////////////////////////////////////////////////////////
// Set graph_scanline work times
////////////////////////////////////////////////////////

//#define _SIM_SMALL_SCALE_
#ifdef _SIM_SMALL_SCALE_
const int num_line_test = 1;
const int num_invoking = num_line_test * NUM_ELEMENT_t * NUM_SEG_t;
#else
const int num_line_test = NUM_LINE_t;
const int num_invoking = NUM_LINE_t * NUM_ELEMENT_t * NUM_SEG_t;
#endif

//#define USING_JSON_AS_INPUT
int main(int argc, char* argv[]) {
#ifndef _SIM_SMALL_SCALE_
    printf("[HOST]: full test emulation of scanline will take about one hour.\n");
#endif

    //////////////////////////////////////////
    // Run c-model
    //////////////////////////////////////////
    us_models<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t> models;
    models.Init(example_1_speed_sound,   // = 1540.0;
                example_1_freq_sound,    // = 5*1000*1000;
                example_1_freq_sampling, // = 100*1000*1000;
                example_1_num_sample,    // = 2048;
                example_1_num_line,      // =  41;
                example_1_num_element,   // = 128;
                example_1_start_positions, example_1_info,
#ifdef USING_JSON_AS_INPUT
                RF_JSON, "./out_abt_beamform.json"
#else
                RF_2ELEMENT, "./data/rf_2e.txt"
#endif
                );
    int ret = 0;

    ret += scanline_MbyM<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(models);
    models.saveJson("xf_output_res_MbyM.json");
    models.rstMem();

    ret += scanline_LbyL<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(models);
    models.saveJson("xf_output_res_LbyL.json");
    models.rstMem();

    ret += scanline_EbyE<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(models);
    models.saveJson("xf_output_res_EbyE.json");
    models.rstMem();

    ret += scanline_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(models);
    models.saveJson("xf_output_res_UbyU.json");
    models.rstMem();

    // return ret;
    //////////////////////////////////////////
    // input cmd parser
    //////////////////////////////////////////
    ArgParser parser(argc, (const char**)argv);

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
    // input memory
    // 1.Allocating the input size of sizeIn to MM2S
    // 2.Loading scanline RF-data input
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////
    const int sizeIn = num_line_test * NUM_ELEMENT_t * NUM_SAMPLE_t;

    // Use data buffer from model
    float* buf_mem = models.dev1.rf_data_les;

    xrtBufferHandle in_bohdl_set;
    float* in_bomapped_set;

    in_bohdl_set = xrtBOAlloc(dhdl, sizeIn * sizeof(float), 0, 0);
    in_bomapped_set = reinterpret_cast<float*>(xrtBOMap(in_bohdl_set));
    memcpy(in_bomapped_set, buf_mem, sizeIn * sizeof(float));
    std::cout << "[HOST]: Input memory"
              << " virtual addr 0x" << in_bomapped_set << std::endl;

    xrtBOSync(in_bohdl_set, XCL_BO_SYNC_BO_TO_DEVICE, sizeIn * sizeof(float), 0);

    //////////////////////////////////////////
    // output memory
    // Allocating the output size of sizeOut to S2MM
    // This is using low-level XRT call xclAllocBO to allocate the memory
    //////////////////////////////////////////

    const int output_range = NUM_mult_CU_t;

    int sizeOut[output_range];

    xrtBufferHandle out_bohdl_set[output_range];
    float* out_bomapped_set[output_range];

    for (int i = 0; i < NUM_mult_CU_t; i++) {
        sizeOut[i] = num_line_test * NUM_SAMPLE_t * NUM_UPSample_t / NUM_mult_CU_t;
        out_bohdl_set[i] = xrtBOAlloc(dhdl, sizeOut[i] * sizeof(float), 0, 0);
        out_bomapped_set[i] = reinterpret_cast<float*>(xrtBOMap(out_bohdl_set[i]));
        memset(out_bomapped_set[i], 0xABCDEF00, sizeOut[i] * sizeof(float));
        std::cout << "[HOST]: Output memory" << i << " virtual addr 0x " << out_bomapped_set[i] << std::endl;
    }

    //////////////////////////////////////////
    // mm2s ips
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    // set xrt kernels & run kernels for PL mm2s
    xrtKernelHandle mm2s_khdl_set;
    xrtRunHandle mm2s_rhdl_set;

    // get kl name
    char kl_name[256];
    sprintf(kl_name, "mm2s1");

    // Open mm2s PL kernels and Set arguments for run
    mm2s_khdl_set = xrtPLKernelOpen(dhdl, top->m_header.uuid, kl_name);
    mm2s_rhdl_set = xrtRunOpen(mm2s_khdl_set);
    xrtRunSetArg(mm2s_rhdl_set, 0, in_bohdl_set);
    xrtRunSetArg(mm2s_rhdl_set, 2, sizeIn);
    xrtRunStart(mm2s_rhdl_set);

    std::cout << "input kernel complete" << std::endl;

    //////////////////////////////////////////
    // s2mm ips
    // Using the xrtPLKernelOpen function to manually control the PL Kernel
    // that is outside of the AI Engine graph
    //////////////////////////////////////////

    // set xrt kernels & run kernels for PL s2mm
    xrtKernelHandle s2mm_khdl_set[output_range];
    xrtRunHandle s2mm_rhdl_set[output_range];

    std::string kl_name_set[output_range];

    kl_name_set[0] = "s2mm0";
    kl_name_set[1] = "s2mm1";
    kl_name_set[2] = "s2mm2";
    kl_name_set[3] = "s2mm3";

    for (int i = 0; i < output_range; i++) {
        // Open s2mm PL kernels and Set arguments for run
        const char* kl_name = kl_name_set[i].c_str();
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

    us::L3::scanline_AllinAIE(num_invoking);

    //////////////////////////////////////////
    // wait for mm2s1 & mm2s2 done
    //////////////////////////////////////////
    // mm2s
    auto state = xrtRunWait(mm2s_rhdl_set);
    std::cout << "mm2s"
              << " completed with status(" << state << ")\n";
    xrtRunClose(mm2s_rhdl_set);
    xrtKernelClose(mm2s_khdl_set);

    std::cout << "mm2s wait complete" << std::endl;

    // s2mm
    for (int i = 0; i < NUM_mult_CU_t; i++) {
        auto state = xrtRunWait(s2mm_rhdl_set[i]);
        std::cout << "s2mm[" << i << "]completed with status(" << state << ")\n";
        xrtRunClose(s2mm_rhdl_set[i]);
        xrtKernelClose(s2mm_khdl_set[i]);
    }
    std::cout << "s2mm wait compete" << std::endl;

    for (int i = 0; i < NUM_mult_CU_t; i++) {
        int ret = xrtBOSync(out_bohdl_set[i], XCL_BO_SYNC_BO_FROM_DEVICE, sizeOut[i] * sizeof(float), 0);
        printf("[HOST] : xrtBOSync[%d] = %d, 0 on success\n", i, ret);
    }

    //////////////////////////////////////////
    //  output json data
    //////////////////////////////////////////

    std::string output_txt_path = data_path + "xf_output_res.txt";
    const char* output_txt_path_c = output_txt_path.c_str();
    // us_op_mult<float> mult;
    float* p_rf_data_out = (float*)malloc(num_line_test * NUM_SAMPLE_t * NUM_UPSample_t * sizeof(float));
    const int num_length_oneline = NUM_SAMPLE_t * NUM_UPSample_t / NUM_mult_CU_t;
    for (int i = 0; i < num_line_test; i++)
        for (int j = 0; j < NUM_mult_CU_t; j++)
            for (int n = 0; n < num_length_oneline; n++) {
                p_rf_data_out[i * NUM_SAMPLE_t * NUM_UPSample_t + j * num_length_oneline + n] =
                    out_bomapped_set[j][i * num_length_oneline + n];
#if defined(__HW__)
                printf("%.9f\n", p_rf_data_out[i * NUM_SAMPLE_t * NUM_UPSample_t + j * num_length_oneline + n]);
#endif
            }

    int size_out_json = num_line_test * NUM_SAMPLE_t * NUM_UPSample_t;
    save_les_bin<float>(p_rf_data_out, "data/xf_output_res.bin", num_line_test, NUM_SAMPLE_t, NUM_UPSample_t);
    system_md5<float>("data/xf_output_res.bin");
    writeFile<float, float>(p_rf_data_out, size_out_json, output_txt_path);
    system_md5<float>(output_txt_path);

    //////////////////////////////////////////
    // Comparison with data_model
    //////////////////////////////////////////
    int ret_cmp = 0;
    float th_err_abs = 0.00001; // 0.001mm
    float th_err_ratio = 0.001; // 0.1%
    printf("[HOST]: model result in %s\n", models.dev1.fname[DATA_MUL]);
    ret_cmp += models.diffCheck("data/xf_output_res.txt", DATA_MUL, th_err_abs, th_err_ratio);
    printf("[HOST]: total error %d\n", ret_cmp);

    //////////////////////////////////////////
    // clean up XRT
    //////////////////////////////////////////
    printf("Releasing remaining XRT objects...\n");
    if (p_rf_data_out) free(p_rf_data_out);

    // intput handle and buffer clean
    xrtBOFree(in_bohdl_set);

    // output handle clean
    for (int i = 0; i < NUM_mult_CU_t; i++) {
        int ret = xrtBOFree(out_bohdl_set[i]);
        printf("[HOST] : xrtBOFree[%d] = %d, 0 on success\n", i, ret);
    }

    // device close
    if (ret_cmp == 0)
        printf("[HOST]: success!\n");
    else
        printf("[HOST]: failed!\n");
    fflush(stdout);
    _exit(0);
    xrtDeviceClose(dhdl);

    return ret_cmp;
}
