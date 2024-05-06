/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include <adf.h>
#include <array>
#include <type_traits>

#include "kernels.h"

using namespace adf;

/*
 * computing scalefactors
*/
template <int FBITS>
uint32_t compute_scalefactor(int M, int N) {
    float x_scale = (float)M / (float)N;
    float scale = x_scale * (1 << FBITS);
    return (uint32_t)(std::roundf(scale));
}

template <int FBITS_ALPHA = 0, int FBITS_BETA = 4>
void get_alpha_beta(float mean[4], float std[4], unsigned char alpha[4], char beta[4]) {
    for (int i = 0; i < 4; i++) {
        if (i < 3) {
            float a_v = mean[i] * (1 << FBITS_ALPHA);
            float b_v = (std[i]) * (1 << FBITS_BETA);

            alpha[i] = (unsigned char)a_v;
            beta[i] = (char)b_v;
        } else {
            alpha[i] = 0;
            beta[i] = 0;
        }
    }
}

class denormResizeGraph : public adf::graph {
   private:
    kernel k;

   public:
    input_plio in1;
    output_plio out1;
    port<input> row;
    port<input> scalex;
    port<input> scaley;
    port<input> img_h_in;
    port<input> img_w_in;
    port<input> line_stride_in;
    port<input> img_w_out;
    port<input> coeff;

    denormResizeGraph() {
        k = kernel::create_object<DenormResizeRunner>(IN_FBITS, ALPHA_FBITS, BETA_FBITS, OUT_FBITS);
        in1 = input_plio::create("DataIn0", adf::plio_32_bits, "data/input.txt");
        out1 = output_plio::create("DataOut0", adf::plio_32_bits, "data/output.txt");

        // create nets to connect kernels and IO ports
        connect<window<TILE_WINDOW_SIZE_IN> >(in1.out[0], k.in[0]);
        connect<window<TILE_WINDOW_SIZE_OUT> >(k.out[0], out1.in[0]);
        connect<parameter>(row, async(k.in[1]));
        connect<parameter>(scalex, async(k.in[2]));
        connect<parameter>(scaley, async(k.in[3]));
        connect<parameter>(img_h_in, async(k.in[4]));
        connect<parameter>(img_w_in, async(k.in[5]));
        connect<parameter>(line_stride_in, async(k.in[6]));
        connect<parameter>(img_w_out, async(k.in[7]));
        connect<parameter>(coeff, async(k.in[8]));

        // specify kernel sources
        source(k) = "xf_denorm_resize.cc";

        runtime<ratio>(k) = 1.0;
    }

    void updateInputOutputSize(int width_in, int height_in, int width_out, int height_out) {
        uint32_t scale_x_fix = compute_scalefactor<16>(width_in, width_out);
        uint32_t scale_y_fix = compute_scalefactor<16>(height_in, height_out);
        update(scalex, scale_x_fix);
        update(scaley, scale_y_fix);
        update(img_h_in, height_in);
        update(img_w_in, width_in);
        update(line_stride_in, width_in);
        update(img_w_out, width_out);
    }

    void updateMeanStddev(float mean[4], float std_deviation[4]) {
        unsigned char alpha[4];
        char beta[4];
        get_alpha_beta<ALPHA_FBITS, BETA_FBITS>(mean, std_deviation, alpha, beta);

        std::vector<int16_t> coeff_v({alpha[0], alpha[1], alpha[2], alpha[3], beta[0], beta[1], beta[2], beta[3]});
        update(coeff, coeff_v.data(), coeff_v.size());
    }
};

#endif
