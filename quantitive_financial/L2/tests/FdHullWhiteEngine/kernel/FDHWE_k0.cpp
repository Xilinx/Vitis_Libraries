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

#include "FDHWE_kernel.hpp"

extern "C" void FDHWE_k0(mytype stoppingTimes[_ETSizeMax + 1],
                         mytype payerAccrualTime[_legPSizeMax + 1],
                         mytype receiverAccrualTime[_legRSizeMax + 1],
                         mytype receiverAccrualPeriod[_legRSizeMax + 1],
                         mytype iborTime[_legRSizeMax + 1],
                         mytype iborPeriod[_legRSizeMax + 1],
                         unsigned int ETSize,
                         unsigned int xGrid,
                         unsigned int legPSize,
                         unsigned int legRSize,
                         mytype a_,
                         mytype sigma_,
                         mytype theta_,
                         mytype nominal_,
                         mytype fixedRate_,
                         mytype floatingRate_,
                         unsigned int tGrid,
                         mytype NPV[1]) {
#pragma HLS INTERFACE s_axilite port = stoppingTimes bundle = control
#pragma HLS INTERFACE m_axi port = stoppingTimes bundle = gmem0 offset = slave

#pragma HLS INTERFACE s_axilite port = payerAccrualTime bundle = control
#pragma HLS INTERFACE m_axi port = payerAccrualTime bundle = gmem1 offset = slave

#pragma HLS INTERFACE s_axilite port = receiverAccrualTime bundle = control
#pragma HLS INTERFACE m_axi port = receiverAccrualTime bundle = gmem2 offset = slave

#pragma HLS INTERFACE s_axilite port = receiverAccrualPeriod bundle = control
#pragma HLS INTERFACE m_axi port = receiverAccrualPeriod bundle = gmem3 offset = slave

#pragma HLS INTERFACE s_axilite port = iborTime bundle = control
#pragma HLS INTERFACE m_axi port = iborTime bundle = gmem4 offset = slave

#pragma HLS INTERFACE s_axilite port = iborPeriod bundle = control
#pragma HLS INTERFACE m_axi port = iborPeriod bundle = gmem5 offset = slave

#pragma HLS INTERFACE s_axilite port = ETSize bundle = control
#pragma HLS INTERFACE s_axilite port = xGrid bundle = control
#pragma HLS INTERFACE s_axilite port = legPSize bundle = control
#pragma HLS INTERFACE s_axilite port = legRSize bundle = control
#pragma HLS INTERFACE s_axilite port = a_ bundle = control
#pragma HLS INTERFACE s_axilite port = sigma_ bundle = control
#pragma HLS INTERFACE s_axilite port = theta_ bundle = control
#pragma HLS INTERFACE s_axilite port = nominal_ bundle = control
#pragma HLS INTERFACE s_axilite port = fixedRate_ bundle = control
#pragma HLS INTERFACE s_axilite port = floatingRate_ bundle = control
#pragma HLS INTERFACE s_axilite port = tGrid bundle = control

#pragma HLS INTERFACE s_axilite port = NPV bundle = control
#pragma HLS INTERFACE m_axi port = NPV bundle = gmem6 offset = slave

#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::fintech::FdHullWhiteEngine<mytype, _ETSizeMax, _xGridMax, _legPSizeMax, _legRSizeMax> hw_engine;

    // ensure xGrid is always an odd number
    if (xGrid % 2 == 0) {
        xGrid += 1;
    }
    hw_engine.engineInitialization(stoppingTimes, payerAccrualTime, receiverAccrualTime, receiverAccrualPeriod,
                                   iborTime, iborPeriod, ETSize, xGrid, legPSize, legRSize, a_, sigma_, theta_,
                                   nominal_, fixedRate_, floatingRate_);
    NPV[0] = hw_engine.rollbackImplementation(hw_engine.array_, stoppingTimes[ETSize], 0.0, tGrid);
}
