/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef GEMV_HELPER_HPP
#define GEMV_HELPER_HPP

#include <cmath>
#include <iomanip>
#include <string>

#define IDX2R(i, j, ld) (((i) * (ld)) + (j))

using namespace std;

// Deprecated (Recommend using gemm_mkl to generate the golden output.)
XFBLAS_dataType* getGoldenMat(XFBLAS_dataType* a, XFBLAS_dataType* x, XFBLAS_dataType* y, int m, int n) {
    XFBLAS_dataType* goldenY;
    goldenY = (XFBLAS_dataType*)malloc(m * 1 * sizeof(XFBLAS_dataType));
    for (int row = 0; row < m; row++) {
        XFBLAS_dataType l_val = 0;
        for (int i = 0; i < n; i++) {
            l_val += a[IDX2R(row, i, n)] * x[i];
        }
        goldenY[row] = l_val + y[row];
    }
    return goldenY;
}

bool compareGemv(XFBLAS_dataType* y, XFBLAS_dataType* goldenY, int m, float p_TolRel = 1e-3, float p_TolAbs = 1e-5) {
    bool l_check = true;
    for (int row = 0; row < m; row++) {
        XFBLAS_dataType l_ref = goldenY[row];
        XFBLAS_dataType l_result = y[row];
        float l_diffAbs = abs(l_ref - l_result);
        float l_diffRel = l_diffAbs;
        if (goldenY[row] != 0) {
            l_diffRel /= abs(l_ref);
        }
        bool check = (l_diffRel <= p_TolRel) || (l_diffAbs <= p_TolAbs);
        if (!check) {
            cout << "#" << row << " golden result " << setprecision(10) << goldenY[row]
                 << " is not equal to fpga result " << setprecision(10) << y[row] << "\n";
            l_check = false;
        }
    }
    return l_check;
}

#endif
