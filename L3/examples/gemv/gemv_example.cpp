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

/*
 * usage: ./gemm_test.exe PATH_TO_XCLBIN/gemx.xclbin PATH_TO_XCLBIN/config_info.dat
 *
 */

#include <iomanip>
#include "xf_blas.hpp"

#define IDX2R(i, j, ld) (((i) * (ld)) + (j))
#define m 256 // a - mxk matrix
#define n 256 // b - kxn matrix

using namespace std;

XFBLAS_dataType* getGoldenMat(XFBLAS_dataType* a, XFBLAS_dataType* x, XFBLAS_dataType* y) {
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

bool compareGemv(XFBLAS_dataType* y, XFBLAS_dataType* goldenY, float p_TolRel = 1e-3, float p_TolAbs = 1e-5) {
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
            cout << "golden result " << setprecision(10) << goldenY[row] << " is not equal to fpga result "
                 << setprecision(10) << y[row] << "\n";
            l_check = false;
        }
    }
    return l_check;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << " usage: \n"
             << " gemv_test.exe gemx.xclbin config_info.dat 1\n"
             << " gemv_test.exe gemx.xclbin config_info.dat\n";
        return EXIT_FAILURE;
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    string l_logFile;
    ofstream logFile("xrt_report.txt");
    logFile.close();
    l_logFile = "xrt_report.txt";
    int l_numKernel = 1;

    if (argc == 4) {
        cout << "read custom number of kernels\n";
        l_numKernel = stoi(argv[l_argIdx++]);
    }

    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMV;
    xfblasStatus_t status =
        xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Create Handle failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1
    XFBLAS_dataType *a, *x, *y;

    posix_memalign((void**)&a, 4096, m * n * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&x, 4096, n * 1 * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&y, 4096, m * 1 * sizeof(XFBLAS_dataType));

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

    XFBLAS_dataType* goldenY = getGoldenMat(a, x, y);

    status = xfblasMallocRestricted(m, n, sizeof(*a), a, n, l_numKernel - 1);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix A failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasMallocRestricted(n, 1, sizeof(*x), x, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix B failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }
    status = xfblasMallocRestricted(m, 1, sizeof(*y), y, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix C failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasSetMatrixRestricted(a, l_numKernel - 1);
    status = xfblasSetVectorRestricted(x, l_numKernel - 1);
    status = xfblasSetVectorRestricted(y, l_numKernel - 1);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Set Matrix failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasGemv(XFBLAS_OP_N, m, n, 1, a, n, x, 1, 1, y, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Matrix Vector Multiplication failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasGetVectorRestricted(y, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Get Matirx failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    for (i = 0; i < 10; i++) {
        cout << (y[i]) << " ";
        cout << "\n";
    }

    if (compareGemv(y, goldenY)) {
        cout << "Test passed!\n";
    } else {
        cout << "Test failed!\n";
    }

    xfblasFree(a, l_numKernel - 1);
    xfblasFree(x, l_numKernel - 1);
    xfblasFree(y, l_numKernel - 1);
    free(a);
    free(x);
    free(y);
    free(goldenY);

    xfblasDestroy(l_numKernel);

    return EXIT_SUCCESS;
}
