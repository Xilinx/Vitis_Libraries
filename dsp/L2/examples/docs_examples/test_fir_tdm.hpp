/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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

#include <adf.h>

#include "fir_tdm_graph.hpp"

#define FIR_TDM_LENGTH 8
#define FIR_TDM_CHANNELS 8
#define FIR_TDM_SHIFT 15
#define FIR_TDM_ROUND_MODE 0
#define FIR_TDM_INPUT_SAMPLES 256

using namespace adf;

namespace fir_tdm_example {

class test_fir_tdm : public graph {
   private:
    // FIR TDM coefficients - 256 - 32 TDM Channels with 8 taps per channel
    std::vector<int16> m_taps = std::vector<int16>{
        0,      5,      21,     49,     86,     133,    190,    254,    326,    403,    486,    572,    661,    750,
        839,    925,    1008,   1085,   1156,   1218,   1271,   1312,   1342,   1358,   1359,   1346,   1316,   1270,
        1208,   1128,   1031,   917,    786,    640,    478,    302,    113,    -87,    -298,   -518,   -746,   -978,
        -1213,  -1449,  -1684,  -1915,  -2140,  -2357,  -2564,  -2758,  -2936,  -3098,  -3240,  -3362,  -3460,  -3533,
        -3580,  -3600,  -3591,  -3553,  -3484,  -3386,  -3256,  -3097,  -2907,  -2688,  -2441,  -2167,  -1868,  -1544,
        -1199,  -834,   -453,   -57,    349,    765,    1186,   1609,   2030,   2447,   2855,   3251,   3632,   3995,
        4334,   4649,   4935,   5190,   5410,   5594,   5739,   5842,   5903,   5920,   5892,   5818,   5698,   5532,
        5319,   5062,   4760,   4416,   4031,   3608,   3149,   2658,   2136,   1588,   1018,   429,    -173,   -786,
        -1403,  -2022,  -2636,  -3240,  -3831,  -4403,  -4952,  -5472,  -5960,  -6410,  -6820,  -7185,  -7501,  -7766,
        -7976,  -8129,  -8224,  -8257,  -8229,  -8138,  -7985,  -7769,  -7491,  -7152,  -6754,  -6300,  -5791,  -5231,
        -4624,  -3973,  -3283,  -2559,  -1805,  -1026,  -230,   579,    1395,   2212,   3023,   3822,   4603,   5360,
        6087,   6778,   7426,   8027,   8576,   9068,   9497,   9861,   10155,  10377,  10524,  10594,  10585,  10497,
        10329,  10082,  9756,   9353,   8876,   8326,   7708,   7025,   6282,   5483,   4634,   3741,   2810,   1847,
        860,    -144,   -1158,  -2175,  -3187,  -4186,  -5164,  -6114,  -7029,  -7901,  -8723,  -9488,  -10190, -10824,
        -11384, -11864, -12260, -12569, -12788, -12913, -12943, -12877, -12714, -12455, -12100, -11651, -11111, -10484,
        -9772,  -8980,  -8114,  -7179,  -6182,  -5129,  -4028,  -2887,  -1714,  -517,   693,    1910,   3125,   4327,
        5507,   6657,   7768,   8831,   9837,   10779,  11649,  12440,  13146,  13759,  14275,  14690,  14999,  15198,
        15287,  15262,  15123,  14871,  14506,  14030,  13446,  12757,  11968,  11083,  10108,  9050,   7917,   6715,
        5454,   4142,   2790,   1405};
    // FIR Graph class
    using fir_g = xf::dsp::aie::fir::tdm::fir_tdm_graph<cint16,
                                                        int16,
                                                        FIR_TDM_LENGTH,
                                                        FIR_TDM_SHIFT,
                                                        FIR_TDM_ROUND_MODE,
                                                        FIR_TDM_INPUT_SAMPLES,
                                                        FIR_TDM_CHANNELS>;

   public:
    port<input> in;
    port<output> out;
    // Constructor - with FIR graph class initialization
    test_fir_tdm() {
        // optional location constraints on the graph kernel
        // kernel *filter_kernels = filter.getKernels();
        // runtime<ratio>(*filter_kernels) = 0.515625;
        // location<kernel>(filter_kernels[0]) = tile(LOC_XBASE, LOC_YBASE);
        //// Make connections
        // Size of window in Bytes.
        // Margin gets automatically added within the FIR graph class.
        // Margin equals to FIR length rounded up to nearest multiple of 32 Bytes.
        fir_g firTdmGraph(m_taps);
        connect<>(in, firTdmGraph.in[0]);
        connect<>(firTdmGraph.out[0], out);
    };
};
};
