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
#ifndef __SCANLINE_MBYM_HPP__
#define __SCANLINE_MBYM_HPP__
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "json.hpp"
#include "us_system.hpp"
#include "us_models.hpp"
#include <kernel_focusing.hpp>

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
int scanline_MbyM(us_models<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, 4>& mdls) {
    printf(
        "\n************************ Now performacing a scanline testing in module by module mode "
        "************************\n");
    int m_num_line = mdls.dev1.m_num_line;
    int m_num_sample = mdls.dev1.m_num_sample;
    int m_num_element = mdls.dev1.m_num_element;
    const int m_num_upSample = 4;
    const int num_dim = 4;

    // printf("MODEL_TEST_SCANLINE_MbyM: ******* Generating full-size data blocks by algorithm modules \n");
    mdls.obj_img.runAll_ImagePoints(example_1_directions[DIM_X], example_1_directions[DIM_Z]);
    // for(int line = 0; line < m_num_ine; line++)
    // for(int n = 0; n < m_num_samplle; n++)
    mdls.obj_foc.runAll_Focus();
    // for(int line = 0; line < m_num_line; line++)
    // for(int e = 0; e < m_num_element; e++)
    mdls.obj_dly.runAll_Delay(mdls.obj_img.m_p_data);
    // for(int line = 0; line < m_num_line; line++)
    // for(int n = 0; n < m_num_sample; n++)
    mdls.obj_apo.runAll_Apodization(mdls.obj_img.m_p_data, mdls.obj_foc.m_p_data);
    // for(int line = 0; line < m_num_line; line++)
    // for(int e = 0; e < m_element; e++)
    mdls.obj_smp.runAll_Sample(mdls.obj_img.m_p_data, mdls.obj_dly.m_p_data);
    // for(int line = 0; line < m_num_line; line++)
    // for(int e = 0; e < num_element; e++)
    // for(int n = 0; n < num_sample; n++)
    mdls.obj_int.runAll_Interpolation(mdls.dev1.rf_data, mdls.obj_smp.m_p_data);
    // for(int line = 0; line < m_num_line; line++)
    // for(int e = 0; e < m_num_element; e++)
    // for(int n = 0; n < m_num_sample; n++)
    // for(int i = 0; i < m_num_upSample; i++)
    mdls.obj_mul.runAll_Mult(mdls.obj_int.m_p_data_int, mdls.obj_int.m_p_data_ins, mdls.obj_apo.m_p_data);
    // for(int line = 0; line < m_num_line; line++)
    // for(int e = 0; e < m_num_element; e++)
    // for(int n = 0; n < m_num_sample; n++)
    // for(int i = 0; i < m_num_upSample; i++)
    // printf("
    // clang-format off
    printf("MODEL_TEST_SCANLINE_MbyM:  ______________________________________________________________________\n");
    printf("MODEL_TEST_SCANLINE_MbyM:   Algorithm |               Data unit pattern           | Invoking times\n");
    printf("MODEL_TEST_SCANLINE_MbyM:    Modules  |  Dim1-element   Dim2-sample   Dim3-line   |               \n");
    printf("MODEL_TEST_SCANLINE_MbyM:  ---------------------------------------------------------------------  \n");
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_img  |                     [%4d]     x    [%d]  |                \n",
           m_num_sample, m_num_line);
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_foc  |  [%4d]                                   |                \n",
           m_num_element);
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_dly  |             x       [%4d]     x    [%d]  |                \n",
           m_num_sample, m_num_line);
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_apo  |  [%4d]      x      [%4d]     x    [%d]  |   x [1]         \n",
           m_num_element, m_num_sample, m_num_line);
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_smp  |  [%4d]      x      [%4d]     x    [%d]  |                 \n",
           m_num_element, m_num_sample, m_num_line);
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_int  |  [%4d]      x      [%4d]     x    [%d]  |                 \n",
           m_num_element, m_num_sample * 4, m_num_line);
    printf("MODEL_TEST_SCANLINE_MbyM:    obj_mul  |  [%4d]      x      [%4d]     x    [%d]  |                 \n",
           m_num_element, m_num_sample * 4, m_num_line);
    printf("MODEL_TEST_SCANLINE_MbyM:  ______________________________________________________________________ \n");
    // clang-format on
    // colum text for AIE usage
    mdls.save4aie_oneScanline("MbyM");
    mdls.save4aie_fullSize(0, "MbyM", DATA_RFD);
    mdls.save4aie_fullSize(0, "MbyM", DATA_MUL);

    return 0;
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
int scanline_MbyM() {
    us_models<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, 4> models;
    models.Init(example_1_speed_sound,   // = 1540.0;
                example_1_freq_sound,    // = 5*1000*1000;
                example_1_freq_sampling, // = 100*1000*1000;
                example_1_num_sample,    // = 2048;
                example_1_num_line,      // =  41;
                example_1_num_element,   // = 128;
                example_1_start_positions, example_1_info, RF_JSON, "./out_abt_beamform.json");
    scanline_MbyM<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(models);
    // json output
    models.saveJson("xf_output_res_MbyM.json");
    return 0;
}

#endif
