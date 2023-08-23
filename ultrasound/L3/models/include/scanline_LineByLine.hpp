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

#ifndef __SCANLINE_LBYL_HPP__
#define __SCANLINE_LBYL_HPP__
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "json.hpp"
#include "us_system.hpp"
#include "us_models.hpp"
#include <kernel_focusing.hpp>

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
int scanline_LbyL(us_models<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, 4>& mdls) {
    printf(
        "\n************************ Now performacing a scanline testing in line by line mode "
        "************************\n");
    int id = 1;
    int m_num_line = mdls.dev1.m_num_line;
    int m_num_sample = mdls.dev1.m_num_sample;
    int m_num_element = mdls.dev1.m_num_element;
    const int m_num_upSample = 4;
    const int num_dim = 4;

    ///////////////////////////
    // Const data for AIE, but can be established by host
    ///////////////////////////
    float* p_const_img_start_x = mdls.obj_img.m_p_start[DIM_X];
    float* p_const_img_start_z = mdls.obj_img.m_p_start[DIM_Z];
    float* p_const_foc = (float*)example_1_xdc_def_pos;
    float* p_const_dly = (float*)mdls.obj_dly.m_t_start;
    float* p_const_smp = (float*)example_1_xdc_def_pos;
    float* p_const_apo_tileVApo = mdls.obj_apo.m_tileVApo;
    float* p_const_apo_ref_pos = mdls.obj_apo.m_ref_pos;
    ///////////////////////////
    // Can be iterated by AIE, and can be established by host
    ///////////////////////////
    para_ImagePoint<float> para_img_x(mdls.dev1.m_p_start[DIM_X], example_1_directions[DIM_X], 0, m_num_sample);
    para_ImagePoint<float> para_img_z(mdls.dev1.m_p_start[DIM_Z], example_1_directions[DIM_Z], 0, m_num_sample);
    para_Focus<float> para_foc(m_num_element);
    para_Delay<float> para_dly(m_num_sample, mdls.obj_dly.m_t_start, mdls.obj_dly.m_tx_def_ref_point,
                               mdls.obj_dly.m_tileVApo, mdls.obj_dly.m_tx_def_focal_point,
                               mdls.obj_dly.tx_def_delay_distance, mdls.obj_dly.tx_def_delay_distance_,
                               mdls.obj_dly.m_inv_speed_of_sound);
    para_Sample2<float> para_smp(m_num_sample, m_num_element, mdls.obj_smp.m_freq_sampling,
                                 mdls.obj_smp.m_inv_speed_of_sound, example_1_rf_data_dim);
    para_Interpolation<float> para_int(m_num_element, m_num_sample, m_num_upSample, example_1_rf_data_dim);
    para_Apodization<float> para_apo(m_num_element, m_num_sample, mdls.obj_apo.m_f_number);
    para_Mult<float> para_mult(m_num_element, m_num_sample, m_num_upSample);

    float* p_data_img_x = mdls.obj_img.m_p_data[DIM_X];
    float* p_data_img_z = mdls.obj_img.m_p_data[DIM_Z];
    float* p_data_foc = mdls.obj_foc.m_p_data;
    float* p_data_dly = mdls.obj_dly.m_p_data;
    float* p_data_smp = mdls.obj_smp.m_p_data;
    unsigned* p_data_ins = mdls.obj_int.m_p_data_ins;
    float* p_data_int = mdls.obj_int.m_p_data_int;
    float* p_data_apo = mdls.obj_apo.m_p_data;
    float* p_data_mult = mdls.obj_mul.m_p_data;

    for (int line = 0; line < m_num_line; line++) {
        ///////////////////////////
        // for image point_x image point_y
        ///////////////////////////
        fun_UpdatingImagePoints_line_1d_wrapper(&para_img_x, p_const_img_start_x, p_data_img_x);
        fun_UpdatingImagePoints_line_1d_wrapper(&para_img_z, p_const_img_start_z, p_data_img_z);
        ///////////////////////////
        // for focus
        ///////////////////////////
        fun_genLineFocus(p_data_foc, &para_foc, p_const_foc);
        ///////////////////////////
        // for delay
        ///////////////////////////
        fun_UpdatingDelay_line(p_data_dly, &para_dly, p_const_dly, p_data_img_x, p_data_img_z);
        ///////////////////////////
        // for apodization
        ///////////////////////////
        fun_genLineApodization_warpper<float>(p_data_apo, &para_apo, p_const_apo_tileVApo, p_const_apo_ref_pos,
                                              p_data_img_x, p_data_img_z, p_data_foc);
        ///////////////////////////
        // for sample
        ///////////////////////////
        fun_genLineSample<float>(p_data_smp, p_data_ins, &para_smp, p_const_smp, p_data_img_x, p_data_img_z,
                                 p_data_dly);
        ///////////////////////////
        // for interpolation
        ///////////////////////////
        fun_genLineInterpolation_warpper<float>(
            p_data_int, &para_int, &(mdls.dev1.rf_data[m_num_element * m_num_sample * line]), p_data_smp, p_data_ins);
        ///////////////////////////
        // for final mult
        ///////////////////////////
        fun_genLineMult<float>(p_data_mult, &para_mult, p_data_int, p_data_apo);

        p_data_img_x += m_num_sample;
        p_data_img_z += m_num_sample;
        p_data_foc += m_num_element;
        p_data_dly += m_num_sample;
        p_data_apo += m_num_element * m_num_sample;
        p_data_smp += m_num_element * m_num_sample;
        p_data_ins += m_num_element * m_num_sample;
        ;
        p_data_int += m_num_element * m_num_sample * m_num_upSample;
        p_data_mult += m_num_sample * m_num_upSample;

    } // for line
    // clang-format off
    printf("MODEL_TEST_SCANLINE_LbyL:  ____________________________________________________________________\n");  
    printf("MODEL_TEST_SCANLINE_LbyL:   Algorithm |       Data unit pattern        | Invoking times        \n");
    printf("MODEL_TEST_SCANLINE_LbyL:    Modules  |  Dim1-element   Dim2-sample    |  [line]    \n");
    printf("MODEL_TEST_SCANLINE_LbyL:  --------------------------------------------------------------------       \n");
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_img  |                     [%4d]     |                 \n", m_num_sample );
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_foc  |  [%4d]                        |                 \n", m_num_element);
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_dly  |                     [%4d]     |                 \n", m_num_sample );
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_apo  |  [%4d]      x      [%4d]     |  x [%d]          \n", m_num_element, m_num_sample, m_num_line);
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_smp  |  [%4d]      x      [%4d]     |                  \n", m_num_element, m_num_sample );
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_int  |  [%4d]      x      [%4d]     |                  \n", m_num_element, m_num_sample*4 );
    printf("MODEL_TEST_SCANLINE_LbyL:    obj_mul  |  [%4d]      x      [%4d]     |                  \n", m_num_element, m_num_sample*4 );
    printf("MODEL_TEST_SCANLINE_LbyL:  ____________________________________________________________________       \n");
    // clang-format on
    // colum text for AIE usage
    mdls.save4aie_oneScanline("LbyL");
    mdls.save4aie_fullSize(0, "LbyL", DATA_RFD);
    mdls.save4aie_fullSize(0, "LbyL", DATA_MUL);

    return 0;
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t>
int scanline_LbyL() {
    us_models<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, 4> models;
    models.Init(example_1_speed_sound,   // = 1540.0;
                example_1_freq_sound,    // = 5*1000*1000;
                example_1_freq_sampling, // = 100*1000*1000;
                example_1_num_sample,    // = 2048;
                example_1_num_line,      // =  41;
                example_1_num_element,   // = 128;
                example_1_start_positions, example_1_info, RF_JSON, "./out_abt_beamform.json");
    scanline_LbyL<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(models);
    // json output
    models.saveJson("xf_output_res_LbyL.json");
    return 0;
}

#endif