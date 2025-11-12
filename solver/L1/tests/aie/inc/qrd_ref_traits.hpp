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
#ifndef _SOLVERLIB_QRD_REF_TRAITS_HPP_
#define _SOLVERLIB_QRD_REF_TRAITS_HPP_

#include "fir_ref_utils.hpp"
#include "single_mul_ref_out_types.hpp"
#include "single_mul_ref_acc_types.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

template <typename TT_A, typename TT_B>
struct vectByte {
    unsigned val_byteA = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteB = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteOut = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteBuffWin = sizeof(outTypeMult_t<TT_A, TT_B>);
    unsigned val_byteBuffStream = ::std::min(sizeof(TT_A), sizeof(TT_B));
    unsigned int kCaptureDataA = 1;
    unsigned int kCaptureDataB = 1;
    unsigned int kCaptureDataOut = 1;
};
template <>
struct vectByte<int16, cint32> {
    unsigned int val_byteA = 4;
    unsigned int val_byteB = 8;
    unsigned int val_byteOut = 8;
    unsigned int val_byteBuffWin = 4;
    unsigned int val_byteBuffStream = 2;
    unsigned int kCaptureDataA = 1;
    unsigned int kCaptureDataB = 2;
    unsigned int kCaptureDataOut = 2;
};

template <>
struct vectByte<cint32, int16> {
    unsigned int val_byteA = 8;
    unsigned int val_byteB = 4;
    unsigned int val_byteOut = 8;
    unsigned int val_byteBuffWin = 4;
    unsigned int val_byteBuffStream = 2;
    unsigned int kCaptureDataA = 2;
    unsigned int kCaptureDataB = 1;
    unsigned int kCaptureDataOut = 2;
};

}
}
}
}

#endif