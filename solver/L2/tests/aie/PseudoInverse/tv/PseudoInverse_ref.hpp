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

#ifndef _PSEUDOINVERSE_REF_HPP_
#define _PSEUDOINVERSE_REF_HPP_
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <complex>
#include <cstdlib>
#include <ctime>
#include <random>
#include <cmath>

const std::complex<float> CZERO = {0, 0};
const std::complex<float> CONE = {1, 0};

const float eta = 1.5e-8;
const float tol = 1.0e-5;

void printVector(float* vec, int size) {
    std::cout << "Vector[" << size << "]: \n";
    for (int i = 0; i < size; i++) {
        std::cout << vec[i] << "  ";
    }
    std::cout << std::endl;
}
void printCVector(std::complex<float>* vec, int size) {
    std::cout << "CVector[" << size << "]: \n";
    for (int i = 0; i < size; i++) {
        std::cout << vec[i] << "  ";
    }
    std::cout << std::endl;
}
static float Norm_Square(std::complex<float> a) {
    return (a.real() * a.real() + a.imag() * a.imag());
}

static float CABS(std::complex<float> a) {
    return std::sqrt(Norm_Square(a));
}

class CMatrix {
   public:
    CMatrix() : m_rows(0), m_cols(0), m_val(nullptr){};
    CMatrix(unsigned rows, unsigned cols);
    CMatrix(unsigned rows, unsigned cols, std::complex<float> val);
    CMatrix(std::complex<float>** data, unsigned rows, unsigned cols);
    CMatrix(std::complex<float>* val, unsigned m, unsigned n);
    CMatrix(const CMatrix&);
    ~CMatrix();

    CMatrix& operator=(const CMatrix& m);
    CMatrix& operator+=(const CMatrix&);
    CMatrix& operator-=(const CMatrix&);
    CMatrix& operator*=(const CMatrix&);
    CMatrix& operator*=(float);
    CMatrix& operator*=(std::complex<float>);
    CMatrix& operator/=(float);

    CMatrix ADD(const CMatrix&, const CMatrix&);
    CMatrix SUB(const CMatrix&, const CMatrix&);
    CMatrix MUL(const CMatrix&, const CMatrix&);
    CMatrix MUL(const CMatrix&, float);
    CMatrix MUL(const CMatrix&, std::complex<float>);
    CMatrix DIV(const CMatrix&, float);

    std::complex<float>& operator()(unsigned row, unsigned cols);
    std::complex<float> operator()(unsigned row, unsigned cols) const;

    int size() const { return m_rows * m_cols; }
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

    static std::complex<float> random_init_complex();

    CMatrix ConjugateTranspose();
    bool Compare(const CMatrix& A, const CMatrix& B);
    void PseudoInverse(const CMatrix& A, CMatrix& Pinv);
    bool Verify_Pinv(const CMatrix& A, CMatrix& Pinv);

    void PrintCMatrix();
    void WriteCMatrix(std::string filename0);
    std::complex<float>** m_val;

   private:
    unsigned m_rows, m_cols;
    void allocSpace();
};

/* PRIVATE HELPER FUNCTIONS
 *  ********************************/
void CMatrix::allocSpace() {
    m_val = new std::complex<float>*[m_rows];
    for (int i = 0; i < m_rows; ++i) {
        m_val[i] = new std::complex<float>[ m_cols ];
    }
}

/* PUBLIC MEMBER FUNCTIONS
 *  ********************************/
CMatrix::CMatrix(unsigned rows, unsigned cols) : m_rows(rows), m_cols(cols) {
    allocSpace();
    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            std::complex<float> tmp = random_init_complex();
            m_val[i][j] = tmp;
        }
    }
}
CMatrix::CMatrix(unsigned rows, unsigned cols, std::complex<float> val) : m_rows(rows), m_cols(cols) {
    allocSpace();
    for (int i = 0; i < m_rows; i++) {
        for (int j = 0; j < m_cols; j++) {
            m_val[i][j] = val;
        }
    }
}

CMatrix::CMatrix(std::complex<float>** data, unsigned rows, unsigned cols) : m_rows(rows), m_cols(cols) {
    allocSpace();
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] = data[i][j];
        }
    }
}

CMatrix::CMatrix(std::complex<float>* val, unsigned rows, unsigned cols) : m_rows(rows), m_cols(cols) {
    int k = 0;
    allocSpace();
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] = val[k++];
        }
    }
}

