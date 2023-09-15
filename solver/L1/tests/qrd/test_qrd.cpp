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
#include "hw/qrd.hpp"
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

// IP + load buffer + output buffer to estimate resource
// pingpong load buffer will get best performance
extern "C" void qrd_ip_cfloat_top(hls::x_complex<Type>* dataA, hls::stream<hls::x_complex<Type> >& R_strm);

// just IP to estimate performance
extern "C" void qrd_cosim_top4(hls::x_complex<Type> dataA[NCU_test][M / NCU_test][N],
                               hls::stream<hls::x_complex<Type> >& R_strm);

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

template <typename T>
void writeFile(ComplexMatrix<T>& A, std::string filename0, std::string filename1) {
    ofstream myfile0;
    ofstream myfile1;
    myfile0.open(filename0);
    myfile1.open(filename1);
    for (int j = 0; j < A.N; j++) {
        for (int i = 0; i < A.M; i += 4) {
            myfile0 << A.elem(i, j).real() << endl;
            myfile0 << A.elem(i, j).imag() << endl;
            myfile0 << A.elem(i + 1, j).real() << endl;
            myfile0 << A.elem(i + 1, j).imag() << endl;

            myfile1 << A.elem(i + 2, j).real() << endl;
            myfile1 << A.elem(i + 2, j).imag() << endl;
            myfile1 << A.elem(i + 3, j).real() << endl;
            myfile1 << A.elem(i + 3, j).imag() << endl;
        }
    }
    myfile0.close();
    myfile1.close();
}

