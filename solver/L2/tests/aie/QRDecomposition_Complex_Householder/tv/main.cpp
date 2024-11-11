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
using namespace std;

// const int M = 4; // rows
// const int N = 3; // columns
const int M = 16; // rows
const int N = 5;  // columns
// const int M = 64; // rows
// const int N = 5; // columns
// const int M = 64; // rows
// const int N = 32; // columns
// const int M = 512; // rows
// const int N = 256; // columns
const int row_num = M;
const int col_num = N;
std::string fmatAU_0 = "../data/matAU_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fmatAU_1 = "../data/matAU_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fmatRQ_0 = "../data/matRQ_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fmatRQ_1 = "../data/matRQ_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fgldRQ_0 = "../data/gldRQ_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fgldRQ_1 = "../data/gldRQ_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";

void printMatrix(std::string file0, std::string file1, int M, int N) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[M][N];
    std::complex<float> Q[M][M];
    // getline (fd0, str0, '\n');
    // getline (fd1, str1, '\n');
    for (unsigned int j = 0; j < N; j++) {
        for (unsigned int i = 0; i < M; i += 4) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R, data0I;
            float data1R, data1I;
            stream0 >> data0R;
            stream0 >> data0I;
            stream0 >> data1R;
            stream0 >> data1I;

            R[i + 0][j].real(data0R);
            R[i + 0][j].imag(data0I);
            R[i + 1][j].real(data1R);
            R[i + 1][j].imag(data1I);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }
            std::stringstream stream1(str1);
            float data2R, data2I;
            float data3R, data3I;
            stream1 >> data2R;
            stream1 >> data2I;
            stream1 >> data3R;
            stream1 >> data3I;

            R[i + 2][j].real(data2R);
            R[i + 2][j].imag(data2I);
            R[i + 3][j].real(data3R);
            R[i + 3][j].imag(data3I);
        }
    }
    for (unsigned int j = 0; j < M; j++) {
        for (unsigned int i = 0; i < M; i += 4) {
            while (getline(fd0, str0, '\n')) {
                if (str0.front() != c) {
                    break;
                }
            }
            std::stringstream stream0(str0);
            float data0R, data0I;
            float data1R, data1I;
            stream0 >> data0R;
            stream0 >> data0I;
            stream0 >> data1R;
            stream0 >> data1I;

            Q[i + 0][j].real(data0R);
            Q[i + 0][j].imag(data0I);
            Q[i + 1][j].real(data1R);
            Q[i + 1][j].imag(data1I);

            while (getline(fd1, str1, '\n')) {
                if (str1.front() != c) {
                    break;
                }
            }

            std::stringstream stream1(str1);
            float data2R, data2I;
            float data3R, data3I;
            stream1 >> data2R;
            stream1 >> data2I;
            stream1 >> data3R;
            stream1 >> data3I;

            Q[i + 2][j].real(data2R);
            Q[i + 2][j].imag(data2I);
            Q[i + 3][j].real(data3R);
            Q[i + 3][j].imag(data3I);
        }
    }

    std::cout << "Matrix_R[" << M << " x " << N << "]" << std::endl;
    for (unsigned int i = 0; i < M; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < N; j++) {
            std::cout << "(" << R[i][j].real() << ", " << R[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Matrix_Q[" << M << " x " << M << "]" << std::endl;
    for (unsigned int i = 0; i < M; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < M; j++) {
            std::cout << "(" << Q[i][j].real() << ", " << Q[i][j].imag() << "), ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template <typename T>
void writeFile(ComplexMatrix<T>& A, ComplexMatrix<T>& U, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0.c_str());
    myfile1.open(filename1.c_str());
    for (int j = 0; j < A.N; j++) {
        for (int i = 0; i < A.M; i += 4) {
            myfile0 << A.elem(i, j).real() << " ";
            myfile0 << A.elem(i, j).imag() << " ";
            myfile0 << A.elem(i + 1, j).real() << " ";
            myfile0 << A.elem(i + 1, j).imag() << " " << std::endl;

            myfile1 << A.elem(i + 2, j).real() << " ";
            myfile1 << A.elem(i + 2, j).imag() << " ";
            myfile1 << A.elem(i + 3, j).real() << " ";
            myfile1 << A.elem(i + 3, j).imag() << " " << std::endl;
        }
    }
    for (int j = 0; j < U.N; j++) {
        for (int i = 0; i < U.M; i += 4) {
            myfile0 << U.elem(i, j).real() << " ";
            myfile0 << U.elem(i, j).imag() << " ";
            myfile0 << U.elem(i + 1, j).real() << " ";
            myfile0 << U.elem(i + 1, j).imag() << " " << std::endl;

            myfile1 << U.elem(i + 2, j).real() << " ";
            myfile1 << U.elem(i + 2, j).imag() << " ";
            myfile1 << U.elem(i + 3, j).real() << " ";
            myfile1 << U.elem(i + 3, j).imag() << " " << std::endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

int main() {
    using namespace std;

    // A matrix
    typedef float Type;

    ComplexMatrix<Type> A(M, N);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A.elem(i, j).real(rand() % 100 - 49.5);
            A.elem(i, j).imag(rand() % 100 - 49.5);
        }
    }
    ComplexMatrix<Type> A3(A);

    std::cout << std::endl;
    std::cout << "QRDecomposition with Householder_3 \n";
    ComplexMatrix<Type> Q3(M, M);
    ComplexMatrix<Type> R3(M, N);

    Q3.createIdentity();
    writeFile<Type>(A3, Q3, fmatAU_0, fmatAU_1);
    // std::cout << "Matrix_A3: \n";
    // A3.print();
    // std::cout << "Matrix_U3: \n";
    // Q3.print();

    A3.qrd_householder_4(Q3, R3);
    // std::cout << "Matrix_Q3: \n";
    // Q3.print();
    // std::cout << "Matrix_R3: \n";
    // R3.print();
    ComplexMatrix<Type> QT3(Q3);
    QT3.conj_transpose();
    // std::cout << "Matrix_QT3: \n";
    // QT3.print();

    writeFile<Type>(R3, QT3, fgldRQ_0, fgldRQ_1);

    std::cout << "Verify HouseHolder_3 \n";
    ComplexMatrix<Type> QR3(A3);
    QR3.matrix_mul(Q3, R3);
    std::cout << "Diff QR3-Base \n";
    QR3.diff(A);
    std::cout << "Check Q3 unitary \n";
    bool is_unitary_3 = Q3.is_unitary();

    std::cout << "Check Q*QT unitary \n";
    ComplexMatrix<Type> I3(Q3);
    I3.matrix_mul(Q3, QT3);
    bool is_unitary_Q = I3.is_unitary();

    std::cout << std::endl;
    // std::cout << "Matrix_R && Matrix_Q \n";
    // printMatrix(fmatRQ_0, fmatRQ_1, row_num, col_num);
    // std::cout << "Gld Matrix_R && Matrix_Q \n";
    // printMatrix(fgldRQ_0, fgldRQ_1, row_num, col_num);

    return 0;
}
