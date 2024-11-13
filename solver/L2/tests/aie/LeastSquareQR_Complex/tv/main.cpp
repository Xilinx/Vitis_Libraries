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

const int row_num = M;
const int col_num = N;
std::string fin_0 = "../data/in_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fin_1 = "../data/in_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";

std::string fRC_0 = "../data/RC_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fRC_1 = "../data/RC_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fRC_QRD_0 = "../data/RC_QRD_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fRC_QRD_1 = "../data/RC_QRD_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";

std::string fgldRC_0 = "../data/gldRC_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fgldRC_1 = "../data/gldRC_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fRC_Trans_0 = "../data/RC_Trans_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fRC_Trans_1 = "../data/RC_Trans_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";

std::string fout_0 = "../data/out_0_" + std::to_string(col_num) + ".txt";
std::string fout_1 = "../data/out_1_" + std::to_string(col_num) + ".txt";
std::string fgld_0 = "../data/gld_0_" + std::to_string(col_num) + ".txt";
std::string fgld_1 = "../data/gld_1_" + std::to_string(col_num) + ".txt";


int main() {
    ComplexMatrix<Type> A(M, N);
    ComplexMatrix<Type> Q(M, M);
    ComplexMatrix<Type> R(M, N);
    ComplexMatrix<Type> X(N, K);
    ComplexMatrix<Type> B(M, K);
    ComplexMatrix<Type> E(M, K);
    //generate random matrix A
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A.elem(i, j).real(rand() % 100 - 49.5);
            A.elem(i, j).imag(rand() % 100 - 49.5);
        }
    }
    //std::cout << "Generate Random Matrix_A, size is mxn: \n";
    //A.print();
    //A.qrd_householder(Q, R);
    //R.print();    
    ////generate random upper triangual matrix R
    //for (int i = 0; i < M; i++) {
    //    for (int j = 0; j < i; j++) {
    //        R.elem(i, j).real(0.0);
    //        R.elem(i, j).imag(0.0);
    //    }
    //    for (int j = i; j < N; j++) {
    //        R.elem(i, j).real(rand() % 100 - 49.5);
    //        R.elem(i, j).imag(rand() % 100 - 49.5);
    //    }
    //}
    //std::cout << "Generate Random Upper Tiangular Matrix_R, size is mxn: \n";
    //R.print();
    //A.matrix_mul(Q, R);
    ComplexMatrix<Type> A1(A);
    //generate random vector X
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < K; j++) {
            X.elem(i, j).real(rand() % 100 - 49.5);
            X.elem(i, j).imag(rand() % 100 - 49.5);
        }
    }
    std::cout << "Generate random Vector X: \n";
    X.print();
    writeComplexToFiles<Type>(X, fgld_0, fgld_1, N);

    B.matrix_mul(A, X);
    //generate random Error vector E
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            Type a = (Type)rand()/RAND_MAX;
            Type b = (Type)rand()/RAND_MAX;
            E.elem(i,j).real(a*a);
            E.elem(i,j).imag(b*b);
        }
    }
    B.matrix_add(B, E);
    std::cout << "Generate Matrix_B with random Error: \n";
    B.print();
    ComplexMatrix<Type> B1(B);
    ComplexMatrix<Type> C(B);

    writeComplexToFiles128<Type>(A, fin_0, fin_1);
    writeComplexToFiles128<Type>(B, fin_0, fin_1);
    //std::cout << "Write Input MatrixA && MatrixB to files: \n";
    //printComplexMatrix128(fin_0, fin_1, M, N, M, K);

    
    ComplexMatrix<Type> Y(N, K, 0.0);
    if(N<65){
        std::cout << "Least Squares Method Input: Matrix A && Matrix B: " << std::endl;
        std::cout << "Matrix A: " << std::endl;
        A.print();
        std::cout << "Matrix B: " << std::endl;
        B.print();
    }
    //A.lstqr(B, Y);
    A.lstqr_householder(R, C);

    if(N<65){
        std::cout << "Back_Substitution Input: Matrix R && Matrix C: " << std::endl;
        std::cout << "Matrix_R: \n";
        R.print();
        std::cout << "Matrix_C: \n";
        C.print();
    }

    writeComplexToFiles128<Type>(R, fRC_QRD_0, fRC_QRD_1);
    writeComplexToFiles128<Type>(C, fRC_QRD_0, fRC_QRD_1);
    //std::cout << "Write QR decomposition results MatrixR && MatrixQB to files: \n";
    //printComplexMatrix128(fRC_0, fRC_1, M, N, M, K);

    writeComplexTriangularToFiles<Type>(R, fRC_Trans_0, fRC_Trans_1);
    writeComplexToFiles<Type>(C, fRC_Trans_0, fRC_Trans_1, N);
    std::cout << "Write Backsubstitution's input MatrixR && MatrixC to files: \n";
    printTriangularMatrix(fRC_Trans_0, fRC_Trans_1, N, N, N, K);

    R.back_substitution(C, Y);
    std::cout << "Output Matrix Y: " << std::endl;
    Y.print();

    Type CRes[K];
    R.lstqr_residuals(C, CRes);
    //C.print();

    //writeCVec4File<Type>(Y, fout_0, fout_1);
    //writeComplexToFiles_backwords<Type>(Y, fout_backwords_0, fout_backwords_1, N);
    writeComplexToFiles<Type>(Y, fout_0, fout_1, N);

    std::cout << "Verify LeastSquare: Diff gld(X) - out(Y)"<< std::endl;
    Y.diff(X);

    std::cout << "Verify LeastSquare: Diff (B - AY)"<< std::endl;
    ComplexMatrix<Type> AY(M,K,0.0);
    AY.matrix_mul(A1, Y);
    ComplexMatrix<Type> BDiff(N, K, 0.0);
    BDiff.matrix_sub(B1, AY);
    //BDiff.print();
    //AY.print();
    //B1.print();

    Type BRes[K];
    BDiff.norm(BDiff, BRes);

    return 0;
}
