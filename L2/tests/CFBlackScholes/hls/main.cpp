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
/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/**
 * @file main.cpp
 * @brief HLS unit test of wrapped close-form solver
 */

#include <cmath>
#include <iostream>
#include <vector>
#include "bs_wrapper.hpp"

#define DATA_COUNT 1
#define DATA_SIZE (DATA_COUNT * 5)

/// @brief Standard calculation of Normal CDF
///
/// This is a straightforward implementation of the Normal CDF as defined by the
/// error function erfc()
/// using the standard library implementation.
///
/// @param[in] x variable
/// @returns   Normal CDF of input variable
float normalCDF(float x) {
    // Implementation of Normal CDF
    return 0.5f * std::erfc(-x / std::sqrt(2.0f));
}

/// @brief Refence Black-Scholes calculation
///
/// This is a minimal function to return the call premium only using the BS
/// model
///
/// @param[in] s underlying
/// @param[in] v volatility (decimal form)
/// @param[in] r risk-free rate (decimal form)
/// @param[in] t time to maturity
/// @param[in] k strike price
/// @returns   call premium
float bsRef(float s, float v, float r, float t, float k) {
    // Returns the reference call premium
    float d1 = (std::log(s / k) + (r + v * v / 2.0f) * t) / (v * std::sqrt(t));
    float d2 = d1 - v * std::sqrt(t);
    return (s * normalCDF(d1) - k * normalCDF(d2) * std::exp(-r * t));
}

/// @brief     Simple helper to return a float within a range
/// @param[in] range_min Lower bound of random value
/// @param[in] range_max Upper bound of random value
/// @returns   Random double in range range_min to range_max
double random_range(double range_min, double range_max) {
    // Random double in a range
    return range_min + (rand() / (RAND_MAX / (range_max - range_min)));
}

/// @brief Main entry point to test
///
/// This is a command-line application to test the kernel.  This should not be
/// run manually, instead it is used as part of the HLS unit test.
///
int main(int argc, char** argv) {
    // Create the test data
    unsigned int num = 128;

    float* s = (float*)malloc(num * sizeof(float));
    float* v = (float*)malloc(num * sizeof(float));
    float* r = (float*)malloc(num * sizeof(float));
    float* t = (float*)malloc(num * sizeof(float));
    float* k = (float*)malloc(num * sizeof(float));

    float* ref_price = (float*)malloc(num * sizeof(float));
    float* price = (float*)malloc(num * sizeof(float));
    float* delta = (float*)malloc(num * sizeof(float));
    float* gamma = (float*)malloc(num * sizeof(float));
    float* vega = (float*)malloc(num * sizeof(float));
    float* theta = (float*)malloc(num * sizeof(float));
    float* rho = (float*)malloc(num * sizeof(float));

    // Generate data and reference
    for (unsigned int i = 0; i < num; i++) {
        s[i] = random_range(10, 200);
        v[i] = random_range(0.1, 1.0);
        r[i] = random_range(0.001, 0.2);
        t[i] = random_range(0.5, 3);
        k[i] = random_range(10, 200);

        ref_price[i] = bsRef(s[i], v[i], r[i], t[i], k[i]);
    }

    // Run the model (request only call prices)
    bs_wrapper(s, v, r, t, k, 1, num, price, delta, gamma, vega, theta, rho);

    // Check results
    double max_price_diff = 0.0f;

    // Compare reference model to the kernel
    for (unsigned int i = 0; i < num; i++) {
        double temp = 0.0f;
        if (std::abs(temp = (price[i] - ref_price[i])) > std::abs(max_price_diff)) max_price_diff = temp;
    }

    // Display the worst case difference between model and reference.  This
    // difference exists because of the
    // use of the Phi() approximation in the kernel.
    std::cout << "Tested " << num << " prices, worst case difference between reference and kernel is " << max_price_diff
              << std::endl;

    return 0;
}
