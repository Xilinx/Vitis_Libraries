/*
 * Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_QRD_HH_REF_UTILS_HPP_
#define _SOLVERLIB_QRD_HH_REF_UTILS_HPP_

#include "aie_api/utils.hpp" // for vector print function
#include "single_mul_ref_out_types.hpp"
#include "single_mul_ref_acc_types.hpp"

#include "aie_api/aie_adf.hpp"
#include <adf.h>

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {

template <typename T_A, typename T_B, unsigned int ARRAY_LENGTH>
class array_operations {
   private:
    using out_t = outTypeMult_t<T_A, T_B>;

   public:
    array_operations(){};

    // dot product of two vectors
    out_t calc_dot_product(::std::array<T_A, ARRAY_LENGTH> va, ::std::array<T_B, ARRAY_LENGTH> vb) {
        out_t ret_val;
        out_t mult_val;
        T_accRef<out_t> outAcc;

        outAcc = null_accRef<out_t>(); // initial condition for accumulation process

        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            multiplyAcc<T_A, T_B, out_t>(outAcc, va[i], vb[i]);
        }

        ret_val = castAcc(outAcc);
        return ret_val;
    }

    template <typename T_DATA>
    T_DATA calc_norm_val(::std::array<T_DATA, ARRAY_LENGTH> va) {
        T_DATA norm_val;
        T_DATA dot_product_val;

        dot_product_val = calc_dot_product(va, va); // find the dot product of the given vector
        norm_val = sqrt(dot_product_val);           // take the squareroot value to calculate r
        return norm_val;
    }

    template <typename T_DATA>
    ::std::array<T_DATA, ARRAY_LENGTH> calc_normalized_vector(::std::array<T_DATA, ARRAY_LENGTH> va) {
        T_DATA norm_len;
        ::std::array<T_DATA, ARRAY_LENGTH> vn;

        norm_len = calc_norm_val(va);
        vn = div_array(va, norm_len);
        return vn;
    }

    // initialize a vector with all zero
    template <typename T_DATA>
    void all_zero_init(::std::array<T_DATA, ARRAY_LENGTH>& va) {
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            va[i] = nullElem<T_DATA>();
        }
    }

    template <typename T_DATA>
    ::std::array<T_DATA, ARRAY_LENGTH> mul_const_arr(T_DATA coeff, ::std::array<T_DATA, ARRAY_LENGTH> va) {
        std::array<T_DATA, ARRAY_LENGTH> v_mul;
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            v_mul[i] = coeff * va[i];
        }
        return v_mul;
    }

    template <typename T_DATA>
    ::std::array<T_DATA, ARRAY_LENGTH> sub_arrays(::std::array<T_DATA, ARRAY_LENGTH> va,
                                                  ::std::array<T_DATA, ARRAY_LENGTH> vb) {
        ::std::array<T_DATA, ARRAY_LENGTH> v_sub;
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            v_sub[i] = va[i] - vb[i];
        }
        return v_sub;
    }

    template <typename T_DATA>
    ::std::array<T_DATA, ARRAY_LENGTH> div_array(::std::array<T_DATA, ARRAY_LENGTH> va, T_DATA divider) {
        ::std::array<T_DATA, ARRAY_LENGTH> v_div;
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            v_div[i] = va[i] / divider;
        }
        return v_div;
    }
};

template <unsigned int ARRAY_LENGTH>
class array_operations<cfloat, cfloat, ARRAY_LENGTH> {
   private:
    using out_t = outTypeMult_t<cfloat, cfloat>;
    using arr_t = ::std::array<cfloat, ARRAY_LENGTH>;

   public:
    array_operations(){};

    out_t calc_dot_product(arr_t va, arr_t vb) {
        out_t ret_val;
        out_t mult_val;
        T_accRef<out_t> outAcc;
        cfloat conj_va;

        outAcc = null_accRef<out_t>(); // initial condition for accumulation process

        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            va[i].imag = -va[i].imag; // conjugate the vector va
            multiplyAcc<cfloat, cfloat, out_t>(outAcc, va[i], vb[i]);
        }

        ret_val = castAcc(outAcc);
        return ret_val;
    }

    cfloat calc_norm_val(arr_t va) {
        cfloat norm_val;
        cfloat dot_product_val;

        dot_product_val = calc_dot_product(va, va);        // find the dot product of the given vector
        norm_val.real = sqrt((float)dot_product_val.real); // take the squareroot value to calculate r
        norm_val.imag = 0;
        return norm_val;
    }

    arr_t calc_normalized_vector(arr_t va) {
        cfloat norm_len;
        arr_t vn;

        norm_len = calc_norm_val(va);
        vn = div_array(va, norm_len);
        return vn;
    }

    // initialize a vector with all zero
    void all_zero_init(arr_t& va) {
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            va[i] = nullElem<cfloat>();
        }
    }

    arr_t mul_const_arr(cfloat coeff, arr_t va) {
        arr_t v_mul;
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            v_mul[i].real = coeff.real * va[i].real - coeff.imag * va[i].imag;
            v_mul[i].imag = coeff.real * va[i].imag + coeff.imag * va[i].real;
        }
        return v_mul;
    }

    arr_t sub_arrays(arr_t va, arr_t vb) {
        arr_t v_sub;
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            v_sub[i].real = va[i].real - vb[i].real;
            v_sub[i].imag = va[i].imag - vb[i].imag;
        }
        return v_sub;
    }

    arr_t div_array(arr_t va, cfloat divider) {
        arr_t v_div;
        for (unsigned int i = 0; i < ARRAY_LENGTH; i++) {
            v_div[i].real = va[i].real / divider.real;
            v_div[i].imag = va[i].imag / divider.real;
        }
        return v_div;
    }
};

template <typename T_A>
inline T_A divbyfloat(T_A data_in, T_A divider) {
    T_A data_out;
    data_out = data_in / divider;
    return data_out;
};

template <>
inline cfloat divbyfloat<cfloat>(cfloat data_in, cfloat divider) {
    cfloat data_out;
    data_out.real = data_in.real / divider.real;
    data_out.imag = data_in.imag / divider.real;
    return data_out;
}

template <typename T_DATA>
T_DATA calc_abs(T_DATA val_in) {
    T_DATA val_out;
    val_out = (val_in < 0) ? -val_in : val_in;
    return val_out;
}

template <>
cfloat calc_abs(cfloat val_in) {
    cfloat val_out;
    val_out.real = sqrt(val_in.real * val_in.real + val_in.imag * val_in.imag);
    val_out.imag = 0.0;
    return val_out;
}

template <typename T_DATA>
INLINE_DECL T_DATA calc_phase(T_DATA val_in) {
    T_DATA phase_out;
    T_DATA val_in_abs;
    if (val_in == constVals<T_DATA>().c0) { // if value is zero
        phase_out = constVals<T_DATA>().c1; // return 1
    } else {
        val_in_abs = calc_abs(val_in);
        phase_out = divbyfloat(val_in, val_in_abs);
    }
    return phase_out;
}

} // namespace qrd_hh
} // namespace aie
} // namespace solver
} // namespace xf
#endif