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

#ifndef _HERMITIAN_MATRIX_HPP_
#define _HERMITIAN_MATRIX_HPP_
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <complex>
#include <cstdlib>
#include <ctime>
#include <random>
#include <cmath>

template <typename T>
class HermitianMatrix {
   public:
    T* ptr;
    unsigned int N; // num_rows/num_columns
    unsigned int N_512_aligned;
    static const unsigned int ALW = (512 / 8);

   public:
    HermitianMatrix(unsigned int rows) {
        N = rows;
        N_512_aligned = (N * sizeof(T) + ALW - 1) / ALW * ALW / sizeof(T);
        ptr = new T[N_512_aligned * N];
    }

    HermitianMatrix(HermitianMatrix<T>& hdl) {
        std::cout << "copy constructor" << std::endl;
        N = hdl.N;
        N_512_aligned = hdl.N_512_aligned;
        ptr = new T[N_512_aligned * N];
        std::memcpy(ptr, hdl.ptr, sizeof(T) * N_512_aligned * N);
    }

    HermitianMatrix& operator=(const HermitianMatrix<T>& hdl) {
        std::cout << "copy assign" << std::endl;
        delete[] ptr;

        N = hdl.N;
        N_512_aligned = hdl.N_512_aligned;
        ptr = new T[N_512_aligned * N];
        std::memcpy(ptr, hdl.ptr, sizeof(T) * N_512_aligned * N);
    }

    ~HermitianMatrix() { delete[] ptr; }

    T& elem(unsigned int row_idx, unsigned int column_idx) { return ptr[row_idx * N_512_aligned + column_idx]; }

    void print() {
        std::cout << "HermitianMatrix[" << N << " x " << N << "], N_512_aligned = " << N_512_aligned << std::endl;
        for (unsigned int i = 0; i < N; i++) {
            std::cout << "Row[" << i << "]: ";
            for (unsigned int j = 0; j < N; j++) {
                std::cout << elem(i, j) << " ";
            }
            std::cout << std::endl;
        }
    }

    void cholesky_basic(HermitianMatrix<T>& matA) {
        for (int j = 0; j < N; j++) {
            double diag = 0;
            double invDiag = 1;
            for (int i = 0; i < j; i++) {
                this->elem(i, j) = 0;
            }
            for (int i = j; i < j + 1; i++) {
                T s(0, 0);
                for (int k = 0; k < j; k++) {
                    s += this->elem(i, k) * this->elem(j, k);
                }
                diag = sqrt((matA.elem(i, i) - s));
                invDiag = 1 / diag;
                this->elem(i, i) = diag;
            }
            for (int i = j + 1; i < N; i++) {
                T s(0, 0);
                for (int k = 0; k < j; k++) {
                    s += this->elem(i, k) * this->elem(j, k);
                }
                this->elem(i, j) = (matA.elem(i, j) - s) * invDiag;
            }
        }
    }