template <typename T, int M, int N>
void readFile(ComplexMatrix<T>& A, hls::x_complex<T>** A_hls, std::string filename0, std::string filename1) {
    ifstream myfile0;
    ifstream myfile1;
    myfile0.open(filename0);
    myfile1.open(filename1);
    float tmp;
    // printf("[HOST] input file %s\n", filename0.c_str());
    for (int j = 0; j < A.N; j++) {
        for (int i = 0; i < A.M; i += 4) {
            myfile0 >> tmp;
            A.elem(i, j).real(tmp);
            A_hls[i][j].real(tmp);
#ifdef DEBUG_QRD
            printf("real f=%f\n", A_hls[i][j].real());
#endif
            myfile0 >> tmp;
            A.elem(i, j).imag(tmp);
            A_hls[i][j].imag(tmp);
#ifdef DEBUG_QRD
            printf("imag f=%f\n", A_hls[i][j].imag());
#endif
            myfile0 >> tmp;
            A.elem(i + 1, j).real(tmp);
            A_hls[i + 1][j].real(tmp);
            // printf("real f=%f\n", tmp);
            myfile0 >> tmp;
            A.elem(i + 1, j).imag(tmp);
            A_hls[i + 1][j].imag(tmp);
            // printf("imag f=%f\n", tmp);

            myfile1 >> tmp;
            A.elem(i + 2, j).real(tmp);
            A_hls[i + 2][j].real(tmp);
            // printf("I=%d, f=%f\n", tmp);
            myfile1 >> tmp;
            A.elem(i + 2, j).imag(tmp);
            A_hls[i + 2][j].imag(tmp);
            myfile1 >> tmp;
            if (A.M > 3) {
                A.elem(i + 3, j).real(tmp);
                A_hls[i + 3][j].real(tmp);
            }
            myfile1 >> tmp;
            if (A.M > 3) {
                A.elem(i + 3, j).imag(tmp);
                A_hls[i + 3][j].imag(tmp);
            }
        }
    }
    myfile0.close();
    myfile1.close();
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
    printf("[HOST] input file name 0: %s\n", in0.c_str());
    printf("[HOST] input file name 1: %s\n", in1.c_str());

    ComplexMatrix<Type> A(M, N);
    hls::x_complex<Type> R_hls[N][N];

    hls::x_complex<Type>** A_hls;
    hls::x_complex<Type>** Q_hls;
    A_hls = new hls::x_complex<Type>*[M];
    Q_hls = new hls::x_complex<Type>*[M];
    for (int i = 0; i < M; ++i) {
        A_hls[i] = new hls::x_complex<Type>[ N ];
        Q_hls[i] = new hls::x_complex<Type>[ N ];
    }

    readFile<Type, M, N>(A, A_hls, in0, in1);
    // xf::solver::print_matrix<M, N, hls::x_complex<Type>, xf::solver::NoTranspose>(A_hls, " ", 6);
    // ComplexMatrix<Type> B(M, N);
    // for (int i = 0; i < M; i++) {
    //     for (int j = 0; j < N; j++) {
    //         A.elem(i, j).real(rand() % 100 - 49.5);
    //         A.elem(i, j).imag(rand() % 100 - 49.5);
    //     }
    // }

    // writeFile<Type>(A, "A0.txt", "A1.txt");

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

    hls::x_complex<Type>* dataA = aligned_alloc<hls::x_complex<Type> >(M * N);
    hls::x_complex<Type>* dataR = aligned_alloc<hls::x_complex<Type> >(N * N);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            dataA[i * N + j] = A_hls[i][j];
            if (i < N) dataR[i * N + j] = 0;
        }
    }

    hls::stream<hls::x_complex<Type> > R_strm("R_strm");

    // for axi strm top
    // qrd_strm_top(dataA, R_strm);
    // qrd_cosim(dataA, R_strm);
    // qrd_cosim2(dataA, R_strm);
    qrd_ip_cfloat_top(dataA, R_strm);

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
    ComplexMatrix<Type> Q(M, M);
    ComplexMatrix<Type> R(M, N);

    A.gram_schmidt_model(Q, R, A, TEST_Mode::FINAL_OPT);
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

    for (int i = 0; i < A.M; i++) {
        for (int j = 0; j < A.N; j++) {
            int64_t ulp_r = calcULP(Q_hls[i][j].real(), Q.elem(i, j).real());
            int64_t ulp_i = calcULP(Q_hls[i][j].imag(), Q.elem(i, j).imag());
            if (ulp_r > 1 || ulp_i > 1) {
                errQ++;
                int diff = std::max(ulp_r, ulp_i);
                max_diffQ = std::max(diff, max_diffQ);
                total_diffQ += ulp_r + ulp_i;

                if (Q.elem(i, j).real() != 0.0f) {
                    real_relative_err = (Q_hls[i][j].real() - Q.elem(i, j).real()) / Q.elem(i, j).real();
                }
                if (Q.elem(i, j).imag() != 0.0f) {
                    imag_relative_err = (Q_hls[i][j].imag() - Q.elem(i, j).imag()) / Q.elem(i, j).imag();
                }
                max_real_relative_errQ = std::max(std::fabs(real_relative_err), max_real_relative_errQ);
                max_imag_relative_errQ = std::max(std::fabs(imag_relative_err), max_imag_relative_errQ);

                class_errQ += classify_compare(Q_hls[i][j].real(), Q.elem(i, j).real(), classify, accuracy);
                class_errQ += classify_compare(Q_hls[i][j].imag(), Q.elem(i, j).imag(), classify, accuracy);
                class_errQ4 += classify_compare(Q_hls[i][j].real(), Q.elem(i, j).real(), classify, accuracy4);
                class_errQ4 += classify_compare(Q_hls[i][j].imag(), Q.elem(i, j).imag(), classify, accuracy4);
                class_errQ5 += classify_compare(Q_hls[i][j].real(), Q.elem(i, j).real(), classify, accuracy5);
                class_errQ5 += classify_compare(Q_hls[i][j].imag(), Q.elem(i, j).imag(), classify, accuracy5);
#ifdef DEBUG_QRD
                printf("real_relative_err %f, imag_relative_err %f\n", real_relative_err, imag_relative_err);
                printf("Q(%d, %d) ulp diff is %d real, %d imag, Qhls is %f+%fi, Qgld is %f+%fi\n", i, j, ulp_r, ulp_i,
                       Q_hls[i][j].real(), Q_hls[i][j].imag(), Q.elem(i, j).real(), Q.elem(i, j).imag());
                printf("hls : \n");
                dumpfloat(Q_hls[i][j].real());
                dumpfloat(Q_hls[i][j].imag());
                printf("golden : \n");
                dumpfloat(Q.elem(i, j).real());
                dumpfloat(Q.elem(i, j).imag());
                printf("\n");
#endif
                // printf("%f %f\n", Q.elem(i, j).real(), real_relative_err);
            }
        }
    }

    // printf("=============== Compare R matrix    =================== \n\n");
    int errR = 0;
    int max_diffR = 0;
    uint64_t total_diffR = 0;
    float max_real_relative_errR = 0.0;
    float max_imag_relative_errR = 0.0;
    for (int i = 0; i < A.N; i++) {
        for (int j = 0; j < A.N; j++) {
            int64_t ulp_r = calcULP(R_hls[i][j].real(), R.elem(i, j).real());
            int64_t ulp_i = calcULP(R_hls[i][j].imag(), R.elem(i, j).imag());
            if (ulp_r > 1 || ulp_i > 1) {
                errR++;
                int diff = std::max(ulp_r, ulp_i);
                max_diffR = std::max(diff, max_diffR);
                total_diffR += ulp_r + ulp_i;

                if (R.elem(i, j).real() > 0.00001f || R.elem(i, j).real() < -0.00001f) {
                    real_relative_err = (R_hls[i][j].real() - R.elem(i, j).real()) / R.elem(i, j).real();
                }
                if (R.elem(i, j).imag() > 0.00001f || R.elem(i, j).imag() < -0.00001f) {
                    imag_relative_err = (R_hls[i][j].imag() - R.elem(i, j).imag()) / R.elem(i, j).imag();
                }
                max_real_relative_errR = std::max(std::fabs(real_relative_err), max_real_relative_errR);
                max_imag_relative_errR = std::max(std::fabs(imag_relative_err), max_imag_relative_errR);

                class_errR += classify_compare(R_hls[i][j].real(), R.elem(i, j).real(), 1.0, 0.001);
                class_errR += classify_compare(R_hls[i][j].imag(), R.elem(i, j).imag(), 1.0, 0.001);
                class_errR4 += classify_compare(R_hls[i][j].real(), R.elem(i, j).real(), classify, accuracy4);
                class_errR4 += classify_compare(R_hls[i][j].imag(), R.elem(i, j).imag(), classify, accuracy4);
                class_errR5 += classify_compare(R_hls[i][j].real(), R.elem(i, j).real(), classify, accuracy5);
                class_errR5 += classify_compare(R_hls[i][j].imag(), R.elem(i, j).imag(), classify, accuracy5);

// printf("%f %f\n", R.elem(i, j).real(), real_relative_err);
#ifdef DEBUG_QRD
                printf("real_relative_err %f, imag_relative_err %f\n", real_relative_err, imag_relative_err);

                // printf("(%f, %f)\n", abs_hls, abs_hls);
                printf("R(%d, %d), ulp diff is %d real, %d imag, Rhls is %f+%fi, Rgld is %f+%fi\n", i, j, ulp_r, ulp_i,
                       R_hls[i][j].real(), R_hls[i][j].imag(), R.elem(i, j).real(), R.elem(i, j).imag());
                printf("hls : \n");
                dumpfloat(R_hls[i][j].real());
                dumpfloat(R_hls[i][j].imag());
                printf("golden : \n");
                dumpfloat(R.elem(i, j).real());
                dumpfloat(R.elem(i, j).imag());
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
        delete[] Q_hls[i];
    }
    delete[] A_hls;
    delete[] Q_hls;

    free(dataA);
    free(dataR);

    return (class_errQ + class_errR);
}
