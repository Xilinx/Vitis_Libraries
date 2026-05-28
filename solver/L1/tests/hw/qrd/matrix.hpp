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

#ifndef _MATRIX_HPP_
#define _MATRIX_HPP_
#include <iostream>
#include <cmath>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <stdio.h>
#include <complex>

template <typename T>
std::complex<T> complex_sqrt(std::complex<T> in) {
    T a = in.real();
    T b = in.imag();
    T r = std::sqrt(a * a + b * b);
    T x = std::sqrt((r + a) * 0.5);
    T y = std::sqrt((r - a) * 0.5);
    if (b < 0) {
        y = -y;
    }
    return std::complex<T>(x, y);
}

enum TEST_Mode {
    // usr modes
    BASELINE = 0,
    OPT1,
    FINAL_OPT
};

template <typename T>
class ComplexMatrix {
   public:
    std::complex<T>* ptr;
    unsigned int M;
    unsigned int N;

   public:
    ComplexMatrix(unsigned int rows, unsigned int columns) {
        M = rows;
        N = columns;
        ptr = new std::complex<T>[ M * N ];
    }

    ComplexMatrix(ComplexMatrix<T>& hdl) {
        M = hdl.M;
        N = hdl.N;
        ptr = new std::complex<T>[ M * N ];
        std::memcpy(ptr, hdl.ptr, sizeof(std::complex<T>) * M * N);
    }

    ComplexMatrix& operator=(const ComplexMatrix<T> hdl) {
        delete[] ptr;
        M = hdl.M;
        N = hdl.N;
        ptr = new std::complex<T>[ M * N ];
        memcpy(ptr, hdl.ptr, sizeof(std::complex<T>) * M * N);
    }

    ~ComplexMatrix() { delete[] ptr; }

    std::complex<T>& elem(unsigned int row_idx, unsigned int column_idx) { return ptr[row_idx * N + column_idx]; }

