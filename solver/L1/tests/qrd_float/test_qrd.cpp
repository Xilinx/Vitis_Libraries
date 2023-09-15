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

#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include "matrix.hpp"
#include "hw/qrdfloat.hpp"
using namespace std;

//////////////////////////////////////////
// top setting
//////////////////////////////////////////
// A matrix
typedef float Type;
const int M = QRF_A_ROWS; // rows
const int N = QRF_A_COLS; // columns

#if (QRF_A_ROWS <= 1024 && QRF_A_ROWS > 128)
const int PowNCU_test = 5;
#elif (QRF_A_ROWS <= 128 && QRF_A_ROWS > 8)
const int PowNCU_test = 2;
#elif (QRF_A_ROWS <= 8 && QRF_A_ROWS > 4)
const int PowNCU_test = 1;
//#else
// std::cout<<"ERROR input support is max 1024 rows, min 2 rows, [2, 3 ,4, ... 1023, 1024]"<< std::endl;
#endif
const int NCU_test = 1 << PowNCU_test;

// just IP to estimate performance
extern "C" void qrd_top(Type dataA[NCU_test][M / NCU_test][N], hls::stream<Type>& R_strm);

// IP + load buffer + output buffer to estimate resource
// pingpong load buffer will get best performance
extern "C" void qrd_ip_top(Type* dataA, hls::stream<Type>& R_strm);

//////////////////////////////////////////
// Tools
//////////////////////////////////////////
class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

template <typename T, int M, int N>
void writeFile(T** A, std::string filename0) {
    ofstream myfile0;
    myfile0.open(filename0);
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < M; i++) {
            myfile0 << A[i][j] << endl;
        }
    }
    myfile0.close();
}

template <typename T, int M, int N>
void readFile(T** A_hls, std::string filename0, std::string filename1) {
    ifstream myfile0;
    myfile0.open(filename0);
    float tmp;
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < M; i += 1) {
            myfile0 >> tmp;
            A_hls[i][j] = (tmp);
        }
    }
    myfile0.close();
}

using namespace std;

int classify_compare(float out, float gld, float classify, float accuracy) {
    if (1 == isnan(out)) {
        std::cout << "out is abnorman, out=" << out << std::endl;
        return 1;
    } else {
        if (abs(gld) > classify) {
            if ((abs(out - gld) / abs(gld)) > accuracy) {
                return 1;
            }
        } else {
            if (abs(out - gld) > accuracy) {
                return 1;
            }
        }
    }
    return 0;
}

// for ULP comparison
static void dumpfloat(float da) {
    class fp_struct<float> ds(da);
    std::cout << ds << "\n";
    float sigd = ds.sig.to_int();
    sigd = 1.0 + sigd / 8388608.0;
    sigd = sigd * (pow(2, ds.expv()));

    printf("single: %f (sign: %d, exp: %d(%x), sig: %e(%x))\n", da, ds.sign.to_int(), ds.expv(), ds.exp.to_int(), sigd,
           ds.sig.to_int());
}

