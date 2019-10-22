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
             << " gemm_test.exe gemx.xclbin config_info.dat iterIndex dataDir\n";
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
    XFBLAS_dataType *a, *b, *c, *goldenC;

    ifstream l_instrFile;
    l_instrFile.open(l_dataDir + "param_in" + to_string(iterIndex) + ".bin");
    int* l_instr;
    posix_memalign((void**)&l_instr, 4096, 11 * sizeof(int));
    if (l_instrFile.is_open()) {
        l_instrFile.read((char*)l_instr, 11 * sizeof(int));
        l_instrFile.close();
    } else {
        cerr << "could not find instruction file " << (l_dataDir + "param_in" + to_string(iterIndex) + ".bin") << "\n";
        exit(1);
    }

    int m = l_instr[2];
    int n = l_instr[3];
    int k = l_instr[4];
    int alpha = l_instr[5];
    int lda = l_instr[6];
    int ldb = l_instr[7];
    int beta = l_instr[8];
    int ldc = l_instr[9];
    int l_numKernel = l_instr[10] + 1;

    posix_memalign((void**)&a, 4096, m * k * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&b, 4096, k * n * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&c, 4096, m * n * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&goldenC, 4096, m * n * sizeof(XFBLAS_dataType));

    readMatBin((char*)a, iterIndex, m, k, l_dataDir, "matA_in", sizeof(XFBLAS_dataType));
    readMatBin((char*)b, iterIndex, k, n, l_dataDir, "matB_in", sizeof(XFBLAS_dataType));
    readMatBin((char*)c, iterIndex, m, n, l_dataDir, "matC_in", sizeof(XFBLAS_dataType));
    readMatBin((char*)goldenC, iterIndex, m, n, l_dataDir, "matC_out", sizeof(XFBLAS_dataType));

    XFBLAS_dataType* d_a = NULL;
    XFBLAS_dataType* d_b = NULL;
    XFBLAS_dataType* d_c = NULL;

    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status =
        xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Create Handle failed with error code: " << status << "\n";
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_a, m, k, sizeof(*a), l_numKernel - 1);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix A failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_b, k, n, sizeof(*b), l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix B failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasMalloc(&d_c, m, n, sizeof(*c), l_numKernel - 1);
    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Malloc memory for matrix C failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasSetMatrix(m, k, sizeof(*a), a, k, d_a, l_numKernel - 1);
    status = xfblasSetMatrix(k, n, sizeof(*b), b, n, d_b, l_numKernel - 1);
    status = xfblasSetMatrix(m, n, sizeof(*c), c, n, d_c, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Set Matrix failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, n, k, 1, d_a, k, d_b, n, 1, d_c, n, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Matrix Multiplication failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    status = xfblasGetMatrix(m, n, sizeof(*c), d_c, c, n, l_numKernel - 1);

    if (status != XFBLAS_STATUS_SUCCESS) {
        cout << "Get Matirx failed with error code: " << status << "\n";
        xfblasDestroy();
        return EXIT_FAILURE;
    }

    if (compareMat<XFBLAS_dataType>(c, goldenC, m, n)) {
        cout << "Test passed!\n";
    } else {
        cout << "Test failed!\n";
    }

    xfblasFree(d_a, l_numKernel - 1);
    xfblasFree(d_b, l_numKernel - 1);
    xfblasFree(d_c, l_numKernel - 1);
    free(a);
    free(b);
    free(c);
    free(goldenC);
    free(l_instr);

    xfblasDestroy(l_numKernel);

    return EXIT_SUCCESS;
}