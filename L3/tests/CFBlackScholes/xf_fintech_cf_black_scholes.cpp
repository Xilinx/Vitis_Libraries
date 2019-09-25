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

static const unsigned int numAssets = 100000;

CFBlackScholes cfBlackScholes(numAssets);

int main() {
    int retval = XLNX_OK;

    std::vector<Device*> deviceList;
    Device* pChosenDevice;

    // Get a list of U250s available on the system (just because our current
    // bitstreams are built for U250s)
    deviceList = DeviceManager::getDeviceList("u250");

    if (deviceList.size() == 0) {
        printf("[XLNX] No matching devices found\n");
        exit(0);
    }

    printf("[XLNX] Found %zu matching devices\n", deviceList.size());

    // we'll just pick the first device in the...
    pChosenDevice = deviceList[0];

    retval = cfBlackScholes.claimDevice(pChosenDevice);

    if (retval == XLNX_OK) {
        // Populate the asset data...
        for (unsigned int i = 0; i < numAssets; i++) {
            cfBlackScholes.stockPrice[i] = 100.0f;
            cfBlackScholes.strikePrice[i] = 100.0f;
            cfBlackScholes.volatility[i] = 0.1f;
            cfBlackScholes.riskFreeRate[i] = 0.025f;
            cfBlackScholes.timeToMaturity[i] = 1.0f;
        }

        ///////////////////
        // Run the model...
        ///////////////////
        cfBlackScholes.run(OptionType::Put, numAssets);

        printf(
            "[XLNX] "
            "+-------+----------+----------+----------+----------+----------+------"
            "----+\n");
        printf(
            "[XLNX] | Index |  Price   |  Delta   |  Gamma   |   Vega   |  Theta   "
            "|   Rho    |\n");
        printf(
            "[XLNX] "
            "+-------+----------+----------+----------+----------+----------+------"
            "----+\n");

        for (unsigned int i = 0; i < numAssets; i++) {
            printf("[XLNX] | %5u | %8.5f | %8.5f | %8.5f | %8.5f | %8.5f | %8.5f |\n", i, cfBlackScholes.optionPrice[i],
                   cfBlackScholes.delta[i], cfBlackScholes.gamma[i], cfBlackScholes.vega[i], cfBlackScholes.theta[i],
                   cfBlackScholes.rho[i]);
        }

        printf(
            "[XLNX] "
            "+-------+----------+----------+----------+----------+----------+------"
            "----+\n");
        printf("[XLNX] Processed %u assets in %lld us\n", numAssets, cfBlackScholes.getLastRunTime());
    }

    cfBlackScholes.releaseDevice();

    return 0;
}
