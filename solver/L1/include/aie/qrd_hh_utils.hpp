/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_QRD_HH_UTILS_HPP_
#define _SOLVERLIB_QRD_HH_UTILS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include "device_defs.h"
#include "fir_utils.hpp"
#include "single_mul_out_types.hpp"
#include "single_mul_acc_types.hpp"
#include "aie_api/aie_adf.hpp"
#include <adf.h>
#include "qrd_hh_traits.hpp"


namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {             

#if __USES_NATIVE_SQRT_FUNC__
#else
    static constexpr int kHwVecSize = __MAX_READ_WRITE__ / 8 / sizeof(float);

    static INLINE_DECL float hw_mul(float a, float b) {
        ::aie::vector<float, kHwVecSize> va = ::aie::broadcast<float, kHwVecSize>(a);
        return ::aie::mul(va, b).to_vector<float>()[0];
    }

    static INLINE_DECL float hw_inv(float x) {
        ::aie::vector<float, kHwVecSize> v = ::aie::broadcast<float, kHwVecSize>(x);
        return ::aie::inv(v)[0];
    }

    static NOINLINE_DECL float hw_invsqrt(float x) {
        ::aie::vector<float, kHwVecSize> vconst_0_5 = ::aie::broadcast<float, kHwVecSize>(0.5f);
        ::aie::vector<float, kHwVecSize> vconst_1_5 = ::aie::broadcast<float, kHwVecSize>(1.5f);
        ::aie::vector<float, kHwVecSize> vx = ::aie::broadcast<float, kHwVecSize>(x);
        ::aie::vector<float, kHwVecSize> vy = ::aie::invsqrt(vx);   // ~12-bit hardware approximation
        // NR step: y1 = y0*(1.5 - 0.5*x*y0^2) — full float precision
        ::aie::vector<float, kHwVecSize> vysqr = ::aie::mul(vy, vy);
        ::aie::vector<float, kHwVecSize> vhalf = ::aie::mul(vconst_0_5, vx);
        ::aie::vector<float, kHwVecSize> t = ::aie::mul(vysqr, vhalf);
        ::aie::vector<float, kHwVecSize> t_sub = ::aie::sub(vconst_1_5, t);

        return ::aie::mul(vy, t_sub).to_vector<float>()[0];
    }

    static INLINE_DECL float hw_sqrt(float x) {
        uint32_t bits;
        __builtin_memcpy(&bits, &x, sizeof(bits));
        if ((bits & 0x7FFFFFFFu) == 0u) return 0.0f;
        return hw_mul(x, hw_invsqrt(x));
    }

#endif

template <typename T_DATA>
INLINE_DECL void calc_sqrt(T_DATA val, T_DATA &val_sqrt)
    {
#if __USES_NATIVE_SQRT_FUNC__ 
        val_sqrt = ::aie::sqrt(val);
#else
        val_sqrt = hw_sqrt(val);
#endif   
    }

    
template <>
INLINE_DECL void calc_sqrt<cfloat>(cfloat val, cfloat &val_sqrt)
    {
#if __USES_NATIVE_SQRT_FUNC__ 
        val_sqrt.real = ::aie::sqrt(val.real);
#else
        val_sqrt.real = hw_sqrt(val.real);
#endif
        val_sqrt.imag = 0;
    }

INLINE_DECL float inv_float(float x) {
#if __USES_NATIVE_SQRT_FUNC__ 
    return ::aie::inv(x);
#else
    return hw_inv(x);
#endif
}

template <typename T_DATA>
INLINE_DECL void calc_phase(T_DATA val_in, T_DATA &phase_out) {
    if (val_in.real == 0.0f && val_in.imag == 0.0f) {
        phase_out = {1.0f, 0.0f};
        return;
    }
    float mag_sq   = val_in.real * val_in.real + val_in.imag * val_in.imag;
#if __USES_NATIVE_SQRT_FUNC__ 
    float inv_mag  = ::aie::invsqrt(mag_sq);
#else
    float inv_mag  = hw_invsqrt(mag_sq);
#endif
    phase_out.real = val_in.real * inv_mag;
    phase_out.imag = val_in.imag * inv_mag;
}

template <>
INLINE_DECL void calc_phase<float>(float val_in, float &phase_out) {
    phase_out = (val_in >= 0.0f) ? 1.0f : -1.0f;
}

template <typename T_A>
INLINE_DECL void mul_scalars(T_A a, T_A b, T_A &val_out)
{   
    val_out = a * b;
}

template <>
INLINE_DECL void mul_scalars<cfloat>(cfloat a, cfloat b, cfloat &val_out)
{
    cfloat va_conj = ::aie::conj(a);
    val_out = va_conj * b;
}


template <typename T_A>
INLINE_DECL void mul_vectors(
    ::aie::accum<accTypeMult_t<T_A, T_A>, kMaxReadInBytes / sizeof(T_A)> &acc,
    ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> va,
    ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> vb)
{
    acc = ::aie::mul(va, vb);
}

template <>
INLINE_DECL void mul_vectors<cfloat>(
                ::aie::accum<accTypeMult_t<cfloat, cfloat>, kMaxReadInBytes / sizeof(cfloat)> &acc, 
                ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> va,
                ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> vb)
{
    acc = ::aie::mul(::aie::op_conj(va), vb);
}

template <typename T_A>
INLINE_DECL void mac_vectors(
                            ::aie::accum<accTypeMult_t<T_A, T_A>, kMaxReadInBytes / sizeof(T_A)> &acc,
                            ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> va,
                            ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> vb)
{
    acc = ::aie::mac(acc, va, vb);
}

template <>
INLINE_DECL void mac_vectors<cfloat>(
              ::aie::accum<accTypeMult_t<cfloat, cfloat>, kMaxReadInBytes / sizeof(cfloat)> &acc,
              ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> va,
              ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> vb)
{
    acc = ::aie::mac(acc, ::aie::op_conj(va), vb);
}


} // namespace qrd_hh
} // namespace aie
} // namespace solver
} // namespace xf
#endif // _SOLVERLIB_QRD_TRAITS_HPP_
