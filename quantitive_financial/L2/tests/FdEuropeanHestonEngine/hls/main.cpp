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

#include <cmath>
#include <iostream>
#include <vector>

#include "fd_top.hpp"
#include "test_vectors.hpp"
#include "xf_fintech/fd_solver.hpp"

void print_vector(const std::string str, const double* vIn, const int size) {
    std::cout << str << std::endl << "[";
    int d = size / 4;
    int r = size - 4 * d;

    std::cout << std::setprecision(9) << std::scientific << std::right;

    for (int i = 0; i < d; i++) {
        if (i) std::cout << " ";
        std::cout << std::setw(16) << vIn[4 * i + 0] << ", " << std::setw(16) << vIn[4 * i + 1] << ", " << std::setw(16)
                  << vIn[4 * i + 2] << ", " << std::setw(16) << vIn[4 * i + 3];
        if (!r && (i < (d - 1))) std::cout << ", ";
        if (i < (d - 1)) std::cout << std::endl;
    }
    if (r) {
        std::cout << std::endl << " ";
        for (int i = 0; i < r; i++) {
            std::cout << std::setw(16) << vIn[4 * d + i];
            if (i < (r - 1)) std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl << std::endl;
}

int main(int argc, char** argv) {
    // ***************** HLS UNIT TEST BENCH *****************************
    // Test parameters and vectors read in from header file test_vectors.h
    // *******************************************************************

    // Price as calculated by the Heston Engine
    double price[m_size];

    // The FD engine is templated with the grid size parameters and the number of
    // data words
    // that fit in a 512-bit DDR word.  This is used to vectorize the array
    // accesses.  In this
    // testbench, this additional vector wrapper is not present.
    const unsigned int float_size = sizeof(double);
    const unsigned int data_words = 64 / sizeof(double);
    const unsigned int index_width = (float_size * data_words) / 4;

    std::cout << std::endl;
    std::cout << "          ************************************" << std::endl;
    std::cout << "                      Heston Model            " << std::endl;
    std::cout << "           Finite Difference estimation using " << std::endl;
    std::cout << "                    ADI method (Douglas)      " << std::endl;
    std::cout << "          ************************************" << std::endl;
    std::cout << std::endl;

    // Call the kernel with the precanned header file data
    fd_top(A, Ar, Ac, a_nnz, A1, A2, X1, X2, b, u0, m1, m2, n, price);

    // Calculate and display maximum difference between calculated price and
    // reference
    double diff[m_size] = {};
    double maxv = 0.0;
    int maxi = 0;
    for (int i = 0; i < m_size; ++i) {
        diff[i] = price[i] - ref[i];
        if (std::abs(diff[i]) > std::abs(maxv)) {
            maxv = diff[i];
            maxi = i;
        }
    }

    // Display the resulting grid and difference
    print_vector("Difference between calculated price and reference...", diff, m_size);
    std::cout << "Maximum difference is " << maxv << ", found at array index " << maxi << std::endl << std::endl;

    return 0;
}
