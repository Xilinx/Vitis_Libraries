/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/


//#include "matrix.hpp"
//#include "test_cholesky_cfloat.hpp"
#include "kernel_cholesky_cfloat.hpp"
//#include "dut_type.hpp"
#include "utils/x_matrix_utils.hpp"
#include "src/utils.hpp"
#include "src/matrix_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

//////////////////////////////////////////
// Tools
//////////////////////////////////////////
//
void writeComplextoFile(MATRIX_OUT_T* X, std::string filename0, unsigned M, unsigned N) {
    ofstream myfile0;
    myfile0.open(filename0.c_str());
    if (!myfile0) {
        std::cout << "[ERROR]: file " << filename0 << "could not be opened !" << std::endl;
        exit(1);
    }
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < M; i++) {
            MATRIX_OUT_T x;
            x = *(X+j*M + i);
            myfile0 << x.real() << " ";
            myfile0 << x.imag() << " " << std::endl;
        }
    }
    myfile0.close();
}


int main(int argc, char* argv[]) {
    unsigned allowed_ulp_mismatch = 0;
    unsigned print_precision = 2;
    unsigned int debug = 0;

    // Parse command line options
    // ====================================================================
    std::vector<std::string> args(argv + 1, argv + argc);
    for (std::vector<std::string>::iterator i = args.begin(); i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            std::cout << "Syntax: main.exe [-DIM <unsigned> ]" << std::endl;
            return 0;
        } else if (*i == "-max_ulp_mismatch_in_l") {
            allowed_ulp_mismatch = (unsigned)atol((*++i).c_str());
        } else if (*i == "-prec") {
            print_precision = (unsigned)atol((*++i).c_str());
        } else if (*i == "-debug") {
            debug = (unsigned)atol((*++i).c_str());
        } else {
            printf("Unknown command line option %s.  Try again\n", i->c_str());
            exit(1);
        }
    }
    std::cout << "DEBUG: DIM=" << DIM << std::endl;

    // Matrix arrays
    MATRIX_IN_T A[DIM][DIM];                    // The input array.  Cast from A_generated
    MATRIX_OUT_T L[DIM][DIM];                   // The L result from the DUT
    MATRIX_OUT_T L_expected[DIM][DIM];          // The L result from reference model
    MATRIX_OUT_T L_delta_vs_expected[DIM][DIM]; // The difference between the DUT's L and expected L

    int A_size = DIM * DIM;
    int L_size = DIM * DIM;

    MATRIX_IN_T* A_ptr = reinterpret_cast<MATRIX_IN_T*>(A);
    MATRIX_OUT_T* Lgld_ptr = reinterpret_cast<MATRIX_OUT_T*>(L_expected);

    std::string data_path = std::string(DATA_PATH);

    std::string file_A = data_path + "/A_" + std::to_string(DIM) + ".txt";
    std::string file_Lgld = data_path + "/Lgld_" + std::to_string(DIM) + ".txt";
    std::string file_Lout = data_path + "/Lout_" + std::to_string(DIM) + ".txt";
    std::cout << "File_A: " << file_A << std::endl;
    std::cout << "File_Lgld: " << file_Lgld << std::endl;
    std::cout << "File_Lout: " << file_Lout << std::endl;

    readTxt(file_A, A_ptr, A_size);
    readTxt(file_Lgld, Lgld_ptr, L_size);

    hls::stream<MATRIX_IN_T> matrixAStrm;
    hls::stream<MATRIX_OUT_T> matrixLStrm;
    for (int r = 0; r < DIM; r++) {
        for (int c = 0; c < DIM; c++) {
            matrixAStrm.write(A[r][c]);
        }
    }

    // Get Actual results
    // ====================================================================
    kernel_cholesky_cfloat_0(matrixAStrm, matrixLStrm);

    for (int c = 0; c < DIM; c++) {
        for (int r = 0; r < DIM; r++) {
            L[r][c] = matrixLStrm.read();
        }
    }

    writeComplextoFile((MATRIX_OUT_T*)L, file_Lout, DIM, DIM);

    bool mismatch_check = are_matrices_equal<DIM, DIM, MATRIX_OUT_T>(
        (MATRIX_OUT_T*)L, (MATRIX_OUT_T*)L_expected, allowed_ulp_mismatch, NULL);

    unsigned int pass_fail = 0; // Pass=0 Fail =1
    if(mismatch_check == false) pass_fail +=1; 

    std::cout << "pass_fail=" << pass_fail << ", mismatch_check=" << mismatch_check << std::endl;

    //debug = 2;
    if (( pass_fail > 0 && debug > 0) || debug > 1 ) {
        printf("  A=\n");
        xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(A, "   ", print_precision, 1);
        printf("  L_out=\n");
        xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(L, "   ", print_precision, 1);
        printf("  L_gld=\n");
        xf::solver::print_matrix<DIM, DIM, L_TYPE, xf::solver::NoTranspose>(L_expected, "   ", print_precision, 1);
    }
    if (pass_fail) {
        std::cout << "TB:Fail" << std::endl;
    } else {
        std::cout << "TB:Pass" << std::endl;
    }
    std::cout << "" << std::endl;

    return (pass_fail);

    }
