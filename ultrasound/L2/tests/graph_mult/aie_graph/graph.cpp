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

#include "graph_mult.hpp"
#include "us_example_parameter.hpp"

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int NUM_DEP_SEG_t,
          int VECDIM_mult_t,
          int LEN_IN_mult_t,
          int LEN_OUT_mult_t,
          int LEN32b_PARA_mult_t,
          int MULT_ID_t>
class mult_test : public graph {
   public:
    // input and output port
    port<direction::in> para_const_pre;
    port<direction::in> para_const_0;
    port<direction::in> para_const_1;
    port<direction::in> para_const_2;
    port<direction::in> para_const_3;

    port<direction::in> para_local_0_0;
    port<direction::in> para_local_0_1;
    port<direction::in> para_local_0_2;
    port<direction::in> para_local_0_3;
    port<direction::in> para_local_1_0;
    port<direction::in> para_local_1_1;
    port<direction::in> para_local_1_2;
    port<direction::in> para_local_1_3;

    input_plio interp;
    input_plio apod;
    output_plio mult_0;
    output_plio mult_1;
    output_plio mult_2;
    output_plio mult_3;

    // L2 graph
    us::L2::mult_graph_wrapper<T,
                               NUM_LINE_t,
                               NUM_ELEMENT_t,
                               NUM_SAMPLE_t,
                               NUM_SEG_t,
                               NUM_DEP_SEG_t,
                               VECDIM_mult_t,
                               LEN_IN_mult_t,
                               LEN_OUT_mult_t,
                               LEN32b_PARA_mult_t,
                               MULT_ID_t>
        g_mult;

    mult_test() {
        // input & output plio
        interp = input_plio::create(plio_32_bits, "data/interp.txt");
        apod = input_plio::create(plio_32_bits, "data/apod.txt");

        connect<parameter>(para_const_pre, g_mult.para_const_pre);
        connect<>(interp.out[0], g_mult.interp);
        connect<>(apod.out[0], g_mult.apod);

        mult_0 = output_plio::create(plio_32_bits, "data/mult0.txt");
        mult_1 = output_plio::create(plio_32_bits, "data/mult1.txt");
        mult_2 = output_plio::create(plio_32_bits, "data/mult2.txt");
        mult_3 = output_plio::create(plio_32_bits, "data/mult3.txt");
        // connections
        connect<parameter>(para_const_0, g_mult.para_const_0);
        connect<parameter>(para_const_1, g_mult.para_const_1);
        connect<parameter>(para_const_2, g_mult.para_const_2);
        connect<parameter>(para_const_3, g_mult.para_const_3);

        connect<parameter>(para_local_0_0, g_mult.para_local_0_0);
        connect<parameter>(para_local_0_1, g_mult.para_local_0_1);
        connect<parameter>(para_local_0_2, g_mult.para_local_0_2);
        connect<parameter>(para_local_0_3, g_mult.para_local_0_3);

        connect<parameter>(para_local_1_0, g_mult.para_local_1_0);
        connect<parameter>(para_local_1_1, g_mult.para_local_1_1);
        connect<parameter>(para_local_1_2, g_mult.para_local_1_2);
        connect<parameter>(para_local_1_3, g_mult.para_local_1_3);

        connect<>(g_mult.mult_0, mult_0.in[0]);
        connect<>(g_mult.mult_1, mult_1.in[0]);
        connect<>(g_mult.mult_2, mult_2.in[0]);
        connect<>(g_mult.mult_3, mult_3.in[0]);
    }
};

const int NUM_LINE_t = example_1_num_line;
const int NUM_SAMPLE_t = example_1_num_sample;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SEG_t = 4;
const int NUM_UP_SAMPLE_t = 4;
const int NUM_DEP_SEG_t = NUM_SAMPLE_t * NUM_UP_SAMPLE_t / NUM_SEG_t;
const int LEN_IN_mult_t = NUM_SAMPLE_t * NUM_UP_SAMPLE_t / NUM_SEG_t;
const int LEN_OUT_mult_t = NUM_SAMPLE_t * NUM_UP_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_mult_t = 8;
const int VECDIM_mult_t = 8;
const int MULT_ID_t = 0;

