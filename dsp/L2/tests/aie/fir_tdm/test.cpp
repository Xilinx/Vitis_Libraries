/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
/*
This file holds the body of the test harness for single rate asymmetric FIR filter graph
*/

#include <stdio.h>
#include <vector>
#include <cstring> // For memcpy
#include "test.hpp"

xf::dsp::aie::testcase::test_graph filter;

int main(void) {
    filter.init();

// #if (USE_COEFF_RELOAD == 1)
//     for (int i = 0; i < P_SSR; i++) {
//         filter.update(filter.coeff[i], filter.m_taps[0], FIR_LEN);
//     }
//     filter.run(NITER / 2);
//     filter.wait();
//     for (int i = 0; i < P_SSR; i++) {
//         filter.update(filter.coeff[i], filter.m_taps[1], FIR_LEN);
//     }
//     filter.run(NITER / 2);
// #else
//     filter.run(NITER);
// #endif
#if (USE_COEFF_RELOAD == 1)
    int rtpPortNumber = filter.firGraph.getTotalRtpPorts();
    printf("rtpPortNumber %d \n", rtpPortNumber);
    // COEFF_TYPE* m_taps_vPtr = filter.m_taps_v.data();

    // for (unsigned int i = 0; i < filter.m_taps_v.size(); i++) {
    //     printf("m_taps_v[%d] = %d \n", i, m_taps_vPtr[i]);
    // }
    for (unsigned int i = 0; i < rtpPortNumber; i++) {
        std::vector<COEFF_TYPE> tapsForRtpPort = filter.firGraph.extractTaps(filter.m_taps_v, i);
        unsigned long tapsPerRtpPort = filter.firGraph.getTapsPerRtpPort(i);
        printf("tapsPerRtpPort[%d] %d, tapsForRtpPort.size() = %d \n", i, tapsPerRtpPort, tapsForRtpPort.size());
        COEFF_TYPE* tapsForRtpPortPtr = tapsForRtpPort.data();
        for (unsigned int j = 0; j < tapsForRtpPort.size(); j++) {
            printf("tapsForRtpPortPtr[%d][%d] = %d  \n", i, j, tapsForRtpPort[j]);
        }
        // Create a C-style array with the same size as the vector
        COEFF_TYPE tapsArray[filter.m_taps_v.size()];

        // Copy the data from the vector to the C-style array
        std::memcpy(tapsArray, tapsForRtpPort.data(), tapsForRtpPort.size() * sizeof(COEFF_TYPE));
        printf("update %d out of  %d to be done. \n", i, rtpPortNumber);
        filter.update(filter.coeff[i], tapsArray, tapsForRtpPort.size());
        // filter.update(filter.coeff[i], tapsForRtpPort.data(), tapsPerRtpPort);
        printf("update %d out of  %d done! \n", i, rtpPortNumber);
    }
    filter.run(NITER / 2);
    filter.wait();
    printf("update2 %d", rtpPortNumber);
    // for (int i = 0; i <  rtpPortNumber; i++) {
    //     std::vector<COEFF_TYPE> tapsForRtpPort = filter.firGraph.extractTaps(filter.m_taps_v, rtpPortNumber - 1 - i);
    //     // change order
    //     COEFF_TYPE* tapsForRtpPortPtr = tapsForRtpPort.data();
    //     filter.update(filter.coeff[i], tapsForRtpPortPtr, tapsPerRtpPort);
    // }
    filter.run(NITER / 2);
#else
    filter.run(NITER);
#endif

    filter.end();

    return 0;
}

// class Base {
// public:
//     virtual void display() {
//         std::cout << "Base class display method" << std::endl;
//     }
// };

// class Derived : public Base {
// public:
//     void display() override {
//         // Call the base class method
//         Base::display();
//         // Add additional functionality
//         std::cout << "Derived class display method" << std::endl;
//     }
// };

// int main() {
//     Derived obj;
//     obj.display();
//     return 0;
// }