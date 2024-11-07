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
#ifndef _DSPLIB_FIR_TDM_TRAITS_HPP_
#define _DSPLIB_FIR_TDM_TRAITS_HPP_

/*
TDM FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#define NOT_SUPPORTED 0
#define SUPPORTED 1

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {
enum eArchType {
    kArchExternalMargin = 0,
    kArchExternalMarginEvenFrames,
    kArchInternalMargin,
    kArchInternalMarginEvenFrames,
};

// Calculate ASYM FIR range for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnFirRangeAsym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // make sure there's no runt filters ( lengths < 4)
    // make Stream architectures rounded to fnStreamFirRangeRound and only last in the chain possibly odd
    // TODO: make Window architectures rounded to fnNumColumnsTdm
    return fnFirRange<TP_FL, TP_CL, TP_KP, (1)>();
}
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnFirRangeRemAsym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // this is for last in the cascade
    return fnFirRangeRem<TP_FL, TP_CL, TP_KP, (1)>();
}

// Calculate ASYM FIR range offset for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnFirRangeOffsetAsym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    return fnFirRangeOffset<TP_FL, TP_CL, TP_KP, (1)>();
}

// function to return the number of lanes for a type combo, for a given IO API type
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanesTdm() {
    return fnNumLanes<TT_DATA, TT_COEFF>();
};

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnVOutSizeTdm() {
    if
        constexpr(std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int32>::value) {
            // use 8x1 VMAC with 48-bit accs.
            return (__FIR_TDM_USE_48BIT_ACC__ == 1) ? 8 : fnNumLanesTdm<TT_DATA, TT_COEFF>();
        }
    else if
        constexpr(std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint32>::value) {
            // use 4x1 VMAC with 48-bit accs.
            // Use an 8 lane MUL for AIE-ML, as it offers best performance.
            return (__FIR_TDM_USE_48BIT_ACC__ == 1)
                       ? 4
                       : (__HAS_ACCUM_PERMUTES__ == 1) ? fnNumLanesTdm<TT_DATA, TT_COEFF>() : 8;
        }
    else if
        constexpr(std::is_same<TT_DATA, int16>::value && std::is_same<TT_COEFF, int32>::value) {
            // use 16x1 VMAC with 48-bit accs.
            return (__FIR_TDM_USE_48BIT_ACC__ == 1) ? 16 : fnNumLanesTdm<TT_DATA, TT_COEFF>();
        }
    else if
        constexpr(std::is_same<TT_DATA, cint32>::value && std::is_same<TT_COEFF, int16>::value) {
            // use 2 4x2 VMACs to allow 8 int16 coeffs to be read.
            return (__HAS_ACCUM_PERMUTES__ == 1) ? 8 : fnNumLanesTdm<TT_DATA, TT_COEFF>();
        }
    else {
        //
        return fnNumLanesTdm<TT_DATA, TT_COEFF>();
    }
};
}
}
}
}
}
#endif // _DSPLIB_FIR_TDM_TRAITS_HPP_
