/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_CHOLESKY_REF_UTILS_HPP_
#define _DSPLIB_CHOLESKY_REF_UTILS_HPP_

#include "device_defs.h"
#include "aie_api/utils.hpp" // for vector print function


template <typename TT>
void debugPrintMatrix(TT* matrix, unsigned int M, unsigned int N, unsigned int rowWise) {
    if (rowWise == 1) {
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                printf("%10.1f ", matrix[i*M + j]);
            }
            printf("\n");
        }
    } else if (rowWise == 0) {
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < M; i++) {
                printf("%10.1f ", matrix[i*M + j]);
            }
            printf("\n");
        }
    }
};
template <>
void debugPrintMatrix(cfloat* matrix, unsigned int M, unsigned int N, unsigned int rowWise) {
    if (rowWise == 1) {
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                printf("%10.1f ", matrix[i*M + j].real);
                printf("+ %10.1fj ", matrix[i*M + j].imag);
            }
            printf("\n");
        }
    } else if (rowWise == 0) {
        for (int j = 0; j < N; j++) {
            for (int i = 0; i < M; i++) {
                printf("%10.1f ", matrix[i*M + j].real);
                printf("+ %10.1fj ", matrix[i*M + j].imag);
            }
            printf("\n");
        }
    }
};

// Keeping these for future proofing if float16 support added
// template <typename TT>
// struct real_t {
//     using type = TT;
// };
// template <>
// struct real_t<cfloat> {
//     using type = float;
// };

template <typename TT>
TT zero() { 
    return 0;
};
template <>
cfloat zero() {
    return {0.0, 0.0};
};

template <typename TT>
float getReal(TT val) {
    return val;
};
template <>
float getReal(cfloat val) {
    return val.real;
};


template <typename TT>
float getAbs(TT num) {
    return std::abs(num);
};
template <>
float getAbs(cfloat num) {
    float real = num.real;
    float imag = num.imag;
    return std::sqrt(real*real + imag*imag);
};

template <typename TT>
TT getConj(TT num) {
    return num;
};
template <>
cfloat getConj(cfloat num) {
    cfloat conjVal = num;
    conjVal.imag = -num.imag;
    return conjVal;
};


template <typename TT>
void validateUpperTriangular(TT* matrix, int M, float tolerance) {
    for (int i = 1; i < M; i++) {
        for (int j = 0; j < i; j++) {
            if (getAbs<TT>(matrix[i*M + j]) > tolerance) {
                printf("WARNING: Input matrix is not hermetian / symmetric positive-definite.\n");
                // assert(!"Input matrix is not hermetian / symmetric positive-definite." );
            }
        }
    }
}

template <typename TT>
void makeConjugate(TT* matrix, int M) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            matrix[i*M + j] = getConj<TT>(matrix[i*M + j]);
        }
    }
}

template <typename TT>
void getTranspose(TT* matrixIn, TT* matrixOut, int M) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            matrixOut[i*M + j] = matrixIn[j*M + i];
        }
    }
}

template <typename TT>
void matrixMult(TT* matrixA, TT* matrixB, TT* matrixOut, int M) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) { 
            for (int k = 0; k < M; k++) {
                matrixOut[i*M + j] +=  matrixA[i*M + k] * matrixB[k*M + j];
            }
        }
    }
}

template <typename TT>
void compareMatrices(TT* matrixA, TT* matrixB, int M, float tolerance) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) { 
            if (getAbs<TT>( matrixA[i*M + j] - matrixB[i*M + j] ) > tolerance) {
                printf("WARNING: chol.T @ chol != original matrix with tolerance of %3f. Comparison failed with %3f.\n", 
                tolerance, getAbs<TT>( matrixA[i*M + j] - matrixB[i*M + j] ) );
                // assert(!"chol.T @ chol != original matrix with tolerance of 0.1" );
            }
        }
    }
}

#endif