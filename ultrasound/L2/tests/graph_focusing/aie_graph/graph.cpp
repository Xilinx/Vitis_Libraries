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

#include "graph_focusing.hpp"
#include "us_example_parameter.hpp"

using namespace adf;
using namespace aie;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_foc_t,
          int LEN32b_PARA_foc_t>
class graph_foc_test : public graph {
   public:
    port<input> p_para_const;
    port<input> p_para_pos;
    output_plio dataout1;

    us::L2::graph_foc_wrapper<T, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t, LEN32b_PARA_foc_t>
        g_foc;

    graph_foc_test(const char* file_out) {
        dataout1 = output_plio::create(plio_32_bits, file_out);

        connect<>(p_para_const, g_foc.p_para_const);
        connect<>(p_para_pos, g_foc.p_para_pos);
        connect<>(g_foc.dataout1, dataout1.in[0]);
    }
};

const int NUM_LINE_t = example_1_num_line;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SAMPLE_t = example_1_num_sample;
const int VECDIM_foc_t = 8;
const int NUM_SEG_t = 4;
const int EN_OUT_foc_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_foc_t = 6;
const int num_invoking = NUM_SEG_t;

graph_foc_test<float, NUM_LINE_t, NUM_ELEMENT_t, NUM_SAMPLE_t, NUM_SEG_t, VECDIM_foc_t, LEN32b_PARA_foc_t> g_foc(
    "data/foc_out.txt");

#if defined(__AIESIM__) || defined(__X86SIM__)

int main(void) {
    us::L1::para_foc_t para_foc_const(NUM_LINE_t, NUM_ELEMENT_t, NUM_SEG_t);
    int size_const = sizeof(para_foc_const) / sizeof(float);
    if (size_const != LEN32b_PARA_foc_t) {
        printf("size_const %d != len32b_para %d ", size_const, LEN32b_PARA_foc_t);
        exit(1);
    }
    printf("********** Static Parameter ********************\n");
    printf("Const: NUM_LINE_t        = %d\n", NUM_LINE_t);        //%d\n", )//NUM_LINE_t );
    printf("Const: NUM_ELEMENT_t     = %d\n", NUM_ELEMENT_t);     // 128;
    printf("Const: NUM_SAMPLE_t      = %d\n", NUM_SAMPLE_t);      // 2048;
    printf("Const: VECDIM_foc_t      = %d\n", VECDIM_foc_t);      // 8;
    printf("Const: NUM_SEG_t         = %d\n", NUM_SEG_t);         // 4;
    printf("Const: EN_OUT_foc_t      = %d\n", EN_OUT_foc_t);      // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_foc_t = %d\n", LEN32b_PARA_foc_t); // 6;
    printf("Const: size_const        = %d\n", size_const);        // 6;
    printf("Const: num_invoking      = %d\n", num_invoking);      // 6;
    printf("********** RTP Parameter ********************\n");
    printf("RTP: para_foc_const,            size = %d Byte\n", sizeof(para_foc_const));
    printf("RTP: example_1_xdc_def_pos_xz   size = %d Byte\n", sizeof(example_1_xdc_def_pos_xz));
#ifdef _USING_SHELL_
    printf("\nNOTE!! Shell-kernel used\n");
#else
    printf("\nNOTE!! Normal kernel used\n");
#endif
    g_foc.init();
    g_foc.update(g_foc.p_para_const, (float*)(&para_foc_const), size_const);
    g_foc.update(g_foc.p_para_pos, (float*)(&example_1_xdc_def_pos_xz), NUM_ELEMENT_t * 4);
    g_foc.run(NUM_SEG_t);
    g_foc.end();

    return 0;
}
#endif
