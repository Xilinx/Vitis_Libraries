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

#include "xf_blas.hpp"

#define IDX2R(i, j, ld) (((i) * (ld)) + (j))
#define m 6 // a - mxk matrix
#define n 5 // b - kxn matrix

using namespace std;

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << " usage: \n"
             << " gemv_common_test.exe gemx.xclbin config_info.dat 1\n"
             << " gemv_common_test.exe gemx.xclbin config_info.dat\n";
        return EXIT_FAILURE;
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);

    int l_numKernel = 1;

    if (argc == 4) {
        cout << "read custom number of kernels\n";
        l_numKernel = stoi(argv[l_argIdx++]);
    }

    int i, j; // i-row index ,j- column index

    XFBLAS_dataType *a, *x, *y;
    a = (XFBLAS_dataType*)malloc(m * n * sizeof(XFBLAS_dataType)); // host memory for a
    x = (XFBLAS_dataType*)malloc(n * 1 * sizeof(XFBLAS_dataType));
    y = (XFBLAS_dataType*)malloc(m * 1 * sizeof(XFBLAS_dataType));

    int ind = 1;

    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            a[IDX2R(i, j, n)] = (XFBLAS_dataType)ind++;
        }
    }

    ind = 1;
    for (i = 0; i < n; i++) {
        x[i] = (XFBLAS_dataType)ind;
    }

    for (i = 0; i < m; i++) {
        y[i] = 0;
    }

    XFBLAS_dataType* d_a = NULL;
    XFBLAS_dataType* d_x = NULL;
    XFBLAS_dataType* d_y = NULL;

    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMV;
    xfblasStatus_t status = XFBLAS_STATUS_SUCCESS;

    status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, engineName, l_numKernel);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Create Handle failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_a, m, n, sizeof(*a), l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix A failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }
    status = xfblasMalloc(&d_x, n, 1, sizeof(*x), l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix B failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_y, m, 1, sizeof(*y), l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix C failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    status = xfblasSetMatrix(m, n, sizeof(*a), a, n, d_a, l_numKernel - 1);
    status = xfblasSetVector(n, sizeof(*x), x, 1, d_x, l_numKernel - 1);
    status = xfblasSetVector(m, sizeof(*y), y, 1, d_y, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Set Matrix failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    status = xfblasGemv(XFBLAS_OP_N, m, n, 1, d_a, n, d_x, 1, 1, d_y, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Matrix Vector Multiplication failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }
    status = xfblasGetVector(m, sizeof(*y), d_y, y, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Get Matrix failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    for (i = 0; i < m; i++) {
        cout << (y[i]) << " ";
        cout << " ";
    }
    cout << "\n";
    // 15 40 65 90 115 140

    xfblasFree(d_a, l_numKernel - 1);
    xfblasFree(d_x, l_numKernel - 1);
    xfblasFree(d_y, l_numKernel - 1);
    xfblasDestroy(l_numKernel);
    free(a);
    free(x);
    free(y);
}
