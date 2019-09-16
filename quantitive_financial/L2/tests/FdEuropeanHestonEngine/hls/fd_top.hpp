/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef _FD_TOP_H_
#define _FD_TOP_H_

const unsigned int m1 = 16;
const unsigned int m2 = 8;
const unsigned int n = 200;
const unsigned int m_size = 128;
const unsigned int log2_m_size = 7;
const unsigned int a_size = 1280;
const unsigned int a1_dim = 3;
const unsigned int a2_dim = 5;

const unsigned int a_nnz = 896;

void fd_top(double A[a_size],
            unsigned int Ar[a_size],
            unsigned int Ac[a_size],
            unsigned int nnz,
            double A1[m_size][a1_dim],
            double A2[m_size][a2_dim],
            double X1[m_size][a1_dim],
            double X2[m_size][a2_dim],
            double b[m_size],
            double u0[m_size],
            unsigned int M1,
            unsigned int M2,
            unsigned int N,
            double price[m_size]);

#endif