const int invoking = NUM_ELEMENT_t * NUM_SEG_t;
// template<class T, int NUM_LINE_t, int NUM_ELEMENT_t, int NUM_SAMPLE_t, int NUM_SEG_t, int NUM_DEP_SEG_t, int
// VECDIM_mult_t, int LEN_IN_mult_t, int LEN_OUT_mult_t, int LEN32b_PARA_mult_t, int MULT_ID_t>
mult_test<float,
          NUM_LINE_t,
          NUM_ELEMENT_t,
          NUM_SAMPLE_t,
          NUM_SEG_t,
          NUM_DEP_SEG_t,
          VECDIM_mult_t,
          LEN_IN_mult_t,
          LEN_OUT_mult_t,
          LEN32b_PARA_mult_t,
          MULT_ID_t>
    g_mult_test;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    us::L1::para_mult_t<float> para_const_pre;
    us::L1::para_mult_t<float> para_const_0;
    us::L1::para_mult_t<float> para_const_1;
    us::L1::para_mult_t<float> para_const_2;
    us::L1::para_mult_t<float> para_const_3;
    para_const_0.iter_line = 0;
    para_const_0.iter_element = 0;
    para_const_0.iter_seg = 0;
    para_const_0.num_line = NUM_LINE_t;
    para_const_0.num_element = NUM_ELEMENT_t;
    para_const_0.num_seg = NUM_SEG_t;
    para_const_0.num_dep_seg = NUM_DEP_SEG_t;
    para_const_0.mult_id = MULT_ID_t;

    para_const_1 = para_const_0;
    para_const_2 = para_const_0;
    para_const_3 = para_const_0;

    para_const_pre = para_const_0;

    para_const_1.mult_id = 1;
    para_const_2.mult_id = 2;
    para_const_3.mult_id = 3;

    float para_local_0_0[NUM_DEP_SEG_t] = {0};
    float para_local_0_1[NUM_DEP_SEG_t] = {0};
    float para_local_0_2[NUM_DEP_SEG_t] = {0};
    float para_local_0_3[NUM_DEP_SEG_t] = {0};

    float para_local_1_0[NUM_DEP_SEG_t] = {0};
    float para_local_1_1[NUM_DEP_SEG_t] = {0};
    float para_local_1_2[NUM_DEP_SEG_t] = {0};
    float para_local_1_3[NUM_DEP_SEG_t] = {0};
    g_mult_test.init();

    g_mult_test.update(g_mult_test.para_const_pre, (float*)(&para_const_pre), LEN32b_PARA_mult_t);

    g_mult_test.update(g_mult_test.para_const_0, (float*)(&para_const_0), LEN32b_PARA_mult_t);
    g_mult_test.update(g_mult_test.para_const_1, (float*)(&para_const_1), LEN32b_PARA_mult_t);
    g_mult_test.update(g_mult_test.para_const_2, (float*)(&para_const_2), LEN32b_PARA_mult_t);
    g_mult_test.update(g_mult_test.para_const_3, (float*)(&para_const_3), LEN32b_PARA_mult_t);

    g_mult_test.update(g_mult_test.para_local_0_0, para_local_0_0, NUM_DEP_SEG_t);
    g_mult_test.update(g_mult_test.para_local_0_1, para_local_0_1, NUM_DEP_SEG_t);
    g_mult_test.update(g_mult_test.para_local_0_2, para_local_0_2, NUM_DEP_SEG_t);
    g_mult_test.update(g_mult_test.para_local_0_3, para_local_0_3, NUM_DEP_SEG_t);

    g_mult_test.update(g_mult_test.para_local_1_0, para_local_1_0, NUM_DEP_SEG_t);
    g_mult_test.update(g_mult_test.para_local_1_1, para_local_1_1, NUM_DEP_SEG_t);
    g_mult_test.update(g_mult_test.para_local_1_2, para_local_1_2, NUM_DEP_SEG_t);
    g_mult_test.update(g_mult_test.para_local_1_3, para_local_1_3, NUM_DEP_SEG_t);

    g_mult_test.run(invoking);

    g_mult_test.end();

    return 0;
}
#endif
