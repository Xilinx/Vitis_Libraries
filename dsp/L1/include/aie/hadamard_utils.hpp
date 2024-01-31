/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_HADAMARD_UTILS_HPP_
#define _DSPLIB_HADAMARD_UTILS_HPP_

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "kernel_api_utils.hpp"
#include "hadamard_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {

template <typename TT_DATA_A, typename TT_DATA_B>
struct tHadamardACC {
    using type = acc48;
};
template <>
struct tHadamardACC<int16, int16> {
    using type = acc48;
};
template <>
struct tHadamardACC<cint16, int16> {
    using type = cacc48;
};
template <>
struct tHadamardACC<int16, cint16> {
    using type = cacc48;
};
template <>
struct tHadamardACC<cint16, cint16> {
    using type = cacc48;
};
template <>
struct tHadamardACC<int32, int16> {
    using type = acc48;
};
template <>
struct tHadamardACC<int32, int32> {
    using type = acc80;
};
template <>
struct tHadamardACC<int32, cint16> {
    using type = cacc48;
};
template <>
struct tHadamardACC<int32, cint32> {
    using type = cacc80;
};
template <>
struct tHadamardACC<cint32, int16> {
    using type = cacc80;
}; // internal int32 calculation wil be done
template <>
struct tHadamardACC<cint32, cint16> {
    using type = cacc48;
};
template <>
struct tHadamardACC<cint32, int32> {
    using type = cacc80;
};
template <>
struct tHadamardACC<cint32, cint32> {
    using type = cacc80;
};
template <>
struct tHadamardACC<int16, int32> {
    using type = acc48;
};
template <>
struct tHadamardACC<int16, cint32> {
    using type = cacc80;
}; // internal int32 calculation wil be done
template <>
struct tHadamardACC<cint16, int32> {
    using type = cacc48;
};
template <>
struct tHadamardACC<cint16, cint32> {
    using type = cacc48;
};
template <>
struct tHadamardACC<float, float> {
    using type = accfloat;
};
template <>
struct tHadamardACC<float, cfloat> {
    using type = caccfloat;
};
template <>
struct tHadamardACC<cfloat, float> {
    using type = caccfloat;
};
template <>
struct tHadamardACC<cfloat, cfloat> {
    using type = caccfloat;
};

template <typename T_D, unsigned int vec_length>
constexpr::aie::vector<T_D, vec_length> unit_vector() {
    ::aie::vector<T_D, vec_length> vect_temp;
    T_D val_temp = 1;
    for (auto i = 0; i < vec_length; i++) {
        vect_temp[i] = val_temp;
    }
    return vect_temp;
};

template <typename TT_DATA_A, typename TT_DATA_B>
inline ::aie::accum<typename tHadamardACC<TT_DATA_A, TT_DATA_B>::type,
                    256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffWin>
mul_samples_win(::aie::vector<TT_DATA_A, 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffWin> vectA,
                ::aie::vector<TT_DATA_B, 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffWin> vectB) {
    static constexpr unsigned int vecSampleNum = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteOut;
    ::aie::accum<typename tHadamardACC<TT_DATA_A, TT_DATA_B>::type, vecSampleNum> acc;

    acc = ::aie::mul(vectA, vectB);
    return acc;
};

template <>
inline ::aie::accum<typename tHadamardACC<int16, cint32>::type, 256 / 8 / vectByte<int16, cint32>().val_byteBuffWin>
mul_samples_win<int16, cint32>(::aie::vector<int16, 256 / 8 / vectByte<int16, cint32>().val_byteBuffWin> vectA,
                               ::aie::vector<cint32, 256 / 8 / vectByte<int16, cint32>().val_byteBuffWin> vectB) {
    static constexpr unsigned int vecSampleNum = 256 / 8 / vectByte<int16, cint32>().val_byteA;
    static const ::aie::vector<int32, vecSampleNum> vectint32 = unit_vector<int32, vecSampleNum>();

    ::aie::vector<int32, vecSampleNum> vect_temp;
    ::aie::accum<typename tHadamardACC<int16, int32>::type, vecSampleNum> acc_temp;
    ::aie::accum<typename tHadamardACC<int16, cint32>::type, vecSampleNum> acc;

    acc_temp = ::aie::mul(vectA, vectint32);
    vect_temp = acc_temp.template to_vector<int32>(0);

    acc = ::aie::mul(vect_temp, vectB);
    return acc;
};

