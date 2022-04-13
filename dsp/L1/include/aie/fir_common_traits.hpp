/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef _DSPLIB_FIR_COMMON_TRAITS_HPP_
#define _DSPLIB_FIR_COMMON_TRAITS_HPP_

/*
Common FIR traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal classes. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
template <typename T_D, typename T_C, int T_PORTS>
constexpr unsigned int fnGetOptTapsPerKernel() {
    return -1;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<int16, int16, 1>() {
    return 16;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<int16, int16, 2>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint16, int16, 1>() {
    return 16;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint16, int16, 2>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint16, cint16, 1>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint16, cint16, 2>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<int32, int16, 1>() {
    return 16;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<int32, int16, 2>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<int32, int32, 1>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<int32, int32, 2>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, int16, 1>() {
    return 16;
}; // user must also look at max in case max < opt
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, int16, 2>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, cint16, 1>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, cint16, 2>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, int32, 1>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, int32, 2>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, cint32, 1>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cint32, cint32, 2>() {
    return 2;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<float, float, 1>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<float, float, 2>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cfloat, float, 1>() {
    return 8;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cfloat, float, 2>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cfloat, cfloat, 1>() {
    return 4;
};
template <>
constexpr unsigned int fnGetOptTapsPerKernel<cfloat, cfloat, 2>() {
    return 2;
};

enum eFIRVariant {
    kSrAsym = 0,
    kSrSym = 1,
    kIntHB = 2,
    kDecHB = 3,
    kIntAsym = 4,
    kDecAsym = 5,
    kDecSym = 6,
    kResamp = 7
};

template <unsigned int T_FIR, unsigned int T_API, typename T_D>
constexpr unsigned int fnGetMaxTapsPerKernel() {
    return -1;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 0, int16>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 0, cint16>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 0, int32>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 0, cint32>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 0, float>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 0, cfloat>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 1, int16>() {
    return 48;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 1, cint16>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 1, int32>() {
    return 24;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 1, cint32>() {
    return 12;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 1, float>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrAsym, 1, cfloat>() {
    return 12;
};

// For symmetric filters, window is used internally for the stream implementation, so even stream cases use window
// lookup.
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 0, int16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 0, cint16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 0, int32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 0, cint32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 0, float>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 0, cfloat>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 1, int16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 1, cint16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 1, int32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 1, cint32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 1, float>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kSrSym, 1, cfloat>() {
    return 256;
};

// For symmetric filters, window is used internally for the stream implementation, so even stream cases use window
// lookup.
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 0, int16>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 0, cint16>() {
    return 1024;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 0, int32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 0, cint32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 0, float>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 0, cfloat>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 1, int16>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 1, cint16>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 1, int32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 1, cint32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 1, float>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntHB, 1, cfloat>() {
    return 1024;
};

// For symmetric filters, window is used internally for the stream implementation, so even stream cases use window
// lookup.
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 0, int16>() {
    return 1024;
}; // type combo not supported
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 0, cint16>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 0, int32>() {
    return 902;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 0, cint32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 0, float>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 0, cfloat>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 1, int16>() {
    return 1024;
}; // type combo not supported
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 1, cint16>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 1, int32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 1, cint32>() {
    return 1024;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 1, float>() {
    return 500;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecHB, 1, cfloat>() {
    return 1024;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 0, int16>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 0, cint16>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 0, int32>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 0, cint32>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 0, float>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 0, cfloat>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 1, int16>() {
    return 56;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 1, cint16>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 1, int32>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 1, cint32>() {
    return 14;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 1, float>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kIntAsym, 1, cfloat>() {
    return 14;
};

// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 0,  int16>() {return 256;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 0, cint16>() {return 256;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 0,  int32>() {return 256;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 0, cint32>() {return 256;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 0,  float>() {return 256;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 0, cfloat>() {return 256;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 1,  int16>() {return 56;};//64-(8-1)*DF
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 1, cint16>() {return 28;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 1,  int32>() {return 28;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 1, cint32>() {return 14;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 1,  float>() {return 28;};
// template<> constexpr unsigned int fnGetMaxTapsPerKernel< kDecAsym, 1, cfloat>() {return 14;};

// For symmetric filters, window is used internally for the stream implementation, so even stream cases use window
// lookup.
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 0, int16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 0, cint16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 0, int32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 0, cint32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 0, float>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 0, cfloat>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 1, int16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 1, cint16>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 1, int32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 1, cint32>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 1, float>() {
    return 512;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kDecSym, 1, cfloat>() {
    return 512;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 0, int16>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 0, cint16>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 0, int32>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 0, cint32>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 0, float>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 0, cfloat>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 1, int16>() {
    return 56;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 1, cint16>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 1, int32>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 1, cint32>() {
    return 14;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 1, float>() {
    return 28;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernel<kResamp, 1, cfloat>() {
    return 14;
};

// Decimate Asym requires a special function taking TT_COEFF and TD_DECIMATE_FACTOR into account
template <unsigned int T_API, typename T_D, typename T_C, unsigned int T_DF>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym() {
    return -1;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int16, int16, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int16, int16, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int16, int16, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int16, int16, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int16, int16, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int16, int16, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, int16, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, int16, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, int16, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, int16, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, int16, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, int16, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, cint16, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, cint16, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, cint16, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, cint16, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, cint16, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint16, cint16, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int16, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int16, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int16, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int16, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int16, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int16, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int32, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int32, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int32, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int32, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int32, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, int32, int32, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int16, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int16, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int16, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int16, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int16, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int16, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint16, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint16, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint16, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint16, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint16, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint16, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int32, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int32, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int32, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int32, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int32, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, int32, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint32, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint32, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint32, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint32, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint32, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cint32, cint32, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, float, float, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, float, float, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, float, float, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, float, float, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, float, float, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, float, float, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, float, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, float, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, float, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, float, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, float, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, float, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, cfloat, 2>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, cfloat, 3>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, cfloat, 4>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, cfloat, 5>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, cfloat, 6>() {
    return 256;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<0, cfloat, cfloat, 7>() {
    return 256;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int16, int16, 2>() {
    return 64 - (8 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int16, int16, 3>() {
    return 64 - (8 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int16, int16, 4>() {
    return 64 - (8 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int16, int16, 5>() {
    return 64 - (8 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int16, int16, 6>() {
    return 64 - (8 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int16, int16, 7>() {
    return 64 - (8 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, int16, 2>() {
    return 32 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, int16, 3>() {
    return 32 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, int16, 4>() {
    return 32 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, int16, 5>() {
    return 32 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, int16, 6>() {
    return 32 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, int16, 7>() {
    return 32 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, cint16, 2>() {
    return 32 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, cint16, 3>() {
    return 32 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, cint16, 4>() {
    return 32 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, cint16, 5>() {
    return 32 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, cint16, 6>() {
    return 32 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint16, cint16, 7>() {
    return 32 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int16, 2>() {
    return 32 - (8 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int16, 3>() {
    return 32 - (8 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int16, 4>() {
    return 32 - (8 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int16, 5>() {
    return 32 - (8 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int16, 6>() {
    return 32 - (8 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int16, 7>() {
    return 32 - (8 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int32, 2>() {
    return 32 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int32, 3>() {
    return 32 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int32, 4>() {
    return 32 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int32, 5>() {
    return 32 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int32, 6>() {
    return 32 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, int32, int32, 7>() {
    return 32 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int16, 2>() {
    return 16 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int16, 3>() {
    return 16 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int16, 4>() {
    return 16 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int16, 5>() {
    return 16 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int16, 6>() {
    return 16 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int16, 7>() {
    return 16 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint16, 2>() {
    return 16 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint16, 3>() {
    return 16 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint16, 4>() {
    return 16 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint16, 5>() {
    return 16 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint16, 6>() {
    return 16 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint16, 7>() {
    return 16 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int32, 2>() {
    return 16 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int32, 3>() {
    return 16 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int32, 4>() {
    return 16 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int32, 5>() {
    return 16 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int32, 6>() {
    return 16 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, int32, 7>() {
    return 16 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint32, 2>() {
    return 16 - (2 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint32, 3>() {
    return 16 - (2 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint32, 4>() {
    return 16 - (2 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint32, 5>() {
    return 16 - (2 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint32, 6>() {
    return 16 - (2 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cint32, cint32, 7>() {
    return 16 - (2 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, float, float, 2>() {
    return 32 - (8 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, float, float, 3>() {
    return 32 - (8 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, float, float, 4>() {
    return 32 - (8 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, float, float, 5>() {
    return 32 - (8 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, float, float, 6>() {
    return 32 - (8 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, float, float, 7>() {
    return 32 - (8 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, float, 2>() {
    return 16 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, float, 3>() {
    return 16 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, float, 4>() {
    return 16 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, float, 5>() {
    return 16 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, float, 6>() {
    return 16 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, float, 7>() {
    return 16 - (4 - 1) * 7;
};

template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, cfloat, 2>() {
    return 16 - (4 - 1) * 2;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, cfloat, 3>() {
    return 16 - (4 - 1) * 3;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, cfloat, 4>() {
    return 16 - (4 - 1) * 4;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, cfloat, 5>() {
    return 16 - (4 - 1) * 5;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, cfloat, 6>() {
    return 16 - (4 - 1) * 6;
};
template <>
constexpr unsigned int fnGetMaxTapsPerKernelDecAsym<1, cfloat, cfloat, 7>() {
    return 16 - (4 - 1) * 7;
};

// Min Cascade Length
template <int T_FIR_LEN, int T_API, typename T_D>
constexpr unsigned int fnGetMinCascLenSrAsym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kSrAsym, T_API, T_D>();
    return T_FIR_LEN % kMaxTaps == 0 ? T_FIR_LEN / kMaxTaps : T_FIR_LEN / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D>
constexpr unsigned int fnGetMinCascLenSrSym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kSrSym, T_API, T_D>();
    return (T_FIR_LEN / 2) % kMaxTaps == 0 ? (T_FIR_LEN / 2) / kMaxTaps : (T_FIR_LEN / 2) / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D>
constexpr unsigned int fnGetMinCascLenIntHB() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kIntHB, T_API, T_D>();
    return ((T_FIR_LEN + 1) / 4) % kMaxTaps == 0 ? ((T_FIR_LEN + 1) / 4) / kMaxTaps
                                                 : ((T_FIR_LEN + 1) / 4) / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D>
constexpr unsigned int fnGetMinCascLenDecHB() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kDecHB, T_API, T_D>();
    return ((T_FIR_LEN + 1) / 4) % kMaxTaps == 0 ? ((T_FIR_LEN + 1) / 4) / kMaxTaps
                                                 : ((T_FIR_LEN + 1) / 4) / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D, int T_IF>
constexpr unsigned int fnGetMinCascLenIntAsym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kIntAsym, T_API, T_D>();
    return (T_FIR_LEN / T_IF) % kMaxTaps == 0 ? (T_FIR_LEN / T_IF) / kMaxTaps : (T_FIR_LEN / T_IF) / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D, typename T_C, int T_DF>
constexpr unsigned int fnGetMinCascLenDecAsym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernelDecAsym<T_API, T_D, T_C, T_DF>();
    return (T_FIR_LEN) % kMaxTaps == 0 ? (T_FIR_LEN) / kMaxTaps : (T_FIR_LEN) / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D, int T_DF>
constexpr unsigned int fnGetMinCascLenDecSym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kDecSym, T_API, T_D>();
    return (T_FIR_LEN * T_DF / 2) % kMaxTaps == 0 ? (T_FIR_LEN * T_DF / 2) / kMaxTaps
                                                  : (T_FIR_LEN * T_DF / 2) / kMaxTaps + 1;
};
template <int T_FIR_LEN, int T_API, typename T_D, int T_IF, int T_DF>
constexpr unsigned int fnGetMinCascLenResamp() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kResamp, T_API, T_D>();
    return (T_FIR_LEN * T_DF / T_IF) % kMaxTaps == 0 ? (T_FIR_LEN * T_DF / T_IF) / kMaxTaps
                                                     : (T_FIR_LEN * T_DF / T_IF) / kMaxTaps + 1;
};

// Optimal Cascade Length
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS>
constexpr unsigned int fnGetOptCascLenSrAsym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kSrAsym, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return T_FIR_LEN % kOptTaps == 0 ? T_FIR_LEN / kOptTaps : T_FIR_LEN / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS>
constexpr unsigned int fnGetOptCascLenSrSym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kSrSym, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return (T_FIR_LEN) % kOptTaps == 0 ? (T_FIR_LEN) / kOptTaps : (T_FIR_LEN) / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS>
constexpr unsigned int fnGetOptCascLenIntHB() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kIntHB, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return ((T_FIR_LEN + 1) / 4) % kOptTaps == 0 ? ((T_FIR_LEN + 1) / 4) / kOptTaps
                                                 : ((T_FIR_LEN + 1) / 4) / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS>
constexpr unsigned int fnGetOptCascLenDecHB() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kDecHB, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return ((T_FIR_LEN + 1) / 4) % kOptTaps == 0 ? ((T_FIR_LEN + 1) / 4) / kOptTaps
                                                 : ((T_FIR_LEN + 1) / 4) / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS, int T_IF>
constexpr unsigned int fnGetOptCascLenIntAsym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kIntAsym, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return (T_FIR_LEN / T_IF) % kOptTaps == 0 ? (T_FIR_LEN / T_IF) / kOptTaps : (T_FIR_LEN / T_IF) / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS, int T_DF>
constexpr unsigned int fnGetOptCascLenDecAsym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernelDecAsym<T_API, T_D, T_C, T_DF>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return (T_FIR_LEN * T_DF) % kOptTaps == 0 ? (T_FIR_LEN * T_DF) / kOptTaps : (T_FIR_LEN * T_DF) / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS, int T_DF>
constexpr unsigned int fnGetOptCascLenDecSym() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kDecSym, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return (T_FIR_LEN * T_DF / 2) % kOptTaps == 0 ? (T_FIR_LEN * T_DF / 2) / kOptTaps
                                                  : (T_FIR_LEN * T_DF / 2) / kOptTaps + 1;
};
template <int T_FIR_LEN, typename T_D, typename T_C, int T_API, int T_PORTS, int T_IF, int T_DF>
constexpr unsigned int fnGetOptCascLenResamp() {
    constexpr int kMaxTaps = fir::fnGetMaxTapsPerKernel<fir::kResamp, T_API, T_D>();
    constexpr int kRawOptTaps = fir::fnGetOptTapsPerKernel<T_D, T_C, T_PORTS>();
    constexpr int kOptTaps = kRawOptTaps < kMaxTaps ? kRawOptTaps : kMaxTaps;
    return (T_FIR_LEN) % kOptTaps == 0 ? (T_FIR_LEN) / kOptTaps : (T_FIR_LEN) / kOptTaps + 1;
};
}
}
}
}
#endif // _DSPLIB_FIR_COMMON_TRAITS_HPP_
