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
    ComplexMatrix<Type> A(ROW, COL);
    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < COL; j++) {
            A.elem(i, j).real(rand() % 100 - 49.5);
            A.elem(i, j).imag(rand() % 100 - 49.5);
        }
    }
    ComplexMatrix<Type> A3(A);

    std::cout << std::endl;
    ComplexMatrix<Type> Q3(ROW, ROW);
    ComplexMatrix<Type> R3(ROW, COL);

    Q3.createIdentity();
    writeComplexVec2toFile<Type>(A3, fin);
    writeComplexVec2toFile<Type>(Q3, fin);
    std::cout << "Matrix_A3: \n";
    // A3.print();

    std::cout << "QRDecomposition with Householder_3 \n";
    // A3.qrd_householder_4(Q3, R3);
    A3.qrd_householder(Q3, R3);
    // std::cout << "Matrix_Q3: \n";
    // Q3.print();
    // std::cout << "Matrix_R3: \n";
    // R3.print();
    ComplexMatrix<Type> QT3(Q3);
    QT3.conj_transpose();
    // std::cout << "Matrix_QT3: \n";
    // QT3.print();

    writeComplexVec2toFile<Type>(R3, fgld);
    writeComplexVec2toFile<Type>(QT3, fgld);

    std::cout << "Verify HouseHolder_3 \n";
    std::cout << "1. Check Q3 unitary \n";
    bool is_unitary_3 = Q3.is_unitary();
    std::cout << "2. Check Diff(QR3-A3) \n";
    ComplexMatrix<Type> QR3(A3);
    QR3.matrix_mul(Q3, R3);
    QR3.diff(A);
    // QR3.print();
    // A.print();

    std::cout << std::endl;
    std::cout << "3. Check diffCompelxMatrix \n";
    // int errs = diffComplexLowerTriangularMatrix(QR3, A3);
    int errs = diffComplexMatrix(QR3, A);

    return 0;
}
