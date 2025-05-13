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
#ifndef _DSPLIB_TEST_UTILS_HPP_
#define _DSPLIB_TEST_UTILS_HPP_

/*
This file holds the definition of the dds_mixer
Reference model graph.
*/

#include <adf.h>
#include <vector>
#include "device_defs.h"
#include "test_stim.hpp"
#include "graph_utils.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
using namespace adf;

template <unsigned int ssr, unsigned int dual, typename plioType, unsigned int myPlioWidth = 64>
void createPLIOFileConnections(std::array<plioType, ssr*(dual + 1)>& plioPorts,
                               std::string filename,
                               std::string plioDescriptor = "in") {
    plio_type plioAlias = (myPlioWidth == 64) ? adf::plio_64_bits : adf::plio_32_bits;
    for (unsigned int ssrIdx = 0; ssrIdx < ssr; ++ssrIdx) {
        for (unsigned int dualIdx = 0; dualIdx < (dual + 1); ++dualIdx) {
            std::string filenameInternal = filename;
            // Insert SSR index and dual stream index into filename before extension (.txt)
            filenameInternal.insert(filenameInternal.length() - 4,
                                    ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
            plioPorts[ssrIdx * (dual + 1) + dualIdx] = plioType::create(
                "PLIO_" + plioDescriptor + "_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx),
                (myPlioWidth == 64) ? adf::plio_64_bits : adf::plio_32_bits, filenameInternal);
        }
    }
}

template <typename coeff_type, int stim_type, size_t tapsNo, int seed>
std::vector<coeff_type> generateTaps(std::string filename = std::string("data/input.txt")) {
    // Generate random taps
    // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
    coeff_type taps[tapsNo];
    std::vector<coeff_type> taps_v;
    test_stim<coeff_type, tapsNo, 0> taps_gen(filename);
    taps_gen.prepSeed(seed);
    taps_gen.gen(stim_type, taps);

    // Copy taps from C++ array into std::vector
    for (int i = 0; i < tapsNo; i++) {
        taps_v.push_back(taps[i]);
    }
    return taps_v;
}
// Unused parameters get a default here.
#ifndef INTERPOLATE_FACTOR
#define INTERPOLATE_FACTOR 1
#endif
#ifndef DECIMATE_FACTOR
#define DECIMATE_FACTOR 1
#endif
#ifndef P_PARA_INTERP_POLY
#define P_PARA_INTERP_POLY 1
#endif
#ifndef P_PARA_DECI_POLY
#define P_PARA_DECI_POLY 1
#endif

void printConfig() {
    // Prints UUT configuration, reading info from macros.
    printf("========================\n");
    printf("== UUT Graph Class: ");
    printf(QUOTE(UUT_GRAPH));
    printf("\n");
    printf("========================\n");
    printf("Data type               = ");
    printf(QUOTE(DATA_TYPE));
    printf("\n");
    printf("Coeff type              = ");
    printf(QUOTE(COEFF_TYPE));
    printf("\n");
    printf("Input samples           = %d \n", INPUT_SAMPLES);
    printf("Input window [B]        = %lu \n", INPUT_SAMPLES * sizeof(DATA_TYPE));
    printf("Input margin            = %lu \n", INPUT_MARGIN(FIR_LEN, DATA_TYPE));
    printf("Output samples          = %d \n", OUTPUT_SAMPLES);
    printf("FIR Length              = %d \n", FIR_LEN);
    printf("INTERPOLATE_FACTOR      = %d \n", INTERPOLATE_FACTOR);
    printf("DECIMATE_FACTOR         = %d \n", DECIMATE_FACTOR);
    printf("Shift                   = %d \n", SHIFT);
    printf("ROUND_MODE              = %d \n", ROUND_MODE);
    printf("CASC_LEN                = %d \n", CASC_LEN);
    printf("NUM_OUTPUTS             = %d \n", NUM_OUTPUTS);
    printf("USE_COEFF_RELOAD        = %d \n", USE_COEFF_RELOAD);
    printf("PORT_API                = %d \n", PORT_API);
    printf("DUAL_IP                 = %d \n", DUAL_IP);
    printf("SSR                     = %d \n", P_SSR);
    printf("PARA_INTERP_POLY        = %d \n", P_PARA_INTERP_POLY);
    printf("PARA_DECI_POLY          = %d \n", P_PARA_DECI_POLY);
    printf("Number of iterations    = %d \n", NITER);
}
}
}
}
}
#endif // _DSPLIB_TEST_UTILS_HPP_
