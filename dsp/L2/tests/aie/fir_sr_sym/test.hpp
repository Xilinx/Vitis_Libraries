#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

/*
This file holds the declaraion of the test harness graph class for the
Single Rate Symmetrical FIR graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_sr_sym_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

class test_graph : public graph {
   private:
    static constexpr unsigned int kNumTaps = (FIR_LEN + 1) / 2;
    COEFF_TYPE taps[kNumTaps];

   public:
    port<input> in;
    port<output> out;
#if (NUM_OUTPUTS == 2)
    port<output> out2;
#endif
#if (USE_COEFF_RELOAD == 1)
    port<input> coeff;
#endif

    COEFF_TYPE m_taps[2][kNumTaps];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples   = %d \n", INPUT_SAMPLES);
        printf("Input window [B]= %lu \n", INPUT_SAMPLES * sizeof(DATA_TYPE));
        printf("Input margin    = %lu \n", INPUT_MARGIN(FIR_LEN, DATA_TYPE));
        printf("Output samples  = %d \n", OUTPUT_SAMPLES);
        printf("FIR Length      = %d \n", FIR_LEN);
        printf("Shift           = %d \n", SHIFT);
        printf("ROUND_MODE      = %d \n", ROUND_MODE);
        printf("CASC_LEN        = %d \n", CASC_LEN);
        printf("NUM_OUTPUTS     = %d \n", NUM_OUTPUTS);
        printf("Data type       = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Coeff type      = ");
        printf(QUOTE(COEFF_TYPE));
        printf("\n");
        namespace dsplib = xf::dsp::aie;

        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, kNumTaps, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
        int error_tap =
            rand() %
            kNumTaps; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
#ifdef _DSPLIB_FIR_DEBUG_ADL_
        error_tap = kNumTaps - 1; // Always overwrite the last coeff only.
#endif                            // _DSPLIB_FIR_DEBUG_ADL_
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(STIM_TYPE, taps);
            for (int i = 0; i < kNumTaps; i++) {
                m_taps[j][i] = taps[i];
                if (i == error_tap && j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
            }
        }
        // Copy taps from C++ array into std::vector
        for (int i = 0; i < kNumTaps; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }

// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        dsplib::fir::sr_sym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, SHIFT, ROUND_MODE, INPUT_SAMPLES, CASC_LEN,
                                       USE_COEFF_RELOAD, NUM_OUTPUTS>
            firGraph;
#else // Static coefficients
        dsplib::fir::sr_sym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, SHIFT, ROUND_MODE, INPUT_SAMPLES, CASC_LEN,
                                       USE_COEFF_RELOAD, NUM_OUTPUTS>
            firGraph(m_taps_v);
#endif

        // Make connections
        connect<>(in, firGraph.in);
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
        // Chained connections mutually explusive with multiple outputs.
        dsplib::fir::sr_sym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, SHIFT, ROUND_MODE, INPUT_SAMPLES, CASC_LEN>
            firGraph2(m_taps_v);
        connect<>(firGraph.out, firGraph2.in);
        connect<>(firGraph2.out, out);
#else
        connect<>(firGraph.out, out);
#if (NUM_OUTPUTS == 2)
        connect<>(firGraph.out2, out2);
#endif
#endif
#if (USE_COEFF_RELOAD == 1)
        connect<>(coeff, firGraph.coeff);
#endif

#ifdef USING_UUT
        // Report out for AIE Synthesizer QoR harvest
        kernel* myKernel;
        dsplib::fir::sr_sym::fir_sr_sym<DATA_TYPE, COEFF_TYPE, FIR_LEN, SHIFT, ROUND_MODE, INPUT_SAMPLES, false, false,
                                        FIR_LEN, 0, CASC_LEN, USE_COEFF_RELOAD, NUM_OUTPUTS>* mySr;
        if (&firGraph.getKernels()[0] != NULL) {
            printf("KERNEL_ARCHS: [");
            for (int i = 0; i < CASC_LEN; i++) {
                myKernel = &firGraph.getKernels()[i];
                mySr = (dsplib::fir::sr_sym::fir_sr_sym<DATA_TYPE, COEFF_TYPE, FIR_LEN, SHIFT, ROUND_MODE,
                                                        INPUT_SAMPLES, false, false, FIR_LEN, 0, CASC_LEN,
                                                        USE_COEFF_RELOAD, NUM_OUTPUTS>*)myKernel;
                printf("%d", mySr->get_m_kArch());
                if (i == CASC_LEN - 1) {
                    printf("]\n");
                } else {
                    printf(",");
                }
            }
        }
#endif
        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
