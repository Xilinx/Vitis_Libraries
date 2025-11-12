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
#ifndef _DSPLIB_FUNC_APPROX_HPP_
#define _DSPLIB_FUNC_APPROX_HPP_

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

#include "device_defs.h"
#include <adf.h>
#include "func_approx_traits.hpp"
#include <vector>

using namespace adf;
//#define _DSPLIB_FUNC_APPROX_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {
// Single kernel base specialization.
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD = 0>
class kernelFuncApproxClass {
   private:
    void funcApproxBasic(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
    void funcApproxBasicFloat(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
    void funcApproxLutAPI(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
    void funcApproxLutAPIFloat(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);

   public:
#ifdef _SUPPORTS_BFLOAT16_
    using TT_LUT = typename std::conditional<std::is_same<TT_DATA, bfloat16>::value, float, TT_DATA>::type;
    // useLutAPI : enable LUT API for bfloat16 or int16 types
    static constexpr int useLutAPI =
        (std::is_same<TT_DATA, bfloat16>::value || std::is_same<TT_DATA, int16>::value) ? 1 : 0;
    // isFloatingPoint : treat only bfloat16 as floating when bfloat16 support enabled
    static constexpr int isFloatingPoint =
        (std::is_same<TT_DATA, bfloat16>::value || std::is_same<TT_DATA, float>::value) ? 1 : 0;
#else
    using TT_LUT = TT_DATA;
    // No bfloat16 support: disable LUT API by default
    static constexpr int useLutAPI = 0;
    // Float path only when TT_DATA == float
    static constexpr int isFloatingPoint = std::is_same<TT_DATA, float>::value ? 1 : 0;
#endif //_SUPPORTS_BFLOAT16_
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    // If using LUT API, each 128 bits of lut is duplicated. Therefore, double size
    static constexpr unsigned int kLutSize = (2 << (TP_COARSE_BITS - ignoreTopDomainBit)) * (useLutAPI + 1);
    static constexpr TT_DATA fineMask = ((1 << TP_FINE_BITS) - 1);
    static constexpr TT_DATA coarseMask = ((1 << (TP_FINE_BITS + TP_COARSE_BITS - ignoreTopDomainBit)) - 1);
    TT_LUT* m_staticLut0 = nullptr;
    TT_LUT* m_staticLut1 = nullptr;
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&m_slopeBuff)[TP_WINDOW_VSIZE];
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&m_offsetBuff)[TP_WINDOW_VSIZE];

    // Constructor for static LUT mode
    kernelFuncApproxClass(TT_LUT (&lut_ab)[kLutSize],
                          TT_LUT (&lut_cd)[kLutSize],
                          TT_LUT (&slopeBuff)[TP_WINDOW_VSIZE],
                          TT_LUT (&offsetBuff)[TP_WINDOW_VSIZE])
        : m_slopeBuff(slopeBuff),
          m_offsetBuff(offsetBuff),
          m_staticLut0((TT_LUT*)&lut_ab[0]),
          m_staticLut1((TT_LUT*)&lut_cd[0]) {}

    // Constructor for RTP mode or when LUTs are not provided
    kernelFuncApproxClass(TT_LUT (&slopeBuff)[TP_WINDOW_VSIZE], TT_LUT (&offsetBuff)[TP_WINDOW_VSIZE])
        : m_staticLut0(nullptr), m_staticLut1(nullptr), m_slopeBuff{slopeBuff}, m_offsetBuff{offsetBuff} {}

    TT_LUT* __restrict m_rtpLutPtr0;
    TT_LUT* __restrict m_rtpLutPtr1;
    void setInLutPtr(const TT_LUT (&lut0)[kLutSize], const TT_LUT (&lut1)[kLutSize]) {
        m_rtpLutPtr0 = (TT_LUT*)lut0;
        m_rtpLutPtr1 = (TT_LUT*)lut1;
    }

    // FUNC_APPROX
    void funcApproxKernel(T_inputIF<TT_DATA> inInterface, T_outputIF<TT_DATA> outInterface);
};

// func approx - static lut buffers
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
class func_approx : public kernelFuncApproxClass<TT_DATA,
                                                 TP_COARSE_BITS,
                                                 TP_FINE_BITS,
                                                 TP_DOMAIN_MODE,
                                                 TP_WINDOW_VSIZE,
                                                 TP_SHIFT,
                                                 TP_RND,
                                                 TP_SAT,
                                                 TP_USE_LUT_RELOAD> {
   public:
    using thisKernelApproxClass = kernelFuncApproxClass<TT_DATA,
                                                        TP_COARSE_BITS,
                                                        TP_FINE_BITS,
                                                        TP_DOMAIN_MODE,
                                                        TP_WINDOW_VSIZE,
                                                        TP_SHIFT,
                                                        TP_RND,
                                                        TP_SAT,
                                                        TP_USE_LUT_RELOAD>;
    using TT_LUT = typename thisKernelApproxClass::TT_LUT;
    static constexpr unsigned int lutSize = thisKernelApproxClass::kLutSize;
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&m_slopeBuff)[TP_WINDOW_VSIZE];
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&m_offsetBuff)[TP_WINDOW_VSIZE];
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&staticLut0)[lutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&staticLut1)[lutSize];

    func_approx(TT_LUT (&lut_ab)[lutSize],
                TT_LUT (&lut_cd)[lutSize],
                TT_LUT (&slopeBuff)[TP_WINDOW_VSIZE],
                TT_LUT (&offsetBuff)[TP_WINDOW_VSIZE])
        : staticLut0(lut_ab),
          staticLut1(lut_cd),
          m_slopeBuff(slopeBuff),
          m_offsetBuff(offsetBuff),
          thisKernelApproxClass(lut_ab, lut_cd, slopeBuff, offsetBuff) {}

    static void registerKernelClass() {
        REGISTER_FUNCTION(func_approx::approx);
        REGISTER_PARAMETER(staticLut0);
        REGISTER_PARAMETER(staticLut1);
        REGISTER_PARAMETER(m_slopeBuff);
        REGISTER_PARAMETER(m_offsetBuff);
    }
    void approx(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};

