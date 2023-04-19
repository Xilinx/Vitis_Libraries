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
                this->elem(i, j).real(0);
                this->elem(i, j).imag(0);
            }
            for (int i = j; i < j + 1; i++) {
                T s(0, 0);
                for (int k = 0; k < j; k++) {
                    s += this->elem(i, k) * std::conj(this->elem(j, k));
                }
                diag = sqrt((matA.elem(i, i) - s).real());
                invDiag = 1 / diag;
                this->elem(i, i).real(diag);
                this->elem(i, i).imag(0);
            }
            for (int i = j + 1; i < N; i++) {
                T s(0, 0);
                for (int k = 0; k < j; k++) {
                    s += this->elem(i, k) * std::conj(this->elem(j, k));
                }
                this->elem(i, j) = (matA.elem(i, j) - s) * invDiag;
            }
        }
    }

    void cholesky_opt(HermitianMatrix<T>& matA) {
        for (int j = 0; j < N; j++) {
            std::string inputFileName = "data/gen_input_" + std::to_string(N) + "_" + std::to_string(j) + ".txt";
            // std::cout << "generate_inputFile: " << inputFileName << std::endl;
            gen_file_low_triangular_matrix_by_row(matA, j, inputFileName);

            double diag = 0;
            double invDiag = 1;
            for (int i = 0; i < j; i++) {
                matA.elem(i, j).real(0);
                matA.elem(i, j).imag(0);
            }
            // calculate diagonal element
            diag = sqrt(matA.elem(j, j).real());
            invDiag = 1 / diag;
            matA.elem(j, j).real(diag);
            matA.elem(j, j).imag(0);
            for (int i = j + 1; i < N; i++) {
                // calculate other column elements
                matA.elem(i, j) = (matA.elem(i, j)) * invDiag;
                for (int k = j + 1; k < (i + 1); k++) {
                    // update the rest of matA elements
                    matA.elem(i, k) = matA.elem(i, k) - matA.elem(i, j) * std::conj(matA.elem(k, j));
                }
            }

            std::string updatedFileName =
                "data/gen_updatedData_" + std::to_string(N) + "_" + std::to_string(j) + ".txt";
            gen_file_low_triangular_matrix_by_row(matA, j + 1, updatedFileName);

            std::string gldFileName = "data/gen_gldCol_" + std::to_string(N) + "_" + std::to_string(j) + ".txt";
            gen_file_gld_column(matA, j, gldFileName);
        }
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = matA.elem(i, j);
            }
        }
        std::string outputFileName = "data/gen_output_" + std::to_string(N) + ".txt";
        gen_file_low_triangular_matrix_by_row(matA, 0, outputFileName);
    }

    void cholesky_opt_1(HermitianMatrix<T>& matA) {
        for (int j = 0; j < N; j++) {
            double diag = 0;
            double invDiag = 1;
            for (int i = 0; i < j; i++) {
                matA.elem(i, j).real(0);
                matA.elem(i, j).imag(0);
            }

            // calculate diagonal element
            diag = sqrt(matA.elem(j, j).real());
            invDiag = 1 / diag;
            matA.elem(j, j).real(diag);
            matA.elem(j, j).imag(0);

            // update matA elements
            for (int k = j + 1; k < N; k++) {
                T A_kj = (matA.elem(k, j)) * invDiag;
                for (int i = k; i < N; i++) {
                    T A_ij = (matA.elem(i, j)) * invDiag;
                    matA.elem(i, k) = matA.elem(i, k) - A_ij * std::conj(A_kj);
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
        double real, imag;
        // srand(time(NULL));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < i; j++) {
                real = this->random_float(min_exp, max_exp, sign_flag);
                imag = this->random_float(min_exp, max_exp, sign_flag);
                this->elem(i, j).real(real);
                this->elem(i, j).imag(imag);
            }
            real = this->random_float(min_exp, max_exp, sign_flag);
            this->elem(i, i).real(real);
            this->elem(i, i).imag(0);
        }
    }

    void gen_upper_triangular_randf(int min_exp, int max_exp, char sign_flag) {
        double real, imag;
        // srand(time(NULL));
        for (int i = 0; i < N; i++) {
            real = this->random_float(min_exp, max_exp, sign_flag);
            this->elem(i, i).real(real);
            this->elem(i, i).imag(0);
            for (int j = i + 1; j < N; j++) {
                real = this->random_float(min_exp, max_exp, sign_flag);
                imag = this->random_float(min_exp, max_exp, sign_flag);
                this->elem(i, j).real(real);
                this->elem(i, j).imag(imag);
            }
        }
    }

    void transpose_conjugate(HermitianMatrix<T>& matL) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = std::conj(matL.elem(j, i));
            }
        }
    }

    void gen_hermitianMatrix(HermitianMatrix<T>& matL) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                T tmp = 0;
                for (int k = 0; k < N; k++) {
                    tmp += matL.elem(i, k) * std::conj(matL.elem(j, k));
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
     ** @param sign_flag if sign_flag = 0 the random number is always positive, if
     **              sign_flag = 1 then the sign bit is random as well
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

        // int tmp = (s << 31) | (e << 23) | f;
        int tmp = 0;
        tmp = (s << 31) | (e << 23);

        float r = (float)*((float*)(&tmp));

        return r;
    }

    // store lower triangular elements, row by row, row id from updateID to N;
    void gen_file_low_triangular_matrix_by_row(HermitianMatrix<T>& matA, int updateID, std::string file) {
        std::fstream fhdl;
        fhdl.open(file.c_str(), std::ios::out);
        if (!fhdl) {
            std::cout << "[ERROR]: file " << file << "could not be opened !" << std::endl;
            exit(1);
        }
        for (int i = updateID; i < N; i++) {
            for (int j = updateID; j < i + 1; j++) {
                fhdl << matA.elem(i, j).real() << std::endl;
                fhdl << matA.elem(i, j).imag() << std::endl;
            }
        }
        fhdl.close();
    }
    // store lower triangular elements of matrixL, column by column, column id range: [startId, endId];
    void gen_file_matrixL_by_column(
        HermitianMatrix<T>& matA, int startId, int endId, std::string file_real, std::string file_imag) {
        std::fstream fhdl_real;
        std::fstream fhdl_imag;
        fhdl_real.open(file_real.c_str(), std::ios::out);
        fhdl_imag.open(file_imag.c_str(), std::ios::out);
        if (!fhdl_real) {
            std::cout << "[ERROR]: file " << file_real << "could not be opened !" << std::endl;
            exit(1);
        }
        if (!fhdl_imag) {
            std::cout << "[ERROR]: file " << file_imag << "could not be opened !" << std::endl;
            exit(1);
        }
        fhdl_real << N << std::endl;
        fhdl_real << endId + 1 << std::endl;
        int num = (startId + 1 + endId + 1) * (endId - startId + 1) / 2;
        fhdl_imag << num << std::endl;
        fhdl_imag << 0 << std::endl;
        for (int j = startId; j <= endId; j++) {
            // for (int i = 0; i <= endId; i++) {
            for (int i = j; i <= endId; i++) {
                fhdl_real << matA.elem(i, j).real() << std::endl;
                fhdl_imag << matA.elem(i, j).imag() << std::endl;
            }
        }
        fhdl_real.close();
        fhdl_imag.close();
    }
    // store lower triangular elements, column by column, column id range: [0, N-1];
    void gen_file_matrixA_by_column(HermitianMatrix<T>& matA, std::string file_real, std::string file_imag) {
        std::fstream fhdl_real;
        std::fstream fhdl_imag;
        fhdl_real.open(file_real.c_str(), std::ios::out);
        if (!fhdl_real) {
            std::cout << "[ERROR]: file " << file_real << "could not be opened !" << std::endl;
            exit(1);
        }
        fhdl_imag.open(file_imag.c_str(), std::ios::out);
        if (!fhdl_imag) {
            std::cout << "[ERROR]: file " << file_imag << "could not be opened !" << std::endl;
            exit(1);
        }
        // int num = (startId+1 + endId+1) * (endId - startId +1) / 2;
        int pid = 0;
        int num = 0;
        int info = 0;
        fhdl_real << N << std::endl;
        fhdl_real << pid << std::endl;
        fhdl_imag << num << std::endl;
        fhdl_imag << info << std::endl;
        for (int j = 0; j < N; j++) {
            for (int i = j; i < N; i++) {
                fhdl_real << matA.elem(i, j).real() << std::endl;
                fhdl_imag << matA.elem(i, j).imag() << std::endl;
            }
        }
        fhdl_real.close();
        fhdl_imag.close();
    }
};

#endif