CMatrix::~CMatrix() {
    for (int i = 0; i < m_rows; ++i) {
        delete[] m_val[i];
    }
    delete[] m_val;
}

CMatrix::CMatrix(const CMatrix& m) : m_rows(m.m_rows), m_cols(m.m_cols) {
    allocSpace();
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] = m.m_val[i][j];
        }
    }
}
CMatrix& CMatrix::operator=(const CMatrix& m) {
    if (this == &m) {
        return *this;
    }
    if (m_rows != m.m_rows || m_cols != m.m_cols) {
        for (int i = 0; i < m_rows; ++i) {
            delete[] m_val[i];
        }
        delete[] m_val;
        m_rows = m.m_rows;
        m_cols = m.m_cols;
        allocSpace();
    }
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] = m.m_val[i][j];
        }
    }
    return *this;
}

CMatrix& CMatrix::operator+=(const CMatrix& m) {
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] += m.m_val[i][j];
        }
    }
    return *this;
}

CMatrix& CMatrix::operator-=(const CMatrix& m) {
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] -= m.m_val[i][j];
        }
    }
    return *this;
}

CMatrix& CMatrix::operator*=(const CMatrix& m) {
    CMatrix temp(m_rows, m.m_cols, CZERO);
    for (int i = 0; i < temp.m_rows; ++i) {
        for (int j = 0; j < temp.m_cols; ++j) {
            for (int k = 0; k < m_cols; ++k) {
                temp.m_val[i][j] += (m_val[i][k] * m.m_val[k][j]);
            }
        }
    }
    return (*this = temp);
}

CMatrix& CMatrix::operator*=(float num) {
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] *= num;
        }
    }
    return *this;
}

CMatrix& CMatrix::operator*=(std::complex<float> num) {
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] *= num;
        }
    }
    return *this;
}

CMatrix& CMatrix::operator/=(float num) {
    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            m_val[i][j] /= num;
        }
    }
    return *this;
}

CMatrix CMatrix::ADD(const CMatrix& m1, const CMatrix& m2) {
    CMatrix temp(m1);
    return (temp += m2);
}

CMatrix CMatrix::SUB(const CMatrix& m1, const CMatrix& m2) {
    CMatrix temp(m1);
    return (temp -= m2);
}

CMatrix CMatrix::MUL(const CMatrix& m1, const CMatrix& m2) {
    CMatrix temp(m1.m_rows, m2.m_cols, CZERO);
    for (int i = 0; i < m1.m_rows; ++i) {
        for (int j = 0; j < m2.m_cols; ++j) {
            for (int k = 0; k < m1.m_cols; ++k) {
                std::complex<float> val = (m1.m_val[i][k] * m2.m_val[k][j]);
                temp.m_val[i][j] += val;
            }
        }
    }
    return temp;
}

CMatrix CMatrix::MUL(const CMatrix& m, float num) {
    CMatrix temp(m);
    return (temp *= num);
}

CMatrix CMatrix::MUL(const CMatrix& m, std::complex<float> num) {
    CMatrix temp(m);
    return (temp *= num);
}

CMatrix CMatrix::DIV(const CMatrix& m, float num) {
    CMatrix temp(m);
    return (temp /= num);
}

std::complex<float> CMatrix::random_init_complex() {
    std::complex<float> ret;
    ret.real(rand() % 128 / 10.0);
    ret.imag(rand() % 128 / 10.0);
    return ret;
}

void CMatrix::PrintCMatrix() {
    std::cout << "CMatrix[" << m_rows << " x " << m_cols << "]: " << std::endl;
    for (unsigned int i = 0; i < m_rows; i++) {
        std::cout << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < m_cols; j++) {
            std::cout << std::setw(8) << m_val[i][j] << "  ";
        }
        std::cout << std::endl;
    }
}

void CMatrix::WriteCMatrix(std::string filename0) {
    std::ofstream myfile0;
    myfile0.open(filename0);
    myfile0 << "CMatrix[" << m_rows << " x " << m_cols << "]: " << std::endl;
    for (unsigned int i = 0; i < m_rows; i++) {
        myfile0 << "Row[" << i << "]: ";
        for (unsigned int j = 0; j < m_cols; j++) {
            myfile0 << std::setw(8) << m_val[i][j] << "  ";
        }
        myfile0 << std::endl;
    }
    myfile0.close();
}