    void random_init() {
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j).real((rand() % 100 - 50.5) / 50);
                this->elem(i, j).imag((rand() % 100 - 50.5) / 50);
            }
        }
    }

    void print() {
        std::cout << "Matrix[" << M << " x " << N << "]" << std::endl;
        for (unsigned int i = 0; i < M; i++) {
            std::cout << "Row[" << i << "]: ";
            for (unsigned int j = 0; j < N; j++) {
                std::cout << elem(i, j) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    std::complex<T> inner_product(ComplexMatrix<T>& rop, unsigned int lop_col, unsigned int rop_col) {
        if (this->M != rop.M) {
            std::cout << "rows number not match, inner_product fail" << std::endl;
            exit(0);
        }
        std::complex<T> result;
        result.real(0);
        result.imag(0);
        for (int i = 0; i < M; i++) {
            result += conj(this->elem(i, lop_col)) * rop.elem(i, rop_col);
#ifdef DEBUG_QRD
            std::cout << conj(this->elem(i, lop_col)) << " det with" << rop.elem(i, rop_col) << " is " << result
                      << std::endl;
#endif
        }
        return result;
    }

    void gram_schmidt(ComplexMatrix<T>& Q, ComplexMatrix<T>& R) {
        if (Q.M != this->M || Q.N != this->M || R.M != this->M || R.N != this->N) {
            std::cout << "Q or R size does not match, Gram Schmidt QRD failed" << std::endl;
            exit(0);
        }
        for (int i = 0; i < this->M; i++) {
            for (int j = 0; j < this->M; j++) {
                Q.elem(i, j) = 0;
            }
            for (int j = 0; j < this->N; j++) {
                R.elem(i, j) = 0;
            }
        }
#ifdef DEBUG_QRD
        this->print();
#endif
        for (int k = 0; k < this->N; k++) {
            //
            std::complex<T> norm_2 = inner_product(*this, k, k);
            std::cout << "   " << norm_2 << std::endl;
            T norm_1 = 1.0 / std::sqrt(norm_2.real());
            // calculate vector e
            for (int i = 0; i < this->M; i++) {
                Q.elem(i, k) = this->elem(i, k) * norm_1;
            }
            // calcualte <e, a>
            R.elem(k, k) = Q.inner_product(*this, k, k);
            for (int j = k + 1; j < this->N; j++) {
                R.elem(k, j) = Q.inner_product(*this, k, j);
                for (int i = 0; i < M; i++) {
                    this->elem(i, j) = this->elem(i, j) - R.elem(k, j) * Q.elem(i, k);
                }
#ifdef DEBUG_QRD
                this->print();
#endif
            }
        }
#ifdef DEBUG_QRD
// Q.print();
// R.print();
#endif
    }

    void gram_schmidt_opt1(ComplexMatrix<T>& Q, ComplexMatrix<T>& R) {
        if (Q.M != this->M || Q.N != this->M || R.M != this->M || R.N != this->N) {
            std::cout << "Q or R size does not match, Gram Schmidt QRD failed" << std::endl;
            exit(0);
        }
        for (int i = 0; i < this->M; i++) {
            for (int j = 0; j < this->M; j++) {
                Q.elem(i, j) = 0;
            }
            for (int j = 0; j < this->N; j++) {
                R.elem(i, j) = 0;
            }
        }

#ifdef DEBUG_QRD
        this->print();
#endif
        for (int k = 0; k < this->N; k++) {
            //<ai, ai>
            std::complex<T> norm_2 = inner_product(*this, k, k);

            //<ai, aj>
            for (int j = k + 1; j < this->N; j++) {
                R.elem(k, j) = inner_product(*this, k, j); // ua
            }

            std::cout << "   " << norm_2 << std::endl;
            T rnorm_1 = 1.0 / std::sqrt(norm_2.real());
            R.elem(k, k) = std::sqrt(norm_2.real());
            // calculate vector q
            for (int i = 0; i < this->M; i++) {
                Q.elem(i, k) = this->elem(i, k) * rnorm_1;
            }

            // calcualte <e, a>
            // R.elem(k, k) = Q.inner_product(*this, k, k);
            for (int j = k + 1; j < this->N; j++) {
                for (int i = 0; i < M; i++) {
                    this->elem(i, j) = this->elem(i, j) - (R.elem(k, j) / norm_2.real()) * this->elem(i, k);
                }
            }
#ifdef DEBUG_QRD
            this->print();
#endif
            for (int j = k + 1; j < this->N; j++) {
                R.elem(k, j) = R.elem(k, j) * rnorm_1;
            }
        }
#ifdef DEBUG_QRD
        Q.print();
        R.print();
#endif
    }

    void gram_schmidt_opt2(ComplexMatrix<T>& Q, ComplexMatrix<T>& R) {
        if (Q.M != this->M || Q.N != this->M || R.M != this->M || R.N != this->N) {
            std::cout << "Q or R size does not match, Gram Schmidt QRD failed" << std::endl;
            exit(0);
        }
        ComplexMatrix<T> proj(this->M, this->M);
        for (int i = 0; i < this->M; i++) {
            for (int j = 0; j < this->M; j++) {
                Q.elem(i, j) = 0;
                proj.elem(i, j) = 0;
            }
            for (int j = 0; j < this->N; j++) {
                R.elem(i, j) = 0;
            }
        }

#ifdef DEBUG_QRD
        this->print();
#endif
        // update dot_self
        std::complex<T> norm_2 = inner_product(*this, 0, 0);
        T rnorm_1 = 1.0 / std::sqrt(norm_2.real());
        R.elem(0, 0) = std::sqrt(norm_2.real());
        printf("dot check : %7f + %7fi\n", norm_2.real(), norm_2.imag());

        // update proj
        for (int j = 1; j < this->N; j++) {
            R.elem(0, j) = inner_product(*this, 0, j); // dot <ai, aj> tmp save into R
            proj.elem(0, j) = R.elem(0, j) / norm_2.real();
            R.elem(0, j) = R.elem(0, j) * rnorm_1;
        }

        for (int k = 0; k < this->N - 1; k++) {
            // calculate vector Q
            for (int i = 0; i < this->M; i++) {
                Q.elem(i, k) = this->elem(i, k) * rnorm_1;
            }

            // aj = aj - proj * ai
            for (int j = k + 1; j < this->N; j++) {
                for (int i = 0; i < M; i++) {
                    this->elem(i, j) = this->elem(i, j) - (proj.elem(k, j)) * this->elem(i, k);
#ifdef DEBUG_QRD
                    std::cout << "porj(" << k << "," << j << ")= " << proj.elem(k, j) << "," << this->elem(i, j)
                              << std::endl;
#endif
                }
                if (j == k + 1) { // update dot_self
                    norm_2 = inner_product(*this, k + 1, k + 1);
                    rnorm_1 = 1.0 / std::sqrt(norm_2.real());
                    // printf("dot check : %7f + %7fi, rnorm_1: %7f\n", norm_2.real(), norm_2.imag(), rnorm_1);
                    R.elem(k + 1, k + 1) = std::sqrt(norm_2.real());
                } else {                                               // update proj
                    R.elem(k + 1, j) = inner_product(*this, k + 1, j); // dot <ai, aj> tmp save into R
                    proj.elem(k + 1, j) = R.elem(k + 1, j) / norm_2.real();
                    R.elem(k + 1, j) = R.elem(k + 1, j) * rnorm_1;
                }
            }
#ifdef DEBUG_QRD
            this->print();
#endif
        }

        // for the last line of Q
        for (int i = 0; i < this->M; i++) {
            Q.elem(i, this->N - 1) = this->elem(i, this->N - 1) * rnorm_1;
        }
#ifdef DEBUG_QRD
        Q.print();
        R.print();
#endif
    }

    void gram_schmidt_model(ComplexMatrix<T>& Q, ComplexMatrix<T>& R, ComplexMatrix<T> A, TEST_Mode mode) {
        if (mode == TEST_Mode::BASELINE) {
            A.gram_schmidt(Q, R);
        } else if (mode == TEST_Mode::OPT1) {
            A.gram_schmidt_opt1(Q, R);
        } else {
            A.gram_schmidt_opt2(Q, R);
        }
    }

    void givens_param(unsigned int row_idx, unsigned int column_idx, std::complex<T>& c, std::complex<T>& s) {
        std::complex<T> a11 = this->elem(column_idx, column_idx);
        std::complex<T> a21 = this->elem(row_idx, column_idx);

        T r = std::sqrt(a11.imag() * a11.imag() + a11.real() * a11.real() + a21.imag() * a21.imag() +
                        a21.real() * a21.real());
        // std::cout << "r = " << r << std::endl;

        if (r == 0.0) {
            c.real(1.0);
            c.imag(0.0);
            s.real(0.0);
            s.imag(0.0);
        } else {
            r = 1 / r;
            c.real(a11.real() * r);
            c.imag(-a11.imag() * r);
            s.real(a21.real() * r);
            s.imag(-a21.imag() * r);
        }

        // std::cout << "givens params: r = " << r << ", c = " << c << ", s = " << s << std::endl;
        // std::cout << "a11 = " << a11 << ", a21 = " << a21 << std::endl;
    }

    void givens_rotation(unsigned int row_idx, unsigned int column_idx, std::complex<T> c, std::complex<T> s) {
        if (row_idx <= column_idx) {
            std::cout << "Error in givens_rotation: row_idx should be larger than column_idx" << std::endl;
        }

        for (int b = 0; b < this->N; b++) {
            std::complex<T> Ajb = this->elem(column_idx, b);
            std::complex<T> Aib = this->elem(row_idx, b);
            std::complex<T> Rjb = c * Ajb + s * Aib;
            std::complex<T> Rib = -conj(s) * Ajb + conj(c) * Aib;
            this->elem(column_idx, b) = Rjb;
            this->elem(row_idx, b) = Rib;
        }
    }

    void qrd_givens() {
        for (int j = 0; j < this->N - 1; j++) {
            for (int i = this->M - 1; i > j; i--) {
                std::complex<T> c, s;
                givens_param(i, j, c, s);
                givens_rotation(i, j, c, s);
                // std::cout << "iteration: row = " << i << " column = " << j << std::endl;
                // this->print();
            }
        }
    }

    void gemm(ComplexMatrix<T>& lop, ComplexMatrix<T>& rop) {
        if (lop.N != rop.M) {
            std::cout << "lop's column number should be equal to rop's row number" << std::endl;
            exit(0);
        }
        if (lop.M != this->M) {
            std::cout << "lop's row number should be equal to res's row number" << std::endl;
            exit(0);
        }
        if (rop.N != this->N) {
            std::cout << "rop's column number should be equal to res's column number" << std::endl;
            exit(0);
        }

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                std::complex<T> tmp = 0;
                for (int k = 0; k < lop.N; k++) {
                    tmp += lop.elem(i, k) * rop.elem(k, j);
                }
                this->elem(i, j) = tmp;
            }
        }
    }

    void random_lower_init() {
        random_init();
        for (int i = 0; i < M; i++) {
            for (int j = i + 1; j < N; j++) {
                this->elem(i, j) = std::complex<T>(0.0, 0.0);
            }
        }
    }

    void transpose() {
        unsigned int new_M = N;
        unsigned int new_N = M;
        std::complex<T>* new_ptr = new std::complex<T>[ new_N * new_M ];

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                new_ptr[j * N + i] = elem(i, j);
            }
        }

        M = new_M;
        N = new_N;
        delete[] ptr;
        ptr = new_ptr;
    }

    void conj_transpose() {
        unsigned int new_M = N;
        unsigned int new_N = M;
        std::complex<T>* new_ptr = new std::complex<T>[ new_N * new_M ];

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                new_ptr[j * N + i] = conj(elem(i, j));
            }
        }

        M = new_M;
        N = new_N;
        delete[] ptr;
        ptr = new_ptr;
    }

    void cholesky_decomposition() {
        for (int i = 0; i < M; i++) {
            for (int j = i + 1; j < N; j++) {
                elem(i, j) = std::complex<T>(0.0, 0.0);
            }
        }

        for (int k = 0; k < N; k++) {
            //
            if (k == 0) {
                std::cout << M << std::endl;
                for (int j = 0; j < N; j++) {
                    for (int i = j; i < M; i++) {
                        std::cout << elem(i, j).real() << std::endl;
                    }
                }
                std::cout << "real input above" << std::endl;
                std::cout << 0 << std::endl;
                for (int j = 0; j < N; j++) {
                    for (int i = j; i < M; i++) {
                        std::cout << elem(i, j).imag() << std::endl;
                    }
                }
                std::cout << "imag input above" << std::endl;
            }
            //
            T l0 = std::sqrt(elem(k, k).real());
            elem(k, k).real(l0);
            std::cout << "l0 = " << l0 << std::endl;
            std::cout << "k = " << k << std::endl;
            std::cout << "elem(k, k) = " << elem(k, k) << std::endl;

            if (l0 == 0.0) {
                for (int i = k + 1; i < M; i++) {
                    elem(i, k) = std::complex<T>(0.0, 0.0);
                }
            } else {
                l0 = 1.0 / l0;
                for (int i = k + 1; i < M; i++) {
                    elem(i, k).real(elem(i, k).real() * l0);
                    elem(i, k).imag(elem(i, k).imag() * l0);
                }
            }

            for (int j = k + 1; j < N; j++) {
                std::complex<T> sj(conj(elem(j, k)));
                for (int i = j; i < M; i++) {
                    std::complex<T> si = elem(i, k);
                    elem(i, j) = elem(i, j) - si * sj;
                    if (i == j && j == (k + 1)) {
                        std::cout << "sj = " << sj << std::endl;
                        std::cout << "si = " << si << std::endl;
                        std::cout << "elem = " << elem(i, j) << std::endl;
                        std::cout << "i = " << i << " j = " << j << std::endl;
                    }
                }
            }

            // std::cout << "iter k = " << k << ", after matrix = " << std::endl;
            // print();
        }
    }

    void qrd_householder() {
        std::complex<T>* v = new std::complex<T>[ M ];
        for (int k = 0; k < N; k++) {
            { // calcuate v for house holder transformation
                T x_m2 = 0;
                for (int i = k; i < M; i++) {
                    x_m2 += elem(i, k).real() * elem(i, k).real();
                    x_m2 += elem(i, k).imag() * elem(i, k).imag();
                }
                T x_m = std::sqrt(x_m2);

                T xk_m2 = 0;
                xk_m2 += elem(k, k).real() * elem(k, k).real();
                xk_m2 += elem(k, k).imag() * elem(k, k).imag();
                T xk_m = std::sqrt(xk_m2);

                std::complex<T> alpha = elem(k, k) / xk_m * x_m;

                v[k] = elem(k, k) + alpha;
                for (int i = k + 1; i < M; i++) {
                    v[i] = elem(i, k);
                }

                x_m2 -= xk_m2;
                x_m2 += v[k].real() * v[k].real();
                x_m2 += v[k].imag() * v[k].imag();
                x_m = std::sqrt(x_m2);

                for (int i = k; i < M; i++) {
                    v[i] /= x_m;
                }
            }

            {
                // Q = I - 2*V*V`
                for (int j = k; j < N; j++) {
                    std::complex<T> tmp = 0;
                    for (int i = k; i < M; i++) {
                        tmp += conj(v[i]) * elem(i, j);
                    }
                    tmp *= 2.0;
                    for (int i = k; i < M; i++) {
                        elem(i, j) -= (tmp * v[i]);
                    }
                }
            }
            // std::cout << "after " << k << "th vectors transformation" << std::endl;
            // print();
        }
        delete[] v;
    }

    void diff(ComplexMatrix<T>& hdl) {
        T max_err = 0;
        std::complex<T> a;
        std::complex<T> b;
        int x, y;
        // T max_rel_err = 0;
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                std::complex<T> diff = elem(i, j) - hdl.elem(i, j);
                // T base = std::sqrt(elem(i,j).real() * elem(i,j).real() + elem(i,j).imag() + elem(i,j).imag());
                T tmp_err = std::sqrt(diff.real() * diff.real() + diff.imag() * diff.imag());
                // T tmp_rel_err = tmp_err / base;
                if (max_err < tmp_err) {
                    max_err = tmp_err;
                    a = elem(i, j);
                    b = hdl.elem(i, j);
                    x = i;
                    y = j;
                }
                /*
                if(max_rel_err < tmp_rel_err) {
                    max_rel_err = tmp_rel_err;
                }
                */
            }
        }
        std::cout << "max err = " << max_err << std::endl;
        std::cout << "on [" << x << ", " << y << "], a = " << a << ", b = " << b << std::endl;
        // std::cout << "max err rate = " << max_rel_err << std::endl;
    }

    void neg() {
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                elem(i, j) = -elem(i, j);
            }
        }
    }
};