    void cholesky_opt(HermitianMatrix<T>& matA) {
        for (int j = 0; j < N; j++) {
            double diag = 0;
            double invDiag = 1;
            for (int i = 0; i < j; i++) {
                matA.elem(i, j) = 0;
            }
            // calculate diagonal element
            diag = sqrt(matA.elem(j, j));
            invDiag = 1 / diag;
            matA.elem(j, j) = diag;
            for (int i = j + 1; i < N; i++) {
                // calculate other column elements
                matA.elem(i, j) = (matA.elem(i, j)) * invDiag;
                for (int k = j + 1; k < (i + 1); k++) {
                    // update the rest of matA elements
                    matA.elem(i, k) = matA.elem(i, k) - matA.elem(i, j) * matA.elem(k, j);
                }
            }
        }
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = matA.elem(i, j);
            }
        }
    }

    void cholesky_opt_1(HermitianMatrix<T>& matA) {
        for (int j = 0; j < N; j++) {
            double diag = 0;
            double invDiag = 1;
            for (int i = 0; i < j; i++) {
                matA.elem(i, j) = 0;
            }

            // calculate diagonal element
            diag = sqrt(matA.elem(j, j));
            invDiag = 1 / diag;
            matA.elem(j, j) = diag;

            // update matA elements
            for (int k = j + 1; k < N; k++) {
                T L_kj = (matA.elem(k, j)) * invDiag;
                for (int i = k; i < N; i++) {
                    T L_ij = (matA.elem(i, j)) * invDiag;
                    matA.elem(i, k) = matA.elem(i, k) - L_ij * L_kj;
                }
            }
            // calculate other column elements
            for (int i = j + 1; i < N; i++) {
                matA.elem(i, j) = (matA.elem(i, j)) * invDiag;
            }
        }
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = matA.elem(i, j);
            }
        }
    }

    void gen_lower_triangular_randf(int min_exp, int max_exp, char sign_flag) {
        double val;
        // srand(time(NULL));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j <= i; j++) {
                val = this->random_float(min_exp, max_exp, sign_flag);
                this->elem(i, j) = val;
            }
        }
    }

    void gen_upper_triangular_randf(int min_exp, int max_exp, char sign_flag) {
        double val;
        // srand(time(NULL));
        for (int i = 0; i < N; i++) {
            val = this->random_float(min_exp, max_exp, sign_flag);
            this->elem(i, i) = val;
            for (int j = i + 1; j < N; j++) {
                val = this->random_float(min_exp, max_exp, sign_flag);
                this->elem(i, j) = val;
            }
        }
    }

    void transpose(HermitianMatrix<T>& matL) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = matL.elem(j, i);
            }
        }
    }

    void gen_hermitianMatrix(HermitianMatrix<T>& matL) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                T tmp = 0;
                for (int k = 0; k < N; k++) {
                    tmp += matL.elem(i, k) * matL.elem(j, k);
                }
                this->elem(i, j) = tmp;
            }
        }
    }

    void random_init() {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = rand() % 100 / 10.0;
            }
        }
    }

    /**
     ** Function generates a random float using the upper_bound float to determine
     ** the upper bound for the exponent and for the fractional part.
     ** @param min_exp sets the minimum number (closest to 0) to 1 * e^min_exp (min -127)
     ** @param max_exp sets the maximum number to 2 * e^max_exp (max 126)
     ** @param sign_flag if sign_flag = 0 the random number is always positive,
     **                  if sign_flag = 1 then the sign bit is random as well
     ** @return a random float
     **/
    float random_float(int min_exp, int max_exp, char sign_flag) {
        int min_exp_mod = min_exp + 126;
        int sign_mod = sign_flag + 1;
        // int frac_mod = (1 << 23);
        int frac_mod = (1 << 0);

        int s = rand() % sign_mod; // note x % 1 = 0
        int e = (rand() % max_exp) + min_exp_mod;
        int f = rand() % frac_mod;

        int tmp = (s << 31) | (e << 23) | f;

        float r = (float)*((float*)(&tmp));

        return r;
    }

    // store lower triangular elements, row by row, row id from 0 to N;
    void gen_file_matrixA_by_row(HermitianMatrix<T>& matA, std::string file) {
        std::fstream fhdl;
        fhdl.open(file.c_str(), std::ios::out);
        if (!fhdl) {
            std::cout << "[ERROR]: file " << file << "could not be opened !" << std::endl;
            exit(1);
        }
        int pid = 0;
        int num = 0;
        int cnt = 0;
        fhdl << N << std::endl;
        fhdl << pid << std::endl;
        fhdl << num << std::endl;
        fhdl << cnt << std::endl;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < i + 1; j++) {
                fhdl << matA.elem(i, j) << std::endl;
            }
        }
        fhdl.close();
    }
    // store lower triangular elements of matrixL, column by column, column id range: [startId, endId];
    void gen_file_matrixL_by_column(HermitianMatrix<T>& matA, std::string file) {
        std::fstream fhdl;
        fhdl.open(file.c_str(), std::ios::out);
        if (!fhdl) {
            std::cout << "[ERROR]: file " << file << "could not be opened !" << std::endl;
            exit(1);
        }
        fhdl << N << std::endl;
        fhdl << N << std::endl;
        int num = (1 + N) * N / 2;
        fhdl << num << std::endl;
        fhdl << 0 << std::endl;
        for (int j = 0; j < N; j++) {
            for (int i = j; i < N; i++) {
                fhdl << matA.elem(i, j) << std::endl;
            }
        }
        fhdl.close();
    }
    // store lower triangular elements, column by column, column id range: [0, N-1];
    void gen_file_matrixA_by_column(HermitianMatrix<T>& matA, std::string file) {
        std::fstream fhdl;
        fhdl.open(file.c_str(), std::ios::out);
        if (!fhdl) {
            std::cout << "[ERROR]: file " << file << "could not be opened !" << std::endl;
            exit(1);
        }
        int pid = 0;
        int num = 0;
        int cnt = 0;
        fhdl << N << std::endl;
        fhdl << pid << std::endl;
        fhdl << num << std::endl;
        fhdl << cnt << std::endl;
        for (int j = 0; j < N; j++) {
            for (int i = j; i < N; i++) {
                fhdl << matA.elem(i, j) << std::endl;
            }
        }
        fhdl.close();
    }
};

#endif
