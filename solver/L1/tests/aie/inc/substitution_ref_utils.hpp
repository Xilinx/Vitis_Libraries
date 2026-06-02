#ifndef _SUBSTITUTION_REF_UTILS_HPP_
#define _SUBSTITUTION_REF_UTILS_HPP_

#include <iostream>
#include <stdlib.h>

namespace xf {
namespace solver {
namespace aie {
namespace substitution {

//----------------------
// NullElem
template <typename T_D>
T_D nullElem(){
    // empty default
};
template <>
inline float nullElem<float>() {
    return 0.0f;
};
template <>
inline cfloat nullElem<cfloat>() {
    cfloat retVal;
    retVal.real = 0.0f;
    retVal.imag = 0.0f;
    return retVal;
};

template <typename T_D>
float fnGetReal(T_D inVal) {
    // empty default
    return -1.0;
};
template <>
inline float fnGetReal<float>(float inVal) {
    return inVal;
};
template <>
inline float fnGetReal<cfloat>(cfloat inVal) {
    return inVal.real;
};

template <typename TT_DATA, unsigned int TP_DIM_SIZE, unsigned int TP_L_LEADING>
void fwd_subst_ref(const TT_DATA* L, const TT_DATA* y, TT_DATA* x) {
    for (int i = 0; i < TP_DIM_SIZE; ++i) {
        TT_DATA sum = nullElem<TT_DATA>();
        for (int j = 0; j < i; ++j) {
            if (TP_L_LEADING) {
                sum += L[j * TP_DIM_SIZE + i] * x[j];
            } else {
                sum += L[i * TP_DIM_SIZE + j] * x[j];
            }
        }
        TT_DATA diag;
        if (TP_L_LEADING) {
            diag = L[i * TP_DIM_SIZE + i];
        } else {
            diag = L[i * TP_DIM_SIZE + i];
        }
        x[i] = (y[i] - sum) / diag;
    }
}

} // namespace substitution
} // namespace aie
} // namespace solver
} // namespace xf

#endif
