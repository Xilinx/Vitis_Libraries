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

#include "json.hpp"
#include "us_models.hpp"
#include "scanline_ModuleByModule.hpp"
#include "scanline_LineByLine.hpp"
#include "scanline_ElementByElement.hpp"
#include "scanline_UnitByUnit.hpp"

#include "graph_scanline.hpp"

// This is used for the PL Kernels
#include "xrt.h"
#include "experimental/xrt_kernel.h"

// Using the ADF API that call XRT API
#include "adf/adf_api/XRTConfig.h"

// comment
const int NUM_LINE_t = example_1_num_line;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SAMPLE_t = example_1_num_sample; // 128;//2048;//64;
const int VECDIM_foc_t = 8;
const int NUM_SEG_t = 4; // cut each sample to 4 segment to minimize pipeline data
const int NUM_DEP_SEG_t = NUM_SAMPLE_t / NUM_SEG_t;
// for img
const int LEN_OUT_img_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_img_t = 7;
const int VECDIM_img_t = 8;
// for foc
const int LEN_OUT_foc_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_foc_t = 6;
// for delay
const int LEN_IN_delay_t = LEN_OUT_img_t;
const int LEN_OUT_delay_t = LEN_IN_delay_t;
const int LEN32b_PARA_delay_t = 17;
const int VECDIM_delay_t = 8;
// for apodi_preprocess
const int LEN_OUT_apodi_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_apodi_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_apodi_t = 12;
const int VECDIM_apodi_t = 8;
// for apodi_main
const int LEN_IN_apodi_f_t = NUM_ELEMENT_t;
const int LEN_IN_apodi_d_t = NUM_SAMPLE_t / NUM_SEG_t;
// for interpolation
const int NUM_UPSample_t = example_1_num_upSample; // just for calu LEN_OUT
const int LEN_OUT_interp_t = NUM_SAMPLE_t * NUM_UPSample_t / NUM_SEG_t;
const int LEN_IN_interp_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_interp_rf_t = NUM_SAMPLE_t;
const int LEN32b_PARA_interp_t = 9;
const int VECDIM_interp_t = 8;
// for sample
const int LEN_IN_sample_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_OUT_sample_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_sample_t = 12;
const int VECDIM_sample_t = 8;
// for mult
const int NUM_DEP_SEG_mult_t = NUM_SAMPLE_t * NUM_UPSample_t / NUM_SEG_t;
const int LEN_IN_mult_t = NUM_SAMPLE_t * NUM_UPSample_t / NUM_SEG_t;
const int LEN_OUT_mult_t = NUM_SAMPLE_t * NUM_UPSample_t / NUM_SEG_t;
const int LEN32b_PARA_mult_t = 8;
const int VECDIM_mult_t = 8;
const int MULT_ID_t = 0;

const int NUM_mult_CU_t = 4;

