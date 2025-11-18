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
#include "../kernel/dut_type.hpp"

////const unsigned int M = 512;
////const unsigned int K = 256;
////const unsigned int N = 256;
//const unsigned int M = 64;
//const unsigned int K = 32;
//const unsigned int N = 16;
//const unsigned ROWA= M;
//const unsigned COLA= K;
//const unsigned ROWB= K;
//const unsigned COLB= N;
//const unsigned ROWC= M;
//const unsigned COLC= N;

std::string file_A = "datas/A_" + std::to_string(ROWA) + "_" + std::to_string(COLA) + ".txt";
std::string file_B = "datas/B_" + std::to_string(ROWB) + "_" + std::to_string(COLB) + ".txt";
std::string file_C = "datas/Cgld_" + std::to_string(ROWC) + "_" + std::to_string(COLC) + ".txt";

using namespace std;
typedef float Type;

int main() {
    ComplexMatrix<Type> A(M, K);
    ComplexMatrix<Type> B(K, N);
    ComplexMatrix<Type> C(M, N);

    // generate random matrix L
    // std::cout << "Generate lower triangular matrixL \n";
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            float dataAR = A.random_float(1, 4, 0);
            float dataAI = A.random_float(1, 4, 0);
            //float dataAR = i+1;
            //float dataAI = j+1;
            A.elem(i, j).real(dataAR);
            A.elem(i, j).imag(dataAI);
        }
    }

    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            float dataBR = B.random_float(1, 4, 0);
            float dataBI = B.random_float(1, 4, 0);
            //float dataBR;
            //float dataBI;
            //if(i==j){
            //    dataBR = 1;
            //    dataBI = 0;
            //}
            //else {
            //    dataBR = 0;
            //    dataBI = 0;
            //}
            B.elem(i, j).real(dataBR);
            B.elem(i, j).imag(dataBI);
        }
    }
    writeComplextoFileByRow<Type>(A, file_A);
    writeComplextoFileByRow<Type>(B, file_B);
    //A.print();
    //B.print();

    std::cout << "Gernerate matrix C: \n";
    C.matrix_mul(A, B);
    //C.print();
    writeComplextoFileByRow<Type>(C, file_C);

    //std::cout << "Output matrix C : \n";

    //std::cout << "Verify matrit multiplication: Diff gld - out" << std::endl;

    return 0;
}