CMatrix CMatrix::ConjugateTranspose() {
    unsigned int M = m_rows;
    unsigned int N = m_cols;
    CMatrix ret(N, M);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            ret.m_val[j][i] = std::conj(m_val[i][j]);
        }
    }
    return ret;
}

// Compare two matrix, return true if the diff between their elements are all less than the threshold.
bool CMatrix::Compare(const CMatrix& A, const CMatrix& B) {
    if (A.rows() != B.rows() || A.cols() != B.cols()) return false;
    for (int i = 0; i < A.rows(); i++) {
        for (int j = 0; j < A.cols(); j++) {
            if (CABS(B.m_val[i][j]) > 1.0) {
                if (CABS((A.m_val[i][j] - B.m_val[i][j])) / CABS(B.m_val[i][j]) >= 0.001) return false;
            } else {
                if (CABS((A.m_val[i][j] - B.m_val[i][j])) > 0.001) return false;
            }
        }
    }
    return true;
}

// PseudoInverse
// Input: CMatrix A
// Output: CMatrix Pinv
void CMatrix::PseudoInverse(const CMatrix& A, CMatrix& Pinv) {
    // It takes two steps to compute PseudoInverse of CMatrix A.
    // 1. Compute SVD of A: A = U * W * transpose(V)
    // 2. Compute PseudoInverse of A from the result of SVD: A† = V * W† * transpose(U)

    // ********** Step 1 **********
    // Compute SVD of CMatrix A
    int m = A.rows();
    int n = A.cols();
    CMatrix A_modified = A;
    CMatrix U = CMatrix(m, m, CZERO);
    CMatrix V = CMatrix(n, n, CZERO);
    CMatrix W = CMatrix(m, n, CZERO);

    float* b = (float*)malloc(n * sizeof(float));
    float* c = (float*)malloc(n * sizeof(float));
    float* t = (float*)malloc(n * sizeof(float));
    float* S = (float*)malloc(n * sizeof(float));

    int32_t i, j, k, k1, L, L1;
    int32_t flag, its, nm;

    std::complex<float> p;
    std::complex<float> q;
    std::complex<float> tmp;
    float eps = 0.0;
    float anorm = 0.0;
    float cs, sn;              // for Givens Rotation
    float w, x, y, z, f, g, h; // for Givens Rotation

    // Householder Reduction
    // std::cout << " *** Householder Reduction *** " << std::endl;
    // std::cout << "Before Householder Reduction" << std::endl;
    // A_modified.PrintCMatrix();
    for (int k = 0; k < n; k++) {
        k1 = k + 1;
        // Left Householder Reduction
        // Elimination of A_modified[i][k], i = k, ..., m-1
        // std::cout << "Left Householder Reduction: k=" << k << std::endl;
        z = 0.0;
        for (int i = k; i < m; i++) {
            // std::cout << k << ": " << Norm_Square(A_modified.m_val[i][k]) << std::endl;
            z += Norm_Square(A_modified.m_val[i][k]);
        }
        b[k] = 0.0;
        if (z > tol) {
            z = sqrt(z);
            // std::cout << "z: " << z << std::endl;
            b[k] = z;
            w = CABS(A_modified.m_val[k][k]);
            // std::cout << "float w: " << w << std::endl;
            q = CONE;
            if (w != (float)0) {
                q = A_modified.m_val[k][k] / w;
                // std::cout << "cfloat q: " << q << std::endl;
            }
            // std::cout << "zw: " << z + w << std::endl;
            A_modified.m_val[k][k] = q * (z + w);
            p = -std::conj(A_modified.m_val[k][k]) / CABS(A_modified.m_val[k][k]);
            // std::cout << "p: " << p << std::endl;
            if (k != n - 1) {
                for (int j = k1; j < n; j++) {
                    q = CZERO;
                    // std::cout << "j: " << j << std::endl;
                    for (int i = k; i < m; i++) {
                        q += std::conj(A_modified.m_val[i][k]) * A_modified.m_val[i][j];
                    }
                    q = q / (z * (z + w));
                    // std::cout << "q: " << q << std::endl;
                    for (int i = k; i < m; i++) {
                        A_modified.m_val[i][j] -= q * A_modified.m_val[i][k];
                        if (i == k) {
                            A_modified.m_val[i][j] *= p;
                        }
                        // std::cout << "A_modified.m_val[i][j]: " << A_modified.m_val[i][j] << std::endl;
                        // std::cout << std::endl;
                    }
                }
            }
        }
        // std::cout << "After Left Householder reduction" << std::endl;
        // A_modified.PrintCMatrix();
        // Right Householder Reduction
        // Elimination of A_modified[k][j], j=k+2, ..., n-1
        // std::cout << "Right Householder Reduction: k=" << k << std::endl;
        z = 0.0;
        for (int j = k1; j < n; j++) {
            // std::cout << k << ": " << Norm_Square(A_modified.m_val[k][j]) << std::endl;
            z += Norm_Square(A_modified.m_val[k][j]);
        }
        c[k1] = 0.0;
        if (z > tol) {
            z = sqrt(z);
            // std::cout << "z: " << z << std::endl;
            c[k1] = z;
            w = CABS(A_modified.m_val[k][k1]);
            // std::cout << "float w: " << w << std::endl;
            q = CONE;
            if (w != (float)0) {
                q = A_modified.m_val[k][k1] / w;
                // std::cout << "cfloat q: " << q << std::endl;
            }
            A_modified.m_val[k][k1] = q * (z + w);
            // std::cout << "q * zw: " << A_modified.m_val[k][k1] << std::endl;
            p = -std::conj(A_modified.m_val[k][k1]) / CABS(A_modified.m_val[k][k1]);
            // std::cout << "p: " << p << std::endl;
            for (int i = k1; i < m; i++) {
                q = CZERO;
                for (int j = k1; j < n; j++) {
                    q += std::conj(A_modified.m_val[k][j]) * A_modified.m_val[i][j];
                }
                q = q / (z * (z + w));
                for (int j = k1; j < n; j++) {
                    A_modified.m_val[i][j] -= q * A_modified.m_val[k][j];
                    if (j == k1) {
                        A_modified.m_val[i][j] *= p;
                    }
                }
            }
        }
        // std::cout << "After Right Householder reduction" << std::endl;
        // A_modified.PrintCMatrix();
    }
    // std::cout << "After ALL Householder reduction" << std::endl;
    // A_modified.PrintCMatrix();
    for (int k = 0; k < n; k++) {
        S[k] = b[k];
        t[k] = c[k];
        anorm = b[k] + c[k];
        if (anorm > eps) {
            eps = anorm;
        }
        eps *= eta;
    }
    // std::cout << "anorm=" << anorm << ", eps=" << eps << std::endl;
    for (int j = 0; j < n; j++) {
        U.m_val[j][j] = CONE;
        V.m_val[j][j] = CONE;
    }
    // std::cout << "Householder: diagonal: " << std::endl;
    // printVector(S, n);
    // std::cout << "Householder: sub-diagonal: " << std::endl;
    // printVector(t, n);

    // std::cout << " *** Diagornalization of the bidiagonal form *** " << std::endl;
    for (int k = n - 1; k >= 0; k--) {       // Diagonalization of the bidiagonal form: Loop over singular values,
        for (int its = 0; its < 30; its++) { // and over allowed iterations.
            flag = 1;
            // Test for splitting.
            for (L = k; L >= 0; L--) {
                if ((float)(std::abs(t[L])) <= eps) {
                    flag = 0;
                    break;
                }
                if ((float)(std::abs(S[L - 1])) <= eps) {
                    break;
                }
                // std::cout << "Diagornalization Splitting Test: k=" << k << ", iteration=" << its << ", L=" << L
                //   << ", flag=" << flag << ", t[" << L << "]=" << t[L] << ", S[" << L - 1 << "]=" << S[L - 1]
                //   << std::endl;
            }
            // Cancellation of E[L]
            if (flag) {
                cs = 0.0;
                sn = 1.0;
                L1 = L - 1;
                for (int i = L; i <= k; i++) {
                    f = sn * t[i];
                    t[i] = cs * t[i];
                    if ((float)(std::abs(f)) <= eps) {
                        break;
                    }
                    h = S[i];
                    w = sqrt(f * f + h * h);
                    S[i] = w;
                    cs = h / w;
                    sn = -f / w;
                    for (int j = 0; j < n; j++) {
                        x = U.m_val[j][L1].real();
                        y = U.m_val[j][i].real();
                        U.m_val[j][L1] = {x * cs + y * sn, 0};
                        U.m_val[j][i] = {y * cs - x * sn, 0};
                    }
                    // std::cout << "Diagornalization Cancellation: k=" << k << ", iteration=" << its << ", t[" << i
                    //           << "]=" << t[i] << ", S[" << i << "]=" << S[i] << std::endl;
                }
            }

            // test for convergence
            w = S[k];
            if (L == k) {
                break;
            }

            // Shift from bottom 2-by-2 minor.
            x = S[L];
            y = S[k - 1];
            g = t[k - 1];
            h = t[k];
            f = ((y - w) * (y + w) + (g - h) * (g + h)) / (2.0 * h * y);
            g = sqrt(f * f + 1);
            if (f < 0.0) {
                g = -g;
            }
            f = ((x - w) * (x + w) + h * (y / (f + g) - h)) / x;
            // Next QR transformation:
            cs = 1.0;
            sn = 1.0;
            L1 = L + 1;
            for (int i = L1; i <= k; i++) {
                g = t[i];
                y = S[i];
                h = sn * g;
                g = cs * g;
                w = sqrt(h * h + f * f);
                t[i - 1] = w;
                cs = f / w;
                sn = h / w;
                f = x * cs + g * sn;
                g = g * cs - x * sn;
                h = y * sn;
                y = y * cs;
                for (int j = 0; j < n; j++) {
                    x = V.m_val[j][i - 1].real();
                    w = V.m_val[j][i].real();
                    V.m_val[j][i - 1] = {x * cs + w * sn, 0};
                    V.m_val[j][i] = {w * cs - x * sn, 0};
                }
                w = sqrt(h * h + f * f);
                S[i - 1] = w;
                cs = f / w;
                sn = h / w;
                f = cs * g + sn * y;
                x = cs * y - sn * g;
                for (int j = 0; j < n; j++) {
                    y = U.m_val[j][i - 1].real();
                    w = U.m_val[j][i].real();
                    U.m_val[j][i - 1] = {y * cs + w * sn, 0};
                    U.m_val[j][i] = {w * cs - y * sn, 0};
                }
            }
            t[L] = 0.0;
            t[k] = f;
            S[k] = x;
            // std::cout << "Diagornalization QR iteration: k=" << k << ", iteration=" << its << ", t[" << k
            //           << "]=" << t[k] << ", S[" << k << "]=" << S[k] << std::endl;
        }
        // Convergence
        if (w >= 0.0) {
            continue;
        }
        S[k] = -w;
        for (int j = 0; j < n; j++) {
            V.m_val[j][k] = -V.m_val[j][k];
        }
    }
    // std::cout << "Diagornization: diagonal: " << std::endl;
    // printVector(S, n);
    // std::cout << "Diagornization: Left Singular: " << std::endl;
    // U.PrintCMatrix();
    // std::cout << "Diagornization: Right Singular: " << std::endl;
    // V.PrintCMatrix();

    // Back transformation
    // Accumulation of left-hand transformations
    for (int k = n - 1; k >= 0; k--) {
        if (b[k] == 0.0) {
            continue;
        }
        q = -A_modified.m_val[k][k] / CABS(A_modified.m_val[k][k]);
        for (int j = 0; j < n; j++) {
            U.m_val[k][j] = U.m_val[k][j] * q;
        }
        for (int j = 0; j < n; j++) {
            q = CZERO;
            for (int i = k; i < m; i++) {
                q += std::conj(A_modified.m_val[i][k]) * U.m_val[i][j];
            }
            q = q / (CABS(A_modified.m_val[k][k]) * b[k]);
            for (int i = k; i < m; i++) {
                U.m_val[i][j] -= q * A_modified.m_val[i][k];
            }
        }
    }
    // Accumulation of right-hand transformations
    if (n > 1) {
        for (int k = n - 2; k >= 0; --k) {
            k1 = k + 1;
            if (c[k1] == 0.0) {
                continue;
            }
            q = -std::conj(A_modified.m_val[k][k1]) / CABS(A_modified.m_val[k][k1]);
            for (int j = 0; j < n; j++) {
                V.m_val[k1][j] = V.m_val[k1][j] * q;
            }
            for (int j = 0; j < n; j++) {
                q = CZERO;
                for (int i = k1; i < n; i++) {
                    q += A_modified.m_val[k][i] * V.m_val[i][j];
                }
                q = q / (CABS(A_modified.m_val[k][k1]) * c[k1]);
                for (int i = k1; i < n; i++) {
                    V.m_val[i][j] -= q * std::conj(A_modified.m_val[k][i]);
                }
            }
        }
    }

    // std::cout << "Diagornization: diagonal: " << std::endl;
    // printVector(S, n);
    // std::cout << "Diagornization: Left Singular: " << std::endl;
    // U.PrintCMatrix();
    // std::cout << "Diagornization: Right Singular: " << std::endl;
    // V.PrintCMatrix();
    // Form W from S.
    for (int i = 0; i < n; i++) {
        W.m_val[i][i] = {S[i], 0};
    }

    // ********** Step 2 **********
    // Postprocess of SVD to get Pseudoinverse.
    // Assume SVD of matrix A: A = U * W * transpose(V).
    // Pseudoinverse of matrix A: A† = V * W† * transpose(U)
    CMatrix UT = U.ConjugateTranspose(); // transpose(U)
    CMatrix W_Pinv = W;                  // W†
    for (int i = 0; i < n; i++) {
        float norm_square = Norm_Square(W.m_val[i][i]);
        if (norm_square != 0) {
            W_Pinv.m_val[i][i] = std::conj(W_Pinv.m_val[i][i]) / norm_square;
        }
    }
    W_Pinv = W_Pinv.ConjugateTranspose();

    // std::cout << "Psudoinverse of A (V * W† * transpose(U)): " << std::endl;
    Pinv = MUL(V, W_Pinv);
    Pinv = MUL(Pinv, UT);
    // Pinv.PrintCMatrix();

    // Result Verify
    bool res = Verify_Pinv(A, Pinv);
    if (res) {
        // std::cout << "Pinv result pass!" << std::endl;
    } else {
        // std::cout << "Pinv is NOT correct!" << std::endl;
    }

    // release temporary memory
    free(b);
    free(c);
    free(t);
    free(S);
}

