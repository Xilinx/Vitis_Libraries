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

#include "park_direct.h"
#include <math.h>

/** \brief Number of values to test with. */
#define TEST_SIZE 10

/** \brief Mathematical constant \f$\pi\f$. */
#define M_PI 3.14159265358979323846

/** \brief Values of \f$I_a\f$ to test Park_Direct() with. */
int Ia[TEST_SIZE] = {-600, 2000, 100, 555, -255, 3333, -765, 333, 200, -543};

/** \brief Values of \f$I_b\f$ to test Park_Direct() with. */
int Ib[TEST_SIZE] = {-888, 3000, -500, 7000, 1000, -123, -800, 9000, 789, -444};

using namespace hls;

/**
 * \brief Main function of the C testbench.
 *
 * The function Park_Direct() will be called with the values of \f$I_a\f$ and \f$I_b\f$ in #Ia and #Ib
 * and the results will be printed along with separately calculated values.
 */
int main() {
    hls::stream<int64_t> inputStream;
    hls::stream<int64_t> outputStream;
    int64_t tx_data, rx_data;
    int16_t Ialpha, Ibeta;
    int16_t Theta, RPM;
    float Ialphaf = 0;
    float Ibetaf = 0;
    float Thetaf = 0;
    float max_err_a = 0;
    float max_err_b = 0;

    Theta = 100;
    RPM = 100;
    // test ap_int
    for (int i = 0; i < TEST_SIZE; i++) {
        tx_data = ((int64_t(Theta) << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48]
                  ((int64_t(RPM) << 32) & 0x0000FFFF00000000) |   // Put RPM bits[47:32]//no use in this case
                  ((int64_t(Ib[i]) << 16) & 0x00000000FFFF0000) | // Put Ib bits[31:16]
                  (int64_t(Ia[i]) & 0x000000000000FFFF);          // Put Ia bits[15:0]
        inputStream << tx_data;

        Park_Direct_axi(inputStream, outputStream);

        outputStream.read(rx_data);
        Ialpha = int16_t(rx_data & 0xFFFF);
        Ibeta = int16_t(rx_data >> 16);

        Thetaf = ((2 * M_PI * 2) / 1000.0) * Theta;
        Ialphaf = float(Ia[i]) * cos(Thetaf) + float(Ib[i]) * sin(Thetaf);
        Ibetaf = float(Ib[i]) * cos(Thetaf) - float(Ia[i]) * sin(Thetaf);

        printf("ap_int Values is Ia=%d Ib=%d (%f %f)\n", Ialpha, Ibeta, Ialphaf, Ibetaf);

        max_err_a = std::max(fabs(max_err_a), fabs(Ialphaf - Ialpha));
        max_err_b = std::max(fabs(max_err_b), fabs(Ibetaf - Ibeta));
    }

    // test ap_fixed
    /*
    for (int i = 0; i < TEST_SIZE; i++) {
        tx_data = ((int64_t(Theta) << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48]
                  ((int64_t(RPM) << 32) & 0x0000FFFF00000000) |   // Put RPM bits[47:32]//no use in this case
                  ((int64_t((Ib[i]/333)<<QnW_park )<< 16) & 0x00000000FFFF0000) | // Put Ib bits[31:16]
                  (int64_t((Ia[i]/333)<<QnW_park )& 0x000000000000FFFF);          // Put Ia bits[15:0]
        inputStream << tx_data;

        Park_Direct_axi_ap_fixed(inputStream, outputStream);
        //Park_Direct_axi_Qmn(inputStream, outputStream);

        outputStream.read(rx_data);
        ap_int<16> Ialpha_buf = int16_t(rx_data & 0xFFFF);
        ap_int<16> Ibeta_buf = int16_t(rx_data >> 16);

        ap_fixed<(QmW_park + QnW_park + 1), (QmW_park + 1)> Ialpha_fixed, Ibeta_fixed;
        if(QmW_park + QnW_park + 1 <= out_width){
            Ialpha_fixed.range(QmW_park + QnW_park, 0) = Ialpha_buf.range(QmW_park + QnW_park, 0);
            Ibeta_fixed.range(QmW_park + QnW_park, 0) = Ibeta_buf.range(QmW_park + QnW_park, 0);
        } else {
            Ialpha_fixed = Ialpha_buf;
            Ibeta_fixed = Ibeta_buf;
        }

        Thetaf = ((2 * M_PI * 2) / 1000.0) * Theta;
        Ialphaf = float(Ia[i]/333) * cos(Thetaf) + float(Ib[i]/333) * sin(Thetaf);
        Ibetaf = float(Ib[i]/333) * cos(Thetaf) - float(Ia[i]/333) * sin(Thetaf);

        printf("ap_fixed Values is Ia=%f Ib=%f (%f %f)\n", Ialpha_fixed.to_float(), Ibeta_fixed.to_float(), Ialphaf,
    Ibetaf);

        max_err_a = std::max(fabs(max_err_a), fabs(Ialphaf - Ialpha_fixed.to_float()));
        max_err_b = std::max(fabs(max_err_b), fabs(Ibetaf - Ibeta_fixed.to_float()));
    }
    */

    if ((max_err_a < 2.0f) && (max_err_b < 2.0f))
        return 0;
    else
        return 1;
}
