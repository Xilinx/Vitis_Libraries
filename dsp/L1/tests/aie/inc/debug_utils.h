#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <adf.h>
#include "aie_api/utils.hpp"

// Vector Print functions. These are overloaded according to vector type for ease of use.

//--------------------------------------------------------------------------------------
// int16

// No support for v2int16 or v4int16 as less than the shortest AIE vector size supported

namespace xf {
namespace dsp {
namespace aie {
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

template <typename T, unsigned Elems>
void vPrintf(const char* preamble, const ::aie::vector<T, Elems>& myVec) {
    ::aie::print(myVec, true, preamble);
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
}
}
}
#endif // DEBUG_UTILS_H

/*  (c) Copyright 2019 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
