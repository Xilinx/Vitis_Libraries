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
#include <sstream>
#include <cstring>
#include <fstream>
#include <iostream>
#include <complex>

#include "aie_graph.h"

using namespace adf;

TopGraph mygraph;

#if defined(__AIESIM__) || defined(__X86SIM__)
#include <iostream>
#include <fstream>
using namespace std;

void printMatrix(std::string file0, int M, int N) {
    ifstream fd0;
    char c = 'T';
    string str0;
    fd0.open(file0, ios::in);
    std::complex<float> R[M][N];
    std::complex<float> Q[M][M];
    for (unsigned int j = 0; j < N; j++) {
        for (unsigned int i = 0; i < M; i += 2) {
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
        }
    }
    for (unsigned int j = 0; j < M; j++) {
        for (unsigned int i = 0; i < M; i += 2) {
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

int compare(float out, float gld) {
    if (1 == isnan(out)) {
        std::cout << "out is abnorman, out=" << out << std::endl;
        return 1;
    } else {
        if (abs(gld) > 1.0) {
            if ((abs(out - gld) / abs(gld)) > 0.01) {
                return 1;
            }
        } else {
            if (abs(out - gld) > 0.05) {
                return 1;
            }
        }
    }
    return 0;
}

int golden_check(std::string golden_file, std::string output_file, int row, int col) {
    int numR = row * col * 2;
    int numQ = row * row * 2;
    int checked = 0;
    ifstream f_gld, f_out;
    f_gld.open(golden_file, ios::in);
    f_out.open(output_file, ios::in);

    float outR, outI;
    float gldR, gldI;
    char c = 'T';
    string str_gld, str_out;
    string s_gld, s_out;

    for (int j = 0; j < numR; j += 4) {
        getline(f_gld, str_gld, '\n');
        std::stringstream ss_gld(str_gld);

        while (getline(f_out, str_out, '\n')) {
            if (str_out.front() != c) {
                break;
            }
        }
        std::stringstream ss_out(str_out);

        for (int i = 0; i < 2; i++) {
            getline(ss_gld, s_gld, ' ');
            gldR = atof(s_gld.c_str());
            getline(ss_gld, s_gld, ' ');
            gldI = atof(s_gld.c_str());
            getline(ss_out, s_out, ' ');
            outR = atof(s_out.c_str());
            getline(ss_out, s_out, ' ');
            outI = atof(s_out.c_str());
            if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                checked++;
                std::cout << "Golden check: matR mis-matched, error_num=" << checked << ", (" << (i + j / 2) % row
                          << ", " << (i + j / 2) / row << "), gld=(" << gldR << ", " << gldI << "), out=(" << outR
                          << ", " << outI << ")" << std::endl;
            }
        }
    }
    for (int j = 0; j < numQ; j += 4) {
        getline(f_gld, str_gld, '\n');
        std::stringstream ss_gld(str_gld);

        while (getline(f_out, str_out, '\n')) {
            if (str_out.front() != c) {
                break;
            }
        }
        std::stringstream ss_out(str_out);

        for (int i = 0; i < 2; i++) {
            getline(ss_gld, s_gld, ' ');
            gldR = atof(s_gld.c_str());
            getline(ss_gld, s_gld, ' ');
            gldI = atof(s_gld.c_str());
            getline(ss_out, s_out, ' ');
            outR = atof(s_out.c_str());
            getline(ss_out, s_out, ' ');
            outI = atof(s_out.c_str());
            if ((0 != compare(outR, gldR)) || (0 != compare(outI, gldI))) {
                checked++;
                std::cout << "Golden check: matQ mis-matched, error_num=" << checked << ", (" << (i + j / 2) % row
                          << ", " << (i + j / 2) / row << "), gld=(" << gldR << ", " << gldI << "), out=(" << outR
                          << ", " << outI << ")" << std::endl;
            }
        }
    }
    return checked;
}

int main(void) {
    adf::return_code ret;
    mygraph.init();
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

    // compare output with golden
    int num = ROW * COL * 2 + ROW * ROW * 2;
    int errs = 0;
    // std::cout << "Input Matrix_A && Matrix_U \n";
    // printMatrix(fin, ROW, COL);
    // std::cout << "Output Matrix_R && Matrix_Q \n";
    // printMatrix(fout, ROW, COL);
    // std::cout << "Golden Golden_R && Golden_Q \n";
    // printMatrix(fgld, ROW, COL);
    errs = golden_check(fgld, fout, ROW, COL);
    if (errs != 0) {
        std::cout << "Test failed! errs=" << errs << "\n";
    } else {
        std::cout << "Test passed! \n";
    }
    return (errs);
}

#endif
