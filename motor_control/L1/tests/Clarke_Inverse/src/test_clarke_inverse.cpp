/*
 * Copyright 2022 Xilinx, Inc.
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

#include "clarke_inverse.h"

using namespace hls;

/** \brief Number of values to test with. */
#define TEST_SIZE 10

/** \brief Values of \f$V_\alpha\f$ to test Clarke_Inverse() with. */
int Valpha[TEST_SIZE] = {-600, 2000, 100, 555, -255, 3333, -765, 333, 200, -543};

/** \brief Values of \f$V_\beta\f$ to test Clarke_inverse() with. */
int Vbeta[TEST_SIZE] = {-888, 3000, -500, 7000, 1000, -123, -800, 9000, 789, -444};

/**
 * \brief Main function of the C testbench.
 *
 * The function Clarke_Inverse() will be called with the values of \f$V_\alpha\f$ and \f$V_\beta\f$ in #Valpha and
 * #Vbeta
 * and the results will be printed along with separately calculated values.
 */
int main() {
    int i;
    hls::stream<int64_t> inputStream;
    hls::stream<int64_t> outputStream;
    int64_t tx_data;
    int64_t rx_data;
    int16_t ia, ib, ic;
    float fa, fb, fc;
    float max_err_a = 0;
    float max_err_b = 0;
    float max_err_c = 0;

    for (i = 0; i < TEST_SIZE; i++) {
        tx_data = (int32_t(Vbeta[i]) << 16) | (int32_t(Valpha[i]) & 0x0000FFFF);
        inputStream << tx_data;

        Clarke_Inverse_axi(inputStream, outputStream);

        outputStream.read(rx_data);
        ia = int16_t(rx_data & 0xFFFF);
        ib = int16_t((rx_data & 0xFFFF0000) >> 16);
        ic = int16_t((rx_data & 0xFFFF00000000) >> 32);

        fa = float(Valpha[i]);
        fb = (-float(Valpha[i]) + sqrt(3.0) * Vbeta[i]) / 2.0;
        fc = (-float(Valpha[i]) - sqrt(3.0) * Vbeta[i]) / 2.0;
        printf("Values is Ia=%d Ib=%d Ic=%d (%f %f %f)\n", ia, ib, ic, fa, fb, fc);

        max_err_a = std::max(fabs(max_err_a), fabs(fa - ia));
        max_err_b = std::max(fabs(max_err_b), fabs(fb - ib));
        max_err_c = std::max(fabs(max_err_c), fabs(fc - ic));
    }

    if ((max_err_a < 2.0f) && (max_err_b < 2.0f) && (max_err_c < 2.0f))
        return 0;
    else
        return 1;
}
