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

#include "graph_samples.hpp"
#include "us_example_parameter.hpp"

using namespace adf;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int VECDIM_sample_t,
          int LEN_IN_sample_t,
          int LEN_OUT_sample_t,
          int LEN32b_PARA_sample_t>
class sample_test : public graph {
   public:
    // input and output port
    port<direction::in> para_const;
    port<direction::in> para_rfdim;
    port<direction::in> para_elem;
    output_plio sample;
    output_plio inside;
    input_plio img_x;
    input_plio img_z;
    input_plio delay;

    us::L2::sample_graph_wrapper<T,
                                 NUM_LINE_t,
                                 NUM_ELEMENT_t,
                                 VECDIM_sample_t,
                                 LEN_IN_sample_t,
                                 LEN_OUT_sample_t,
                                 LEN32b_PARA_sample_t>
        g_sample_test;

    sample_test() {
        // input & output port
        img_x = input_plio::create(plio_32_bits, "data/img_x.txt");
        img_z = input_plio::create(plio_32_bits, "data/img_z.txt");
        delay = input_plio::create(plio_32_bits, "data/delay.txt");
        sample = output_plio::create(plio_32_bits, "data/sample.txt");
        inside = output_plio::create(plio_32_bits, "data/inside.txt");

        // connections
        connect<parameter>(para_const, g_sample_test.para_const);
        connect<parameter>(para_rfdim, g_sample_test.para_rfdim);
        connect<parameter>(para_elem, g_sample_test.para_elem);
        connect<>(img_x.out[0], g_sample_test.img_x);
        connect<>(img_z.out[0], g_sample_test.img_z);
        connect<>(delay.out[0], g_sample_test.delay);
        connect<>(g_sample_test.sample, sample.in[0]);
        connect<>(g_sample_test.inside, inside.in[0]);
    }
};

const int NUM_LINE_t = example_1_num_line;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SAMPLE_t = example_1_num_sample;
const int NUM_SEG_t = 4;
const int NUM_DEP_SEG_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_sample_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN_OUT_sample_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_sample_t = 12;
const int VECDIM_sample_t = 8;

// template<class T, int NUM_LINE_t, int NUM_ELEMENT_t, int VECDIM_sample_t, int LEN_IN_sample_t, int LEN_OUT_sample_t,
// int LEN32b_PARA_sample_t>
sample_test<float, NUM_LINE_t, NUM_ELEMENT_t, VECDIM_sample_t, LEN_IN_sample_t, LEN_OUT_sample_t, LEN32b_PARA_sample_t>
    g_sample;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    float* p_para_elem = &example_1_xdc_def_pos[0][0];
    float para_rfdim[NUM_LINE_t] = {0};
    for (int i = 0; i < NUM_LINE_t; i++) para_rfdim[i] = (float)example_1_rf_data_dim[i];

    us::L1::para_sample_t<float> para_const;
    para_const.xdc_x = 0;
    para_const.xdc_z = 0;
    para_const.inv_speed_of_sound = example_1_inverse_speed_of_sound;
    para_const.freq_sampling = example_1_freq_sampling;
    para_const.rf_dim = 0;
    para_const.iter_line = 0;
    para_const.iter_element = 0;
    para_const.iter_seg = 0;
    para_const.num_line = NUM_LINE_t;
    para_const.num_element = NUM_ELEMENT_t;
    para_const.num_seg = NUM_SEG_t;
    para_const.num_dep_seg = NUM_DEP_SEG_t;

    g_sample.init();

    g_sample.update(g_sample.para_const, (float*)(&para_const), LEN32b_PARA_sample_t);
    g_sample.update(g_sample.para_rfdim, para_rfdim, NUM_LINE_t);
    g_sample.update(g_sample.para_elem, p_para_elem, NUM_ELEMENT_t * example_1_NUM_DIM);
    g_sample.run(NUM_SEG_t);

    g_sample.end();

    return 0;
}
#endif
