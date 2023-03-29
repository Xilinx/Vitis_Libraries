/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <adf.h>
#include <aie_api/aie.hpp>

union cc {
    aie::vector<float, 4> f;
    aie::vector<int, 4> in;
};

void aie_adder(input_stream_float* in_real,
               input_stream_float* in_imag,
               output_stream_float* out_real,
               output_stream_float* out_imag) {
    float v_r[512];
    float v_i[512];
    float p0_r[512];
    float p0_i[512];
    int m;
    int n;
    int k;

    // init mat_m, mat_n, id
    {
        float a = readincr(in_real);
        float b = readincr(in_imag);
        writeincr(out_real, a);
        writeincr(out_imag, b);
        m = a;
        n = b;
        a = readincr(in_real);
        k = a;
        a = k + 1;
        writeincr(out_real, a);
    }

    // direct pass
    {
        int total = k * m + k;
        int total_residual = total % 4;
        int pp = 0;
        for (; pp < total_residual; pp++) chess_prepare_for_pipelining {
                int tmp_real = READINCR(SS_rsrc1, in_real);
                int tmp_imag = READINCR(SS_rsrc2, in_imag);
                WRITEINCR(MS_rsrc1, out_real, tmp_real);
                WRITEINCR(MS_rsrc2, out_imag, tmp_imag);
                // writeincr(out_real, readincr(in_real));
                // writeincr(out_imag, readincr(in_imag));
            }
        for (; pp < total; pp += 4) chess_prepare_for_pipelining {
                cc tmp_real, tmp_imag;
                tmp_real.in = READINCRW(WSS_rsrc1, in_real);
                tmp_real.in = READINCRW(WSS_rsrc2, in_imag);
                WRITEINCRW(WMS_rsrc1, out_real, tmp_real.in);
                WRITEINCRW(WMS_rsrc2, out_imag, tmp_imag.in);
            }
    }

    for (int i = k; i < m; i++) {
        p0_r[i] = readincr(in_real);
        p0_i[i] = readincr(in_imag);
        v_r[i] = p0_r[i];
        v_i[i] = p0_i[i];
    }

    float xk_m2 = 0;
    xk_m2 += v_r[k] * v_r[k];
    xk_m2 += v_i[k] * v_i[k];
    float xk_m = aie::sqrt(xk_m2);

    float x_m2 = 0;
    for (int i = k; i < m; i++) {
        x_m2 += v_r[i] * v_r[i];
        x_m2 += v_i[i] * v_i[i];
    }
    float x_m = aie::sqrt(x_m2);

    float a_r = v_r[k] / xk_m * x_m;
    float a_i = v_i[k] / xk_m * x_m;

    v_r[k] += a_r;
    v_i[k] += a_i;

    x_m2 -= xk_m2;
    x_m2 += v_r[k] * v_r[k];
    x_m2 += v_i[k] * v_i[k];
    x_m = 1.0 / aie::sqrt(x_m2);

    for (int i = k; i < m; i++) {
        v_r[i] *= x_m;
        v_i[i] *= x_m;
    }

    //
    for (int j = k; j < n - 1; j++) {
        float tmp_r = 0;
        float tmp_i = 0;
        for (int i = k; i < m; i++) {
            tmp_r += v_r[i] * p0_r[i] + v_i[i] * p0_i[i];
            tmp_i += v_r[i] * p0_i[i] - v_i[i] * p0_r[i];
        }
        tmp_r *= 2.0;
        tmp_i *= 2.0;

        for (int i = k; i < m; i++) {
            p0_r[i] -= tmp_r * v_r[i] - tmp_i * v_i[i];
            p0_i[i] -= tmp_r * v_i[i] + tmp_i * v_r[i];
            writeincr(out_real, p0_r[i]);
            writeincr(out_imag, p0_i[i]);
        }

        //
        for (int i = 0; i < k; i++) {
            writeincr(out_real, readincr(in_real));
            writeincr(out_imag, readincr(in_imag));
        }

        for (int i = k; i < m; i++) {
            p0_r[i] = readincr(in_real);
            p0_i[i] = readincr(in_imag);
        }
    }
    {
        float tmp_r = 0;
        float tmp_i = 0;
        for (int i = k; i < m; i++) {
            tmp_r += v_r[i] * p0_r[i] + v_i[i] * p0_i[i];
            tmp_i += v_r[i] * p0_i[i] - v_i[i] * p0_r[i];
        }
        tmp_r *= 2.0;
        tmp_i *= 2.0;

        for (int i = k; i < m; i++) {
            p0_r[i] -= tmp_r * v_r[i] - tmp_i * v_i[i];
            p0_i[i] -= tmp_r * v_i[i] + tmp_i * v_r[i];
            writeincr(out_real, p0_r[i]);
            writeincr(out_imag, p0_i[i]);
        }
    }
}
