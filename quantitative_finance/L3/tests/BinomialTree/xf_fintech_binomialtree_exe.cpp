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

#include <stdio.h>
#include <string.h>

#include <chrono>
#include <vector>

#include "xf_fintech_api.hpp"

using namespace xf::fintech;

int main() {
    // binomial tree fintech model...
    BinomialTree bt;

    int retval = XLNX_OK;

    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    std::vector<Device*> deviceList;
    Device* pChosenDevice;

    // Get a list of U250s available on the system (just because our current
    // bitstreams are built for U250s)
    deviceList = DeviceManager::getDeviceList("u250");

    if (deviceList.size() == 0) {
        printf("No matching devices found\n");
        exit(0);
    }

    printf("Found %zu matching devices\n", deviceList.size());

    // we'll just pick the first device in the...
    pChosenDevice = deviceList[0];

    if (retval == XLNX_OK) {
        // turn off trace output...turn it on here if you want extra debug output...
        Trace::setEnabled(true);
    }

    printf("\n\n\n");
    printf("[XF_FINTECH] BinomialTree trying to claim device...\n");

    double stockPrice = 110.0;
    double strikePrice = 100.0;
    double timeToMaturity = 1.0;
    double riskFreeInterest = 0.05;
    double volatilityOfVolatility = 0.2;
    double dividendYield = 0.0;
    int numberNodes = 1024 - 1; // 0 to 1023

    if (retval == XLNX_OK) {
        printf("\n");
        printf("\n");
        printf("[XF_FINTECH] ==========\n");
        printf("[XF_FINTECH] Parameters\n");
        printf("[XF_FINTECH] ==========\n");
        printf("[XF_FINTECH] stockPrice = %f\n", stockPrice);
        printf("[XF_FINTECH] strikePrice = %f\n", strikePrice);
        printf("[XF_FINTECH] timeToMaturity = %f\n", timeToMaturity);
        printf("[XF_FINTECH] riskFreeInterest = %f\n", riskFreeInterest);
        printf("[XF_FINTECH] volatilityOfVolatility = %f\n", volatilityOfVolatility);
        printf("[XF_FINTECH] dividendYield = %f\n", dividendYield);
        printf("[XF_FINTECH] numberNodes = %d\n", numberNodes);
        printf("\n");
    }

    start = std::chrono::high_resolution_clock::now();

    retval = bt.claimDevice(pChosenDevice);

    end = std::chrono::high_resolution_clock::now();

    if (retval == XLNX_OK) {
        printf("[XF_FINTECH] Device setup time = %lld microseconds\n",
               (long long int)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    } else {
        printf("[XF_FINTECH] Failed to claim device - error = %d\n", retval);
    }

    static const int numberOptions = 64;
    if (retval == XLNX_OK) {
        printf("[XF_FINTECH] Multiple Options American Put [%d]\n", numberOptions);

        xf::fintech::BinomialTreeInputDataType<double> inputData[numberOptions];
        double outputData[numberOptions];
        start = std::chrono::high_resolution_clock::now();

        double S = 110;
        double K = 100;
        double T = 1;
        double rf = 0.05;
        double V = 0.2;
        double q = 0;

        // populate some data
        for (int i = 0; i < numberOptions; i++) {
            inputData[i].S = S;
            inputData[i].K = K + i;
            inputData[i].T = T;
            inputData[i].rf = rf;
            inputData[i].V = V;
            inputData[i].q = q;
            inputData[i].N = 1024;
            if (i == 63) {
                S = 80;
                K = 85;
            } else if (i == 127) {
                S = 32;
                K = 33;
            } else if (i == 191) {
                S = 55;
                K = 60;
            }
        }

        retval = bt.run(inputData, outputData, xf::fintech::BinomialTreeAmericanPut, numberOptions);

        end = std::chrono::high_resolution_clock::now();

        if (retval == XLNX_OK) {
            for (int i = 0; i < numberOptions; i++) {
                printf("[XF_FINTECH] [%02u] OptionPrice = %f\n", i, outputData[i]);
            }
            long long int executionTime =
                (long long int)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            printf(
                "[XF_FINTECH] ExecutionTime = %lld microseconds (average %lld "
                "microseconds)\n",
                executionTime, executionTime / numberOptions);
        }
    }

    if (retval == XLNX_OK) {
        printf("[XF_FINTECH] Multiple Options American Call [%d]\n", numberOptions);

        xf::fintech::BinomialTreeInputDataType<double> inputData[numberOptions];
        double outputData[numberOptions];
        start = std::chrono::high_resolution_clock::now();

        double S = 110;
        double K = 100;
        double T = 1;
        double rf = 0.05;
        double V = 0.2;
        double q = 0;

        // populate some data
        for (int i = 0; i < numberOptions; i++) {
            inputData[i].S = S;
            inputData[i].K = K + i;
            inputData[i].T = T;
            inputData[i].rf = rf;
            inputData[i].V = V;
            inputData[i].q = q;
            inputData[i].N = 1024;
            if (i == 63) {
                S = 80;
                K = 85;
            } else if (i == 127) {
                S = 32;
                K = 33;
            } else if (i == 191) {
                S = 55;
                K = 60;
            }
        }

        retval = bt.run(inputData, outputData, xf::fintech::BinomialTreeAmericanCall, numberOptions);

        end = std::chrono::high_resolution_clock::now();

        if (retval == XLNX_OK) {
            for (int i = 0; i < numberOptions; i++) {
                printf("[XF_FINTECH] [%02u] OptionPrice = %f\n", i, outputData[i]);
            }
            long long int executionTime =
                (long long int)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            printf(
                "[XF_FINTECH] ExecutionTime = %lld microseconds (average %lld "
                "microseconds)\n",
                executionTime, executionTime / numberOptions);
        }
    }

    printf("[XF_FINTECH] BinomialTree releasing device...\n");
    retval = bt.releaseDevice();

    return 0;
}