template <>
inline ::aie::accum<typename tHadamardACC<cint32, int16>::type, 256 / 8 / vectByte<cint32, int16>().val_byteBuffWin>
mul_samples_win<cint32, int16>(::aie::vector<cint32, 256 / 8 / vectByte<cint32, int16>().val_byteBuffWin> vectA,
                               ::aie::vector<int16, 256 / 8 / vectByte<cint32, int16>().val_byteBuffWin> vectB) {
    static constexpr unsigned int vecSampleNum = 256 / 8 / vectByte<cint32, int16>().val_byteB;
    static const ::aie::vector<int32, vecSampleNum> vectint32 = unit_vector<int32, vecSampleNum>();

    ::aie::vector<int32, vecSampleNum> vect_temp;
    ::aie::accum<typename tHadamardACC<int32, int16>::type, vecSampleNum> acc_temp;
    ::aie::accum<typename tHadamardACC<cint32, int16>::type, vecSampleNum> acc;

    acc_temp = ::aie::mul(vectint32, vectB);
    vect_temp = acc_temp.template to_vector<int32>(0);

    acc = ::aie::mul(vectA, vect_temp);
    return acc;
};

template <typename TT_DATA_A, typename TT_DATA_B>
inline ::aie::accum<typename tHadamardACC<TT_DATA_A, TT_DATA_B>::type,
                    (128 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffStream)>
mul_samples_stream(::aie::vector<TT_DATA_A, (128 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffStream)> vectA,
                   ::aie::vector<TT_DATA_B, (128 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffStream)> vectB) {
    using TT_OUT = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    static constexpr unsigned int kSamplesInVect = (128 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffStream);
    ::aie::vector<TT_OUT, kSamplesInVect> vectOut;
    ::aie::accum<typename tHadamardACC<TT_DATA_A, TT_DATA_B>::type, kSamplesInVect> acc;

    acc = ::aie::mul(vectA, vectB);
    return acc;
};

template <>
inline ::aie::accum<typename tHadamardACC<int16, cint32>::type,
                    (128 / 8 / vectByte<int16, cint32>().val_byteBuffStream)>
mul_samples_stream<int16, cint32>(
    ::aie::vector<int16, (128 / 8 / vectByte<int16, cint32>().val_byteBuffStream)> vectA,
    ::aie::vector<cint32, (128 / 8 / vectByte<int16, cint32>().val_byteBuffStream)> vectB) {
    static constexpr unsigned int kSamplesInVect = (128 / 8 / vectByte<int16, cint32>().val_byteBuffStream);
    static const ::aie::vector<int32, kSamplesInVect> vectint32 = unit_vector<int32, kSamplesInVect>();
    ::aie::vector<int32, kSamplesInVect> vect_temp;
    ::aie::vector<cint32, kSamplesInVect> vectOut;
    ::aie::accum<typename tHadamardACC<int16, int32>::type, kSamplesInVect> acc_temp;
    ::aie::accum<typename tHadamardACC<int16, cint32>::type, kSamplesInVect> acc;

    acc_temp = ::aie::mul(vectint32, vectA);
    vect_temp = acc_temp.template to_vector<int32>(0);

    acc = ::aie::mul(vect_temp, vectB);
    return acc;
};

template <>
inline ::aie::accum<typename tHadamardACC<cint32, int16>::type,
                    (128 / 8 / vectByte<cint32, int16>().val_byteBuffStream)>
mul_samples_stream<cint32, int16>(
    ::aie::vector<cint32, (128 / 8 / vectByte<cint32, int16>().val_byteBuffStream)> vectA,
    ::aie::vector<int16, (128 / 8 / vectByte<cint32, int16>().val_byteBuffStream)> vectB) {
    static constexpr unsigned int kSamplesInVect = (128 / 8 / vectByte<cint32, int16>().val_byteBuffStream);
    static const ::aie::vector<int32, kSamplesInVect> vectint32 = unit_vector<int32, kSamplesInVect>();
    ::aie::vector<int32, kSamplesInVect> vect_temp;
    ::aie::vector<cint32, kSamplesInVect> vectOut;
    ::aie::accum<typename tHadamardACC<int16, int32>::type, kSamplesInVect> acc_temp;
    ::aie::accum<typename tHadamardACC<cint32, int16>::type, kSamplesInVect> acc;

    acc_temp = ::aie::mul(vectint32, vectB);
    vect_temp = acc_temp.template to_vector<int32>(0);

    acc = ::aie::mul(vectA, vect_temp);
    return acc;
};
}
}
}
}

#endif // _DSPLIB_FIR_SR_SYM_UTILS_HPP_
