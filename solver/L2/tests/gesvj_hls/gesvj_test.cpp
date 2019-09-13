#ifndef __SYNTHESIS__
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include "matrixUtility.hpp"
#endif

#include "xf_solver_L2.hpp"

#define M 20
#define N 20
#define MCU 2
#define NCU 2

typedef double DT;

void gesvj_wrapper(int rows, int cols, DT* input_mat, DT* output_U, DT* output_S, DT* output_V) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 64 num_write_outstanding = \
    64 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem0 port = input_mat depth = 4*4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 64 num_write_outstanding = \
    64 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem1 port = output_U depth = 4*4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 64 num_write_outstanding = \
    64 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem2 port = output_S depth = 4
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_read_outstanding = 64 num_write_outstanding = \
    64 max_read_burst_length = 64 max_write_burst_length = 64 bundle = gmem3 port = output_V depth = 4*4

// clang-format on
#pragma HLS INTERFACE s_axilite port = input_mat bundle = control
#pragma HLS INTERFACE s_axilite port = output_U bundle = control
#pragma HLS INTERFACE s_axilite port = output_S bundle = control
#pragma HLS INTERFACE s_axilite port = output_V bundle = control
#pragma HLS INTERFACE s_axilite port = rows bundle = control
#pragma HLS INTERFACE s_axilite port = cols bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::solver::gesvj<DT, M, N, MCU, NCU>(rows, cols, input_mat, output_U, output_S, output_V);
}

#ifndef __SYNTHESIS__
int main(int argc, char* argv[]) {
    bool run_csim = true;
    if (argc >= 2) {
        run_csim = std::stoi(argv[1]);
        if (run_csim) std::cout << "run csim for funcion verify\n";
    }

    // Vectors for input matrix row count and column count
    // First add special cases
    std::vector<int> NR = {4, 0, 1, 0, 5, 1, 3, 36, 56, 516, 516};
    std::vector<int> NC = {4, 0, 1, 5, 0, 3, 1, 37, 1, 512, 516};

    // Then add sizes 1x1, 2x2, ...
    for (int i = 1; i <= std::min(M, N); ++i) {
        NR.push_back(i);
        NC.push_back(i);
    }

    bool allValid = true;

    int runNm;
    if (run_csim) {
        runNm = NR.size();
    } else {
        runNm = 1;
    }
    // generate matrix and test
    for (int idx = 0; idx != runNm; ++idx) {
        int kRows = NR[idx];
        int kCols = NC[idx];
        int kSize = (kRows > kCols) ? kCols : kRows;

        // Skip some big matricies to shorten test duration
        if (kRows > 30 && kCols > 30) {
            continue;
        }
        std::cout << "Matrix Size : " << kRows << "x" << kRows << "...\n";

        // Handle invalid settings and input sizes
        if (M <= 0 || N <= 0) {
            continue;
        }
        if (kRows <= 0 || kRows > M) {
            continue;
        }
        if (kCols <= 0 || kCols > N) {
            continue;
        }
        DT** genA = new DT*[kRows];
        for (int i = 0; i < kRows; i++) {
            genA[i] = new DT[kCols];
        }
        matGen<DT>(kRows, kCols, 113, genA);

        DT* A = new DT[kRows * kCols];
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kCols; j++) {
                A[i * kCols + j] = genA[i][j];
            }
        }

        DT* U = new DT[kRows * kRows];
        DT* V = new DT[kCols * kCols];
        DT* S = new DT[kSize];

        gesvj_wrapper(kRows, kCols, A, U, S, V);

        // used for verify USV=A
        DT** UU;
        UU = new DT*[kRows];
        for (int i = 0; i < kRows; i++) {
            UU[i] = new DT[kRows];
        }
        DT** VV;
        VV = new DT*[kCols];
        for (int i = 0; i < kCols; i++) {
            VV[i] = new DT[kCols];
        }

        //================print U, S, V matrix=====================
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kRows; j++) {
                UU[i][j] = U[i * kRows + j];
            }
        }
        for (int i = 0; i < kCols; i++) {
            for (int j = 0; j < kCols; j++) {
                VV[i][j] = V[i * kCols + j];
            }
        }
        //  calculate US
        DT** UxS;
        UxS = new DT*[kRows];
        for (int i = 0; i < kRows; i++) {
            UxS[i] = new DT[kCols];
        }
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kCols; j++) {
                UxS[i][j] = 0;
            }
        }
        DT** Res;
        Res = new DT*[kRows];
        for (int i = 0; i < kRows; i++) {
            Res[i] = new DT[kCols];
        }
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kCols; j++) {
                Res[i][j] = 0;
            }
        }

        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kCols; j++) {
                for (int k = 0; k < kCols; k++) {
                    DT tmp = 0;
                    if (j == k) {
                        tmp = S[j];
                    } else {
                        tmp = 0;
                    }
                    UxS[i][j] += UU[i][k] * tmp;
                }
            }
        }

        // calculate USV
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kCols; j++) {
                for (int k = 0; k < kCols; k++) {
                    Res[i][j] += UxS[i][k] * VV[j][k]; // multiply VT
                }
            }
        }
        DT diff_max = 0;
        for (int i = 0; i < kRows; i++) {
            for (int j = 0; j < kCols; j++) {
                DT diff_tmp = std::abs(Res[i][j] - A[i * kCols + j]);
                if (diff_max < diff_tmp) {
                    diff_max = diff_tmp;
                }
            }
        }
        DT threshold;
        if (sizeof(DT) == sizeof(double)) {
            threshold = 0.0001;
        } else if (sizeof(DT) == sizeof(float)) {
            threshold = 0.001;
        }
        if (diff_max > threshold) {
            std::cout << "Input matrix : " << kRows << "x" << kCols << " FAIL\n";
            allValid = false;
        }

        //======= delete matrix A, S (MxN) ======
        delete[] A;
        delete[] S;
        delete[] U;
        delete[] V;

        // delete matrix U (kRows x kRows)
        for (int i = 0; i < kRows; i++) {
            delete[] UU[i];
        }
        delete[] UU;

        // delete matrix genA (kRows x kCols)
        for (int i = 0; i < kRows; i++) {
            delete[] genA[i];
        }
        delete[] genA;
        // delete matrix V (kCols x kCols)
        for (int i = 0; i < kCols; i++) {
            delete[] VV[i];
        }
        delete[] VV;
        for (int i = 0; i < kRows; i++) {
            delete[] UxS[i];
            delete[] Res[i];
        }
        delete[] UxS;
        delete[] Res;
    }
    std::cout << "-------------- " << std::endl;
    if ((!allValid)) {
        std::cout << "result false" << std::endl;
        std::cout << "-------------- " << std::endl;
        return -1;
    } else {
        std::cout << "result correct" << std::endl;
        std::cout << "-------------- " << std::endl;
        return 0;
    }
    std::cout << "-------------- " << std::endl;

    return 0;
}
#endif
