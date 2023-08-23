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

#ifndef __GRAPH_SCANLINE_HPP__
#define __GRAPH_SCANLINE_HPP__

#include <adf.h>

#include "us_example_parameter.hpp"
#include "graph_focusing.hpp"
#include "graph_imagepoints.hpp"
#include "graph_delay.hpp"
#include "graph_samples.hpp"
#include "graph_apodization.hpp"
#include "graph_interpolation.hpp"
#include "graph_mult.hpp"

using namespace adf;
using namespace aie;

#define ENABLE_FOC
#define ENABLE_DELAY
#define ENABLE_SAMPLE
#define ENABLE_APODI_PRE
#define ENABLE_APODI_MAIN
#define ENABLE_INTERPOLATION
#define ENABLE_MULT

// Enable output all intermediate results include imagepoint focusing delay sample interpolation apodization.
// #define ENABLE_DEBUGGING
// clang-format off
template<class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t, int NUM_DEP_SEG_t,
int VECDIM_img_t, int LEN_OUT_img_t, int LEN32b_PARA_img_t,
int VECDIM_foc_t, int LEN_OUT_foc_t, int LEN32b_PARA_foc_t,
int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t,
int VECDIM_apodi_t, int LEN_IN_apodi_t, int LEN_OUT_apodi_t, int LEN32b_PARA_apodi_t,
int LEN_IN_apodi_f_t, int LEN_IN_apodi_d_t,
int VECDIM_interp_t, int LEN_IN_interp_t, int LEN_IN_interp_rf_t, int LEN_OUT_interp_t, int NUM_UPSample_t, int LEN32b_PARA_interp_t,
int VECDIM_sample_t, int LEN_IN_sample_t, int LEN_OUT_sample_t, int LEN32b_PARA_sample_t,
int VECDIM_mult_t, int LEN_IN_mult_t, int LEN_OUT_mult_t, int NUM_DEP_SEG_mult_t, int MULT_ID_t, int LEN32b_PARA_mult_t>
// clang-format on
class graph_scanline : public graph {
   public:
#ifdef ENABLE_FOC
    // for foc
    port<input> g_foc_p_para_const;
    port<input> g_foc_p_para_pos;
    // output_plio g_foc_dataout1;//temp for debugging
    us::L2::graph_foc_wrapper<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t, LEN32b_PARA_foc_t>
        g_foc;
#endif
    void init_foc() {
#ifdef ENABLE_FOC
        // g_foc_dataout1=output_plio::create(plio_32_bits,  "data/foc_out.txt");
        connect<>(g_foc_p_para_const, g_foc.p_para_const);
        connect<>(g_foc_p_para_pos, g_foc.p_para_pos);
// connect< >(g_foc.dataout1, g_foc_dataout1.in[0]);
#endif
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // for  img_z1
    port<input> g_img_z1_p_para_const;
    port<input> g_img_z1_p_para_start;
    // output_plio g_img_z1_dataout1; // temp for debugging
    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img_z1;
    // for  img_x1
    port<input> g_img_x1_p_para_const;
    port<input> g_img_x1_p_para_start;
    // output_plio g_img_x1_dataout1; // temp for debugging
    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img_x1;

    // for  img_z2
    port<input> g_img_z2_p_para_const;
    port<input> g_img_z2_p_para_start;
    // output_plio g_img_z2_dataout1; // temp for debugging
    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img_z2;
    // for  img_x2
    port<input> g_img_x2_p_para_const;
    port<input> g_img_x2_p_para_start;
    // output_plio g_img_x2_dataout1; // temp for debugging
    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img_x2;

    // for  img_z3
    port<input> g_img_z3_p_para_const;
    port<input> g_img_z3_p_para_start;
    // output_plio g_img_z3_dataout1; // temp for debugging
    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img_z3;
    // for  img_x3
    port<input> g_img_x3_p_para_const;
    port<input> g_img_x3_p_para_start;
    // output_plio g_img_x3_dataout1; // temp for debugging
    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img_x3;
    void init_img() {
        printf("Scanline L2 Graph initialization by using init_img_foc() \n");

        // g_img_z1_dataout1=output_plio::create(plio_32_bits,  "data/img_z1_file_out.txt");
        // g_img_z2_dataout1=output_plio::create(plio_32_bits,  "data/img_z2_file_out.txt");
        // g_img_z3_dataout1=output_plio::create(plio_32_bits,  "data/img_z3_file_out.txt");
        // g_img_x1_dataout1=output_plio::create(plio_32_bits,  "data/img_x1_file_out.txt");
        // g_img_x2_dataout1=output_plio::create(plio_32_bits,  "data/img_x2_file_out.txt");
        // g_img_x3_dataout1=output_plio::create(plio_32_bits,  "data/img_x3_file_out.txt");

        connect<>(g_img_z1_p_para_const, g_img_z1.p_para_const);
        connect<>(g_img_z1_p_para_start, g_img_z1.p_para_start);
        // connect< >(g_img_z1.dataout1, g_img_z1_dataout1.in[0]);
        connect<>(g_img_z2_p_para_const, g_img_z2.p_para_const);
        connect<>(g_img_z2_p_para_start, g_img_z2.p_para_start);
        // connect< >(g_img_z2.dataout1, g_img_z2_dataout1.in[0]);
        connect<>(g_img_z3_p_para_const, g_img_z3.p_para_const);
        connect<>(g_img_z3_p_para_start, g_img_z3.p_para_start);
        // connect< >(g_img_z3.dataout1, g_img_z3_dataout1.in[0]);

        connect<>(g_img_x1_p_para_const, g_img_x1.p_para_const);
        connect<>(g_img_x1_p_para_start, g_img_x1.p_para_start);
        // connect< >(g_img_x1.dataout1, g_img_x1_dataout1.in[0]);
        connect<>(g_img_x2_p_para_const, g_img_x2.p_para_const);
        connect<>(g_img_x2_p_para_start, g_img_x2.p_para_start);
        // connect< >(g_img_x2.dataout1, g_img_x2_dataout1.in[0]);
        connect<>(g_img_x3_p_para_const, g_img_x3.p_para_const);
        connect<>(g_img_x3_p_para_start, g_img_x3.p_para_start);
        // connect< >(g_img_x3.dataout1, g_img_x3_dataout1.in[0]);
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_DELAY
    // for delay
    adf::port<adf::direction::in> g_delay_para_const;
    adf::port<adf::direction::in> g_delay_para_t_start;
    adf::port<adf::direction::in> g_delay_para_iter;
    adf::input_plio img_x;
    adf::input_plio img_z;
    // adf::output_plio g_delay_delay;
    us::L2::delay_graph_wrapper<T, NUM_LINE_t, VECDIM_delay_t, LEN_IN_delay_t, LEN_OUT_delay_t, LEN32b_PARA_delay_t>
        g_delay;
#endif

    void init_delay() {
#ifdef ENABLE_DELAY
        // g_delay_delay = adf::output_plio::create(adf::plio_32_bits, "data/delay.txt");
        adf::connect<adf::parameter>(g_delay_para_const, g_delay.para_const);
        adf::connect<adf::parameter>(g_delay_para_t_start, g_delay.para_t_start);
        adf::connect<>(g_img_x1.dataout1, g_delay.img_x);
        adf::connect<>(g_img_z1.dataout1, g_delay.img_z);
// adf::connect<>(g_delay.delay, g_delay_delay.in[0]);
#endif
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for sample
#ifdef ENABLE_SAMPLE
    adf::port<adf::direction::in> g_sam_para_const;
    adf::port<adf::direction::in> g_sam_para_rfdim;
    adf::port<adf::direction::in> g_sam_para_elem;
    // adf::output_plio g_sam_sample;
    // adf::output_plio g_sam_inside;
    adf::input_plio g_sam_img_x;
    adf::input_plio g_sam_img_z;
    adf::input_plio g_sam_delay;
    us::L2::sample_graph_wrapper<T,
                                 NUM_LINE_t,
                                 NUM_ELEMENT_t,
                                 VECDIM_sample_t,
                                 LEN_IN_sample_t,
                                 LEN_OUT_sample_t,
                                 LEN32b_PARA_sample_t>
        g_sam;
#endif
    void init_sam() {
#ifdef ENABLE_SAMPLE
        // g_sam_sample = output_plio::create(plio_32_bits, "data/sample.txt");
        // g_sam_inside = output_plio::create(plio_32_bits, "data/inside.txt");
        adf::connect<adf::parameter>(g_sam_para_const, g_sam.para_const);
        adf::connect<adf::parameter>(g_sam_para_rfdim, g_sam.para_rfdim);
        adf::connect<adf::parameter>(g_sam_para_elem, g_sam.para_elem);
        adf::connect<>(g_img_x2.dataout1, g_sam.img_x);
        adf::connect<>(g_img_z2.dataout1, g_sam.img_z);
        adf::connect<>(g_delay.delay, g_sam.delay);
// adf::connect<>(g_sam.sample, g_sam_sample.in[0]);
// adf::connect<>(g_sam.inside, g_sam_inside.in[0]);
#endif
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ENABLE_APODI_PRE
    // for apodi_preprocess
    port<input> para_apodi_const;
    // adf::output_plio g_apodi_pre_dataout;
    us::L2::apodi_pre_graph<T,
                            NUM_LINE_t,
                            NUM_ELEMENT_t,
                            NUM_SAMPLE_t,
                            NUM_SEG_t,
                            LEN_OUT_apodi_t,
                            LEN_IN_apodi_t,
                            VECDIM_apodi_t,
                            LEN32b_PARA_apodi_t>
        g_apodi_preprocess;
#endif

    void init_apodi_pre() {
#ifdef ENABLE_APODI_PRE
        // for apodi_preprocess
        // g_apodi_pre_dataout = output_plio::create(plio_32_bits, "data/apodi_pre.txt");
        connect<>(para_apodi_const, g_apodi_preprocess.para_apodi_const);
        connect<>(g_img_x3.dataout1, g_apodi_preprocess.img_x_in);
        connect<>(g_img_z3.dataout1, g_apodi_preprocess.img_z_in);
// connect<>(g_apodi_preprocess.out, g_apodi_pre_dataout.in[0]);
#endif
    }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_APODI_MAIN
    // for apodi_main
    port<input> para_amain_const;
    // adf::output_plio g_apodi_dataout;
    us::L2::apodi_main_graph<T,
                             NUM_LINE_t,
                             NUM_ELEMENT_t,
                             NUM_SAMPLE_t,
                             NUM_SEG_t,
                             LEN_OUT_apodi_t,
                             LEN_IN_apodi_f_t,
                             LEN_IN_apodi_d_t,
                             VECDIM_apodi_t,
                             LEN32b_PARA_apodi_t>
        g_apodi_main;
#endif
    void init_apodi_main() {
#ifdef ENABLE_APODI_MAIN
        // for apodi_main
        // g_apodi_dataout = adf::output_plio::create(adf::plio_32_bits, "data/apodi_main.txt");
        connect<>(para_amain_const, g_apodi_main.para_amain_const);
        connect<>(g_foc.dataout1, g_apodi_main.p_focal);
        connect<>(g_apodi_preprocess.out, g_apodi_main.p_invD);
// connect<>(g_apodi_main.out, g_apodi_dataout.in[0]);
#endif
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for interpolation
#ifdef ENABLE_INTERPOLATION
    port<input> para_interp_local;
    port<input> para_interp_const_0;
    port<input> para_interp_const_1;
    port<input> para_interp_const_2;
    port<input> para_interp_const_3;
    adf::input_plio g_interp_rfdatain;
    // adf::output_plio g_interp_dataout;
    us::L2::interpolation_graph<T,
                                NUM_LINE_t,
                                NUM_ELEMENT_t,
                                NUM_SAMPLE_t,
                                NUM_SEG_t,
                                LEN_OUT_interp_t,
                                LEN_IN_interp_t,
                                LEN_IN_interp_rf_t,
                                VECDIM_interp_t,
                                LEN32b_PARA_interp_t>
        g_interpolation;
#endif

    void init_interp(std::string input_rfd_file_name) {
// for interpolation
#ifdef ENABLE_INTERPOLATION
        g_interp_rfdatain = adf::input_plio::create("g_interp_rfdatain", adf::plio_32_bits, input_rfd_file_name);
        // g_interp_dataout = adf::output_plio::create(adf::plio_32_bits, "data/interpolation.txt");
        adf::connect<adf::parameter>(para_interp_const_0, g_interpolation.para_interp_const_0);
        adf::connect<adf::parameter>(para_interp_const_1, g_interpolation.para_interp_const_1);
        adf::connect<adf::parameter>(para_interp_const_2, g_interpolation.para_interp_const_2);
        adf::connect<adf::parameter>(para_interp_const_3, g_interpolation.para_interp_const_3);
        adf::connect<adf::parameter>(para_interp_local, g_interpolation.para_local);
        connect<>(g_sam.sample, g_interpolation.p_sample_in);
        connect<>(g_sam.inside, g_interpolation.p_inside_in);
        connect<>(g_interp_rfdatain.out[0], g_interpolation.p_rfdata_in);
// connect<>(g_interpolation.out, g_interp_dataout.in[0]);
#endif
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for mult
#ifdef ENABLE_MULT
    port<direction::in> para_mult_const_pre;
    port<direction::in> para_mult_const_0;
    port<direction::in> para_mult_const_1;
    port<direction::in> para_mult_const_2;
    port<direction::in> para_mult_const_3;

    port<direction::in> para_mult_local_0_0;
    port<direction::in> para_mult_local_0_1;
    port<direction::in> para_mult_local_0_2;
    port<direction::in> para_mult_local_0_3;
    port<direction::in> para_mult_local_1_0;
    port<direction::in> para_mult_local_1_1;
    port<direction::in> para_mult_local_1_2;
    port<direction::in> para_mult_local_1_3;

    input_plio interp;
    input_plio apod;
    output_plio mult_0;
    output_plio mult_1;
    output_plio mult_2;
    output_plio mult_3;

    us::L2::mult_graph_wrapper<T,
                               NUM_LINE_t,
                               NUM_ELEMENT_t,
                               NUM_SAMPLE_t,
                               NUM_SEG_t,
                               NUM_DEP_SEG_mult_t,
                               VECDIM_mult_t,
                               LEN_IN_mult_t,
                               LEN_OUT_mult_t,
                               LEN32b_PARA_mult_t,
                               MULT_ID_t>
        g_mult;
#endif

    void init_mult() {
// for mult
#ifdef ENABLE_MULT
        mult_0 = output_plio::create("mult_0", plio_32_bits, "data/mult0.txt");
        mult_1 = output_plio::create("mult_1", plio_32_bits, "data/mult1.txt");
        mult_2 = output_plio::create("mult_2", plio_32_bits, "data/mult2.txt");
        mult_3 = output_plio::create("mult_3", plio_32_bits, "data/mult3.txt");

        connect<parameter>(para_mult_const_pre, g_mult.para_const_pre);
        connect<parameter>(para_mult_const_0, g_mult.para_const_0);
        connect<parameter>(para_mult_const_1, g_mult.para_const_1);
        connect<parameter>(para_mult_const_2, g_mult.para_const_2);
        connect<parameter>(para_mult_const_3, g_mult.para_const_3);

        connect<parameter>(para_mult_local_0_0, g_mult.para_local_0_0);
        connect<parameter>(para_mult_local_0_1, g_mult.para_local_0_1);
        connect<parameter>(para_mult_local_0_2, g_mult.para_local_0_2);
        connect<parameter>(para_mult_local_0_3, g_mult.para_local_0_3);

        connect<parameter>(para_mult_local_1_0, g_mult.para_local_1_0);
        connect<parameter>(para_mult_local_1_1, g_mult.para_local_1_1);
        connect<parameter>(para_mult_local_1_2, g_mult.para_local_1_2);
        connect<parameter>(para_mult_local_1_3, g_mult.para_local_1_3);

        connect<>(g_interpolation.out, g_mult.interp);
        connect<>(g_apodi_main.out, g_mult.apod);
        connect<>(g_mult.mult_0, mult_0.in[0]);
        connect<>(g_mult.mult_1, mult_1.in[0]);
        connect<>(g_mult.mult_2, mult_2.in[0]);
        connect<>(g_mult.mult_3, mult_3.in[0]);

#endif
    }

/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void init_img_foc_delay_samp_apo_apo_inter()
{
        printf("Scanline L2 Graph initialization by using init_img_foc_delay_samp_apo_apo_inter() \n");
        g_interp_rfdatain=adf::input_plio::create(adf::plio_32_bits,  "data/p_rf.txt");

        //for interpolation
        g_interp_dataout = adf::output_plio::create(adf::plio_32_bits, "data/interpolation.txt");
        adf::connect<adf::parameter>(para_interp_const, g_interpolation.para_interp_const);
        connect<>(g_sam.sample, g_interpolation.p_sample_in);
        connect<>(g_sam.inside, g_interpolation.p_inside_in);
        connect<>(g_interp_rfdatain.out[0], g_interpolation.p_rfdata_in);
        connect<>(g_interpolation.out, g_interp_dataout.in[0]);


        //for apodi_main
        g_apodi_dataout = adf::output_plio::create(adf::plio_32_bits, "data/apodi_main.txt");
        adf::connect<adf::parameter>(para_amain_const, g_apodi_main.para_amain_const);
        connect<>(g_foc.dataout1, g_apodi_main.p_focal);
        connect<>(g_apodi_preprocess.out, g_apodi_main.p_invD);
        connect<>(g_apodi_main.out, g_apodi_dataout.in[0]);

        //for apodi_preprocess
        //g_apodi_pre_dataout = adf::output_plio::create(adf::plio_32_bits, "data/apodi_pre.txt");
        adf::connect<adf::parameter>(para_apodi_const, g_apodi_preprocess.para_apodi_const);
        connect<>(g_img_x3.dataout1, g_apodi_preprocess.img_x_in);
        connect<>(g_img_z3.dataout1, g_apodi_preprocess.img_z_in);
        //connect<>(g_apodi_preprocess.out, g_apodi_pre_dataout.in[0]);

        // for g_sam_
        //img_x = input_plio::create("Datain0", plio_32_bits, "data/img_x.txt");
        //img_z = input_plio::create("Datain1", plio_32_bits, "data/img_z.txt");
        //delay = input_plio::create("Datain2", plio_32_bits, "data/delay.txt");
        //g_sam_sample = output_plio::create(plio_32_bits, "data/sample.txt");
        //g_sam_inside = output_plio::create(plio_32_bits, "data/inside.txt");

        // connections
        adf::connect<adf::parameter>(g_sam_para_const, g_sam.para_const);
        adf::connect<adf::parameter>(g_sam_para_rfdim, g_sam.para_rfdim);
        adf::connect<adf::parameter>(g_sam_para_elem,  g_sam.para_elem);
        adf::connect<>(g_img_x2.dataout1, g_sam.img_x);
        adf::connect<>(g_img_z2.dataout1, g_sam.img_z);
        adf::connect<>(g_delay.delay, g_sam.delay);
        //adf::connect<>(g_sam.sample, g_sam_sample.in[0]);
        //adf::connect<>(g_sam.inside, g_sam_inside.in[0]);

        //for delay
        //g_delay_delay = adf::output_plio::create(adf::plio_32_bits, "data/delay.txt");
        adf::connect<adf::parameter>(g_delay_para_const, g_delay.para_const);
        adf::connect<adf::parameter>(g_delay_para_t_start, g_delay.para_t_start);
        adf::connect<>(g_img_x1.dataout1, g_delay.img_x);
        adf::connect<>(g_img_z1.dataout1, g_delay.img_z);
        //adf::connect<>(g_delay.delay, g_delay_delay.in[0]);

        //tmp outputs to files
        //g_foc_dataout1=output_plio::create(plio_32_bits,  "data/foc_out.txt");
        connect< >(g_foc_p_para_const, g_foc.p_para_const);
        connect< >(g_foc_p_para_pos, g_foc.p_para_pos);
        //connect< >(g_foc.dataout1, g_foc_dataout1.in[0]);


        //g_img_z1_dataout1=output_plio::create(plio_32_bits,  "data/img_z1_file_out.txt");
        g_img_z2_dataout1=output_plio::create(plio_32_bits,  "data/img_z2_file_out.txt");
        g_img_z3_dataout1=output_plio::create(plio_32_bits,  "data/img_z3_file_out.txt");
        //g_img_x1_dataout1=output_plio::create(plio_32_bits,  "data/img_x1_file_out.txt");
        g_img_x2_dataout1=output_plio::create(plio_32_bits,  "data/img_x2_file_out.txt");
        g_img_x3_dataout1=output_plio::create(plio_32_bits,  "data/img_x3_file_out.txt");


        connect< >(g_img_z1_p_para_const, g_img_z1.p_para_const);
        connect< >(g_img_z1_p_para_start, g_img_z1.p_para_start);
        //connect< >(g_img_z1.dataout1, g_img_z1_dataout1.in[0]);
        connect< >(g_img_z2_p_para_const, g_img_z2.p_para_const);
        connect< >(g_img_z2_p_para_start, g_img_z2.p_para_start);
        connect< >(g_img_z2.dataout1, g_img_z2_dataout1.in[0]);
        connect< >(g_img_z3_p_para_const, g_img_z3.p_para_const);
        connect< >(g_img_z3_p_para_start, g_img_z3.p_para_start);
        connect< >(g_img_z3.dataout1, g_img_z3_dataout1.in[0]);

        connect< >(g_img_x1_p_para_const, g_img_x1.p_para_const);
        connect< >(g_img_x1_p_para_start, g_img_x1.p_para_start);
        //connect< >(g_img_x1.dataout1, g_img_x1_dataout1.in[0]);
        connect< >(g_img_x2_p_para_const, g_img_x2.p_para_const);
        connect< >(g_img_x2_p_para_start, g_img_x2.p_para_start);
        connect< >(g_img_x2.dataout1, g_img_x2_dataout1.in[0]);
        connect< >(g_img_x3_p_para_const, g_img_x3.p_para_const);
        connect< >(g_img_x3_p_para_start, g_img_x3.p_para_start);
        connect< >(g_img_x3.dataout1, g_img_x3_dataout1.in[0]);

}
*/
#ifdef ENABLE_DEBUGGING
    output_plio g_img_z1_dataout1; // temp for debugging
    output_plio g_img_x1_dataout1; // temp for debugging
    output_plio g_img_z2_dataout1; // temp for debugging
    output_plio g_img_x2_dataout1; // temp for debugging
    output_plio g_img_z3_dataout1; // temp for debugging
    output_plio g_img_x3_dataout1; // temp for debugging
    output_plio g_foc_dataout1;    // temp for debugging
    output_plio g_delay_delay;
    output_plio g_sam_sample;
    output_plio g_sam_inside;
    output_plio g_apodi_pre_dataout;
    output_plio g_apodi_dataout;
    output_plio g_interp_dataout;
#endif

    void init_debugging() {
#ifdef ENABLE_DEBUGGING
        g_img_z1_dataout1 = output_plio::create(plio_32_bits, "data/img_z1_file_out.txt");
        g_img_z2_dataout1 = output_plio::create(plio_32_bits, "data/img_z2_file_out.txt");
        g_img_z3_dataout1 = output_plio::create(plio_32_bits, "data/img_z3_file_out.txt");
        g_img_x1_dataout1 = output_plio::create(plio_32_bits, "data/img_x1_file_out.txt");
        g_img_x2_dataout1 = output_plio::create(plio_32_bits, "data/img_x2_file_out.txt");
        g_img_x3_dataout1 = output_plio::create(plio_32_bits, "data/img_x3_file_out.txt");

        connect<>(g_img_z1.dataout1, g_img_z1_dataout1.in[0]);
        connect<>(g_img_z2.dataout1, g_img_z2_dataout1.in[0]);
        connect<>(g_img_z3.dataout1, g_img_z3_dataout1.in[0]);
        connect<>(g_img_x1.dataout1, g_img_x1_dataout1.in[0]);
        connect<>(g_img_x2.dataout1, g_img_x2_dataout1.in[0]);
        connect<>(g_img_x3.dataout1, g_img_x3_dataout1.in[0]);

        g_foc_dataout1 = output_plio::create(plio_32_bits, "data/foc_out.txt");
        connect<>(g_foc.dataout1, g_foc_dataout1.in[0]);

        g_delay_delay = output_plio::create(plio_32_bits, "data/delay.txt");
        connect<>(g_delay.delay, g_delay_delay.in[0]);

        g_sam_sample = output_plio::create(plio_32_bits, "data/sample.txt");
        g_sam_inside = output_plio::create(plio_32_bits, "data/inside.txt");
        connect<>(g_sam.sample, g_sam_sample.in[0]);
        connect<>(g_sam.inside, g_sam_inside.in[0]);

        g_apodi_pre_dataout = output_plio::create(plio_32_bits, "data/apodi_pre.txt");
        connect<>(g_apodi_preprocess.out, g_apodi_pre_dataout.in[0]);
        g_apodi_dataout = output_plio::create(plio_32_bits, "data/apodi_main.txt");
        connect<>(g_apodi_main.out, g_apodi_dataout.in[0]);

        g_interp_dataout = output_plio::create(plio_32_bits, "data/interpolation.txt");
        connect<>(g_interpolation.out, g_interp_dataout.in[0]);
#endif
    }

    graph_scanline(std::string input_rfd_file_name = "") {
        init_img();
        init_foc();
        init_delay();
        init_sam();
        init_apodi_pre();
        init_apodi_main();
        init_interp(input_rfd_file_name);
        init_mult();
        init_debugging();
    }
};

#endif