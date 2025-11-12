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
#ifndef _DSPLIB_FUNC_APPROX_REF_HPP_
#define _DSPLIB_FUNC_APPROX_REF_HPP_

/*
Function Approximation reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"
#include "func_approx_traits.hpp"

using namespace adf;

#define _DSPLIB_FUNC_APPROX_REF_DEBUG_

#define SQRT_FUNC 0
#define INVSQRT_FUNC 1
#define LOG_FUNC 2
#define EXP_FUNC 3
#define INV_FUNC 4

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
class func_approx_ref {
   private:
   public:
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    static constexpr int kLutValues = (2 << (TP_COARSE_BITS - ignoreTopDomainBit));
    static constexpr TT_DATA fineMask = ((1 << TP_FINE_BITS) - 1);
    static constexpr TT_DATA coarseMask = ((1 << (TP_FINE_BITS + TP_COARSE_BITS - ignoreTopDomainBit)) - 1);
#ifdef _SUPPORTS_BFLOAT16_
    using TT_LUT = typename std::conditional<std::is_same<TT_DATA, bfloat16>::value, float, TT_DATA>::type;
    static constexpr int isFloatingPoint =
        (std::is_same<TT_DATA, bfloat16>::value || std::is_same<TT_DATA, float>::value) ? 1 : 0;
#else
    using TT_LUT = TT_DATA;
    static constexpr int isFloatingPoint = std::is_same<TT_DATA, float>::value ? 1 : 0;
#endif //_SUPPORTS_BFLOAT16_
    TT_LUT* m_lutRtpPtr;
    TT_LUT* m_lutRtpPtr1;
    TT_LUT* m_lut_ab;

    // Constructor with vector - takes std::vector reference
    func_approx_ref(const std::vector<TT_LUT>& lut_ab)
        : m_lut_ab(const_cast<TT_LUT*>(lut_ab.data())), m_lutRtpPtr(nullptr), m_lutRtpPtr1(nullptr) {}
    // Constructor with DummyArray
    template <typename T>
    func_approx_ref(const T& dummy_array) : m_lut_ab(nullptr), m_lutRtpPtr(nullptr), m_lutRtpPtr1(nullptr) {}
    // Constructor - for RTP luts
    func_approx_ref() : m_lut_ab(nullptr), m_lutRtpPtr(nullptr), m_lutRtpPtr1(nullptr) {
        // do nothing
    }

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_USE_LUT_RELOAD == 1) { REGISTER_FUNCTION(func_approx_ref::approx_rtp); }
        else {
            REGISTER_FUNCTION(func_approx_ref::approx_main);
            REGISTER_PARAMETER(m_lut_ab);
        }
    }

    void approx_main(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow);
    void approx_rtp(input_buffer<TT_DATA>& inWindow,
                    output_buffer<TT_DATA>& outWindow,
                    const TT_LUT (&lut0)[kLutValues],
                    const TT_LUT (&lut1)[kLutValues]);
    void func_approx_main_ref(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow);
};
}
}
}
}

#endif // _DSPLIB_FUNC_APPROX_REF_HPP_
