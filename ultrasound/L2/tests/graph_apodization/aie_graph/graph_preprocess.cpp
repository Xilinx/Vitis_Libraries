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

#include "apodization.hpp"
#include "us_example_parameter.hpp"

using namespace adf;

// 1.setup simulator
PLIO* in1 = new PLIO("Datain1", plio_32_bits, "data/p_points_x.txt");
PLIO* in2 = new PLIO("Datain2", plio_32_bits, "data/p_points_z.txt");
PLIO* out = new PLIO("Dataout", plio_32_bits, "data/output.txt");
simulation::platform<2, 1> plat(in1, in2, out);

// 2.setup parameters
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

// 3.setup graph
// template <typename T, int LEN_OUT, int LEN_IN, int VECDIM, int APODI_PRE_LEN32b_PARA>
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

connect<> net1(plat.src[0], g.img_x_in);
connect<> net2(plat.src[1], g.img_z_in);
connect<> net3(g.out, plat.sink[0]);

// 4.setup test
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
