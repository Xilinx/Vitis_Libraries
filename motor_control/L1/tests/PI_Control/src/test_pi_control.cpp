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
/*!
 * \file test_pi_control.cpp
 */
#include "pi_control.h"
#include <math.h>

/** \brief Number of values to test with. */
#define TEST_SIZE 100
//#define TEST_PID

int main() {
    int i;
    hls::stream<int16_t> inputStream;
    hls::stream<int16_t> outputStream;
    int16_t tx_data, Sp, Kp, Ki, Kd;
    int16_t rx_data;
    int16_t data_pre;
    float inf, Spf, Kpf, Kif, outf;
    int32_t GiE_prev = 0;
    int16_t Err_prev = 0;
    int32_t Mode = 0;
    float last_stable_state_err = 0;
    Sp = 1000;
    Kp = 128;   // 0.5
    Ki = 16384; // 0.25
    Kd = 16384; // 0.25
    rx_data = 0;

    Spf = float(Sp);
    Kpf = Kp / 256.0;
    Kif = Ki / 256.0;
    outf = 0.0;

    for (i = 0; i < TEST_SIZE; i++) {
        data_pre = rx_data;
        tx_data = rx_data; // init
        inputStream << tx_data;
#ifndef TEST_PID
        PI_Control_axi(inputStream, outputStream, Sp, Kp, Ki, Mode);
#else
        PID_Control_axi(inputStream, outputStream, Sp, Kp, Ki, Kd, Mode);
#endif
        outputStream.read(rx_data);

        printf("Values out=%d (%f) \n", rx_data, Spf);
    }

    last_stable_state_err = rx_data - data_pre;
    printf("last_stable_state_err=%f, when this value == 0 means the output=setpoint, system stable \n",
           last_stable_state_err);
    if ((last_stable_state_err < 2.0f))
        return 0;
    else
        return 1;
}