bool CMatrix::Verify_Pinv(const CMatrix& A, CMatrix& Pinv) {
    // std::cout << "*** Verifying the accuracy of Pinv ***" << std::endl;
    // CMatrix Pinv is the pseudo-inverse of CMatrix A if it satisfies all four conditions below.
    // A: A; B: Pinv
    // 1. Verify ABA = A
    CMatrix ABA = MUL(MUL(A, Pinv), A);
    if (!Compare(ABA, A)) {
        // std::cout << "Condition 1 fail!" << std::endl;
        return false;
    }
    // 2. Verify BAB = B
    CMatrix BAB = MUL(MUL(Pinv, A), Pinv);
    if (!Compare(BAB, Pinv)) {
        // std::cout << "Condition 2 fail!" << std::endl;
        return false;
    }
    // 3. Verify transpose(AB) = AB
    CMatrix AB = MUL(A, Pinv);
    CMatrix ABT = AB.ConjugateTranspose();
    if (!Compare(ABT, AB)) {
        // std::cout << "Condition 3 fail!" << std::endl;
        return false;
    }
    // 4. Verify transpose(BA) = BA
    CMatrix BA = MUL(Pinv, A);
    CMatrix BAT = BA.ConjugateTranspose();
    if (!Compare(BAT, BA)) {
        // std::cout << "Condition 4 fail!" << std::endl;
        return false;
    }
    return true;
}

/* NON-MEMBER FUNCTIONS
 *  ********************************/

CMatrix operator+(const CMatrix& m1, const CMatrix& m2) {
    CMatrix temp(m1);
    return (temp += m2);
}

CMatrix operator-(const CMatrix& m1, const CMatrix& m2) {
    CMatrix temp(m1);
    return (temp -= m2);
}

CMatrix operator*(const CMatrix& m1, const CMatrix& m2) {
    CMatrix temp(m1);
    return (temp *= m2);
}

CMatrix operator*(const CMatrix& m, float num) {
    CMatrix temp(m);
    return (temp *= num);
}

CMatrix operator*(float num, const CMatrix& m) {
    return (m * num);
}

CMatrix operator/(const CMatrix& m, float num) {
    CMatrix temp(m);
    return (temp /= num);
}

#endif
