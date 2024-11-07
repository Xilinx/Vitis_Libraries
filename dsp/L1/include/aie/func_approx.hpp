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
          unsigned int TP_SAT>
class func_approx {
   private:
#ifdef _SUPPORTS_BFLOAT16_
    static constexpr int useLutAPI =
        std::is_same<TT_DATA, bfloat16>::value ? 1 : (std::is_same<TT_DATA, int16>::value ? 1 : 0);
#else
    static constexpr int useLutAPI = 0;
#endif //_SUPPORTS_BFLOAT16_
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    // If using LUT API, each 128 bits of lut is duplicated. Therefore, double size
    static constexpr unsigned int kLutSize = (2 << (TP_COARSE_BITS - ignoreTopDomainBit)) * (useLutAPI + 1);
    static constexpr TT_DATA fineMask = ((1 << TP_FINE_BITS) - 1);
    static constexpr TT_DATA coarseMask = ((1 << (TP_FINE_BITS + TP_COARSE_BITS - ignoreTopDomainBit)) - 1);

   public:
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_lut_ab)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_lut_cd)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_slopeBuff)[TP_WINDOW_VSIZE];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_offsetBuff)[TP_WINDOW_VSIZE];
    // Constructor
    func_approx(TT_DATA (&lut_ab)[kLutSize],
                TT_DATA (&lut_cd)[kLutSize],
                TT_DATA (&slopeBuff)[TP_WINDOW_VSIZE],
                TT_DATA (&offsetBuff)[TP_WINDOW_VSIZE])
        : m_lut_ab(lut_ab), m_lut_cd(lut_cd), m_slopeBuff(slopeBuff), m_offsetBuff(offsetBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(func_approx::funcApproxMain);
        REGISTER_PARAMETER(m_lut_ab);
        REGISTER_PARAMETER(m_lut_cd);
        REGISTER_PARAMETER(m_slopeBuff);
        REGISTER_PARAMETER(m_offsetBuff);
    }

    // Main function
    void funcApproxMain(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
//-----------------------------------------------------------------------------------------------------
// floats
template <unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class func_approx<float, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT> {
   private:
    typedef float TT_DATA;
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    static constexpr unsigned int kLutSize = (2 << (TP_COARSE_BITS - ignoreTopDomainBit));
    static constexpr int32 coarseMask = ((1 << (TP_FINE_BITS + TP_COARSE_BITS - ignoreTopDomainBit)) - 1);

   public:
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_lut_ab)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_lut_cd)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_slopeBuff)[TP_WINDOW_VSIZE];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_offsetBuff)[TP_WINDOW_VSIZE];
    // Constructor
    func_approx(TT_DATA (&lut_ab)[kLutSize],
                TT_DATA (&lut_cd)[kLutSize],
                TT_DATA (&slopeBuff)[TP_WINDOW_VSIZE],
                TT_DATA (&offsetBuff)[TP_WINDOW_VSIZE])
        : m_lut_ab(lut_ab), m_lut_cd(lut_cd), m_slopeBuff(slopeBuff), m_offsetBuff(offsetBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(func_approx::funcApproxMain);
        REGISTER_PARAMETER(m_lut_ab);
        REGISTER_PARAMETER(m_lut_cd);
        REGISTER_PARAMETER(m_slopeBuff);
        REGISTER_PARAMETER(m_offsetBuff);
    }

    // Main function
    void funcApproxMain(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
#ifdef _SUPPORTS_BFLOAT16_
//-----------------------------------------------------------------------------------------------------
// int16
template <unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class func_approx<int16, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT> {
   private:
    typedef int16 TT_DATA;
    static constexpr int useLutAPI = 1;
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    static constexpr unsigned int kLutSize = (2 << (TP_COARSE_BITS - ignoreTopDomainBit)) * (useLutAPI + 1);
    static constexpr unsigned int dummyBuffSize = 16;

   public:
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_lut_ab)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_lut_cd)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_slopeBuff)[dummyBuffSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_offsetBuff)[dummyBuffSize];
    // Constructor
    func_approx(TT_DATA (&lut_ab)[kLutSize],
                TT_DATA (&lut_cd)[kLutSize],
                TT_DATA (&slopeBuff)[dummyBuffSize],
                TT_DATA (&offsetBuff)[dummyBuffSize])
        : m_lut_ab(lut_ab), m_lut_cd(lut_cd), m_slopeBuff(slopeBuff), m_offsetBuff(offsetBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(func_approx::funcApproxLutAPI);
        REGISTER_PARAMETER(m_lut_ab);
        REGISTER_PARAMETER(m_lut_cd);
        REGISTER_PARAMETER(m_slopeBuff);
        REGISTER_PARAMETER(m_offsetBuff);
    }

    // Main function
    void funcApproxLutAPI(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
// bfloat16s
template <unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class func_approx<bfloat16, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT> {
   private:
    typedef bfloat16 TT_DATA;
    static constexpr int useLutAPI = 1;
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    static constexpr unsigned int kLutSize = (2 << (TP_COARSE_BITS - ignoreTopDomainBit)) * (useLutAPI + 1);
    static constexpr unsigned int dummyBuffSize = 16;

   public:
    alignas(__ALIGN_BYTE_SIZE__) float (&m_lut_ab)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) float (&m_lut_cd)[kLutSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_slopeBuff)[dummyBuffSize];
    alignas(__ALIGN_BYTE_SIZE__) TT_DATA (&m_offsetBuff)[dummyBuffSize];
    // Constructor
    func_approx(float (&lut_ab)[kLutSize],
                float (&lut_cd)[kLutSize],
                TT_DATA (&slopeBuff)[dummyBuffSize],
                TT_DATA (&offsetBuff)[dummyBuffSize])
        : m_lut_ab(lut_ab), m_lut_cd(lut_cd), m_slopeBuff(slopeBuff), m_offsetBuff(offsetBuff) {}

    // Register Kernel Class
    static void registerKernelClass() {
        REGISTER_FUNCTION(func_approx::funcApproxLutAPI);
        REGISTER_PARAMETER(m_lut_ab);
        REGISTER_PARAMETER(m_lut_cd);
        REGISTER_PARAMETER(m_slopeBuff);
        REGISTER_PARAMETER(m_offsetBuff);
    }

    // Main function
    void funcApproxLutAPI(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
#endif //_SUPPORTS_BFLOAT16_
}
}
}
}

#endif // _DSPLIB_FUNC_APPROX_HPP_
