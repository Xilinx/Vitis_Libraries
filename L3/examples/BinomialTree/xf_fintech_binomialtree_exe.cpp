/*--
 * ---------------------------------------------------------------------------------------------------------------------*/
/*-- DISCLAIMER AND CRITICAL APPLICATIONS */
/*--
 * ---------------------------------------------------------------------------------------------------------------------*/
/*-- */
/*-- (c) Copyright 2019 Xilinx, Inc. All rights reserved. */
/*-- */
/*-- This file contains confidential and proprietary information of Xilinx, Inc.
 * and is protected under U.S. and          */
/*-- international copyright and other intellectual property laws. */
/*-- */
/*-- DISCLAIMER */
/*-- This disclaimer is not a license and does not grant any rights to the
 * materials distributed herewith. Except as      */
/*-- otherwise provided in a valid license issued to you by Xilinx, and to the
 * maximum extent permitted by applicable     */
/*-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS,
 * AND XILINX HEREBY DISCLAIMS ALL WARRANTIES  */
/*-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED
 * TO WARRANTIES OF MERCHANTABILITY, NON-     */
/*-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall
 * not be liable (whether in contract or tort,*/
/*-- including negligence, or under any other theory of liability) for any loss
 * or damage of any kind or nature           */
/*-- related to, arising under or in connection with these materials, including
 * for any direct, or any indirect,          */
/*-- special, incidental, or consequential loss or damage (including loss of
 * data, profits, goodwill, or any type of      */
/*-- loss or damage suffered as a retVal of any action brought by a third party)
 * even if such damage or loss was          */
/*-- reasonably foreseeable or Xilinx had been advised of the possibility of the
 * same.                                    */
/*-- */
/*-- CRITICAL APPLICATIONS */
/*-- Xilinx products are not designed or intended to be fail-safe, or for use in
 * any application requiring fail-safe      */
/*-- performance, such as life-support or safety devices or systems, Class III
 * medical devices, nuclear facilities,       */
/*-- applications related to the deployment of airbags, or any other
 * applications that could lead to death, personal      */
/*-- injury, or severe property or environmental damage (individually and
 * collectively, "Critical                         */
/*-- Applications"). Customer assumes the sole risk and liability of any use of
 * Xilinx products in Critical               */
/*-- Applications, subject only to applicable laws and regulations governing
 * limitations on product liability.            */
/*-- */
/*-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
 * AT ALL TIMES.                             */
/*--
 * ---------------------------------------------------------------------------------------------------------------------*/

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
    deviceList = DeviceManager::getDeviceList("u200");

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
