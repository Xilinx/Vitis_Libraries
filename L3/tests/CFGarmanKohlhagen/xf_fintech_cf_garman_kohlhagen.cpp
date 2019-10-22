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
#include "test_vectors_gk.hpp"

using namespace xf::fintech;

int check_result(float calculated, float expected, float tolerance) {
    if (std::abs(calculated - expected) > tolerance) {
        return 0;
    }
    return 1;
}

int main() {
    int retval = XLNX_OK;
    float tolerance = 0.0001;

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

    unsigned int numAssets = sizeof(test_data) / sizeof(struct test_data_type);
    std::cout << "sizeof(test_data) = " << sizeof(test_data) << std::endl;
    std::cout << "sizeof(struct test_data_type) = " << sizeof(struct test_data_type) << std::endl;
    std::cout << "sizeof(test_data) / sizeof(struct test_data_type) = "
              << sizeof(test_data) / sizeof(struct test_data_type) << std::endl;
    std::cout << "numAssets = " << numAssets << std::endl;
    CFGarmanKohlhagen cfGarmanKohlhagen(numAssets);

    retval = cfGarmanKohlhagen.claimDevice(pChosenDevice);

    if (retval == XLNX_OK) {
        // Populate the asset data...
        for (unsigned int i = 0; i < numAssets; i++) {
            cfGarmanKohlhagen.stockPrice[i] = test_data[i].s;
            cfGarmanKohlhagen.strikePrice[i] = test_data[i].k;
            cfGarmanKohlhagen.volatility[i] = test_data[i].v;
            cfGarmanKohlhagen.timeToMaturity[i] = test_data[i].t;
            cfGarmanKohlhagen.domesticRate[i] = test_data[i].r_domestic;
            cfGarmanKohlhagen.foreignRate[i] = test_data[i].r_foreign;
        }

        ///////////////////
        // Run the model...
        ///////////////////
        cfGarmanKohlhagen.run(OptionType::Call, numAssets);

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
            printf("[XLNX] | %5u | %8.5f | %8.5f | %8.5f | %8.5f | %8.5f | %8.5f |\n", i,
                   cfGarmanKohlhagen.optionPrice[i], cfGarmanKohlhagen.delta[i], cfGarmanKohlhagen.gamma[i],
                   cfGarmanKohlhagen.vega[i], cfGarmanKohlhagen.theta[i], cfGarmanKohlhagen.rho[i]);
        }

        printf(
            "[XLNX] "
            "+-------+----------+----------+----------+----------+----------+------"
            "----+\n");
        printf("[XLNX] Processed %u assets in %lld us\n", numAssets, cfGarmanKohlhagen.getLastRunTime());
    }

    cfGarmanKohlhagen.releaseDevice();

    return 0;
}