template <typename T>
class Matrix {
   public:
    T* ptr;
    unsigned int M; // rows
    unsigned int N; // columns
    unsigned int N_512_aligned;
    static const unsigned int ALW = (512 / 8);

   public:
    Matrix(unsigned int rows, unsigned int columns) {
        M = rows;
        N = columns;
        N_512_aligned = (N * sizeof(T) + ALW - 1) / ALW * ALW / sizeof(T);
        ptr = new T[N_512_aligned * M];
    }

    Matrix(Matrix<T>& hdl) {
        std::cout << "copy constructor" << std::endl;
        M = hdl.M;
        N = hdl.N;
        N_512_aligned = hdl.N_512_aligned;
        ptr = new T[N_512_aligned * M];
        std::memcpy(ptr, hdl.ptr, sizeof(T) * N_512_aligned * M);
    }

    Matrix& operator=(const Matrix<T>& hdl) {
        std::cout << "copy assign" << std::endl;
        delete[] ptr;

        M = hdl.M;
        N = hdl.N;
        N_512_aligned = hdl.N_512_aligned;
        ptr = new T[N_512_aligned * M];
        std::memcpy(ptr, hdl.ptr, sizeof(T) * N_512_aligned * M);
    }

    ~Matrix() { delete[] ptr; }

