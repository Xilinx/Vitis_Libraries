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

#include "graph_imagepoints.hpp"
#include "us_example_parameter.hpp"

using namespace adf;

template <class T,
          int NUM_LINE_t,
          int NUM_ELEMENT_t,
          int NUM_SAMPLE_t,
          int NUM_SEG_t,
          int VECDIM_img_t,
          int LLEN_OUT_img_t,
          int LEN32b_PARA_img_t>
class graph_img_test : public graph {
   public:
    port<input> p_para_const;
    port<input> p_para_start;
    output_plio dataout1;

    us::L2::graph_img_wrapper<T,
                              NUM_LINE_t,
                              NUM_ELEMENT_t,
                              NUM_SAMPLE_t,
                              NUM_SEG_t,
                              VECDIM_img_t,
                              LLEN_OUT_img_t,
                              LEN32b_PARA_img_t>
        g_img;

    graph_img_test(const char* file_out) {
        dataout1 = output_plio::create(plio_32_bits, file_out);

        connect<>(p_para_const, g_img.p_para_const);
        connect<>(p_para_start, g_img.p_para_start);
        connect<>(g_img.dataout1, dataout1.in[0]);
    }
};

const int NUM_LINE_t = example_1_num_line;
const int NUM_ELEMENT_t = example_1_num_element;
const int NUM_SAMPLE_t = example_1_num_sample;
const int VECDIM_img_t = 8;
const int NUM_SEG_t = 4;
const int LEN_OUT_img_t = NUM_SAMPLE_t / NUM_SEG_t;
const int LEN32b_PARA_img_t = 7;
const int num_invoking = NUM_SEG_t;
graph_img_test<float,
               NUM_LINE_t,
               NUM_ELEMENT_t,
               NUM_SAMPLE_t,
               NUM_SEG_t,
               VECDIM_img_t,
               LEN_OUT_img_t,
               LEN32b_PARA_img_t>
    g_img_z("data/output_img.txt");

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(void) {
    us::L1::para_img_t<float> para_img_z(example_1_directions[2], 2, 2, 4);
    int size_const = sizeof(para_img_z) / sizeof(float);
    if (size_const != LEN32b_PARA_img_t) {
        printf("size_const %d != len32b_para %d ", size_const, LEN32b_PARA_img_t);
        exit(1);
    }
    float p_para_start_z[NUM_LINE_t];
    for (int i = 0; i < NUM_LINE_t; i++) {
        p_para_start_z[i] = example_1_start_positions[2];
    }
    printf("********** Static Parameter ********************\n");
    printf("Const: NUM_LINE_t        = %d\n", NUM_LINE_t);        //%d\n", )//NUM_LINE_t );
    printf("Const: NUM_ELEMENT_t     = %d\n", NUM_ELEMENT_t);     // 128;
    printf("Const: NUM_SAMPLE_t      = %d\n", NUM_SAMPLE_t);      // 2048;
    printf("Const: VECDIM_img_t      = %d\n", VECDIM_img_t);      // 8;
    printf("Const: NUM_SEG_t         = %d\n", NUM_SEG_t);         // 4;
    printf("Const: LEN_OUT_img_t     = %d\n", LEN_OUT_img_t);     // NUM_SAMPLE_t/NUM_SEG_t;
    printf("Const: LEN32b_PARA_img_t = %d\n", LEN32b_PARA_img_t); // 7;
    printf("Const: size_const        = %d\n", size_const);        // 6;
    printf("Const: num_invoking      = %d\n", num_invoking);      // 6;
    printf("********** RTP Parameter ********************\n");
    printf("RTP: para_img_z,      size = %d Byte\n", sizeof(para_img_z));
    printf("RTP: p_para_start_z   size = %d Byte\n", sizeof(p_para_start_z));

#ifdef _USING_SHELL_
    printf("\nNOTE!! Shell-kernel used\n");
#else
    printf("\nNOTE!! Normal kernel used\n");
#endif

    g_img_z.init();
    g_img_z.update(g_img_z.p_para_const, (float*)(&para_img_z), size_const);
    g_img_z.update(g_img_z.p_para_start, (float*)(&p_para_start_z), NUM_LINE_t);
    g_img_z.run(num_invoking);
    g_img_z.end();

    return 0;
}
#endif
