/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef _DUT_CHOLESKY_INV_HPP_
#define _DUT_CHOLESKY_INV_HPP_

#include "hls_x_complex.h"
#include "utils/x_hls_traits.h"
#include "kernel_cholesky.hpp"
#include <limits>
#include <stdio.h>

#define STRINGIZE(x) #x
#define TOSTR(x) STRINGIZE(x)
#define DATA_PATH TOSTR(_DATA_PATH)

template <typename T>
struct testbench_traits {
    typedef unsigned ULP_T;
    typedef double A_STIMULI_T;
    typedef T BASE_T;

    // // I need this to transpose a real matrix.  For complex testbenches, I need to pass
    // // CONJUGATE so this sets the correct value at compile time.
    // typedef hls::matrix_transpose_type<T> __TRANSPOSE__T_;
    // static const typename __TRANSPOSE__T_::transpose_t TRANSPOSE_TYPE = __TRANSPOSE__T_::TRANSPOSE;
    //
    //
    // // I need to set the type for L_expected_raw, which is the output of the LAPACK functions.
    // // If T is fized point then it is to be of type float, else T
};

template <typename T>
struct testbench_traits<hls::x_complex<T> > {
    typedef hls::x_complex<unsigned> ULP_T;
    typedef hls::x_complex<double> A_STIMULI_T;
    typedef T BASE_T;

    // // I need this to conjugate transpose a complex matrix
    // typedef hls::matrix_transpose_type<hls::x_complex<T> > __TRANSPOSE__T_;
    // static const typename __TRANSPOSE__T_::transpose_t TRANSPOSE_TYPE = __TRANSPOSE__T_::CONJUGATE;
};

template <typename T>
struct testbench_traits<std::complex<T> > {
    typedef std::complex<unsigned> ULP_T;
    typedef std::complex<double> A_STIMULI_T;
    typedef T BASE_T;

    // // I need this to conjugate transpose a complex matrix
    // typedef hls::matrix_transpose_type<std::complex<T> > __TRANSPOSE__T_;
    // static const typename __TRANSPOSE__T_::transpose_t TRANSPOSE_TYPE = __TRANSPOSE__T_::CONJUGATE;
};

// I need to set the type for L_expected_raw, which is the output of the LAPACK functions.
// If T is fixed point then it is to be of type float, else T

template <typename T>
struct lapack_interface {
    typedef T L_TYPE;
    typedef T L_BASE_TYPE;

    static void identify() { printf(" template <typename T> struct lapack_interface\n"); }
};

template <typename T>
struct lapack_interface<hls::x_complex<T> > {
    typedef hls::x_complex<T> L_TYPE;
    typedef T L_BASE_TYPE;

    static void identify() { printf(" template <typename T> struct lapack_interface< hls::x_complex<T>\n"); }
};

template <typename T>
struct lapack_interface<std::complex<T> > {
    typedef std::complex<T> L_TYPE;
    typedef T L_BASE_TYPE;

    static void identify() { printf(" template <typename T> struct lapack_interface< std::complex<T>\n"); }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct lapack_interface<ap_fixed<W, I, Q, O, N> > {
    typedef double L_TYPE;
    typedef double L_BASE_TYPE;

    static void identify() { printf(" template <int W, int I> struct lapack_interface<ap_fixed<int W, int I> >{\n"); }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct lapack_interface<hls::x_complex<ap_fixed<W, I, Q, O, N> > > {
    typedef hls::x_complex<double> L_TYPE;
    typedef double L_BASE_TYPE;

    static void identify() {
        printf(" template <int W, int I> struct lapack_interface< hls::x_complex<ap_fixed<W, I> > >{\n");
    }
};

template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct lapack_interface<std::complex<ap_fixed<W, I, Q, O, N> > > {
    typedef std::complex<double> L_TYPE;
    typedef double L_BASE_TYPE;

    static void identify() {
        printf(" template <int W, int I> struct lapack_interface< std::complex<ap_fixed<W, I> > >{\n");
    }
};

typedef testbench_traits<MATRIX_IN_T>::BASE_T MATRIX_IN_BASE_T;
typedef lapack_interface<MATRIX_OUT_T>::L_TYPE L_TYPE;
typedef lapack_interface<MATRIX_OUT_T>::L_BASE_TYPE L_BASE_TYPE;

// Define the number of input matrix types to be generated
const unsigned int NUM_MAT_TYPES = 9;
const L_BASE_TYPE eps = hls::numeric_limits<MATRIX_IN_BASE_T>::epsilon();

#endif
