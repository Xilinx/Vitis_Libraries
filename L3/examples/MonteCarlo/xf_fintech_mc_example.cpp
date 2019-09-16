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

#include "xf_fintech_mc_example.hpp"

#include "xf_fintech_api.hpp"

using namespace xf::fintech;

//
// Here are our the objects that represent our the Fintech models we will be
// using...
MCEuropean mcEuropean;
MCAmerican mcAmerican;

double varianceMultiplier;

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

    if (retval == XLNX_OK) {
        // turn off trace output...turn it on here if you want extra debug output...
        Trace::setEnabled(false);
    }

    if (retval == XLNX_OK) {
        retval = MCDemoRunEuropeanSingle(pChosenDevice, &mcEuropean);
    }

    if (retval == XLNX_OK) {
        retval = MCDemoRunEuropeanMultiple1(pChosenDevice, &mcEuropean);
    }

    if (retval == XLNX_OK) {
        retval = MCDemoRunEuropeanMultiple2(pChosenDevice, &mcEuropean);
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Now switch to MC American...
    //////////////////////////////////////////////////////////////////////////////////////////

    if (retval == XLNX_OK) {
        retval = MCDemoRunAmericanSingle(pChosenDevice, &mcAmerican);
    }

    return 0;
}