namespace us {
namespace L3 {

graph_scanline<float,
               NUM_LINE_t,
               NUM_ELEMENT_t,
               NUM_SAMPLE_t,
               NUM_SEG_t,
               NUM_DEP_SEG_t,
               VECDIM_img_t,
               LEN_OUT_img_t,
               LEN32b_PARA_img_t,
               VECDIM_foc_t,
               LEN_OUT_foc_t,
               LEN32b_PARA_foc_t,
               VECDIM_delay_t,
               LEN_IN_delay_t,
               LEN_OUT_delay_t,
               LEN32b_PARA_delay_t,
               VECDIM_apodi_t,
               LEN_IN_apodi_t,
               LEN_OUT_apodi_t,
               LEN32b_PARA_apodi_t,
               LEN_IN_apodi_f_t,
               LEN_IN_apodi_d_t,
               VECDIM_interp_t,
               LEN_IN_interp_t,
               LEN_IN_interp_rf_t,
               LEN_OUT_interp_t,
               NUM_UPSample_t,
               LEN32b_PARA_interp_t,
               VECDIM_sample_t,
               LEN_IN_sample_t,
               LEN_OUT_sample_t,
               LEN32b_PARA_sample_t,
               VECDIM_mult_t,
               LEN_IN_mult_t,
               LEN_OUT_mult_t,
               NUM_DEP_SEG_mult_t,
               MULT_ID_t,
               LEN32b_PARA_mult_t>
    g_scanline;

int scanline_AllinAIE(int num_invoking) {
    //////////////////////////////////////////
    // graph execution for AIE
    //////////////////////////////////////////
    // sample
    us::L1::para_sample_t<float> g_sam_para_const;
    g_sam_para_const.xdc_x = 0;
    g_sam_para_const.xdc_z = 0;
    g_sam_para_const.inv_speed_of_sound = example_1_inverse_speed_of_sound;
    g_sam_para_const.freq_sampling = example_1_freq_sampling;
    g_sam_para_const.rf_dim = 0;
    g_sam_para_const.iter_line = 0;
    g_sam_para_const.iter_element = 0;
    g_sam_para_const.iter_seg = 0;
    g_sam_para_const.num_line = NUM_LINE_t;
    g_sam_para_const.num_element = NUM_ELEMENT_t;
    g_sam_para_const.num_seg = NUM_SEG_t;
    g_sam_para_const.num_dep_seg = NUM_DEP_SEG_t;

    float g_sam_para_rfdim[NUM_LINE_t] = {0};
    for (int i = 0; i < NUM_LINE_t; i++) g_sam_para_rfdim[i] = (float)example_1_rf_data_dim[i];
    float* g_sam_para_elem = (float*)example_1_xdc_def_pos;

    // delay
    us::L1::para_delay_t<float> g_delay_para_const;
    g_delay_para_const.tx_ref_point_x = example_1_tx_ref_point_x;
    g_delay_para_const.tx_ref_point_z = example_1_tx_ref_point_z;
    g_delay_para_const.tileVApo_x = example_1_tileVApo_x;
    g_delay_para_const.tileVApo_z = example_1_tileVApo_z;
    g_delay_para_const.focal_point_x = example_1_focal_point_x;
    g_delay_para_const.focal_point_z = example_1_focal_point_z;
    g_delay_para_const.t_start = 0;
    g_delay_para_const.tx_delay_distance = example_1_tx_delay_distance;
    g_delay_para_const.tx_delay_distance_ = example_1_tx_delay_distance_;
    g_delay_para_const.inverse_speed_of_sound = example_1_inverse_speed_of_sound;
    g_delay_para_const.iter_line = 0; // 0;
    g_delay_para_const.iter_element = 0;
    g_delay_para_const.iter_seg = 0;
    g_delay_para_const.num_line = NUM_LINE_t;
    g_delay_para_const.num_element = NUM_ELEMENT_t;
    g_delay_para_const.num_seg = NUM_SEG_t;
    g_delay_para_const.num_dep_seg = NUM_DEP_SEG_t;

    float g_delay_t_start[NUM_LINE_t] = {0};
    for (int i = 0; i < NUM_LINE_t; i++) g_delay_t_start[i] = example_1_t_start[i];

    // apodi_prepprocess
    us::L1::para_Apodization<float> para_apodi_const;
    if (LEN32b_PARA_apodi_t != sizeof(para_apodi_const) / sizeof(float)) {
        printf("error : LEN32b_PARA_apodi_t != sizeof(para_apodi_const) / sizeof(float) ");
        exit(1);
    }
    para_apodi_const.iter_line = 0;
    para_apodi_const.iter_element = 0;
    para_apodi_const.iter_seg = 0;
    para_apodi_const.num_line = NUM_LINE_t;
    para_apodi_const.num_element = NUM_ELEMENT_t;
    para_apodi_const.num_seg = NUM_SEG_t;
    para_apodi_const.num_dep_seg = NUM_SAMPLE_t / para_apodi_const.num_seg;
    para_apodi_const.f_num = example_1_f_number;
    para_apodi_const.tileVApo_x = example_1_tileVApo_x;
    para_apodi_const.tileVApo_z = example_1_tileVApo_z;
    para_apodi_const.ref_point_x = example_1_ref_pos_x;
    para_apodi_const.ref_point_z = example_1_ref_pos_z;

    // apodi_main
    us::L1::para_Apodization<float> para_amain_const;
    if (LEN32b_PARA_apodi_t != sizeof(para_amain_const) / sizeof(float)) {
        printf("error : LEN32b_PARA_apodi_t != sizeof(para_amain_const) / sizeof(float) ");
        exit(1);
    }
    para_amain_const.iter_line = 0;
    para_amain_const.iter_element = 0;
    para_amain_const.iter_seg = 0;
    para_amain_const.num_line = NUM_LINE_t;
    para_amain_const.num_element = NUM_ELEMENT_t;
    para_amain_const.num_seg = NUM_SEG_t;
    para_amain_const.num_dep_seg = NUM_SAMPLE_t / para_amain_const.num_seg;
    para_amain_const.f_num = example_1_f_number;
    para_amain_const.tileVApo_x = example_1_tileVApo_x;
    para_amain_const.tileVApo_z = example_1_tileVApo_z;
    para_amain_const.ref_point_x = example_1_ref_pos_x;
    para_amain_const.ref_point_z = example_1_ref_pos_z;

    // interpolation
    us::L1::para_Interpolation<float> para_interp_const_0;
    us::L1::para_Interpolation<float> para_interp_const_1;
    us::L1::para_Interpolation<float> para_interp_const_2;
    us::L1::para_Interpolation<float> para_interp_const_3;
    if (LEN32b_PARA_interp_t != sizeof(para_interp_const_0) / sizeof(float)) {
        printf("Error : LEN32b_PARA_interp_t != sizeof(para_interp_const) / sizeof(float) ");
        exit(1);
    }
    para_interp_const_0.iter_line = 0;
    para_interp_const_0.iter_element = 0;
    para_interp_const_0.iter_seg = 0;
    para_interp_const_0.num_line = NUM_LINE_t;
    para_interp_const_0.num_element = NUM_ELEMENT_t;
    para_interp_const_0.num_seg = NUM_SEG_t;
    para_interp_const_0.num_depth = NUM_SAMPLE_t;
    para_interp_const_0.num_dep_seg = NUM_SAMPLE_t / para_interp_const_0.num_seg;
    para_interp_const_0.num_upSamp = NUM_UPSample_t;

    para_interp_const_1 = para_interp_const_0;
    para_interp_const_2 = para_interp_const_0;
    para_interp_const_3 = para_interp_const_0;
    float para_interp_local[NUM_SAMPLE_t] = {0};

    // mult
    us::L1::para_mult_t<float> para_mult_const_pre;
    us::L1::para_mult_t<float> para_mult_const_0;
    us::L1::para_mult_t<float> para_mult_const_1;
    us::L1::para_mult_t<float> para_mult_const_2;
    us::L1::para_mult_t<float> para_mult_const_3;
    para_mult_const_0.iter_line = 0;
    para_mult_const_0.iter_element = 0;
    para_mult_const_0.iter_seg = 0;
    para_mult_const_0.num_line = NUM_LINE_t;
    para_mult_const_0.num_element = NUM_ELEMENT_t;
    para_mult_const_0.num_seg = NUM_SEG_t;
    para_mult_const_0.num_dep_seg = NUM_DEP_SEG_mult_t;
    para_mult_const_0.mult_id = MULT_ID_t;

    para_mult_const_pre = para_mult_const_0;

    para_mult_const_1 = para_mult_const_0;
    para_mult_const_2 = para_mult_const_0;
    para_mult_const_3 = para_mult_const_0;

    para_mult_const_1.mult_id = 1;
    para_mult_const_2.mult_id = 2;
    para_mult_const_3.mult_id = 3;

    float para_mult_local_0_0[NUM_DEP_SEG_mult_t] = {0};
    float para_mult_local_0_1[NUM_DEP_SEG_mult_t] = {0};
    float para_mult_local_0_2[NUM_DEP_SEG_mult_t] = {0};
    float para_mult_local_0_3[NUM_DEP_SEG_mult_t] = {0};

    float para_mult_local_1_0[NUM_DEP_SEG_mult_t] = {0};
    float para_mult_local_1_1[NUM_DEP_SEG_mult_t] = {0};
    float para_mult_local_1_2[NUM_DEP_SEG_mult_t] = {0};
    float para_mult_local_1_3[NUM_DEP_SEG_mult_t] = {0};

    // foc
    us::L1::para_foc_t para_foc_const(NUM_LINE_t, NUM_ELEMENT_t, NUM_SEG_t);
    // image x and z
    us::L1::para_img_t<float> para_img_x(example_1_directions[0], NUM_LINE_t, NUM_ELEMENT_t, NUM_SEG_t);
    us::L1::para_img_t<float> para_img_z(example_1_directions[2], NUM_LINE_t, NUM_ELEMENT_t, NUM_SEG_t);
    float p_para_img_start_x[NUM_LINE_t];
    float p_para_img_start_z[NUM_LINE_t];
    for (int i = 0; i < NUM_LINE_t; i++) {
        p_para_img_start_x[i] = example_1_start_positions[0];
        p_para_img_start_z[i] = example_1_start_positions[2];
    }
    // clang-format off
    printf("********** Static Parameter ********************\n");
    printf("Const: NUM_LINE_t              = %d\n", NUM_LINE_t);    //%d\n", )//NUM_LINE_t );
    printf("Const: NUM_ELEMENT_t           = %d\n", NUM_ELEMENT_t); // 128;
    printf("Const: NUM_SAMPLE_t            = %d\n", NUM_SAMPLE_t);  // 2048;
    printf("Const: VECDIM_foc_t            = %d\n", VECDIM_foc_t);  // 8;
    printf("Const: NUM_SEG_t               = %d\n", NUM_SEG_t);     // 4;
    printf("Const: num_invoking            = %d\n", num_invoking);
    // for img
    printf("Const: VECDIM_img_t            = %d\n", VECDIM_img_t);      // 8;
    printf("Const: LEN_OUT_img_t           = %d\n", LEN_OUT_img_t);     // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_img_t       = %d\n", LEN32b_PARA_img_t); // 7;
    // for foc
    printf("Const: LEN_OUT_foc_t           = %d\n", LEN_OUT_foc_t);     // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_foc_t       = %d\n", LEN32b_PARA_foc_t); // 6;
    // for delay
    printf("Const: LEN32b_PARA_delay_t     = %d\n", LEN32b_PARA_delay_t); // 17;
    printf("Const: LEN_IN_delay_t          = %d\n", LEN_IN_delay_t);      // LEN_OUT_img_t;
    printf("Const: LEN_OUT_delay_t         = %d\n", LEN_OUT_delay_t);     // LEN_IN_delay_t;
    printf("Const: VECDIM_delay_t          = %d\n", VECDIM_delay_t);      // 8;
    // for apodi_preprocess
    printf("Const: LEN_OUT_apodi_t         = %d\n", LEN_OUT_apodi_t);     // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN_IN_apodi_t          = %d\n", LEN_IN_apodi_t);      // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_apodi_t     = %d\n", LEN32b_PARA_apodi_t); // 12;
    printf("Const: VECDIM_apodi_t          = %d\n", VECDIM_apodi_t);      // 8;
    // for apodi_main
    printf("Const: LEN_IN_apodi_f_t        = %d\n", LEN_IN_apodi_f_t); // NUM_ELEMENT_t;
    printf("Const: LEN_IN_apodi_d_t        = %d\n", LEN_IN_apodi_d_t); // NUM_SAMPLE_t/NUM_SEG_t;
    // for interpolation
    printf("Const: NUM_UPSample_t          = %d\n", NUM_UPSample_t);       // 4;//just for calu LEN_OUT
    printf("Const: LEN_OUT_interp_t        = %d\n", LEN_OUT_interp_t);     // NUM_SAMPLE_t*NUM_UPSample_t/NUM_SEG_t;
    printf("Const: LEN_IN_interp_t         = %d\n", LEN_IN_interp_t);      // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN_IN_interp_rf_t      = %d\n", LEN_IN_interp_rf_t);   // NUM_SAMPLE_t;
    printf("Const: LEN32b_PARA_interp_t    = %d\n", LEN32b_PARA_interp_t); // 9;
    printf("Const: VECDIM_interp_t         = %d\n", VECDIM_interp_t);      // 8;
    // for sampel
    printf("Const: VECDIM_sample_t         = %d\n", VECDIM_sample_t);      // 8;
    printf("Const: LEN_IN_sample_t         = %d\n", LEN_IN_sample_t);      // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN_OUT_sample_t        = %d\n", LEN_OUT_sample_t);     // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_sample_t    = %d\n", LEN32b_PARA_sample_t); // 12;
    // for mult
    printf("Const: VECDIM_mult_t           = %d\n", VECDIM_mult_t);      // 8;
    printf("Const: LEN_IN_mult_t           = %d\n", LEN_IN_mult_t);      // NUM_SAMPLE_t*NUM_UPSample_t/NUM_SEG_t;
    printf("Const: LEN_OUT_mult_t          = %d\n", LEN_OUT_mult_t);     // NUM_SAMPLE_t*NUM_UPSample_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_mult_t      = %d\n", LEN32b_PARA_mult_t); // 8;
    printf("********** RTP Parameter ********************\n");
    printf("RTP: para_mult_const           size = %d Byte\n", sizeof(para_mult_const_0));
    printf("RTP: para_interp_const         size = %d Byte\n", sizeof(para_interp_const_0));
    printf("RTP: para_amain_const          size = %d Byte\n", sizeof(para_amain_const));
    printf("RTP: para_apodi_const          size = %d Byte\n", sizeof(para_apodi_const));
    printf("RTP: g_sam_para_const          size = %d Byte\n", sizeof(g_sam_para_const));
    printf("RTP: g_sam_para_rfdim          size = %d Byte\n", sizeof(g_sam_para_rfdim));
    printf("RTP: g_sam_para_elem           size = %d Byte\n", sizeof(g_sam_para_elem));
    printf("RTP: g_delay_para_const        size = %d Byte\n", sizeof(g_delay_para_const));
    printf("RTP: g_delay_t_start           size = %d Byte\n", sizeof(g_delay_t_start));
    printf("RTP: para_foc_const            size = %d Byte\n", sizeof(para_foc_const));
    printf("RTP: example_1_xdc_def_pos_xz  size = %d Byte\n", sizeof(example_1_xdc_def_pos_xz));

#ifdef _USING_SHELL_
    printf("\nNOTE!! Shell-kernel used\n");
#else
    printf("\nNOTE!! Normal kernel used\n");
#endif

    g_scanline.init();
#if defined(__HW_EMU__) || defined(__HW__)
    const int WINDOW_SIZE_in_bytes = NUM_SAMPLE_t / NUM_SEG_t * sizeof(float);
    event::handle handle = event::start_profiling(g_scanline.g_interp_rfdatain, event::io_stream_start_to_bytes_transferred_cycles, WINDOW_SIZE_in_bytes*num_invoking);
    if(handle==event::invalid_handle){
        printf("ERROR:Invalid handle. Only two performance counter in a AIE-PL interface tile\n");
        return 1;
    }
#endif
	//mult
#ifdef ENABLE_MULT
	printf("NOTE!! ENABLE_INTERPOLATION\n");
    g_scanline.update(g_scanline.para_mult_const_pre,       (float*)(&para_mult_const_pre),         LEN32b_PARA_mult_t);

    g_scanline.update(g_scanline.para_mult_const_0, 		(float*)(&para_mult_const_0), 			LEN32b_PARA_mult_t);
    g_scanline.update(g_scanline.para_mult_const_1, 		(float*)(&para_mult_const_1), 			LEN32b_PARA_mult_t);
    g_scanline.update(g_scanline.para_mult_const_2, 		(float*)(&para_mult_const_2), 			LEN32b_PARA_mult_t);
    g_scanline.update(g_scanline.para_mult_const_3, 		(float*)(&para_mult_const_3), 			LEN32b_PARA_mult_t);

    g_scanline.update(g_scanline.para_mult_local_0_0, 		para_mult_local_0_0, 					NUM_DEP_SEG_mult_t);
    g_scanline.update(g_scanline.para_mult_local_0_1, 		para_mult_local_0_1, 					NUM_DEP_SEG_mult_t);
    g_scanline.update(g_scanline.para_mult_local_0_2, 		para_mult_local_0_2, 					NUM_DEP_SEG_mult_t);
    g_scanline.update(g_scanline.para_mult_local_0_3, 		para_mult_local_0_3, 					NUM_DEP_SEG_mult_t);
							
    g_scanline.update(g_scanline.para_mult_local_1_0, 		para_mult_local_1_0, 					NUM_DEP_SEG_mult_t);
    g_scanline.update(g_scanline.para_mult_local_1_1, 		para_mult_local_1_1, 					NUM_DEP_SEG_mult_t);
    g_scanline.update(g_scanline.para_mult_local_1_2, 		para_mult_local_1_2, 					NUM_DEP_SEG_mult_t);
    g_scanline.update(g_scanline.para_mult_local_1_3, 		para_mult_local_1_3, 					NUM_DEP_SEG_mult_t);
#endif
	//interpolation
#ifdef ENABLE_INTERPOLATION
	printf("NOTE!! ENABLE_INTERPOLATION\n");
    g_scanline.update(g_scanline.para_interp_local,       para_interp_local,                    sizeof(para_interp_local) / sizeof(float));
	g_scanline.update(g_scanline.para_interp_const_0,     (int *)(&para_interp_const_0),        sizeof(para_interp_const_0) / sizeof(float));
    g_scanline.update(g_scanline.para_interp_const_1,     (int *)(&para_interp_const_1),        sizeof(para_interp_const_1) / sizeof(float));
    g_scanline.update(g_scanline.para_interp_const_2,     (int *)(&para_interp_const_2),        sizeof(para_interp_const_2) / sizeof(float));
    g_scanline.update(g_scanline.para_interp_const_3,     (int *)(&para_interp_const_3),        sizeof(para_interp_const_3) / sizeof(float));
#endif
	//apodi_main
#ifdef ENABLE_APODI_MAIN
	printf("NOTE!! ENABLE_APODI_MAIN\n");
	g_scanline.update(g_scanline.para_amain_const,      (int *)(&para_amain_const),         sizeof(para_amain_const) / sizeof(float));
#endif
	//apodi_preprocess
#ifdef ENABLE_APODI_PRE
	printf("NOTE!! ENABLE_APODI_PRE\n");
	g_scanline.update(g_scanline.para_apodi_const,      (int *)(&para_apodi_const),         sizeof(para_apodi_const) / sizeof(float));
#endif
	//sample
#ifdef ENABLE_SAMPLE
	printf("NOTE!! ENABLE_SAMPLE\n");
	g_scanline.update(g_scanline.g_sam_para_const, 		(float*)(&g_sam_para_const), 		LEN32b_PARA_sample_t);
    g_scanline.update(g_scanline.g_sam_para_rfdim, 		g_sam_para_rfdim, 					NUM_LINE_t);
    g_scanline.update(g_scanline.g_sam_para_elem, 		g_sam_para_elem, 					NUM_ELEMENT_t*4);
#endif
	//delay 
#ifdef ENABLE_DELAY
	printf("NOTE!! ENABLE_DELAY\n");
	g_scanline.update(g_scanline.g_delay_para_const, 	(float*)(&g_delay_para_const), 		sizeof(g_delay_para_const)/sizeof(float));
    g_scanline.update(g_scanline.g_delay_para_t_start, 	g_delay_t_start, 					NUM_LINE_t);
#endif
	//foc
#ifdef ENABLE_FOC
	printf("NOTE!! ENABLE_FOC\n");
    g_scanline.update(g_scanline.g_foc_p_para_const, 	(float*)(&para_foc_const), 			sizeof(para_foc_const)/sizeof(float));
    g_scanline.update(g_scanline.g_foc_p_para_pos, 		(float*)(&example_1_xdc_def_pos_xz), NUM_ELEMENT_t*4);//sizeof(example_1_xdc_def_pos)/sizeof(float));
#endif
	//3 imagePoints
	g_scanline.update(g_scanline.g_img_z1_p_para_const, (float*)(&para_img_z), 				sizeof(para_img_z)/sizeof(float));
    g_scanline.update(g_scanline.g_img_z1_p_para_start, (float*)(&p_para_img_start_z), 		NUM_LINE_t);
	g_scanline.update(g_scanline.g_img_z2_p_para_const, (float*)(&para_img_z), 				sizeof(para_img_z)/sizeof(float));
    g_scanline.update(g_scanline.g_img_z2_p_para_start, (float*)(&p_para_img_start_z), 		NUM_LINE_t);
	g_scanline.update(g_scanline.g_img_z3_p_para_const, (float*)(&para_img_z), 				sizeof(para_img_z)/sizeof(float));
    g_scanline.update(g_scanline.g_img_z3_p_para_start, (float*)(&p_para_img_start_z), 		NUM_LINE_t);
	g_scanline.update(g_scanline.g_img_x1_p_para_const, (float*)(&para_img_x), 				sizeof(para_img_x)/sizeof(float));
    g_scanline.update(g_scanline.g_img_x1_p_para_start, (float*)(&p_para_img_start_x), 		NUM_LINE_t);
	g_scanline.update(g_scanline.g_img_x2_p_para_const, (float*)(&para_img_x), 				sizeof(para_img_x)/sizeof(float));
    g_scanline.update(g_scanline.g_img_x2_p_para_start, (float*)(&p_para_img_start_x), 		NUM_LINE_t);
	g_scanline.update(g_scanline.g_img_x3_p_para_const, (float*)(&para_img_x), 				sizeof(para_img_x)/sizeof(float));
    g_scanline.update(g_scanline.g_img_x3_p_para_start, (float*)(&p_para_img_start_x), 		NUM_LINE_t);

    g_scanline.run(num_invoking);

// clang-format on
#if defined(__HW_EMU__) || defined(__HW__)
    g_scanline.wait(); // performance counter 0 stops, assumming s2mm able to receive all data
    long long cycle_count = event::read_profiling(handle);
    double throughput = (double)WINDOW_SIZE_in_bytes * num_invoking / (cycle_count * 1e-9); // bytes per second
    printf("[HOST] Profiling of g_scanline! throughput = %.9f bytes per second!\n", throughput);
    printf("[HOST] Total cycle count = %d \n", cycle_count);
    event::stop_profiling(handle); // Performance counter is released and cleared
#endif

    g_scanline.end();

    return 0;
}
} // namespace
} //