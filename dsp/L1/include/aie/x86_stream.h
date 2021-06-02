/*  (c) Copyright 2014 - 2020 Xilinx, Inc. All rights reserved.

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

/* This is used for modelling stream FIFOs in x86/MEX simulation,
 * NOT to be used in ESS. Use it with the testbench for kernels
 * with stream I/Os. Take a look at the examples in x86 folder for
 * tx-chain DPD and CFR. This header file gets included from
 * utils.h when compiled with __X86SIM__ flag.*/

#ifndef __X86_STREAM_H__
#define __X86_STREAM_H__

#include <queue>
#include <adf/window/types.h>

#define CALL_MACRO_FOR_ALL_SCALAR_TYPES(MACRO) \
    MACRO(int8)                                \
    MACRO(uint8)                               \
    MACRO(int16)                               \
    MACRO(uint16)                              \
    MACRO(cint16)                              \
    MACRO(int32)                               \
    MACRO(uint32)                              \
    MACRO(cint32)                              \
    MACRO(int64)                               \
    MACRO(uint64)

// Filling in some gaps.

// typedef int64_t int64;
// typedef uint64_t uint64;

inline int as_int(int16 v) {
    return ((int)v);
}

inline int16 as_int16(int v) {
    return v;
}
inline int32 as_int32(int v) {
    return v;
}

// Scalar

#define STREAM_TYPES(type)        \
    struct input_stream_##type {  \
       public:                    \
        std::queue<type> fifo;    \
    };                            \
    struct output_stream_##type { \
       public:                    \
        std::queue<type> fifo;    \
    };
CALL_MACRO_FOR_ALL_SCALAR_TYPES(STREAM_TYPES)
#undef STREAM_TYPES

#define STREAM_APIS(type)                   \
    type readincr(input_stream_##type* in); \
    void writeincr(output_stream_##type* out, type val);
CALL_MACRO_FOR_ALL_SCALAR_TYPES(STREAM_APIS)
#undef STREAM_APIS

#if (!defined(__PL_KERNEL__))

// Vector

#define CALL_MACRO_FOR_ALL_VECTOR_TYPES(MACRO)               \
    /* MACRO(int8, 16)  v16int8 does not have upd_elem() */  \
    /* MACRO(uint8, 16)  v16int8 does not have upd_elem() */ \
    MACRO(int16, 8) /* no match for 'operator[]' */          \
    /* MACRO(uint16, 8)  v8uint16 not defined */             \
    MACRO(cint16, 4)                                         \
    MACRO(int32, 4)                                          \
/* MACRO(uint32, 4) v4uint32 not defined */                  \
/* MACRO(cint32, 2) v2cint32 does not have upd_elem() */     \
/* MACRO(int64, 2) v2int64 not defined */                    \
/* MACRO(uint64, 2)  v2uint64 not defined */                 \
/* MACRO(acc48, 8) v8acc48 does not have upd_elem() */       \
/* MACRO(cacc48, 4) v4cacc48 does not have upd_elem() */

#define VECTOR_STREAM_APIS(datatype, lanes)                            \
    v##lanes##datatype readincr_v##lanes(input_stream_##datatype* in); \
    void writeincr_v##lanes(output_stream_##datatype* out, v##lanes##datatype val);
CALL_MACRO_FOR_ALL_VECTOR_TYPES(VECTOR_STREAM_APIS)
#undef VECTOR_STREAM_APIS

// Cascade streams.

struct input_stream_acc48 {
   public:
    std::queue<v8acc48> fifo;
};
struct output_stream_acc48 {
   public:
    std::queue<v8acc48> fifo;
};
v8acc48 readincr_v8(input_stream_acc48* in);
void writeincr_v8(output_stream_acc48* out, v8acc48 val);

struct input_stream_cacc48 {
   public:
    std::queue<v4cacc48> fifo;
};
struct output_stream_cacc48 {
   public:
    std::queue<v4cacc48> fifo;
};
v4cacc48 readincr_v4(input_stream_cacc48* in);
void writeincr_v4(output_stream_cacc48* out, v4cacc48 val);

#endif // #if (!defined(__PL_KERNEL__))

#endif // __X86_STREAM_H__
