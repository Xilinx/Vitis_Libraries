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
#ifndef __SCANLINE_UBYU_HPP__
#define __SCANLINE_UBYU_HPP__
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "json.hpp"
#include "us_system.hpp"
#include "us_models.hpp"
#include <kernel_focusing.hpp>

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
int scanline_UbyU(us_models<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>& mdls) {
    printf(
        "\n************************ Now performacing a scanline testing in data-unit-by data-unit mode "
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
    // us::L1::para_foc_t para_foc_const(NUM_LINE_t, NUM_ELEMENT_t, NUM_SEG_t);
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

    float* p_data_rf = mdls.dev1.rf_data_les;
    float* p_data_img_x = mdls.obj_img.m_p_data[DIM_X];
    float* p_data_img_z = mdls.obj_img.m_p_data[DIM_Z];
    float* p_data_foc = mdls.obj_foc.m_p_data;
    float* p_data_dly = mdls.obj_dly.m_p_data;
    float* p_data_smp = mdls.obj_smp.m_p_data;
    unsigned* p_data_ins = mdls.obj_int.m_p_data_ins;
    float* p_data_int = mdls.obj_int.m_p_data_int;
    float* p_data_apo = mdls.obj_apo.m_p_data;
    float* p_data_mult = mdls.obj_mul.m_p_data;

    const int LEN_OUT_img_t = NUM_SAMPLE_t / NUM_SEG_t;
    const int NUM_SAMPLE_SEG_t = NUM_SAMPLE_t / NUM_SEG_t;

    for (int line = 0; line < m_num_line; line++) {
        float dataUnit_mult[NUM_SAMPLE_t * 4] = {0};                  // p_data_mult;*/
        float dataUnit_mults[4][NUM_SAMPLE_t] = {{0}, {0}, {0}, {0}}; // p_data_mult;*/

        for (int e = 0; e < m_num_element; e++) {
            // Loading one elements' sampled data into a data unit
            float dataUnit_rfdata[NUM_SAMPLE_t];
            SYNCBUFF(dataUnit_rfdata, p_data_rf, m_num_sample);

            float dataUnit_img_x[NUM_SAMPLE_SEG_t];   // p_data_img_x;
            float dataUnit_img_z[NUM_SAMPLE_SEG_t];   // p_data_img_z;
            float dataUnit_foc[NUM_ELEMENT_t];        // p_data_foc;
            float dataUnit_dly[NUM_SAMPLE_SEG_t];     // p_data_dly;
            float dataUnit_smp[NUM_SAMPLE_SEG_t];     // p_data_smp;
            unsigned dataUnit_ins[NUM_SAMPLE_SEG_t];  // p_data_ins;
            float dataUnit_int[NUM_SAMPLE_SEG_t * 4]; // p_data_int;
            float dataUnit_apo[NUM_SAMPLE_SEG_t];     // p_data_apo;
            hls::stream<float> strm_resmp;
            hls::stream<unsigned> strm_ins;
            hls::stream<Window_inter<float> > strm_win;
            hls::stream<float> strm_apodization;
            ///////////////////////////
            // for image point_x image point_y
            ///////////////////////////

            for (int seg = 0; seg < NUM_SEG_t; seg++) {
                int off_sampleSeg = 0; // m_num_sample/NUM_SEG_t*seg;
                ///////////////////////////
                // for imagepoints
                ///////////////////////////
                fun_UpdatingImagePoints_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, LEN_OUT_img_t>(
                    line, e, seg, &para_img_x, p_const_img_start_x, dataUnit_img_x + off_sampleSeg);
                fun_UpdatingImagePoints_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, LEN_OUT_img_t>(
                    line, e, seg, &para_img_z, p_const_img_start_z, dataUnit_img_z + off_sampleSeg);
                SYNCBUFF_SEG(p_data_img_x, dataUnit_img_x, m_num_sample / NUM_SEG_t, seg);
                SYNCBUFF_SEG(p_data_img_z, dataUnit_img_z, m_num_sample / NUM_SEG_t, seg);
                ///////////////////////////
                // for focus
                ///////////////////////////
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genFocus_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t>(e, dataUnit_foc, p_const_foc);
                ///////////////////////////
                // for delay
                ///////////////////////////
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_UpdatingDelay_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(
                    line, e, seg, dataUnit_dly + off_sampleSeg, &para_dly, p_const_dly, dataUnit_img_x + off_sampleSeg,
                    dataUnit_img_z + off_sampleSeg);
                SYNCBUFF_SEG(p_data_dly, dataUnit_dly, m_num_sample / NUM_SEG_t, seg);
                ///////////////////////////
                // for sample
                ///////////////////////////

                fun_genLineSample_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(
                    line, e, seg, dataUnit_smp + off_sampleSeg, dataUnit_ins + off_sampleSeg, &para_smp, p_const_smp,
                    dataUnit_img_x + off_sampleSeg, dataUnit_img_z + off_sampleSeg, dataUnit_dly + off_sampleSeg);
                SYNCBUFF_SEG(p_data_smp, dataUnit_smp, m_num_sample / NUM_SEG_t, seg);
                SYNCBUFF_SEG(p_data_ins, dataUnit_ins, m_num_sample / NUM_SEG_t, seg);
                for (int i = 0; i < NUM_SAMPLE_t / NUM_SEG_t; i++) strm_ins.write(dataUnit_ins[off_sampleSeg + i]);

                ///////////////////////////
                // for apodization
                ///////////////////////////
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineApodization_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(
                    line, e, seg, dataUnit_apo + off_sampleSeg, &para_apo, p_const_apo_tileVApo, p_const_apo_ref_pos,
                    dataUnit_img_x + off_sampleSeg, dataUnit_img_z + off_sampleSeg, dataUnit_foc);
                SYNCBUFF_SEG(p_data_apo, dataUnit_apo, m_num_sample / NUM_SEG_t, seg);
                for (int i = 0; i < NUM_SAMPLE_t / NUM_SEG_t; i++)
                    strm_apodization.write(dataUnit_apo[off_sampleSeg + i]);

                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineInterpolation_UbyU_resampling<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(
                    &para_int, dataUnit_rfdata, dataUnit_smp + off_sampleSeg, strm_resmp);
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineInterpolation_UbyU_window_sync3<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(
                    seg, &para_int, strm_resmp, strm_win);

            } // loop for seg
            ///////////////////////////
            // for interpolation
            ///////////////////////////
            for (int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineInterpolation_UbyU_bSpline<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(
                    dataUnit_int + m_num_sample * 4 / NUM_SEG_t * seg, &para_int, strm_ins, strm_win);
            SYNCBUFF(p_data_int, dataUnit_int, m_num_sample * 4);
            ///////////////////////////
            // for final mult
            ///////////////////////////
            // transform port types from buffer to stream

            hls::stream<DU_inter<float> > strm_interpolation; // input
            for (int n = 0; n < m_num_sample; n++) {
                DU_inter<float> dint;
                for (int i = 0; i < m_num_upSample; i++) dint.s[i] = dataUnit_int[m_num_upSample * n + i];
                strm_interpolation.write(dint);
            }
            hls::stream<float> strm_apodization_1;
            hls::stream<float> strm_apodization_2;
            hls::stream<float> strm_apodization_3;
            hls::stream<float> strm_apodization_4; // Not used

            hls::stream<DU_inter<float> > strm_interpolation_1;
            hls::stream<DU_inter<float> > strm_interpolation_2;
            hls::stream<DU_inter<float> > strm_interpolation_3;
            hls::stream<DU_inter<float> > strm_interpolation_4; // Not used

            for (int seg = 0; seg < NUM_SEG_t; seg++) {
                fun_genLineMult_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, 0>(
                    e, seg,
                    dataUnit_mults[0], // dataUnit_mult +m_num_sample*4/NUM_SEG_t*seg,
                    &para_mult, strm_interpolation, strm_apodization, strm_interpolation_1, strm_apodization_1);
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineMult_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, 1>(
                    e, seg,
                    dataUnit_mults[1], // dataUnit_mult +m_num_sample*4/NUM_SEG_t*seg,
                    &para_mult, strm_interpolation_1, strm_apodization_1, strm_interpolation_2, strm_apodization_2);
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineMult_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, 2>(
                    e, seg,
                    dataUnit_mults[2], // dataUnit_mult +m_num_sample*4/NUM_SEG_t*seg,
                    &para_mult, strm_interpolation_2, strm_apodization_2, strm_interpolation_3, strm_apodization_3);
                // for(int seg = 0; seg < NUM_SEG_t; seg++)
                fun_genLineMult_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, 3>(
                    e, seg,
                    dataUnit_mults[3], // dataUnit_mult +m_num_sample*4/NUM_SEG_t*seg,
                    &para_mult, strm_interpolation_3, strm_apodization_3, strm_interpolation_4, strm_apodization_4);
            }

            p_data_rf += m_num_sample;
            p_data_smp += m_num_sample;
            p_data_ins += m_num_sample;
            p_data_apo += m_num_sample;
            p_data_int += m_num_sample * m_num_upSample;
            if (e + 1 == m_num_element) {
                SYNCBUFF(p_data_foc, dataUnit_foc, m_num_element);

                for (int i = 0; i < m_num_sample; i++) {
                    for (int off = 0; off < 4; off++) dataUnit_mult[i * 4 + off] = dataUnit_mults[off][i];
                }
                SYNCBUFF(p_data_mult, dataUnit_mult, m_num_sample * m_num_upSample);
                p_data_img_x += m_num_sample;
                p_data_img_z += m_num_sample;
                p_data_foc += m_num_element;
                p_data_dly += m_num_sample;
                p_data_mult += m_num_sample * 4;
            }
        } // for element
        //

    } // for line
    // clang-format off
    printf("MODEL_TEST_SCANLINE_UbyU:  ____________________________________________________________________\n");  
    printf("MODEL_TEST_SCANLINE_UbyU:   Algorithm | Data unit pattern |        Invoking times         \n");
    printf("MODEL_TEST_SCANLINE_UbyU:    Modules  |  Dim1-seg-sample  | [segment] x [element] x [line]    \n");
    printf("MODEL_TEST_SCANLINE_UbyU:  --------------------------------------------------------------------       \n");
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_img  |       [%4d]      |                                \n", m_num_sample/NUM_SEG_t );
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_foc  |       [%4d]      |                                \n", m_num_element);
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_dly  |       [%4d]      |                                \n", m_num_sample/NUM_SEG_t  );
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_apo  |       [%4d]      | x [%d] x [%3d] x [%d]          \n", m_num_sample/NUM_SEG_t, NUM_SEG_t , m_num_element, m_num_line);
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_smp  |       [%4d]      |                                \n", m_num_sample/NUM_SEG_t );
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_int  |       [%4d]      |                                \n", m_num_sample*4/NUM_SEG_t );
    printf("MODEL_TEST_SCANLINE_UbyU:    obj_mul  |       [%4d]      |                                \n", m_num_sample*4/NUM_SEG_t );
    printf("MODEL_TEST_SCANLINE_UbyU:  ____________________________________________________________________       \n");
    // clang-format on
    // colum text for AIE usage
    mdls.save4aie_oneScanline("UbyU");
    mdls.save4aie_fullSize(0, "UbyU", DATA_RFD);
    mdls.save4aie_fullSize(0, "UbyU", DATA_MUL);

    return 0;
}

template <class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t>
int scanline_UbyU() {
    us_models<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t> models;
    models.Init(example_1_speed_sound,   // = 1540.0;
                example_1_freq_sound,    // = 5*1000*1000;
                example_1_freq_sampling, // = 100*1000*1000;
                example_1_num_sample,    // = 2048;
                example_1_num_line,      // =  41;
                example_1_num_element,   // = 128;
                example_1_start_positions, example_1_info, RF_JSON, "./out_abt_beamform.json");
    scanline_UbyU<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t>(models);
    // json output
    models.saveJson("xf_output_res_UbyU.json");
    return 0;
}

#endif