// func approx - rtp lut
// Partial specialization for TP_USE_LUT_RELOAD = 1
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class func_approx<TT_DATA, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT, 1>
    : public kernelFuncApproxClass<TT_DATA,
                                   TP_COARSE_BITS,
                                   TP_FINE_BITS,
                                   TP_DOMAIN_MODE,
                                   TP_WINDOW_VSIZE,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_SAT,
                                   1> {
   public:
    using thisKernelApproxClass = kernelFuncApproxClass<TT_DATA,
                                                        TP_COARSE_BITS,
                                                        TP_FINE_BITS,
                                                        TP_DOMAIN_MODE,
                                                        TP_WINDOW_VSIZE,
                                                        TP_SHIFT,
                                                        TP_RND,
                                                        TP_SAT,
                                                        1>;
    using TT_LUT = typename thisKernelApproxClass::TT_LUT;
    static constexpr unsigned int lutSize = thisKernelApproxClass::kLutSize;
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&m_slopeBuff)[TP_WINDOW_VSIZE];
    alignas(__ALIGN_BYTE_SIZE__) TT_LUT (&m_offsetBuff)[TP_WINDOW_VSIZE];

    func_approx(TT_LUT (&slopeBuff)[TP_WINDOW_VSIZE], TT_LUT (&offsetBuff)[TP_WINDOW_VSIZE])
        : m_slopeBuff(slopeBuff), m_offsetBuff(offsetBuff), thisKernelApproxClass(slopeBuff, offsetBuff){};

    static void registerKernelClass() {
        REGISTER_FUNCTION(func_approx::approxRtp);
        REGISTER_PARAMETER(m_slopeBuff);
        REGISTER_PARAMETER(m_offsetBuff);
    }

    void approxRtp(input_buffer<TT_DATA>& __restrict inWindow,
                   output_buffer<TT_DATA>& __restrict outWindow,
                   const TT_LUT (&lut0)[lutSize],
                   const TT_LUT (&lut1)[lutSize]);
};
}
}
}
}

#endif // _DSPLIB_FUNC_APPROX_HPP_
