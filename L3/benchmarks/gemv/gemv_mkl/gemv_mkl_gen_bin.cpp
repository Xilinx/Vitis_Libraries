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

#include "gemv_mkl_helper.hpp"

#include <fstream>
#include <string>
using namespace std;

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: gemv_mkl m n dir\n");
        return EXIT_FAILURE;
    }

    int m = atoi(argv[1]), n = atoi(argv[2]);
    XFBLAS_dataType *a, *x, *y, alpha = 1., beta = 1.;

    // Generating Random Input
    a = createMat(m, n);
    x = createMat(n, 1);
    y = createMat(m, 1, true);

    ofstream outFile;
    string data_dir(argv[3]);
#ifdef USE_SHORT
    short *a_short, *x_short, *y_short;
    if (posix_memalign((void**)&a_short, 4096, (size_t)m * (size_t)n * sizeof(short)) != 0) {
        printf("[ERROR] failed to create the matrix a_short\n");
        exit(1);
    }
    if (posix_memalign((void**)&x_short, 4096, (size_t)n * sizeof(short)) != 0) {
        printf("[ERROR] failed to create the vector x_short\n");
        exit(1);
    }
    if (posix_memalign((void**)&y_short, 4096, (size_t)m * sizeof(short)) != 0) {
        printf("[ERROR] failed to create the vector y_short\n");
        exit(1);
    }
    printf("[WARNING] The short data is currently casted from float datatype.\n");
    for (int i = 0; i < m * n; i++) a_short[i] = (short)a[i];
    for (int i = 0; i < n; i++) x_short[i] = (short)x[i];
    for (int i = 0; i < m; i++) y_short[i] = (short)y[i];

    outFile.open(data_dir + "matA_in_" + to_string(m) + "_" + to_string(n) + ".bin", ofstream::binary);
    outFile.write((char*)a_short, sizeof(short) * m * n);
    outFile.close();

    outFile.open(data_dir + "vecX_in_" + to_string(n) + "_" + to_string(1) + ".bin", ofstream::binary);
    outFile.write((char*)x_short, sizeof(short) * n * 1);
    outFile.close();

    outFile.open(data_dir + "vecY_in_" + to_string(m) + "_" + to_string(1) + ".bin", ofstream::binary);
    outFile.write((char*)y_short, sizeof(short) * m * 1);
    outFile.close();
#else
    outFile.open(data_dir + "matA_in_" + to_string(m) + "_" + to_string(n) + ".bin", ofstream::binary);
    outFile.write((char*)a, sizeof(XFBLAS_dataType) * m * n);
    outFile.close();

    outFile.open(data_dir + "vecX_in_" + to_string(n) + "_" + to_string(1) + ".bin", ofstream::binary);
    outFile.write((char*)x, sizeof(XFBLAS_dataType) * n * 1);
    outFile.close();

    outFile.open(data_dir + "vecY_in_" + to_string(m) + "_" + to_string(1) + ".bin", ofstream::binary);
    outFile.write((char*)y, sizeof(XFBLAS_dataType) * m * 1);
    outFile.close();
#endif
    // Generating Golden Output
    GEMV_MKL(m, n, alpha, beta, a, x, y);

#ifdef USE_SHORT
    for (int i = 0; i < m; i++) y_short[i] = (short)y[i];

    outFile.open(data_dir + "vecY_out_" + to_string(m) + "_" + to_string(1) + ".bin", ofstream::binary);
    outFile.write((char*)y_short, sizeof(short) * m * 1);
    outFile.close();
    free(a_short);
    free(x_short);
    free(y_short);
#else
    outFile.open(data_dir + "vecY_out_" + to_string(m) + "_" + to_string(1) + ".bin", ofstream::binary);
    outFile.write((char*)y, sizeof(XFBLAS_dataType) * m * 1);
    outFile.close();
#endif
    free(a);
    free(x);
    free(y);

    return 0;
}
