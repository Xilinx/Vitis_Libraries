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

#include "clarke_direct.h"
#include <math.h>

/**
 * \def TEST_SIZE
 * Loop count for the testbench.
 */
#define TEST_SIZE 1000
/** \brief Values of Icf err percetage to test Clarke_Direct() with. */
float ERR_SYS[2] = {1.0, 0.98};

/**
 * \def MAX_VAL
 * Maximum value for 16 bit signed integer.
 */
#define MAX_VAL 32257

/**
 * \def M_PI
 * The value of \f$\pi\f$ as a floating point number.
 */
#define M_PI 3.14159265358979323846

using namespace hls;

/**
 * \brief Main function of the C testbench.
 *
 * The function Clarke_Direct() will be called 1000 times
 * with different values for \f$i_a\f$ and \f$i_b\f$
 * and the results will be printed along with separately calculated values.
 */
int main() {
    hls::stream<int64_t> inputStream;

    hls::stream<int64_t> outputStream;

    int64_t tx_data, rx_data;
    int16_t Ia, Ib, Ic, Ialpha, Ibeta, Ihomop;

    int i, t;
    float Iaf, Ibf, Icf, Theta_a, Theta_b;
    float Ialphaf, Ibetaf, Ihomopf;

    float max_err_a = 0;
    float max_err_b = 0;
    float max_err_c = 0;

    for (t = 0; t < 2; t++) {
        float err_sys = ERR_SYS[t];
        for (i = 0; i < TEST_SIZE; i++) {
            Theta_a = ((2.0 * M_PI) / float(TEST_SIZE)) * i;
            Iaf = MAX_VAL * cos(Theta_a);
            Ibf = MAX_VAL * cos(Theta_a + 2 * M_PI / 3);
            Icf = err_sys * MAX_VAL * cos(Theta_a - 2 * M_PI / 3); // Manually add 2% error

#ifdef TWOPHASESYSTEM
            tx_data = 0;
            tx_data = (int32_t(round(Ibf)) << 16) | (int32_t(round(Iaf)) & 0x0000FFFF);
            //----------------------------------------------------------------------------
            // Call the RTL function, prepare input values and read the result
            //
            inputStream << tx_data; // send test data to input stream to be read by the function implemented in RTL
            Clarke_Direct_axi(inputStream, outputStream); // This function is executed as RTL simulation
            outputStream.read(rx_data);
            // End of function that interact with the function in RTL
            //-----------------------------------------------------------------------------

            Ialphaf = Iaf;
            Ibetaf = (Iaf + 2.0 * Ibf) / sqrt(3.0);
            Ihomopf = 0.0;

#else // TWOPHASESYSTEM
            tx_data = ((int64_t(Theta_a) << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48] //no use in this case
                      ((int64_t(round(Icf)) << 32) & 0x0000FFFF00000000) | // Put Icf bits[47:32]
                      ((int64_t(round(Ibf)) << 16) & 0x00000000FFFF0000) | // Put Ibf bits[31:16]
                      (int64_t(round(Iaf)) & 0x000000000000FFFF);          // Put Iaf bits[15:0]
            //----------------------------------------------------------------------------
            // Call the RTL function, prepare input values and read the result
            //
            inputStream << tx_data; // send test data to input stream to be read by the function implemented in RTL
            Clarke_Direct_axi(inputStream, outputStream); // This function is executed as RTL simulation
            outputStream.read(rx_data);
            // End of function that interact with the function in RTL
            //-----------------------------------------------------------------------------

            Ialphaf = (2.0 * Iaf - 1.0 * (Ibf - Icf)) / 3.0;
            Ibetaf = 2.0 * (Ibf - Icf) / sqrt(3.0);
            Ihomopf = 2.0 / 3 * (Iaf + Ibf + Icf);

            // round the Ibetaf in ()
            Ibetaf = (Ibetaf > 32767) ? (Ibetaf - 65536) : Ibetaf;
            Ibetaf = (Ibetaf < -32767) ? (Ibetaf + 65536) : Ibetaf;

#endif

            Ialpha = int16_t(rx_data & 0xFFFF);
            Ibeta = int16_t((rx_data & 0x00000000FFFF0000) >> 16);
            Ihomop = int16_t((rx_data & 0x0000FFFF00000000) >> 32);

            // err_a += (Ialphaf - Ialpha);
            max_err_a = std::max(fabs(max_err_a), fabs(Ialphaf - Ialpha));
            max_err_b = std::max(fabs(max_err_b), fabs(Ibetaf - Ibeta));
            max_err_c = std::max(fabs(max_err_c), fabs(Ihomopf - Ihomop));

#ifdef TWOPHASESYSTEM
            printf("\n%d Ialpha=%d(%.1f) Ibeta=%d(%.1f)\n", i, Ialpha, Ialphaf, Ibeta, Ibetaf);
// Ialpha=32241(32241.1) (%) Ib=-15243(-15243.1) Ibeta=1013(1013.2) (%)

#else
            printf("\n%d with (%.2f)*Icf, Ialpha=%d(%.1f) Ibeta=%d(%.1f) Ihomop=%d(%.1f) Icf=(%.1f)\n", i, err_sys,
                   Ialpha, Ialphaf, Ibeta, Ibetaf, Ihomop, Ihomopf, Icf);
#endif
        }
        printf("\n max_err_a = (%.1f) , max_err_b = (%.1f),  max_err_c = (%.1f)\n", max_err_a, max_err_b, max_err_c);
    }

    if ((max_err_a < 2.0f) && (max_err_b < 2.0f) && (max_err_c < 2.0f))
        return 0;
    else
        return 1;
}
