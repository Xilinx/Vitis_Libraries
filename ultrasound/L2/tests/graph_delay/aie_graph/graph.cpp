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

#include "graph_delay.hpp"
#include "us_example_parameter.hpp"

template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
class delay_test : public adf::graph {
   public:
    // input and output port
    adf::port<adf::direction::in> para_const;
    adf::port<adf::direction::in> para_t_start;
    adf::port<adf::direction::in> para_iter;
    adf::input_plio img_x;
    adf::input_plio img_z;
    adf::output_plio delay;

    // L2 graph
    us::L2::delay_graph_wrapper<T, NUM_LINE_t, VECDIM_delay_t, LEN_IN_delay_t, LEN_OUT_delay_t, LEN32b_PARA_delay_t> d;

    delay_test() {
        // input & output plio
        img_x = adf::input_plio::create("Datain0", adf::plio_32_bits, "data/img_x.txt");
        img_z = adf::input_plio::create("Datain1", adf::plio_32_bits, "data/img_z.txt");
        delay = adf::output_plio::create("Dataout", adf::plio_32_bits, "data/delay.txt");

        // connections
        adf::connect<adf::parameter>(para_const, d.para_const);
        adf::connect<adf::parameter>(para_t_start, d.para_t_start);
        adf::connect<>(img_x.out[0], d.img_x);
        adf::connect<>(img_z.out[0], d.img_z);
        adf::connect<>(d.delay, delay.in[0]);
    }
};

const int NUM_LINE_t = example_1_num_line;
const int NUM_SAMPLE_t = example_1_num_sample;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SEG_t = 4;
const int NUM_DEP_SEG_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_delay_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_OUT_delay_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_delay_t = 17;
const int VECDIM_delay_t = 8;
// template<class T,  int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int
// LEN32b_PARA_delay_t>
delay_test<float, NUM_LINE_t, VECDIM_delay_t, LEN_IN_delay_t, LEN_OUT_delay_t, LEN32b_PARA_delay_t> g_delay_test;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    us::L1::para_delay_t<float> para_const;
    para_const.tx_ref_point_x = example_1_tx_ref_point_x;
    para_const.tx_ref_point_z = example_1_tx_ref_point_z;
    para_const.tileVApo_x = example_1_tileVApo_x;
    para_const.tileVApo_z = example_1_tileVApo_z;
    para_const.focal_point_x = example_1_focal_point_x;
    para_const.focal_point_z = example_1_focal_point_z;
    para_const.t_start = 0;
    para_const.tx_delay_distance = example_1_tx_delay_distance;
    para_const.tx_delay_distance_ = example_1_tx_delay_distance_;
    para_const.inverse_speed_of_sound = example_1_inverse_speed_of_sound;
    para_const.iter_line = 0;
    para_const.iter_element = 0;
    para_const.iter_seg = 0;
    para_const.num_line = NUM_LINE_t;
    para_const.num_element = NUM_ELEMENT_t;
    para_const.num_seg = NUM_SEG_t;
    para_const.num_dep_seg = NUM_DEP_SEG_t;

    float t_start[NUM_LINE_t] = {0};
    for (int i = 0; i < NUM_LINE_t; i++) t_start[i] = example_1_t_start[i];

    g_delay_test.init();

    g_delay_test.update(g_delay_test.para_const, (float*)(&para_const), LEN32b_PARA_delay_t);
    g_delay_test.update(g_delay_test.para_t_start, t_start, NUM_LINE_t);
    g_delay_test.run(NUM_SEG_t);

    g_delay_test.end();

    return 0;
}
#endif