    T& elem(unsigned int row_idx, unsigned int column_idx) { return ptr[row_idx * N_512_aligned + column_idx]; }

    void print() {
        std::cout << "Matrix[" << M << " x " << N << "], N_512_aligned = " << N_512_aligned << std::endl;
        for (unsigned int i = 0; i < M; i++) {
            std::cout << "Row[" << i << "]: ";
            for (unsigned int j = 0; j < N; j++) {
                std::cout << elem(i, j) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    void givens_param(unsigned int row_idx, unsigned int column_idx, T& c, T& s) {
        if (row_idx <= column_idx) {
            std::cout << "Error in givens_param: row_idx should be larger than column_idx" << std::endl;
        }

        T a11 = this->elem(column_idx, column_idx);
        T a21 = this->elem(row_idx, column_idx);
        if (a21 == 0.0) {
            c = 1;
            s = 0;
        } else {
            T cot = a11 / a21;
            s = 1.0 / sqrt(1.0 + cot * cot);
            c = s * cot;
        }
    }

    void givens_rotation(unsigned int row_idx, unsigned int column_idx, T c, T s) {
        if (row_idx <= column_idx) {
            std::cout << "Error in givens_rotation: row_idx should be larger than column_idx" << std::endl;
        }

        for (int b = 0; b < this->N_512_aligned; b++) {
            T Ajb = this->elem(column_idx, b);
            T Aib = this->elem(row_idx, b);
            T Rjb = c * Ajb + s * Aib;
            T Rib = -s * Ajb + c * Aib;
            this->elem(column_idx, b) = Rjb;
            this->elem(row_idx, b) = Rib;
        }
    }

    void qrd_givens() {
        for (int j = 0; j < this->N - 1; j++) {
            for (int i = this->M - 1; i > j; i--) {
                T c, s;
                givens_param(i, j, c, s);
                givens_rotation(i, j, c, s);
                std::cout << "iteration: row = " << i << " column = " << j << std::endl;
                // this->print();
            }
        }
    }

    void gemm(Matrix<T>& lop, Matrix<T>& rop) {
        if (lop.N != rop.M) {
            std::cout << "lop's column number should be equal to rop's row number" << std::endl;
            exit(0);
        }
        if (lop.M != this->M) {
            std::cout << "lop's row number should be equal to res's row number" << std::endl;
            exit(0);
        }
        if (rop.N != this->N) {
            std::cout << "rop's column number should be equal to res's column number" << std::endl;
            exit(0);
        }

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                T tmp = 0;
                for (int k = 0; k < lop.N; k++) {
                    tmp += lop.elem(i, k) * rop.elem(k, j);
                }
                this->elem(i, j) = tmp;
            }
        }
    }

    void random_init() {
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                this->elem(i, j) = rand() % 10 - 4.5;
            }
        }
    }

    void random_lower_init() {
        random_init();
        for (int i = 0; i < M; i++) {
            for (int j = i + 1; j < N; j++) {
                this->elem(i, j) = 0;
            }
        }
    }

    void transpose() {
        unsigned int new_M = N;
        unsigned int new_N = M;
        unsigned int new_N_512_aligned = (new_N * sizeof(T) + ALW - 1) / ALW * ALW / sizeof(T);
        T* new_ptr = new T[new_N_512_aligned * new_M];

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                new_ptr[j * new_N_512_aligned + i] = elem(i, j);
            }
        }

        M = new_M;
        N = new_N;
        N_512_aligned = new_N_512_aligned;
        delete[] ptr;
        ptr = new_ptr;
    }

    T cholesky_row_product(int row_1, int row_2, int column) {
        T tmp = 0;

        for (int j = 0; j < column; j++) {
            tmp += elem(row_1, j) * elem(row_2, j);
        }
        return tmp;
    }

    void cholesky_decomposition() {
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < M; i++) {
                if (i < j) {
                    elem(i, j) = 0;
                } else if (i == j) {
                    { //
                        for (int b = 0; b <= j; b++) {
                            std::cout << elem(i, b) << std::endl;
                        }
                    }

                    elem(i, j) = std::sqrt(elem(i, j) - cholesky_row_product(i, j, j));
                } else {
                    { //
                        for (int b = 0; b <= j; b++) {
                            std::cout << elem(i, b) << std::endl;
                        }
                    }

                    elem(i, j) = (elem(i, j) - cholesky_row_product(i, j, j)) / elem(j, j);
                }
            }
        }
    }
};

#endif
