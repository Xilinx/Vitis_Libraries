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

#include <iostream>
#include <cmath>
#include <fstream>
#include <string>
#include <cstring>
#include "matrix.hpp"
#include "utils.hpp"
#include "../aie/aie_graph_params.h"

using namespace std;
typedef float Type;

int main() {
    ComplexMatrix<Type> A(M, N);
    ComplexMatrix<Type> L(M, N);

    // generate random matrix L
    // std::cout << "Generate lower triangular matrixL \n";
    for (int i = 0; i < M; i++) {
        float dataR;
        float dataI;
        for (int j = 0; j < i; j++) {
            dataR = A.random_float(1, 4, 0);
            dataI = A.random_float(1, 4, 0);
            L.elem(i, j).real(dataR);
            L.elem(i, j).imag(dataI);
        }
        dataR = A.random_float(1, 4, 0);
        L.elem(i, i).real(dataR);
        L.elem(i, i).imag(0.0);
        for (int j = i + 1; j < N; j++) {
            L.elem(i, j).real(0.0);
            L.elem(i, j).imag(0.0);
        }
    }
    // L.print();

    std::cout << "Gernerate matrix A: \n";
    ComplexMatrix<Type> LT(L);
    LT.conj_transpose();
    A.matrix_mul(L, LT);
    // A.print();
    writeComplexVec2toFile<Type>(A, fin);

    ComplexMatrix<Type> Agld(A);
    ComplexMatrix<Type> Aout(A);

    // A.cholesky_opt_1();
    A.cholesky_opt_6();
    std::cout << "Output matrix L : \n";
    // A.print();
    std::cout << "Golden matrix L : \n";
    // L.print();
    writeComplexVec2toFile<Type>(A, fgld);

    std::cout << "Verify Cholesky: Diff gld - out" << std::endl;
    int errs = diffComplexLowerTriangularMatrix(L, A);

    return 0;
}
