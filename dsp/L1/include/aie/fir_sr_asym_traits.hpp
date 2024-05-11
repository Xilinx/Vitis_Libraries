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
#ifndef _DSPLIB_FIR_SR_ASYM_TRAITS_HPP_
#define _DSPLIB_FIR_SR_ASYM_TRAITS_HPP_

/*
Single Rate Asymetrical FIR traits.
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
namespace sr_asym {
enum eArchType { kArchBasic = 0, kArchIncLoads, kArchZigZag, kArchStream };

static constexpr unsigned int kMaxColumns = 2;
static constexpr unsigned int kUpdWSize = 32;         // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

//
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnNumLanesStream() {
    return fnNumLanes384<T_D, T_C>();
};
#if __MIN_REGSIZE__ == 128
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<int32, int16>() {
    return fnNumLanes<int32, int16>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<cint32, int16>() {
    return fnNumLanes<cint32, int16>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<cint32, int32>() {
    return fnNumLanes<cint32, int32>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<cint32, cint16>() {
    return fnNumLanes<cint32, cint16>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<float, float>() {
    return fnNumLanes<float, float>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<cfloat, float>() {
    return fnNumLanes<cfloat, float>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanesStream<cfloat, cfloat>() {
    return fnNumLanes<cfloat, cfloat>();
};
#endif
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnNumColsStream() {
    return fnNumCols384<T_D, T_C>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<int32, int16>() {
    return fnNumCols<int32, int16>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<cint32, int16>() {
    return fnNumCols<cint32, int16>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<cint32, int32>() {
    return fnNumCols<cint32, int32>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<cint32, cint16>() {
    return fnNumCols<cint32, cint16>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<float, float>() {
    return fnNumCols<float, float>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<cfloat, float>() {
    return fnNumCols<cfloat, float>();
};
template <>
INLINE_DECL constexpr unsigned int fnNumColsStream<cfloat, cfloat>() {
    return fnNumCols<cfloat, cfloat>();
};

#if __MIN_REGSIZE__ == 128
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnStreamReadWidth() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<int32, int16>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint32, int16>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint32, int32>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint32, cint16>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<float, float>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cfloat, float>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cfloat, cfloat>() {
    return 256;
};
#endif
#if __MIN_REGSIZE__ == 256
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnStreamReadWidth() {
    return 256;
};
#endif

// align to num cols coeffs for FIR cascade splitting for optimal mac efficiency
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnStreamFirRangeRound() {
    return fnNumColsStream<T_D, T_C>();
}

// Calculate ASYM FIR range for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnFirRangeAsym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // make sure there's no runt filters ( lengths < 4)
    // make Stream architectures rounded to fnStreamFirRangeRound and only last in the chain possibly odd
    // TODO: make Window architectures rounded to fnNumColumnsSrAsym
    return fnFirRange<TP_FL, TP_CL, TP_KP, ((TP_API == 1) ? (fnStreamFirRangeRound<TT_DATA, TT_COEFF>()) : 1)>();
}
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnFirRangeRemAsym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // this is for last in the cascade
    return fnFirRangeRem<TP_FL, TP_CL, TP_KP, ((TP_API == 1) ? (fnStreamFirRangeRound<TT_DATA, TT_COEFF>()) : 1)>();
}

// Calculate ASYM FIR range offset for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnFirRangeOffsetAsym() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    return fnFirRangeOffset<TP_FL, TP_CL, TP_KP, ((TP_API == 1) ? (fnStreamFirRangeRound<TT_DATA, TT_COEFF>()) : 1)>();
}

// function to return the number of lanes for a type combo, for a given IO API type
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnNumLanesSrAsym() {
    return (TP_API == 0 ? fnNumLanes<TT_DATA, TT_COEFF>() : fnNumLanesStream<TT_DATA, TT_COEFF>());
};

// function to return the number of columns for a type combo for the intrinsics used for this single rate asym FIR
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnNumColumnsSrAsym() {
    return (TP_API == 0 ? fnNumCols<TT_DATA, TT_COEFF>() : fnNumCols384<TT_DATA, TT_COEFF>());
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnDataLoadsInRegSrAsym() {
    // To fill a full 1024-bit input vector register using a 256-bit upd_w command it takes 4 load operations.
    // Always return 4, the exception (handled by explicit template instantiation)
    // would be needed it a different command was used (e.g. upd_v, upd_x).
    return (TP_API == 0 ? 4 : 1024 / fnStreamReadWidth<TT_DATA, TT_COEFF>());
}

// Parameter to constant resolution functions
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnDataLoadVsizeSrAsym() {
    return (TP_API == 0 ? (256 / 8 / sizeof(TT_DATA)) : fnStreamReadWidth<TT_DATA, TT_COEFF>() / 8 / sizeof(TT_DATA));
}

// Helper function to define max ZigZag Coefficient array
template <typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnZigZagMaxCoeffByteSize() {
    return 256;
}

// function to return the number of samples in an output vector for a type combo
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnVOutSizeSrAsym() {
    return fnNumLanesSrAsym<TT_DATA, TT_COEFF, TP_API>();
};

// Support for stream interface for a give data/coeff type combination
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_API>
INLINE_DECL constexpr unsigned int fnTypeStreamSupport() {
    return SUPPORTED;
};
// Exclude cint32/cint32 combo.
// template<> INLINE_DECL constexpr unsigned int fnTypeStreamSupport< cint32,  cint32, 1>(){  return NOT_SUPPORTED;};
}
}
}
}
}
#endif // _DSPLIB_FIR_SR_ASYM_TRAITS_HPP_
