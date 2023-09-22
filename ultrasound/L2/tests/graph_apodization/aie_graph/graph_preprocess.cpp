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
const int LEN_IN_apodi_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_apodi_t = 12;

const int test_n_ele = 2;

void setup_para_apodi_const(us::L1::para_Apodization<float>& para_apodi_const) {
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
}

// 2.setup test graph and PLIO/GMIO
class preprocess_test : public adf::graph {
   public:
    // input and output port
    output_plio preprocess_out;
    input_plio img_x_in;
    input_plio img_z_in;
    port<input> para_apodi_const;

    // L2 graph
    us::L2::apodi_pre_graph<float,
                            NUM_LINE_t,
                            NUM_ELEMENT_t,
                            NUM_SAMPLE_t,
                            NUM_SEG_t,
                            LEN_OUT_apodi_t,
                            LEN_IN_apodi_t,
                            VECDIM_apodi_t,
                            LEN32b_PARA_apodi_t>
        g;

    preprocess_test() {
        // input & output plio
        in1 = adf::input_plio::create("Datain0", adf::plio_32_bits, "data/p_points_x.txt");
        in2 = adf::input_plio::create("Datain1", adf::plio_32_bits, "data/p_points_z.txt");
        out = adf::output_plio::create("Dataout", adf::plio_32_bits, "data/output.txt");

        // connections
        adf::connect<adf::parameter>(para_apodi_const, g.para_apodi_const);
        adf::connect<>(img_x_in.out[0], g.img_x);
        adf::connect<>(img_z_in.out[0], g.img_z);
        adf::connect<>(g.delay, preprocess_out.in[0]);
    }
};

// 3.setup test
#if defined(__AIESIM__) || defined(__X86SIM__)
int main() {
    us::L1::para_Apodization<float> para_apodi_const;
    setup_para_apodi_const(para_apodi_const);
    g.init();
    g.update(g.para_apodi_const, (int*)(&para_apodi_const), sizeof(para_apodi_const) / sizeof(float));
    g.run(test_n_ele * para_apodi_const.num_seg);
    // g.wait();//no use
    g.end();
    printf("all %d ele/line(s) works!\n", test_n_ele);
    return 0;
};
#endif
