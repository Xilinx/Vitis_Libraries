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
#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <adf.h>
#include "aie_api/utils.hpp" // for vector print function
#include <string>

// Vector Print functions. These are overloaded according to vector type for ease of use.

#define PRINT_KERNEL_NAMES // turn this off if this annoys you

namespace xf {
namespace dsp {
namespace aie {

#ifdef PRINT_KERNEL_NAMES
#include <algorithm>
#include <adf/x86sim/x86simDebug.h> // When debugging on hw, all functions still compile but don't do anything.
std::string getKernelFilename() {
    std::string x86kernelFilename(X86SIM_KERNEL_NAME);

    std::replace(x86kernelFilename.begin(), x86kernelFilename.end(), '.',
                 '_'); // replace dots with underscore for filename.
    // C++20 erase to remove spaces from filename
    std::erase(x86kernelFilename, ' ');
    x86kernelFilename.insert(x86kernelFilename.length(), ".txt");
    return x86kernelFilename;
}
template <class... Types>
void printf(const char* preamble, Types... args) {
    auto* x86kernel_fh = fopen(getKernelFilename().c_str(), "a");
    std::fprintf(x86kernel_fh, preamble, args...);
    fclose(x86kernel_fh);
    std::printf(preamble, args...);
}

#endif // PRINT_KERNEL_NAMES

//--------------------------------------------------------------------------------------
// int16
#if __SUPPORTS_V8INT16__ == 1
void vPrintf(const char* preamble, const v8int16 myVec) {
    const int kVLen = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v16int16 myVec) {
    const int kVLen = 16;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v32int16 myVec) {
    const int kVLen = 32;
    const int kLineLim = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
        if ((i % kLineLim) == kLineLim - 1) printf("\n");
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v64int16 myVec) {
    const int kVLen = 64;
    const int kLineLim = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
        if ((i % kLineLim) == kLineLim - 1) printf("\n");
    }
    printf(")\n");
}
//--------------------------------------------------------------------------------------
// cint16

// No support for v2cint16 as this is less than the shortest AIE vector size supported

#if __SUPPORTS_V16INT16__ == 1
void vPrintf(const char* preamble, const v4cint16 myVec) {
    const int kVLen = 4;
    cint16_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem = ext_elem(myVec, i);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}
void vPrintf(const char* preamble, const v8cint16 myVec) {
    const int kVLen = 8;
    cint16_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem = ext_elem(myVec, i);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}
#endif

void vPrintf(const char* preamble, const v16cint16 myVec) {
    const int kVLen = 16;
    const int kLineLim = 8;
    cint16_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem = ext_elem(myVec, i);
        printf("(%d, %d), ", myElem.real, myElem.imag);
        if ((i % kLineLim) == kLineLim - 1) printf("\n");
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v32cint16 myVec) {
    const int kVLen = 32;
    const int kLineLim = 8;
    cint16_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem = ext_elem(myVec, i);
        printf("(%d, %d), ", myElem.real, myElem.imag);
        if ((i % kLineLim) == kLineLim - 1) printf("\n");
    }
    printf(")\n");
}
// No support for v64cint16 as this exceeds the largest AIE vector size supported

//--------------------------------------------------------------------------------------float

// No support for v2int32 as this is less than the shortest AIE vector size supported
void vPrintf(const char* preamble, const v4int32 myVec) {
    const int kVLen = 4;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v8int32 myVec) {
    const int kVLen = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v16int32 myVec) {
    const int kVLen = 16;
    const int kLineLim = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v32int32 myVec) {
    const int kVLen = 32;
    const int kLineLim = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%d, ", ext_elem(myVec, i));
        // if ((i % kLineLim) == kLineLim-1) printf("\n");
    }
    printf(")\n");
}

// No support for v64int32 as this exceeds the largest AIE vector size supported

