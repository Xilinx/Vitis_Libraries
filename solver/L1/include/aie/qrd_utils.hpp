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
#ifndef _SOLVERLIB_QRD_UTILS_HPP_
#define _SOLVERLIB_QRD_UTILS_HPP_

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
#include "qrd_traits.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd {             


//dot product of two vectors
template <typename T_A>
INLINE_DECL ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> mul_vectors(::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> va, ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> vb)
    {

        T_A ret_val;
        using acc_t = accTypeMult_t<T_A, T_A>;
        // ::aie::accum<acc_t, kMaxReadInBytes/ sizeof(out_t)> outAcc_v_init = ::aie::zero<acc_t>();
        ::aie::accum<acc_t, kMaxReadInBytes/ sizeof(T_A)> outAcc_v;
        ::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> out_v;

        outAcc_v = ::aie::mac<acc_t>(va, vb);
        out_v = outAcc_v.template to_vector<T_A>(0);
        // ret_val = ::aie::reduce_add(out_v);
        // return ret_val;
        return out_v;
    }

template <typename T_A>
INLINE_DECL void conj_vector(::aie::vector<T_A, kMaxReadInBytes / sizeof(T_A)> &va)
    {
        // intentionally does nothing
    }
template <>
INLINE_DECL void conj_vector<cfloat>(::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> &va)
    {
        va = ::aie::conj(va);
    }

//dot product of two vectors
template <typename T_A, typename T_B>
INLINE_DECL outTypeMult_t<T_A, T_B> calc_dot_product(::aie::vector<T_A, kMaxReadInBytes / sizeof(outTypeMult_t<T_A, T_B>)> va, ::aie::vector<T_B, kMaxReadInBytes / sizeof(outTypeMult_t<T_A, T_B>)> vb)
    {
        using out_t = outTypeMult_t<T_A, T_B>;
        using acc_t = accTypeMult_t<T_A, T_B>;

        out_t ret_val;
        ::aie::accum<acc_t, kMaxReadInBytes/ sizeof(out_t)> outAcc_v;
        ::aie::vector<out_t, kMaxReadInBytes / sizeof(out_t)> out_v;

        outAcc_v = ::aie::mul<acc_t>(va, vb);
        out_v = outAcc_v.template to_vector<out_t>(0);
        ret_val = ::aie::reduce_add(out_v);
        return ret_val;
    }
template <>
INLINE_DECL cfloat calc_dot_product<cfloat, cfloat>(::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> va, ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> vb)
        {
            using acc_t = accTypeMult_t<cfloat, cfloat>;

            cfloat ret_val;
            ::aie::accum<acc_t, kMaxReadInBytes / sizeof(cfloat)> outAcc_v;
            ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> out_v;
            ::aie::vector<cfloat, kMaxReadInBytes / sizeof(cfloat)> conj_va;

            conj_va=::aie::conj(va);
            outAcc_v = ::aie::mul<acc_t>(conj_va, vb);
            out_v = outAcc_v.template to_vector<cfloat>(0);
            ret_val = ::aie::reduce_add(out_v);
            return ret_val;
        }

template <typename T_DATA>
INLINE_DECL T_DATA calc_inv(T_DATA val)
    {
        T_DATA val_inv;
        val_inv = ::aie::inv(val);
        return val_inv;
    }  
template <>
INLINE_DECL cfloat calc_inv<cfloat>(cfloat val)
    {
        cfloat val_inv;
        val_inv.real = ::aie::inv(val.real);
        val_inv.imag = 0;
        return val_inv;
    }
template <typename T_DATA>
    INLINE_DECL T_DATA calc_sqrt(T_DATA val)
    {
        T_DATA val_sqrt;
        val_sqrt = ::aie::sqrt(val);
        return val_sqrt;
    }
template <>
INLINE_DECL cfloat calc_sqrt<cfloat>(cfloat val)
    {
        cfloat val_sqrt;
        val_sqrt.real = ::aie::sqrt(val.real);
        val_sqrt.imag = 0;
        return val_sqrt;
    }

template <typename T_DATA>
    INLINE_DECL T_DATA init_zero()
    {
        T_DATA val;
        val = 0;
        return val;
    }
template <>
    INLINE_DECL cfloat init_zero<cfloat>()
    {
        cfloat val;
        val.imag = 0;
        val.real = 0;
        return val;
    }

}
}
}
}
#endif // _SOLVERLIB_QRD_TRAITS_HPP_
