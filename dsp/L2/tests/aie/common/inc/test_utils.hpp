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
#include "plio_file_connections.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
using namespace adf;

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
#ifdef INPUT_SAMPLES
    printf("Input samples           = %d \n", INPUT_SAMPLES);
#endif
#ifdef INPUT_MARGIN
    printf("Input window [B]        = %lu \n", INPUT_SAMPLES * sizeof(DATA_TYPE));
    printf("Input margin            = %lu \n", INPUT_MARGIN(FIR_LEN, DATA_TYPE));
#endif
#ifdef OUTPUT_SAMPLES
    printf("Output samples          = %d \n", OUTPUT_SAMPLES);
#endif
#ifdef FIR_LEN
    printf("FIR Length              = %d \n", FIR_LEN);
#endif
#ifdef INTERPOLATE_FACTOR
    printf("INTERPOLATE_FACTOR      = %d \n", INTERPOLATE_FACTOR);
#endif
#ifdef DECIMATE_FACTOR
    printf("DECIMATE_FACTOR         = %d \n", DECIMATE_FACTOR);
#endif
#ifdef SHIFT
    printf("Shift                   = %d \n", SHIFT);
#endif
#ifdef ROUND_MODE
    printf("ROUND_MODE              = %d \n", ROUND_MODE);
#endif
#ifdef CASC_LEN
    printf("CASC_LEN                = %d \n", CASC_LEN);
#endif
#ifdef NUM_OUTPUTS
    printf("NUM_OUTPUTS             = %d \n", NUM_OUTPUTS);
#endif
#ifdef USE_COEFF_RELOAD
    printf("USE_COEFF_RELOAD        = %d \n", USE_COEFF_RELOAD);
#endif
#ifdef PORT_API
    printf("PORT_API                = %d \n", PORT_API);
#endif
#ifdef DUAL_IP
    printf("DUAL_IP                 = %d \n", DUAL_IP);
#endif
#ifdef P_SSR
    printf("SSR                     = %d \n", P_SSR);
#endif
#ifdef P_PARA_INTERP_POLY
    printf("PARA_INTERP_POLY        = %d \n", P_PARA_INTERP_POLY);
#endif
#ifdef P_PARA_DECI_POLY
    printf("PARA_DECI_POLY          = %d \n", P_PARA_DECI_POLY);
#endif
#ifdef NITER
    printf("Number of iterations    = %d \n", NITER);
#endif
}
}
}
}
}
#endif // _DSPLIB_TEST_UTILS_HPP_
