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
#include "../helper_test.hpp"

#define IDX2R(i, j, ld) (((i) * (ld)) + (j))

using namespace std;

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << " usage: \n"
             << " gemv_test.exe gemx.xclbin config_info.dat iterIndex dataDir\n";
        return EXIT_FAILURE;
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    int iterIndex = atoi(argv[l_argIdx++]);
    string l_dataDir(argv[l_argIdx++]);
    string l_logFile;
    ofstream logFile("out_test/xrt_report.txt");
    logFile.close();
    l_logFile = "out_test/xrt_report.txt";

    int i, j;
    XFBLAS_dataType *a, *x, *y, *goldenY;

    ifstream l_instrFile;
    l_instrFile.open(l_dataDir + "param_in" + to_string(iterIndex) + ".bin");
    int* l_instr;
    posix_memalign((void**)&l_instr, 4096, 9 * sizeof(int));
    if (l_instrFile.is_open()) {
        l_instrFile.read((char*)l_instr, 9 * sizeof(int));
        l_instrFile.close();
    } else {
        cerr << "could not find instruction file " << (l_dataDir + "param_in" + to_string(iterIndex) + ".bin") << "\n";
        exit(1);
    }

    int m = l_instr[1];
    int n = l_instr[2];
    int alpha = l_instr[3];
    int lda = l_instr[4];
    int incx = l_instr[5];
    int beta = l_instr[6];
    int incy = l_instr[7];
    int l_numKernel = l_instr[8] + 1;

    posix_memalign((void**)&a, 4096, m * n * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&x, 4096, n * 1 * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&y, 4096, m * 1 * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&goldenY, 4096, m * 1 * sizeof(XFBLAS_dataType));

    readMatBin((char*)a, iterIndex, m, n, l_dataDir, "matA_in", sizeof(XFBLAS_dataType));
    readMatBin((char*)x, iterIndex, n, 1, l_dataDir, "vecX_in", sizeof(XFBLAS_dataType));
    readMatBin((char*)y, iterIndex, m, 1, l_dataDir, "vecY_in", sizeof(XFBLAS_dataType));
    readMatBin((char*)goldenY, iterIndex, m, 1, l_dataDir, "vecY_out", sizeof(XFBLAS_dataType));

    XFBLAS_dataType* d_a = NULL;
    XFBLAS_dataType* d_x = NULL;
    XFBLAS_dataType* d_y = NULL;

    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMV;
    xfblasStatus_t status =
        xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Create Handle failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_a, m, n, sizeof(*a), l_numKernel - 1);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix A failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_x, n, 1, sizeof(*x), l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix B failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }
    status = xfblasMalloc(&d_y, m, 1, sizeof(*y), l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix C failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasSetMatrix(m, n, sizeof(*a), a, n, d_a, l_numKernel - 1);
    status = xfblasSetVector(n, sizeof(*x), x, 1, d_x, l_numKernel - 1);
    status = xfblasSetVector(m, sizeof(*y), y, 1, d_y, l_numKernel - 1);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Set Matrix failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasGemv(XFBLAS_OP_N, m, n, 1, d_a, n, d_x, 1, 1, d_y, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Matrix Vector Multiplication failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasGetVector(m, sizeof(*y), d_y, y, 1, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Get Matirx failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    if (compareVector<XFBLAS_dataType>(y, goldenY, m)) {
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
    free(l_instr);

    xfblasDestroy(l_numKernel);

    return EXIT_SUCCESS;
}
