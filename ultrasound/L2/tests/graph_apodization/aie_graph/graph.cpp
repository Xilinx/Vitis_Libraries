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

#include "graph_apodization.hpp"
#include "us_example_parameter.hpp"

using namespace adf;

// 1.setup parameters
const int NUM_LINE_t = example_1_num_line;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SAMPLE_t = example_1_num_sample;
const int VECDIM_apodi_t = 8;
const int NUM_SEG_t = 4;
const int LEN_OUT_apodi_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_apodi_f_t = NUM_ELEMENT_t;
const int LEN_IN_apodi_d_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_apodi_t = 12;

#if defined(__X86SIM__)
const int test_n_ele = NUM_ELEMENT_t; // NUM_ELEMENT_t;//NUM_ELEMENT_t could test in x86sim
#else
const int test_n_ele = 1;
#endif

void setup_para_amain_const(us::L1::para_Apodization<float>& para_amain_const) {
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
}

// 2.setup test graph and PLIO/GMIO
class apodization_test : public adf::graph {
   public:
    // input and output port
    output_plio plio_apodization;
    input_plio plio_focal;
    input_plio plio_invD;
    port<input> para_amain_const;

    //<typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
    us::L2::apodi_main_graph<float,
                             NUM_LINE_t,
                             NUM_ELEMENT_t,
                             NUM_SAMPLE_t,
                             NUM_SEG_t,
                             LEN_OUT_apodi_t,
                             LEN_IN_apodi_f_t,
                             LEN_IN_apodi_d_t,
                             VECDIM_apodi_t,
                             LEN32b_PARA_apodi_t>
        g_top;

    apodization_test() {
        // input & output plio
        plio_focal = adf::input_plio::create("Datain0", adf::plio_32_bits, "data/p_focal.txt");
        plio_invD = adf::input_plio::create("Datain1", adf::plio_32_bits, "data/p_invD.txt");
        plio_apodization = adf::output_plio::create("Dataout", adf::plio_32_bits, "data/m_p_apodization.txt");

        // connections
        adf::connect<adf::parameter>(para_amain_const, g_top.para_amain_const);
        adf::connect<>(plio_focal.out[0], g_top.p_focal);
        adf::connect<>(plio_invD.out[0], g_top.p_invD);
        adf::connect<>(g_top.out, plio_apodization.in[0]);
    }
};

apodization_test g;

// support 1 line now
const int invoking = test_n_ele * NUM_SEG_t;

// 4.setup test
#if defined(__AIESIM__) || defined(__X86SIM__)
int main() {
    us::L1::para_Apodization<float> para_amain_const;
    setup_para_amain_const(para_amain_const);
    g.init();
    g.update(g.para_amain_const, (int*)(&para_amain_const), sizeof(para_amain_const) / sizeof(float));
    g.run(invoking);
    // g.wait();//no use
    g.end();
    printf("all %d ele(s) works!, invoking %d\n", test_n_ele, invoking);
    return 0;
};
#endif