static uint64_t calcULP(float A, float B) {
    class fp_struct<float> d1s(A), d2s(B);

    // calcULPs
    if (std::isnan(A) && std::isnan(B)) return 0;
    int32_t aInt = d1s.data();
    // Make aInt lexicographically ordered as a twos-complement int
    if (std::signbit(A)) aInt = 0x80000000 - aInt;
    // Make bInt lexicographically ordered as a twos-complement int
    int32_t bInt = d2s.data();
    if (std::signbit(B)) bInt = 0x80000000 - bInt;
    int32_t intDiff = abs(aInt - bInt);
    return intDiff;
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
#if _WIN32
    ptr = (T*)malloc(num * sizeof(T));
    if (num == 0) {
#else
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) {
#endif
        throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(ptr);
}

//////////////////////////////////////////
// gen golden
//////////////////////////////////////////
template <typename T, int M, int N>
T inner_product(T** lop, T** rop, unsigned int lop_col, unsigned int rop_col) {
    if (M < N) std::cout << "SMOKING TEST NOTE: rows number must match cols!" << std::endl;
    T result = 0;
    for (int i = 0; i < M; i++) {
        result += lop[i][lop_col] * rop[i][rop_col];
#ifdef DEBUG_QRD
        std::cout << lop[i][lop_col] << " det with" << rop[i][rop_col] << " is " << result << std::endl;
#endif
    }
    return result;
}

template <typename T, int M, int N>
void gram_schmidt_opt2(T** A, T** Q, T** R) {
    if (M < N) std::cout << "SMOKING TEST NOTE: Q or R size must match (Q.M == M && Q.N == N && M>=N)!" << std::endl;

    T proj[M][N];
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            Q[i][j] = 0;
            proj[i][j] = 0;
        }
        for (int j = 0; j < N; j++) {
            R[i][j] = 0;
        }
    }

    // update dot_self
    T norm_2 = inner_product<T, M, N>(A, A, 0, 0);
    T rnorm_1 = 1.0 / std::sqrt(norm_2);
    R[0][0] = std::sqrt(norm_2);
    // printf("[SMOKING TEST] check dot : %7f\n", norm_2);

    // update proj
    for (int j = 1; j < N; j++) {
        R[0][j] = inner_product<T, M, N>(A, A, 0, j); // dot <ai, aj> tmp save into R
        proj[0][j] = R[0][j] / norm_2;
        R[0][j] = R[0][j] * rnorm_1;
    }

    for (int k = 0; k < N - 1; k++) {
        // calculate vector Q
        for (int i = 0; i < M; i++) {
            Q[i][k] = A[i][k] * rnorm_1;
        }

        // aj = aj - proj * ai
        for (int j = k + 1; j < N; j++) {
            for (int i = 0; i < M; i++) {
                A[i][j] = A[i][j] - (proj[k][j]) * A[i][k];
#ifdef DEBUG_QRD
                std::cout << "proj(" << k << "," << j << ")= " << proj[k][j] << ", " << A[i][j] << std::endl;
#endif
            }
            if (j == k + 1) { // update dot_self
                norm_2 = inner_product<T, M, N>(A, A, k + 1, k + 1);
                rnorm_1 = 1.0 / std::sqrt(norm_2);
                R[k + 1][k + 1] = std::sqrt(norm_2);
                // printf("[SMOKING TEST] check dot : %7f\n", norm_2);
            } else {                                                  // update proj
                R[k + 1][j] = inner_product<T, M, N>(A, A, k + 1, j); // dot <ai, aj> tmp save into R
                proj[k + 1][j] = R[k + 1][j] / norm_2;
                R[k + 1][j] = R[k + 1][j] * rnorm_1;
            }
        }
    }

    // for the last line of Q
    for (int i = 0; i < M; i++) {
        Q[i][N - 1] = A[i][N - 1] * rnorm_1;
    }
}

int main(int argc, char* argv[]) {
    using namespace std;

    //////////////////////////////////////////
    // 1.input cmd parser
    //////////////////////////////////////////
    ArgParser parser(argc, (const char**)argv);

    std::string data_path;
    if (!parser.getCmdOption("-data", data_path)) {
        std::cout << "ERROR:[-data] data path is not set!\n";
        return 1;
    }
    std::string in0 = data_path + "/A0_" + to_string(M) + "_" + to_string(N) + ".txt";
    std::string in1 = data_path + "/A1_" + to_string(M) + "_" + to_string(N) + ".txt";
    printf("M=%d, N=%d\n", M, N);
    printf("[HOST] input file name : %s\n", in0.c_str());

    Type R_hls[N][N], Q_hls[M][N];
    Type** A_hls = new Type*[M];
    // Type** Q_hls = new Type*[M];
    Type** A = new Type*[M];
    Type** Q = new Type*[M];
    Type** R = new Type*[M];
    for (int i = 0; i < M; ++i) {
        A_hls[i] = new Type[N];
        // Q_hls[i] = new Type[ N ];
        A[i] = new Type[N];
        Q[i] = new Type[N];
        R[i] = new Type[N];
    }

    readFile<Type, M, N>(A_hls, in0, in1);
    // xf::solver::print_matrix<M, N, Type, xf::solver::NoTranspose>(A_hls, " ", 6);
    // Type B(M, N);
    // for (int i = 0; i < M; i++) {
    //     for (int j = 0; j < N; j++) {
    //         A[i][j].real(rand() % 100 - 49.5);
    //         A[i][j].imag(rand() % 100 - 49.5);
    //     }
    // }

    // writeFile<Type>(A, "A0.txt", "A1.txt");
    // writeFile<Type, M, N>(A_hls, "A0.txt");

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            Q_hls[i][j] = (0);
            if (i < N) R_hls[i][j] = (0);
        }
    }

