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

#include "park_inverse.h"
#include <math.h>

/** \brief Number of values to test with. */
#define TEST_SIZE 10

/** \brief Mathematical constant \f$\pi\f$. */
#define M_PI 3.14159265358979323846

/** \brief Values of \f$V_d\f$ to test Park_Inverse() with. */
int Vd[TEST_SIZE] = {-600, 2000, 100, 555, -255, 3333, -765, 333, 200, -543};

/** \brief Values of \f$V_q\f$ to test Park_Inverse() with. */
int Vq[TEST_SIZE] = {-888, 3000, -500, 7000, 1000, -123, -800, 9000, 789, -444};

using namespace hls;

/**
 * \brief Main function of the C testbench.
 *
 * The function Park_Inverse() will be called with the values of \f$V_d\f$ and \f$V_q\f$ in #Vd and #Vq
 * and the results will be printed along with separately calculated values.
 */
int main() {
    hls::stream<int64_t> inputStream;
    hls::stream<int64_t> outputStream;
    int64_t tx_data, rx_data;
    int16_t Valpha, Vbeta;
    int16_t Theta;
    float Valphaf, Vbetaf, Thetaf;
    float max_err_a = 0;
    float max_err_b = 0;

    Theta = 100;

    for (int i = 0; i < TEST_SIZE; i++) {
        tx_data = ((int64_t(0) << 48) & 0xFFFF000000000000) |     // Put Angle bits[63:48]
                  ((int64_t(Theta) << 32) & 0x0000FFFF00000000) | // Put RPM bits[47:32]//no use in this case
                  ((int64_t(Vq[i]) << 16) & 0x00000000FFFF0000) | // Put Ib bits[31:16]
                  (int64_t(Vd[i]) & 0x000000000000FFFF);          // Put Ia bits[15:0]
        inputStream << tx_data;

        Park_Inverse_axi(inputStream, outputStream);

        outputStream.read(rx_data);
        Valpha = int16_t(rx_data & 0xFFFF);
        Vbeta = int16_t(rx_data >> 16);
        // Theta = int16_t((rx_data >> 32) & 0xFFFF);

        Thetaf = ((2 * M_PI * 2) / 1000.0) * Theta;
        Valphaf = float(Vd[i]) * cos(Thetaf) - float(Vq[i]) * sin(Thetaf);
        Vbetaf = float(Vq[i]) * cos(Thetaf) + float(Vd[i]) * sin(Thetaf);

        printf("Values is Valpha=%d Vbeta=%d (%f %f)\n", Valpha, Vbeta, Valphaf, Vbetaf);

        max_err_a = std::max(fabs(max_err_a), fabs(Valphaf - Valpha));
        max_err_b = std::max(fabs(max_err_b), fabs(Vbetaf - Vbeta));
    }

    if ((max_err_a < 2.0f) && (max_err_b < 2.0f))
        return 0;
    else
        return 1;
}
