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
    // population mcmc fintech model
    PopMCMC popmcmc;

    int retval = XLNX_OK;

    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    std::vector<Device*> deviceList;
    Device* pChosenDevice;

    deviceList = DeviceManager::getDeviceList("u250");

    if (deviceList.size() == 0) {
        printf("No matching devices found\n");
        exit(0);
    }

    printf("Found %zu matching devices\n", deviceList.size());

    // we'll just pick the first device in the
    pChosenDevice = deviceList[0];

    if (retval == XLNX_OK) {
        // turn off trace output...turn it on here if you want extra debug output
        Trace::setEnabled(true);
    }

    printf("\n\n\n");
    printf("[XF_FINTECH] PopMCMC trying to claim device...\n");

    start = std::chrono::high_resolution_clock::now();

    retval = popmcmc.claimDevice(pChosenDevice);

    end = std::chrono::high_resolution_clock::now();

    if (retval == XLNX_OK) {
        printf("[XF_FINTECH] Device setup time = %lld microseconds\n",
               (long long int)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    } else {
        printf("[XF_FINTECH] Failed to claim device - error = %d\n", retval);
    }

    static const int maxSamples = 5000;
    int numSamples = 5000;
    int numBurnInSamples = 500;
    double sigma = 0.4;
    double outputData[maxSamples];
    long long int lastRuntime = 0;
    FILE* fp;

    retval = popmcmc.run(numSamples, numBurnInSamples, sigma, outputData);
    lastRuntime = popmcmc.getLastRunTime();

    fp = fopen("pop_mcmc_output.csv", "wb");
    if (retval == XLNX_OK) {
        for (int i = 0; i < (numSamples - numBurnInSamples); i++) {
            printf("%f\n", outputData[i]);

            // write to file
            fprintf(fp, "%lf\n", outputData[i]);
            if (i == ((numSamples - numBurnInSamples) - 1)) {
                fprintf(fp, "%lf", outputData[i]);
            }
        }
        printf("[XF_FINTECH] ExecutionTime = %lld microseconds\n", lastRuntime);
    }

    fclose(fp);

    printf("[XF_FINTECH] PopMCMC releasing device...\n");
    retval = popmcmc.releaseDevice();

    return 0;
}