//////////////////////////////////////////
// 2.test cosim top or ip top
//////////////////////////////////////////
#ifdef DEBUG_QRD
    printf("=============== HLS design details =================== \n\n");
#endif

    Type* dataA = aligned_alloc<Type>(M * N);
    Type* dataR = aligned_alloc<Type>(N * N);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            dataA[i * N + j] = A_hls[i][j];
            if (i < N) dataR[i * N + j] = 0;
        }
    }

    hls::stream<Type> R_strm("R_strm");
    // for ip top or cosim top
    qrd_ip_top(dataA, R_strm);
    // qrd_top(dataA, R_strm);

    //////////////////////////////////////////
    // 3.get hls output
    //////////////////////////////////////////
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < M; i++) {
            Q_hls[i][j] = dataA[i * N + j];
        }
    }

    for (int k = 0; k < N; k++) {
        for (int j = k; j < N; j++) {
            R_hls[k][j] = R_strm.read();
        }
    }

//////////////////////////////////////////
// 4.gen golden and compare
//////////////////////////////////////////
#ifdef DEBUG_QRD
    printf("=============== Golden details     =================== \n\n");
#endif
    gram_schmidt_opt2<Type, M, N>(A_hls, Q, R); // todo
    // writeGLD<Type>(Q, R, "Gld0.txt", "Gld1.txt");

    // printf("=============== Compare Q matrix   =================== \n\n");
    const int max_ulp = 1;
    int errQ = 0;
    int max_diffQ = 0;
    uint64_t total_diffQ = 0;
    // relative err 1
    float real_diff = 0.0;
    float imag_diff = 0.0;
    float relative_err = 0.0;
    float abs_hls = 0.0;
    float abs_glb = 0.0;
    float max_relative_err = 0.0;
    // relative err 2
    float real_relative_err = 0.0;
    float imag_relative_err = 0.0;
    float max_real_relative_errQ = 0.0;
    float max_imag_relative_errQ = 0.0;
    // classify errQ
    int class_errQ = 0;
    int class_errR = 0;
    float classify = 1.0;
    float accuracy = 0.001;
    float accuracy4 = 0.00001;
    float accuracy5 = 0.000001;
    int class_errQ4 = 0;
    int class_errR4 = 0;
    int class_errQ5 = 0;
    int class_errR5 = 0;

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int64_t ulp_r = calcULP(Q_hls[i][j], Q[i][j]);
            int64_t ulp_i = calcULP(Q_hls[i][j], Q[i][j]);
            if (ulp_r > 1 || ulp_i > 1) {
                errQ++;
                int diff = std::max(ulp_r, ulp_i);
                max_diffQ = std::max(diff, max_diffQ);
                total_diffQ += ulp_r + ulp_i;

                if (Q[i][j] != 0.0f) {
                    real_relative_err = (Q_hls[i][j] - Q[i][j]) / Q[i][j];
                }
                if (Q[i][j] != 0.0f) {
                    imag_relative_err = (Q_hls[i][j] - Q[i][j]) / Q[i][j];
                }
                max_real_relative_errQ = std::max(std::fabs(real_relative_err), max_real_relative_errQ);
                max_imag_relative_errQ = std::max(std::fabs(imag_relative_err), max_imag_relative_errQ);

                class_errQ += classify_compare(Q_hls[i][j], Q[i][j], classify, accuracy);
                class_errQ += classify_compare(Q_hls[i][j], Q[i][j], classify, accuracy);
                class_errQ4 += classify_compare(Q_hls[i][j], Q[i][j], classify, accuracy4);
                class_errQ4 += classify_compare(Q_hls[i][j], Q[i][j], classify, accuracy4);
                class_errQ5 += classify_compare(Q_hls[i][j], Q[i][j], classify, accuracy5);
                class_errQ5 += classify_compare(Q_hls[i][j], Q[i][j], classify, accuracy5);
#ifdef DEBUG_QRD
                printf("real_relative_err %f, imag_relative_err %f\n", real_relative_err, imag_relative_err);
                printf("Q(%d, %d) ulp diff is %d real, %d imag, Qhls is %f+%fi, Qgld is %f+%fi\n", i, j, ulp_r, ulp_i,
                       Q_hls[i][j], Q_hls[i][j], Q[i][j], Q[i][j]);
                printf("hls : \n");
                dumpfloat(Q_hls[i][j]);
                dumpfloat(Q_hls[i][j]);
                printf("golden : \n");
                dumpfloat(Q[i][j]);
                dumpfloat(Q[i][j]);
                printf("\n");
#endif
                // printf("%f %f\n", Q[i][j], real_relative_err);
            }
        }
    }

    // printf("=============== Compare R matrix    =================== \n\n");
    int errR = 0;
    int max_diffR = 0;
    uint64_t total_diffR = 0;
    float max_real_relative_errR = 0.0;
    float max_imag_relative_errR = 0.0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int64_t ulp_r = calcULP(R_hls[i][j], R[i][j]);
            int64_t ulp_i = calcULP(R_hls[i][j], R[i][j]);
            if (ulp_r > 1 || ulp_i > 1) {
                errR++;
                int diff = std::max(ulp_r, ulp_i);
                max_diffR = std::max(diff, max_diffR);
                total_diffR += ulp_r + ulp_i;

                if (R[i][j] > 0.00001f || R[i][j] < -0.00001f) {
                    real_relative_err = (R_hls[i][j] - R[i][j]) / R[i][j];
                }
                if (R[i][j] > 0.00001f || R[i][j] < -0.00001f) {
                    imag_relative_err = (R_hls[i][j] - R[i][j]) / R[i][j];
                }
                max_real_relative_errR = std::max(std::fabs(real_relative_err), max_real_relative_errR);
                max_imag_relative_errR = std::max(std::fabs(imag_relative_err), max_imag_relative_errR);

                class_errR += classify_compare(R_hls[i][j], R[i][j], 1.0, 0.001);
                class_errR += classify_compare(R_hls[i][j], R[i][j], 1.0, 0.001);
                class_errR4 += classify_compare(R_hls[i][j], R[i][j], classify, accuracy4);
                class_errR4 += classify_compare(R_hls[i][j], R[i][j], classify, accuracy4);
                class_errR5 += classify_compare(R_hls[i][j], R[i][j], classify, accuracy5);
                class_errR5 += classify_compare(R_hls[i][j], R[i][j], classify, accuracy5);

// printf("%f %f\n", R[i][j], real_relative_err);
#ifdef DEBUG_QRD
                printf("real_relative_err %f, imag_relative_err %f\n", real_relative_err, imag_relative_err);

                // printf("(%f, %f)\n", abs_hls, abs_hls);
                printf("R(%d, %d), ulp diff is %d real, %d imag, Rhls is %f+%fi, Rgld is %f+%fi\n", i, j, ulp_r, ulp_i,
                       R_hls[i][j], R_hls[i][j], R[i][j], R[i][j]);
                printf("hls : \n");
                dumpfloat(R_hls[i][j]);
                dumpfloat(R_hls[i][j]);
                printf("golden : \n");
                dumpfloat(R[i][j]);
                dumpfloat(R[i][j]);
                printf("\n");
#endif
            }
        }
    }

    printf("=============== Result classify err =================== \n\n");
    printf("Summary : %d\t errors in Q matrix, when classify is %f and accuracy is %f\n", class_errQ, classify,
           accuracy);
    printf("Summary : %d\t errors in R matrix, when classify is %f and accuracy is %f\n", class_errR, classify,
           accuracy);
    printf("=============== Compare done %d errs =================== \n\n", (class_errQ + class_errR));

    for (int i = 0; i < M; ++i) {
        delete[] A_hls[i];
        // delete[] Q_hls[i];
        delete[] A[i];
        delete[] Q[i];
        delete[] R[i];
    }
    delete[] A_hls;
    // delete[] Q_hls;
    delete[] A;
    delete[] Q;
    delete[] R;

    free(dataA);
    free(dataR);

    return (class_errQ + class_errR);
}
