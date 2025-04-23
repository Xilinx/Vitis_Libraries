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

#ifndef _MATRIX_HPP_
#define _MATRIX_HPP_
#include <iostream>
#include <cmath>
#include <cstring>
#include <complex>
#include <cstdlib>

const float tol = 5.0e-4;
// const float eta = 1.0e-3;
const float eta = 0.05;

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

    ComplexMatrix(unsigned int rows, unsigned int columns, T data) {
        M = rows;
        N = columns;
        ptr = new std::complex<T>[ M * N ];
        for (int i = 0; i < M * N; i++) {
            ptr[i] = data;
        }
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
    void createIdentity() {
        if (M != N) {
            std::cout << "Create Identity Matrix failed as the input matrix is not a square matrix \n";
        }
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                if (i == j) {
                    elem(i, j).real(1);
                    elem(i, j).imag(0);
                } else {
                    elem(i, j) = 0;
                }
            }
        }
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

        for (int k = 0; k < this->N; k++) {
            //
            std::complex<T> norm_2 = inner_product(*this, k, k);
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
            }
        }
        this->print();
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

    void cholesky_opt_1() {
        std::cout << "Cholesky_opt_1 \n";
        for (int j = 0; j < N; j++) {
            T diag = 0;
            T invDiag = 1;
            for (int i = 0; i < j; i++) {
                elem(i, j).real(0);
                elem(i, j).imag(0);
            }

            // calculate diagonal element
            diag = sqrt(elem(j, j).real());
            invDiag = 1 / diag;
            elem(j, j).real(diag);
            elem(j, j).imag(0);
            // calculate other column elements
            for (int i = j + 1; i < N; i++) {
                elem(i, j) = (elem(i, j)) * invDiag;
            }

            // update matA elements
            for (int k = j + 1; k < N; k++) {
                std::complex<T> A_kj = (elem(k, j));
                for (int i = k; i < N; i++) {
                    std::complex<T> A_ij = (elem(i, j));
                    elem(i, k) = elem(i, k) - A_ij * std::conj(A_kj);
                }
            }
        }
    }
    void cdiv(int j) {
        float diag = sqrt(elem(j, j).real());
        float invDiag = 1 / diag;
        elem(j, j).real(diag);
        elem(j, j).imag(0);
        for (int i = j + 1; i < N; i++) {
            elem(i, j) = (elem(i, j)) * invDiag;
        }
    }
    void cmod(int k, int j) {
        std::complex<T> A_kj = (elem(k, j));
        for (int i = k; i < N; i++) {
            std::complex<T> A_ij = (elem(i, j));
            elem(i, k) = elem(i, k) - A_ij * std::conj(A_kj);
        }
    }

    // Submatrix-Cholesky
    void cholesky_opt_4() {
        std::cout << "Cholesky_opt_4 \n";
        for (int j = 0; j < N; j++) {
            T diag = 0;
            T invDiag = 1;
            for (int i = 0; i < j; i++) {
                elem(i, j)={0.0, 0.0};
            }

            cdiv(j);
            for (int k = j + 1; k < N; k++) {
                cmod(k, j);
            }
        }
    }
    // Column-Cholesky
    void cholesky_opt_5() {
        std::cout << "Cholesky_opt_5 \n";
        const int BLK = 4;
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < j; k++) {
                cmod(j, k);
            }
            cdiv(j);
        }
    }
    void cholesky_opt_6() {
        std::cout << "Cholesky_opt_6 \n";
        const int BlkSize = 4;
        const int BlkNum = N / BlkSize;
        for (int b = 0; b < N; b += BlkSize) {
            for (int c = 0; c < BlkSize; c++) {
                int j = b + c;
                for(int i=0; i<j; i++){
                    elem(i,j) = {0.0, 0.0};
                }
                //std::cout << "BLK_ID=" << b << "_" << c << ", columnId=" << j << std::endl;
                for (int k = b; (k < j); k++) {
                    cmod(j, k);
                    //std::cout << "FLOW0: j=" << j << ", k=" << k << ", cmod_" << j << "_" << k << std::endl;
                }
                cdiv(j);
                //std::cout << "j=" << j << ", cdiv_" << j << std::endl;
            }
            for (int p = BlkSize + b; p < N; p++) {
                for (int k = b; k < BlkSize + b; k++) {
                    cmod(p, k);
                    //std::cout << "FLOW1: j=" << p << ", k=" << k << ", cmod_" << p << "_" << k << std::endl;
                }
            }
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
        }
        std::cout << "Array V: \n";
        for (int k = 0; k < N; k++) {
            std::cout << k << "th: " << v[k] << ", ";
        }
        std::cout << std::endl;

        delete[] v;
    }
    void matrix_add(ComplexMatrix<T>& A1, ComplexMatrix<T>& A2) {
        std::complex<T>* new_ptr = new std::complex<T>[ M * N ];
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                new_ptr[i * N + j] = A1.elem(i, j) + A2.elem(i, j);
            }
        }
        delete[] ptr;
        ptr = new_ptr;
    }
    void matrix_sub(ComplexMatrix<T>& A1, ComplexMatrix<T>& A2) {
        std::complex<T>* new_ptr = new std::complex<T>[ M * N ];
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                new_ptr[i * N + j] = A1.elem(i, j) - A2.elem(i, j);
            }
        }
        delete[] ptr;
        ptr = new_ptr;
    }
    /*
    void matrix_mul(ComplexMatrix<T>& Q, ComplexMatrix<T>& R) {
        if (Q.M != this->M || Q.N != R.M || R.N != this->N) {
            std::cout << "mattrix_mul: matrix size does not match" << std::endl;
            exit(0);
        }
        std::complex<T> sum;
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                sum.real(0);
                sum.imag(0);
                for (int k = 0; k < M; k++) {
                    sum += Q.elem(i, k) * R.elem(k, j);
                }
                elem(i, j) = sum;
            }
        }
    }
    */

    void matrix_mul(ComplexMatrix<T>& Q, ComplexMatrix<T>& R) {
        if (Q.M != this->M || Q.N != R.M || R.N != this->N) {
            std::cout << "matrix_mul: matrix size does not match" << std::endl;
            exit(0);
        }
        unsigned int new_M = Q.M;
        unsigned int new_N = R.N;
        std::complex<T>* new_ptr = new std::complex<T>[ new_N * new_M ];
        std::complex<T> sum;
        for (int i = 0; i < new_M; i++) {
            for (int j = 0; j < new_N; j++) {
                sum.real(0);
                sum.imag(0);
                for (int k = 0; k < Q.N; k++) {
                    sum += Q.elem(i, k) * R.elem(k, j);
                }
                new_ptr[i * new_N + j] = sum;
            }
        }
        delete[] ptr;
        ptr = new_ptr;
    }

    bool is_unitary() {
        if (this->M != this->N) {
            std::cout << "matrix is not a square matrix!" << std::endl;
            exit(0);
        }
        bool ret = false;
        int errNum = 0;
        std::complex<T> tmp;
        ComplexMatrix<T> A(*this);
        ComplexMatrix<T> A1(*this);
        A.conj_transpose();
        ComplexMatrix<T> B(M, M);
        B.matrix_mul(A, A1);
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < M; j++) {
                tmp = B.elem(i, j);
                if (i == j) {
                    if ((std::abs(tmp.real()) - 1 < tol) && (std::abs(tmp.imag()) < tol)) {
                        ret = true;
                    } else {
                        std::cout << errNum << ", Mis-match @ [" << i << ", " << j << "], errNum=" << errNum
                                  << std::endl;
                        errNum++;
                    }
                } else {
                    if ((std::abs(tmp.real()) < tol) && (std::abs(tmp.imag()) < tol)) {
                        ret = true;
                    } else {
                        std::cout << errNum << ", Mis-match @ [" << i << ", " << j << "], errNum=" << errNum
                                  << std::endl;
                        errNum++;
                    }
                }
            }
        }
        if (errNum == 0) {
            std::cout << "Check is_unitary: matrix is unitary! \n";
        } else {
            std::cout << "Check is_unitary: matrix is not a unitary, errNum=" << errNum << std::endl;
        }
        return ret;
    }

    void qrd_householder(ComplexMatrix<T>& Q, ComplexMatrix<T>& R) {
        if (Q.M != this->M || Q.N != this->M || R.M != this->M || R.N != this->N) {
            std::cout << "Q or R size does not match, Householder QRD failed" << std::endl;
            exit(0);
        }
        for (int i = 0; i < this->M; i++) {
            for (int j = 0; j < this->M; j++) {
                Q.elem(i, j) = 0;
                if ((i == j)) {
                    Q.elem(i, i).real(1);
                }
            }
            for (int j = 0; j < this->N; j++) {
                R.elem(i, j) = 0;
            }
        }
        std::complex<T> q;
        std::complex<T> sgn;
        T alph = 1;
        T Vr2 = 0;
        T Vr = 0;
        T akk = 0;
        T akk2 = 0;

        for (int k = 0; k < this->N; k++) {
            Vr2 = 0;
            // To calculate the power of length of column aj: pow( ||aj|| )
            for (int i = k; i < M; i++) {
                Vr2 += elem(i, k).real() * elem(i, k).real();
                Vr2 += elem(i, k).imag() * elem(i, k).imag();
            }
            // Diag[k] = 0;
            if (Vr2 > tol) {
                // Vr is the power of the length of column aj: ||aj||*||aj||
                Vr = std::sqrt(Vr2);

                R.elem(k, k).real(Vr);
                R.elem(k, k).imag(0);

                // akk2 is the power of |akk|;
                // akk is absoult value of akk;
                akk2 = 0;
                akk2 = elem(k, k).real() * elem(k, k).real();
                akk2 += elem(k, k).imag() * elem(k, k).imag();
                akk = std::sqrt(akk2);
                // H = I-2ww* = I-(1/alph)uu*;
                // alph = pow(||ak||) + ||ak||*|akk| = Vr2 + sqrt(Vr2*d1*d1')
                alph = Vr2 + Vr * akk;
                if (akk2 != (T)0) {
                    sgn = elem(k, k) / akk;
                } // endif
                else {
                    sgn.real(1);
                    sgn.imag(0);
                }
                elem(k, k) = sgn * (Vr + akk);

                for (int j = k + 1; j < N; j++) {
                    q = 0;
                    for (int i = k; i < M; i++) {
                        q += std::conj(elem(i, k)) * elem(i, j);
                    }
                    q = q / alph;

                    elem(k, j) -= q * elem(k, k);
                    // Phase Transformation
                    R.elem(k, j) = -std::conj(sgn) * elem(k, j);
                    for (int i = k + 1; i < M; i++) {
                        elem(i, j) -= q * elem(i, k);
                    }
                }

                for (int j = 0; j < M; j++) {
                    q.real(0);
                    q.imag(0);
                    for (int i = k; i < M; i++) {
                        q += std::conj(elem(i, k)) * Q.elem(i, j);
                    }
                    q = q / alph;

                    Q.elem(k, j) -= q * elem(k, k);
                    // Phase Transformation
                    Q.elem(k, j) = -std::conj(sgn) * Q.elem(k, j);

                    for (int i = k + 1; i < M; i++) {
                        Q.elem(i, j) -= q * elem(i, k);
                    }
                }
            } // endif (z > tol)
        }     // endfor k 0:N

        Q.conj_transpose();

    } // end qrd_householder

    void lstqr_householder(ComplexMatrix<T>& R, ComplexMatrix<T>& B) {
        ComplexMatrix<T> Q(M, M);
        std::cout << "Kernel_lstqr_householder: Rx = QT*b, " << std::endl;

        for (int i = 0; i < this->M; i++) {
            for (int j = 0; j < this->M; j++) {
                Q.elem(i, j) = 0;
                if ((i == j)) {
                    Q.elem(i, i).real(1);
                }
            }
            for (int j = 0; j < this->N; j++) {
                R.elem(i, j) = 0;
            }
        }
        std::complex<T> q;
        std::complex<T> sgn;
        T alph = 1;
        T Vr2 = 0;
        T Vr = 0;
        T akk = 0;
        T akk2 = 0;

        for (int k = 0; k < this->N; k++) {
            Vr2 = 0;
            // To calculate the power of length of column aj: pow( ||aj|| )
            for (int i = k; i < M; i++) {
                Vr2 += elem(i, k).real() * elem(i, k).real();
                Vr2 += elem(i, k).imag() * elem(i, k).imag();
            }
            // Diag[k] = 0;
            if (Vr2 > tol) {
                // Vr is the power of the length of column aj: ||aj||*||aj||
                Vr = std::sqrt(Vr2);

                R.elem(k, k).real(Vr);
                R.elem(k, k).imag(0);

                // akk2 is the power of |akk|;
                // akk is absoult value of akk;
                akk2 = 0;
                akk2 = elem(k, k).real() * elem(k, k).real();
                akk2 += elem(k, k).imag() * elem(k, k).imag();
                akk = std::sqrt(akk2);
                // H = I-2ww* = I-(1/alph)uu*;
                // alph = pow(||ak||) + ||ak||*|akk| = Vr2 + sqrt(Vr2*d1*d1')
                alph = Vr2 + Vr * akk;
                if (akk2 != (T)0) {
                    sgn = elem(k, k) / akk;
                } // endif
                else {
                    sgn.real(1);
                    sgn.imag(0);
                }
                elem(k, k) = sgn * (Vr + akk);

                for (int j = k + 1; j < N; j++) {
                    q = 0;
                    for (int i = k; i < M; i++) {
                        q += std::conj(elem(i, k)) * elem(i, j);
                    }
                    q = q / alph;

                    elem(k, j) -= q * elem(k, k);
                    // Phase Transformation
                    R.elem(k, j) = -std::conj(sgn) * elem(k, j);
                    for (int i = k + 1; i < M; i++) {
                        elem(i, j) -= q * elem(i, k);
                    }
                }

                for (int j = 0; j < B.N; j++) {
                    q.real(0);
                    q.imag(0);
                    for (int i = k; i < M; i++) {
                        q += std::conj(elem(i, k)) * B.elem(i, j);
                    }
                    q = q / alph;

                    B.elem(k, j) -= q * elem(k, k);
                    // Phase Transformation
                    B.elem(k, j) = -std::conj(sgn) * B.elem(k, j);

                    for (int i = k + 1; i < M; i++) {
                        B.elem(i, j) -= q * elem(i, k);
                    }
                }
            } // endif (z > tol)
        }     // endfor k 0:N
        std::cout << "Matrix_QB: \n";
        B.print();

    } // end lstqr_householder

    void back_substitution(ComplexMatrix<T>& B, ComplexMatrix<T>& X) {
        std::cout << "Solve RX=B using back_substitution -> X=inv(R)*B " << std::endl;
        // RX=B, to solve X=inv(R)B
        // R is M*N; B is M*K, X is N*K
        for (int k = 0; k < B.N; k++) {
            for (int j = N - 1; j >= 0; j--) {
                T mod = elem(j, j).real() * elem(j, j).real() + elem(j, j).imag() * elem(j, j).imag();
                X.elem(j, k) = B.elem(j, k) * std::conj(elem(j, j)) / mod;
                for (int i = 0; i < j; i++) {
                    B.elem(i, k) = B.elem(i, k) - elem(i, j) * X.elem(j, k);
                }
            }
        }
        // std::cout << "after back_substitition \n";
        // B.print();
    } // end back_substitution

    void lstqr_residuals(ComplexMatrix<T>& B, T* res_ptr) {
        // std::cout << "Kernel_lstqr_residuals: Calculate residuals" << std::endl;
        for (int j = 0; j < B.N; j++) {
            T sum = 0;
            for (int i = N; i < M; i++) {
                sum += B.elem(i, j).real() * B.elem(i, j).real() + B.elem(i, j).imag() * B.elem(i, j).imag();
                // std::cout << "B.elem(" << i << ", " << j << ")=" << B.elem(i, j) << ", sum = " << sum  << std::endl;
            }
            res_ptr[j] = std::sqrt(sum);
            std::cout << "residuals: res_ptr[" << j << "] =  " << res_ptr[j] << std::endl;
        }
    }
    void lstqr(ComplexMatrix<T>& B, ComplexMatrix<T>& X) {
        // std::cout << "Kernel_lstqr: Calculate Least Square with QRD" << std::endl;
        ComplexMatrix<T> R(M, N);
        lstqr_householder(R, B);
        R.back_substitution(B, X);
        T res_ptr[X.N];
        lstqr_residuals(B, res_ptr);
    } // end lstqr

    void diff(ComplexMatrix<T>& hdl) {
        T max_err = eta;
        int err_num = 0;
        std::complex<T> a;
        std::complex<T> b;
        // T max_rel_err = 0;
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                std::complex<T> diff = elem(i, j) - hdl.elem(i, j);
                T tmp_err = std::sqrt(diff.real() * diff.real() + diff.imag() * diff.imag());
                if (max_err < tmp_err) {
                    a = elem(i, j);
                    b = hdl.elem(i, j);
                    err_num++;
                    std::cout << "err_num=" << err_num << ", on [" << i << ", " << j << "],  " << a << ", VS  " << b
                              << ", diff =" << tmp_err << std::endl;
                }
            }
        }
        std::cout << "Total error number is : " << err_num << std::endl;
    }

    void norm(ComplexMatrix<T>& B, T* res_ptr) {
        for (int j = 0; j < B.N; j++) {
            T sum = 0;
            for (int i = 0; i < B.M; i++) {
                sum += B.elem(i, j).real() * B.elem(i, j).real() + B.elem(i, j).imag() * B.elem(i, j).imag();
            }
            res_ptr[j] = std::sqrt(sum);
            std::cout << "norm: res_ptr[" << j << "] =  " << res_ptr[j] << std::endl;
        }
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
