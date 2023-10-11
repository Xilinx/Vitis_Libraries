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
#include "aie_graph_params.h"
using namespace std;

template <typename T>
void writeGLD(ComplexMatrix<T>& Q, ComplexMatrix<T>& R, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str());
    myfile1.open(filename1.c_str());
    for (int j = 0; j < Q.N; j++) {
        for (int i = 0; i < Q.M; i += 4) {
            myfile0 << Q.elem(i, j).real() << " ";
            myfile0 << Q.elem(i, j).imag() << " ";
            myfile0 << Q.elem(i + 1, j).real() << " ";
            myfile0 << Q.elem(i + 1, j).imag() << " " << endl;

            myfile1 << Q.elem(i + 2, j).real() << " ";
            myfile1 << Q.elem(i + 2, j).imag() << " ";
            myfile1 << Q.elem(i + 3, j).real() << " ";
            myfile1 << Q.elem(i + 3, j).imag() << " " << endl;
        }

        for (int i = 0; i < R.N; i += 4) {
            myfile0 << R.elem(i, j).real() << " ";
            myfile0 << R.elem(i, j).imag() << " ";
            myfile0 << R.elem(i + 1, j).real() << " ";
            myfile0 << R.elem(i + 1, j).imag() << " " << endl;

            myfile1 << R.elem(i + 2, j).real() << " ";
            myfile1 << R.elem(i + 2, j).imag() << " ";
            myfile1 << R.elem(i + 3, j).real() << " ";
            myfile1 << R.elem(i + 3, j).imag() << " " << endl;
        }
    }

    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeFile(ComplexMatrix<T>& A, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str());
    myfile1.open(filename1.c_str());
    for (int j = 0; j < A.N; j++) {
        for (int i = 0; i < A.M; i += 4) {
            myfile0 << A.elem(i, j).real() << " ";
            myfile0 << A.elem(i, j).imag() << " ";
            myfile0 << A.elem(i + 1, j).real() << " ";
            myfile0 << A.elem(i + 1, j).imag() << " " << endl;

            myfile1 << A.elem(i + 2, j).real() << " ";
            myfile1 << A.elem(i + 2, j).imag() << " ";
            myfile1 << A.elem(i + 3, j).real() << " ";
            myfile1 << A.elem(i + 3, j).imag() << " " << endl;
        }
        for (int i = 0; i < A.N; i += 4) {
            myfile0 << T(0.0) << " ";
            myfile0 << T(0.0) << " ";
            myfile0 << T(0.0) << " ";
            myfile0 << T(0.0) << " " << endl;

            myfile1 << T(0.0) << " ";
            myfile1 << T(0.0) << " ";
            myfile1 << T(0.0) << " ";
            myfile1 << T(0.0) << " " << endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T>
void writeFilebyRow(ComplexMatrix<T>& A, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str());
    myfile1.open(filename1.c_str());
    for (int i = 0; i < A.M; i++) {
        for (int j = 0; j < A.N; j += 4) {
            myfile0 << A.elem(i, j).real() << endl;
            myfile0 << A.elem(i, j).imag() << endl;
            myfile0 << A.elem(i, j + 1).real() << endl;
            myfile0 << A.elem(i, j + 1).imag() << endl;

            myfile1 << A.elem(i, j + 2).real() << endl;
            myfile1 << A.elem(i, j + 2).imag() << endl;
            myfile1 << A.elem(i, j + 3).real() << endl;
            myfile1 << A.elem(i, j + 3).imag() << endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

int main() {
    using namespace std;

    // A matrix
    typedef float Type;
    const int M = row_num; // rows
    const int N = col_num; // columns

    ComplexMatrix<Type> A(M, N);
    ComplexMatrix<Type> B(M, N);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A.elem(i, j).real(rand() % 100 - 49.5);
            A.elem(i, j).imag(rand() % 100 - 49.5);
        }
    }

    writeFile<Type>(A, "A0.txt", "A1.txt");

    ComplexMatrix<Type> Q(M, M);
    ComplexMatrix<Type> R(M, N);

    A.gram_schmidt(Q, R);
    writeGLD<Type>(Q, R, "Gld0.txt", "Gld1.txt");
    return 0;
}
