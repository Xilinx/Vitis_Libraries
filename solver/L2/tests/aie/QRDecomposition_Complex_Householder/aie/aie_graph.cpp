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

#include "qrd_householder_graph.hpp"
#include "aie_graph_params.h"
using namespace adf;

std::string fmatAU_0 = "data/matAU_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fmatAU_1 = "data/matAU_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fmatRQ_0 = "data/matRQ_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fmatRQ_1 = "data/matRQ_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fgldRQ_0 = "data/gldRQ_0_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";
std::string fgldRQ_1 = "data/gldRQ_1_" + std::to_string(row_num) + "_" + std::to_string(col_num) + ".txt";

xf::solver::QRD_Householder_Graph<row_num, col_num> mygraph(fmatAU_0, fmatAU_1, fmatRQ_0, fmatRQ_1);

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <iostream>
#include <fstream>
using namespace std;

void printMatrix(std::string file0, std::string file1, int M, int N) {
    ifstream fd0, fd1;
    char c = 'T';
    string str0, str1;
    fd0.open(file0, ios::in);
    fd1.open(file1, ios::in);
    std::complex<float> R[M][N];
    std::complex<float> Q[M][M];
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
    // for (unsigned int i = 0; i < M; i++) {
    //    std::cout << "Row[" << i << "]: ";
    //    for (unsigned int j = 0; j < M; j++) {
    //        std::cout << "(" << Q[i][j].real() << ", " << Q[i][j].imag() << "), ";
    //    }
    //    std::cout << std::endl;
    //}
    // std::cout << std::endl;
}

int compare(float out, float gld) {
    if (1 == isnan(out)) {
        std::cout << "out is abnorman, out=" << out << std::endl;
        return 1;
    } else {
        if (abs(gld) > 1.0) {
            if ((abs(out - gld) / abs(gld)) > 0.001) {
                return 1;
            }
        } else {
            if (abs(out - gld) > 0.001) {
                return 1;
            }
        }
    }
    return 0;
}

int golden_check(std::string output_file0,
                 std::string output_file1,
                 std::string golden_file0,
                 std::string golden_file1,
                 int numRow,
                 int numCol) {
    int numErr = 0;
    ifstream f_gld0, f_gld1;
    ifstream f_out0, f_out1;
    f_gld0.open(golden_file0, ios::in);
    f_gld1.open(golden_file1, ios::in);
    f_out0.open(output_file0, ios::in);
    f_out1.open(output_file1, ios::in);

    char c = 'T';
    string str_out0, str_out1;
    string str_gld0, str_gld1;
    int numR = numCol * numRow;
    int numQ = numRow * numRow;
    int num = (numR + numQ);
    int num4 = num / 4;
    int numR4 = numR / 4;
    int numQ4 = numQ / 4;

    for (int j = 0; j < num4; j++) {
        while (getline(f_out0, str_out0, '\n')) {
            if (str_out0.front() != c) {
                break;
            }
        }
        std::stringstream strOut0(str_out0);
        float dat0[4];
        for (int k = 0; k < 4; k++) {
            strOut0 >> dat0[k];
        }
        while (getline(f_gld0, str_gld0, '\n')) {
            if (str_out0.front() != c) {
                break;
            }
        }
        std::stringstream strGld0(str_gld0);
        float gld0[4];
        for (int k = 0; k < 4; k++) {
            strGld0 >> gld0[k];
        }
        for (int l = 0; l < 2; l++) {
            float outR = dat0[l * 2];
            float outI = dat0[l * 2 + 1];
            float gldR = gld0[l * 2];
            float gldI = gld0[l * 2 + 1];
            if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                numErr++;
                if (j < numR4) {
                    int idCol = (j * 4) / numRow;
                    int idRow = (j * 4) % numRow + l;
                    std::cout << "numErr=" << numErr << ", MatR[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                } else {
                    int idCol = ((j - numR4) * 4) / numRow;
                    int idRow = ((j - numR4) * 4) % numRow + l;
                    std::cout << "numErr=" << numErr << ", MatQ[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                }
            }
        }
        while (getline(f_out1, str_out1, '\n')) {
            if (str_out1.front() != c) {
                break;
            }
        }
        std::stringstream strOut1(str_out1);
        float dat1[4];
        for (int k = 0; k < 4; k++) {
            strOut1 >> dat1[k];
        }
        while (getline(f_gld1, str_gld1, '\n')) {
            if (str_out1.front() != c) {
                break;
            }
        }
        std::stringstream strGld1(str_gld1);
        float gld1[4];
        for (int k = 0; k < 4; k++) {
            strGld1 >> gld1[k];
        }
        for (int l = 0; l < 2; l++) {
            float outR = dat1[l * 2];
            float outI = dat1[l * 2 + 1];
            float gldR = gld1[l * 2];
            float gldI = gld1[l * 2 + 1];
            if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                numErr++;
                if (j < numR4) {
                    int idCol = (j * 4) / numRow;
                    int idRow = (j * 4) % numRow + l + 2;
                    std::cout << "numErr=" << numErr << ", MatR[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                } else {
                    int idCol = ((j - numR4) * 4) / numRow;
                    int idRow = ((j - numR4) * 4) % numRow + l + 2;
                    std::cout << "numErr=" << numErr << ", MatQ[" << idRow << "][" << idCol << "] mis-matched"
                              << ", out=(" << outR << ", " << outI << "), gld=(" << gldR << ", " << gldI << ")"
                              << std::endl;
                }
            }
        }
    }
    return numErr;
}

int main(int argc, char** argv) {
    adf::return_code ret;
    mygraph.init();
    // mygraph.update(mygraph.column_id_pre, (int)0);
    for (int i = 0; i < col_num; i++) {
        mygraph.update(mygraph.column_id[i], i);
    }

    ret = mygraph.run(1);
    if (ret != adf::ok) {
        printf("Run Failed\n");
        return ret;
    }

    ret = mygraph.end();
    if (ret != adf::ok) {
        printf("End Failed\n");
        return ret;
    }

    // compare and check
    std::cout << "Golden check" << std::endl;
    std::cout << "Matrix_R && Matrix_Q \n";
    printMatrix(fmatRQ_0, fmatRQ_1, row_num, col_num);
    std::cout << "Gld Matrix_R && Matrix_Q \n";
    printMatrix(fgldRQ_0, fgldRQ_1, row_num, col_num);
    int errNum = golden_check(fmatRQ_0, fmatRQ_1, fgldRQ_0, fgldRQ_1, row_num, col_num);

    return errNum;
}
#endif
