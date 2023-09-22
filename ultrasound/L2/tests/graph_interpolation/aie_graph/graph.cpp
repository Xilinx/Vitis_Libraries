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

#include "graph_interpolation.hpp"
#include "us_example_parameter.hpp"

// 1.setup parameters
const int NUM_LINE_t = example_1_num_line;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SAMPLE_t = example_1_num_sample;
const int VECDIM_interp_t = 8;
const int NUM_SEG_t = 4;
const int NUM_UPSample_t = 4; // just for calu LEN_OUT
const int LEN_OUT_interp_t = NUM_SAMPLE_t * NUM_UPSample_t / NUM_SEG_t;
const int LEN_IN_interp_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_interp_rf_t = NUM_SAMPLE_t;
const int LEN32b_PARA_interp_t = 9;

#if defined(__X86SIM__)
const int test_n_ele = 20;
#else
const int test_n_ele = 1;
#endif

void setup_para(us::L1::para_Interpolation<float>& para_interp_const) {
    if (LEN32b_PARA_interp_t != sizeof(para_interp_const) / sizeof(float)) {
        printf("Error : LEN32b_PARA_interp_t != sizeof(para_interp_const) / sizeof(float) ");
        exit(1);
    }
    para_interp_const.iter_line = 0;
    para_interp_const.iter_element = 0;
    para_interp_const.iter_seg = 0;
    para_interp_const.num_line = 0;
    para_interp_const.num_element = test_n_ele;
    para_interp_const.num_seg = NUM_SEG_t;
    para_interp_const.num_depth = NUM_SAMPLE_t;
    para_interp_const.num_dep_seg = NUM_SAMPLE_t / para_interp_const.num_seg;
    para_interp_const.num_upSamp = NUM_UPSample_t;
}

// 2.setup test graph and PLIO/GMIO
class interpolation_test : public adf::graph {
   public:
    // input and output port
    output_plio plio_interpolation;
    input_plio plio_sample;
    input_plio plio_inside;
    input_plio plio_rfdata;
    port<input> para_interp_const_0;
    port<input> para_interp_const_1;
    port<input> para_interp_const_2;
    port<input> para_interp_const_3;
    port<input> para_local;

// setup graph
#ifdef _USING_SHELL_
    us::L2::interpolation_graph_scaler_shell<float,
                                             NUM_LINE_t,
                                             NUM_ELEMENT_t,
                                             NUM_SAMPLE_t,
                                             NUM_SEG_t,
                                             LEN_OUT_interp_t,
                                             NUM_SAMPLE_t,
                                             NUM_SAMPLE_t,
                                             VECDIM_interp_t,
                                             LEN32b_PARA_interp_t>
        g_top;
#else
    us::L2::interpolation_graph<float,
                                NUM_LINE_t,
                                NUM_ELEMENT_t,
                                NUM_SAMPLE_t,
                                NUM_SEG_t,
                                LEN_OUT_interp_t,
                                LEN_IN_interp_t,
                                LEN_IN_interp_rf_t,
                                VECDIM_interp_t,
                                LEN32b_PARA_interp_t>
        g_top;
#endif

    interpolation_test() {
        // input & output plio
        plio_sample = adf::input_plio::create("Datain0", adf::plio_32_bits, "data/p_sample.txt");
        plio_inside = adf::input_plio::create("Datain1", adf::plio_32_bits, "data/p_inside.txt");
        plio_rfdata = adf::input_plio::create("Datain2", adf::plio_32_bits, "data/rf_2e.txt");
        plio_interpolation = adf::output_plio::create("Dataout", adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::parameter>(para_interp_const_0, g_top.para_interp_const_0);
        adf::connect<adf::parameter>(para_interp_const_1, g_top.para_interp_const_1);
        adf::connect<adf::parameter>(para_interp_const_2, g_top.para_interp_const_2);
        adf::connect<adf::parameter>(para_interp_const_3, g_top.para_interp_const_3);
        adf::connect<adf::parameter>(para_local, g_top.para_local);
        adf::connect<>(plio_sample.out[0], g_top.p_sample_in);
        adf::connect<>(plio_inside.out[0], g_top.p_inside_in);
        adf::connect<>(plio_rfdata.out[0], g_top.p_rfdata_in);
        adf::connect<>(g_top.out, plio_interpolation.in[0]);
    }
};

interpolation_test g;

// support 1 line now
const int invoking = test_n_ele * NUM_SEG_t;

// 4.setup test
#if defined(__AIESIM__) || defined(__X86SIM__)
int main() {
    us::L1::para_Interpolation<float> para_interp_const_0;
    us::L1::para_Interpolation<float> para_interp_const_1;
    us::L1::para_Interpolation<float> para_interp_const_2;
    us::L1::para_Interpolation<float> para_interp_const_3;

    float para_local[NUM_SAMPLE_t] = {0};

    setup_para(para_interp_const_0);
    setup_para(para_interp_const_1);
    setup_para(para_interp_const_2);
    setup_para(para_interp_const_3);
    g.init();
#ifdef _USING_SHELL_
    g.update(g.para_interp_const, (int*)(&para_interp_const_0), sizeof(para_interp_const_0) / sizeof(float));
#else
    g.update(g.para_interp_const_0, (int*)(&para_interp_const_0), sizeof(para_interp_const_0) / sizeof(float));
    g.update(g.para_interp_const_1, (int*)(&para_interp_const_1), sizeof(para_interp_const_0) / sizeof(float));
    g.update(g.para_interp_const_2, (int*)(&para_interp_const_2), sizeof(para_interp_const_0) / sizeof(float));
    g.update(g.para_interp_const_3, (int*)(&para_interp_const_3), sizeof(para_interp_const_0) / sizeof(float));
    g.update(g.para_local, para_local, sizeof(para_local) / sizeof(float));
#endif

    g.run(invoking);
    // g.wait();//no use
    g.end();
    printf("all %d ele(s) works!, invoking %d\n", test_n_ele, invoking);
    return 0;
};
#endif