//--------------------------------------------------------------------------------------
// cint32
// Ext_elem is unavailable for cint32 at time of writing, so a cast to int32 is used.
void vPrintf(const char* preamble, const v2cint32 myVec) {
    const int kVLen = 2;
    v4int32 myVCast = as_v4int32(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v4cint32 myVec) {
    const int kVLen = 4;
    v8int32 myVCast = as_v8int32(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v8cint32 myVec) {
    const int kVLen = 8;
    v16int32 myVCast = as_v16int32(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v16cint32 myVec) {
    const int kVLen = 16;
    const int kLineLim = 8;
    v32int32 myVCast = as_v32int32(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
        // if ((i % kLineLim) == kLineLim-1) printf("\n");
    }
    printf(")\n");
}

// No support for v32cint32 or v64cint32 as these exceed the largest AIE vector size supported

//--------------------------------------------------------------------------------------
// float

// No support for v2float as this is less than the shortest AIE vector size supported

void vPrintf(const char* preamble, const v4float myVec) {
    const int kVLen = 4;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%f, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v8float myVec) {
    const int kVLen = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%f, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v16float myVec) {
    const int kVLen = 16;
    const int kLineLim = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%f, ", ext_elem(myVec, i));
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v32float myVec) {
    const int kVLen = 32;
    const int kLineLim = 8;
    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        printf("%f, ", ext_elem(myVec, i));
        // if ((i % kLineLim) == kLineLim-1) printf("\n");
    }
    printf(")\n");
}
// No support for v64float as this exceeds the largest AIE vector size supported

//--------------------------------------------------------------------------------------
// cfloat
// Ext_elem is unavailable for cfloat at time of writing, so a cast to float is used.
void vPrintf(const char* preamble, const v2cfloat myVec) {
    const int kVLen = 2;
    v4float myVCast = as_v4float(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v4cfloat myVec) {
    const int kVLen = 4;
    v8float myVCast = as_v8float(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v8cfloat myVec) {
    const int kVLen = 8;
    v16float myVCast = as_v16float(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
    }
    printf(")\n");
}

void vPrintf(const char* preamble, const v16cfloat myVec) {
    const int kVLen = 16;
    const int kLineLim = 8;
    v32float myVCast = as_v32float(myVec);
    cint32_t myElem;

    printf("%s = (", preamble);
    for (int i = 0; i < kVLen; ++i) {
        myElem.real = ext_elem(myVCast, i * 2);
        myElem.imag = ext_elem(myVCast, i * 2 + 1);
        printf("(%d, %d), ", myElem.real, myElem.imag);
        if ((i % kLineLim) == kLineLim - 1) printf("\n");
    }
    printf(")\n");
}
#endif
template <typename T, unsigned Elems>
void vPrintf(const char* preamble, const ::aie::vector<T, Elems>& myVec) {
    // Convert to native so that our version of printf overload still works (kernel name in cascade designs)
    // vPrintf(preamble, myVec.to_native());
    ::aie::print(myVec, true, preamble);

    auto* x86kernel_fh = fopen(getKernelFilename().c_str(), "a");
    std::fprintf(x86kernel_fh, preamble);
    for (int i = 0; i < Elems; i++) {
        T tmp = myVec[i];
        if
            constexpr(std::is_same<T, cint16>::value) {
                std::fprintf(x86kernel_fh, "{%d, %d}", tmp.real, tmp.imag);
                std::fprintf(x86kernel_fh, "\t");
            }
        else {
            std::fprintf(x86kernel_fh, "{%d}", tmp);
            std::fprintf(x86kernel_fh, "\t");
        }
    }
    fclose(x86kernel_fh);
}

// No support for v32cfloat or v64cfloat as these exceed the largest AIE vector size supported

//--------------------------------------------------------------------------------------
// Arrays

void vPrintf(const char* preamble, int16_t* array, size_t len) {
    const int kLineLim = 16;
    printf("%s = (", preamble);
    for (int i = 0; i < len; ++i) {
        printf("%d, ", array[i]);
        if ((i % kLineLim) == kLineLim - 1) printf("\n");
    }
    printf(")\n");
}

void vPrintf(const char* preamble, cint16_t* array, size_t len) {
    const int kLineLim = 16;
    printf("%s = (", preamble);
    for (int i = 0; i < len; ++i) {
        printf("(%d, %d), ", array[i].real, array[i].imag);
    }
    printf(")\n");
}

template <typename... VectorArgs>
void vPrintf(const std::string preamble, const VectorArgs... myVec) {
    vPrintf(preamble.c_str(), myVec...);
}
}
}
}
#endif // DEBUG_UTILS_H
