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
#pragma once

#ifndef _DSPLIB_RELOAD_UTILS_HPP_
#define _DSPLIB_RELOAD_UTILS_HPP_

// This file holds sets of templated types and overloaded (or template specialized) functions
// for use by multiple kernels.
// Functions in this file as a rule use intrinsics from a single set. For instance, a set
// may contain a MAC with pre-add which uses a single 1024 bit buffer for both forward
// and reverse data. In cases where a library element has to use an intrinsic which differs
// by more than the types used for some combinations of library element parameter types
// then the set of templatized functions will be particular to that library element and should
// therefore be in <library_element>_utils.hpp
#include <stdio.h>
#include <adf.h>
#include "device_defs.h"
#include "aie_api/aie_adf.hpp"
#include "fir_utils.hpp"
//#include "debug_utils.h"
namespace xf {
namespace dsp {
namespace aie {

// rounding modes
template <unsigned int rndMode>
INLINE_DECL void set_rnd_mode() {
    // floor: Always round towards negative infinity.
    if
        constexpr(rndMode == rnd_floor) { ::aie::set_rounding(::aie::rounding_mode::floor); }
    // ceil: Always round towards positive infinity.
    if
        constexpr(rndMode == rnd_ceil) { ::aie::set_rounding(::aie::rounding_mode::ceil); }
    // positive_inf: Round to nearest integer, with preference to positive infinity at half-way.
    if
        constexpr(rndMode == rnd_pos_inf) { ::aie::set_rounding(::aie::rounding_mode::positive_inf); }
    // negative_inf: Round to nearest integer, with preference to negative infinity at half-way.
    if
        constexpr(rndMode == rnd_neg_inf) { ::aie::set_rounding(::aie::rounding_mode::negative_inf); }
    // symmetric_inf: Round to nearest integer, with preference away from zero at half-way.
    if
        constexpr(rndMode == rnd_sym_inf) { ::aie::set_rounding(::aie::rounding_mode::symmetric_inf); }
    // symmetric_zero: Round to nearest integer, with preference towards zero at half-way.
    if
        constexpr(rndMode == rnd_sym_zero) { ::aie::set_rounding(::aie::rounding_mode::symmetric_zero); }
    // conv_even: Round to nearest integer, with preference to the even number.
    if
        constexpr(rndMode == rnd_conv_even) { ::aie::set_rounding(::aie::rounding_mode::conv_even); }
    // conv_odd: Round to the nearest integer, with preference to the odd number.
    if
        constexpr(rndMode == rnd_conv_odd) { ::aie::set_rounding(::aie::rounding_mode::conv_odd); }
#ifdef __SUPPORTS_ML_ROUND_MODES__
    // symmetric_floor: Always round towards zero. (AIE-2 additional mode)
    if
        constexpr(rndMode == rnd_sym_floor) { ::aie::set_rounding(::aie::rounding_mode::symmetric_floor); }
    // symmetric_ceil: Always round away from zero.(AIE-2 additional mode)
    if
        constexpr(rndMode == rnd_sym_ceil) { ::aie::set_rounding(::aie::rounding_mode::symmetric_ceil); }
#endif
    // ! If this function receives an invalid round mode, it will not be caught here and will fail silently.
}

// saturation modes
template <unsigned int satMode>
INLINE_DECL void set_sat_mode() {
    // none: No saturation is performed and the value is truncated on the MSB side.
    if
        constexpr(satMode == 0) { ::aie::set_saturation(::aie::saturation_mode::none); }
    // saturate: Controls saturation. Saturation rounds an n-bit signed value in the range [- ( 2^(n-1) ) : +2^(n-1) - 1
    // ]. For example if n=8, the range would be [-128:127].
    if
        constexpr(satMode == 1) { ::aie::set_saturation(::aie::saturation_mode::saturate); }
    // symmetric: Controls symmetric saturation. Symmetric saturation rounds an n-bit signed value in the range [-(
    // 2^(n-1) -1 ) : +2^(n-1) - 1 ]. For example if n=8, the range would be [-127:127]
    if
        constexpr(satMode == 3) { ::aie::set_saturation(::aie::saturation_mode::symmetric); }
    // ! If this function receives an invalid saturation mode, it will not be caught here and will fail silently.
}

#ifdef __X86SIM__
#define __chess_behave_as_fundamental_type__
#else
#define __chess_behave_as_fundamental_type__ __attribute__((chess_behave_as_fundamental_type))
#endif

// T_buff structs with ::aie::vectors
template <typename T>
struct __chess_behave_as_fundamental_type__ T_buff_128b {
    using v_type = ::aie::vector<T, 128 / 8 / sizeof(T)>;
    v_type val;
    static constexpr unsigned int size = 128;
    static constexpr unsigned getLanes() { return 128 / 8 / sizeof(T); };
};
template <typename T>
struct __chess_behave_as_fundamental_type__ T_buff_256b {
    using v_type = ::aie::vector<T, 256 / 8 / sizeof(T)>;
    v_type val;
    static constexpr unsigned int size = 256;
    static constexpr unsigned getLanes() { return 256 / 8 / sizeof(T); };
};
template <typename T>
struct __chess_behave_as_fundamental_type__ T_buff_512b {
    using v_type = ::aie::vector<T, 512 / 8 / sizeof(T)>;
    v_type val;
    static constexpr unsigned int size = 512;
    static constexpr unsigned getLanes() { return 512 / 8 / sizeof(T); };
};
template <typename T>
struct __chess_behave_as_fundamental_type__ T_buff_1024b {
    using v_type = ::aie::vector<T, 1024 / 8 / sizeof(T)>;
    v_type val;
    static constexpr unsigned int size = 1024;
    static constexpr unsigned getLanes() { return 1024 / 8 / sizeof(T); };
};

// function to window_readincr a 256 bit vector
template <typename TT_DATA>
T_buff_256b<TT_DATA> INLINE_DECL window_readincr_256b(input_window<TT_DATA>* inWindow) {
    T_buff_256b<TT_DATA> result;
    result.val = window_readincr_v<256 / 8 / sizeof(TT_DATA)>(inWindow);
    return result;
};
// function to window_readincr a 256 bit vector
template <typename TT_DATA>
T_buff_128b<TT_DATA> INLINE_DECL window_readincr_128b(input_window<TT_DATA>* inWindow) {
    T_buff_128b<TT_DATA> result;
    result.val = window_readincr_v<128 / 8 / sizeof(TT_DATA)>(inWindow);
    return result;
};

// function to window_readincr a 256 bit vector
template <typename TT_DATA>
T_buff_256b<TT_DATA> INLINE_DECL window_readdecr_256b(input_window<TT_DATA>* inWindow) {
    T_buff_256b<TT_DATA> result;
    result.val = window_readdecr_v<256 / 8 / sizeof(TT_DATA)>(inWindow);
    return result;
};
// function to window_readincr a 256 bit vector
template <typename TT_DATA>
T_buff_128b<TT_DATA> INLINE_DECL window_readdecr_128b(input_window<TT_DATA>* inWindow) {
    T_buff_128b<TT_DATA> result;
    result.val = window_readdecr_v<128 / 8 / sizeof(TT_DATA)>(inWindow);
    return result;
};

// function to readincr a 128 bit vector from a stream
template <typename TT_DATA>
T_buff_128b<TT_DATA> INLINE_DECL stream_readincr_128b(input_stream<TT_DATA>* inStream) {
    // for use with single stream kernels only.
    T_buff_128b<TT_DATA> result;
    result.val = readincr_v<128 / 8 / sizeof(TT_DATA)>(inStream);
    return result;
};

// function to writeincr a 128 bit vector to a stream
template <typename TT_DATA>
void INLINE_DECL stream_writeincr_128b(output_stream<TT_DATA>* outStream, T_buff_128b<TT_DATA> data) {
    // for use with single stream kernels only.
    writeincr(outStream, data.val);
};

// function to readincr a 128 bit vector from a stream
template <typename TT_DATA>
T_buff_128b<TT_DATA> INLINE_DECL stream_readincr_128b(input_stream<TT_DATA>* inStream, const int phase) {
    T_buff_128b<TT_DATA> result;
    using wss_type = typename T_buff_128b<cint16>::v_type;
    wss_type data0;
#ifdef __SUPPORTS_GETC_WSS__
    if (phase % 2 == 0) {
        data0 = getc_wss(0);
    } else {
        data0 = getc_wss(1);
    }
#endif //__SUPPORTS_GETC_WSS__
    result.val = data0.template cast_to<TT_DATA>();
    return result;
};

// function to writeincr a 128 bit vector to a stream
template <typename TT_DATA>
void INLINE_DECL stream_writeincr_128b(output_stream<TT_DATA>* outStream, T_buff_128b<TT_DATA> data, const int phase) {
    if (phase == 0) {
        put_wms(0, data.val);
    } else {
        put_wms(1, data.val);
    }
};

// Update 256-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_sample(TT_DATA& readSample, auto& inItr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    TT_DATA* vPtr = (TT_DATA*)&*inItr;
    readSample = *vPtr;
    inItr += 1;
};
// Update 256-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_256b(::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>& xbuff, int index, auto& inItr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inItr;
    t_vect vect = *vPtr;
    xbuff.insert(0, vect);
    inItr += kVsize;
};
// Update 512-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_256b(::aie::vector<TT_DATA, 512 / 8 / sizeof(TT_DATA)>& xbuff, int index, auto& inItr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inItr;
    t_vect vect = *vPtr;
    xbuff.insert(index % 2, vect);
    inItr += kVsize;
};
// Update 1024-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_256b(::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>& xbuff, int index, auto& inItr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inItr;
    t_vect vect = *vPtr;
    xbuff.insert(index % 4, vect);
    inItr += kVsize;
};

// Update 1024-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_256b(T_buff_1024b<TT_DATA>& xbuff, int index, auto& inItr) {
    upd_win_incr_256b<TT_DATA>(xbuff.val, index, inItr);
};
// Update 1024-bit buffer with 256-bit read from the input window with pointer decrement.
template <typename TT_DATA>
INLINE_DECL void upd_win_decr_256b(T_buff_1024b<TT_DATA>& xbuff, int index, auto& inItr) {
    constexpr int kVsize = 256 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inItr;
    t_vect vect = *vPtr;
    xbuff.val.insert(index % 4, vect);
    inItr -= kVsize;
};

// Update 1024-bit buffer with 128-bit read from the input window with pointer decrement.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_128b(T_buff_1024b<TT_DATA>& xbuff, int index, auto& inItr) {
    constexpr int kVsize = 128 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inItr;
    t_vect vect = *vPtr;
    xbuff.val.insert(index % 8, vect);
    inItr += kVsize;
};
// Update 1024-bit buffer with 128-bit read from the input window with pointer decrement.
template <typename TT_DATA>
INLINE_DECL void upd_win_decr_128b(T_buff_1024b<TT_DATA>& xbuff, int index, auto& inItr) {
    constexpr int kVsize = 128 / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVsize>;
    t_vect* vPtr = (t_vect*)&*inItr;
    t_vect vect = *vPtr;
    xbuff.val.insert(index % 8, vect);
    inItr -= kVsize;
};
// Update 1024-bit buffer with 128-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_128b(T_buff_1024b<TT_DATA>& xbuff, int index, input_window<TT_DATA>* inWindow) {
    xbuff.val.insert(index % 8, window_readincr_v<128 / 8 / sizeof(TT_DATA)>(inWindow));
};
// Update 1024-bit buffer with 128-bit read from the input window with pointer decrement.
template <typename TT_DATA>
INLINE_DECL void upd_win_decr_128b(T_buff_1024b<TT_DATA>& xbuff, int index, input_window<TT_DATA>* inWindow) {
    xbuff.val.insert(index % 8, window_readdecr_v<128 / 8 / sizeof(TT_DATA)>(inWindow));
};

// Update 512-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_256b(T_buff_512b<TT_DATA>& xbuff, int index, input_window<TT_DATA>* inWindow) {
    xbuff.val.insert(index % 2, window_readincr_v<256 / 8 / sizeof(TT_DATA)>(inWindow));
};
// Update 512-bit buffer with 256-bit read from the input window with pointer decrement.
template <typename TT_DATA>
INLINE_DECL void upd_win_decr_256b(T_buff_512b<TT_DATA>& xbuff, int index, input_window<TT_DATA>* inWindow) {
    xbuff.val.insert(index % 2, window_readdecr_v<256 / 8 / sizeof(TT_DATA)>(inWindow));
};

// Update 512-bit buffer with 128-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_win_incr_128b(T_buff_512b<TT_DATA>& xbuff, int index, input_window<TT_DATA>* inWindow) {
    xbuff.val.insert(index % 4, window_readincr_v<128 / 8 / sizeof(TT_DATA)>(inWindow));
};
// Update 512-bit buffer with 128-bit read from the input window with pointer decrement.
template <typename TT_DATA>
INLINE_DECL void upd_win_decr_128b(T_buff_512b<TT_DATA>& xbuff, int index, input_window<TT_DATA>* inWindow) {
    xbuff.val.insert(index % 4, window_readdecr_v<128 / 8 / sizeof(TT_DATA)>(inWindow));
};

// Update 1024-bit buffer with 256-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_stream_256b(T_buff_1024b<TT_DATA>& xbuff, int index, input_stream<TT_DATA>* inStream) {
    xbuff.val.insert(index % 8, readincr_v<128 / 8 / sizeof(TT_DATA)>(inStream));
    xbuff.val.insert((index + 1) % 8, readincr_v<128 / 8 / sizeof(TT_DATA)>(inStream));
};

// Update 1024-bit buffer with 128-bit read from the input window with pointer increment.
template <typename TT_DATA>
INLINE_DECL void upd_stream_128b(T_buff_1024b<TT_DATA>& xbuff, int index, input_stream<TT_DATA>* inStream) {
    xbuff.val.insert(index % 8, readincr_v<128 / 8 / sizeof(TT_DATA)>(inStream));
};

// function to clear buffer
template <typename TT_DATA>
T_buff_1024b<TT_DATA> INLINE_DECL null_buff_1024b() {
    T_buff_1024b<TT_DATA> ret;
    ret.val = ::aie::zeros<TT_DATA, ret.val.size()>();
    return ret;
};

// function to clear 512b buffer
template <typename TT_DATA>
T_buff_512b<TT_DATA> INLINE_DECL null_buff_512b() {
    T_buff_512b<TT_DATA> ret;
    ret.val = ::aie::zeros<TT_DATA, ret.val.size()>();
    return ret;
};

// function to clear 256b buffer
template <typename TT_DATA>
T_buff_256b<TT_DATA> INLINE_DECL null_buff_256b() {
    T_buff_256b<TT_DATA> ret;
    ret.val = ::aie::zeros<TT_DATA, ret.val.size()>();
    return ret;
};

inline namespace fir_api {
// T_acc struct with ::aie::accum
template <typename TT_DATA, typename TT_COEFF>
struct T_acc {
    using v_type = ::aie::accum<tAccBaseType_t<TT_DATA, TT_COEFF>, fnNumLanes<TT_DATA, TT_COEFF>()>;
    v_type val, uval;
    static constexpr unsigned getLanes() { return fnNumLanes<TT_DATA, TT_COEFF>(); };
    static constexpr unsigned getSize() { return fnAccSize<TT_DATA, TT_COEFF>(); };
};

// T_acc384 struct with ::aie::accum
template <typename TT_DATA, typename TT_COEFF>
struct T_acc384 {
    using v_type = ::aie::accum<tAccBaseType_t<TT_DATA, TT_COEFF>, fnNumLanes384<TT_DATA, TT_COEFF>()>;
    static constexpr unsigned getLanes() { return fnNumLanes384<TT_DATA, TT_COEFF>(); };
    static constexpr unsigned getSize() { return fnAccSize<TT_DATA, TT_COEFF>(); };
    v_type val, uval;
};

// function to clear acc
template <typename TT_DATA, typename TT_COEFF>
T_acc<TT_DATA, TT_COEFF> INLINE_DECL null_acc() {
    T_acc<TT_DATA, TT_COEFF> ret;
    //    using accTag = accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()>;
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;
    ret.val = ::aie::zeros<accTag, ret.val.size()>();
    return ret;
};

// T_outVal struct with ::aie::vector
template <typename TT_DATA, typename TT_COEFF>
struct T_outVal {
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value && std::is_same<TT_COEFF, int32>::value,
        int32_t,
        std::conditional_t<
            std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int32>::value,
            cint32_t,
            std::conditional_t<std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint32>::value,
                               cint32_t,
                               TT_DATA> > >
        T_OutType;

    // using v_type = ::aie::vector<T_OutType, fnNumLanes<TT_DATA, TT_COEFF>()>;
    // using out_type = T_OutType;
    using v_type = ::aie::vector<TT_DATA, fnNumLanes<TT_DATA, TT_COEFF>()>;
    using out_type = TT_DATA;
    v_type val;
    static constexpr unsigned getLanes() { return fnNumLanes<TT_DATA, TT_COEFF>(); };
};

// T_outVal384 struct with ::aie::vector
template <typename TT_DATA, typename TT_COEFF>
struct T_outVal384 {
    using v_type = ::aie::vector<TT_DATA, fnNumLanes384<TT_DATA, TT_COEFF>()>;
    v_type val;
    static constexpr unsigned getLanes() { return fnNumLanes384<TT_DATA, TT_COEFF>(); };
};

// Shift and saturate
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal<TT_DATA, TT_COEFF> shiftAndSaturate(T_acc<TT_DATA, TT_COEFF> acc, int shift) {
    T_outVal<TT_DATA, TT_COEFF> retVal;
    retVal.val = acc.val.template to_vector<TT_DATA>(shift);
    return retVal;
}

// Shift and saturate
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal384<TT_DATA, TT_COEFF> shiftAndSaturate(T_acc384<TT_DATA, TT_COEFF> acc, int shift) {
    T_outVal384<TT_DATA, TT_COEFF> retVal;
    retVal.val = acc.val.template to_vector<TT_DATA>(shift);
    return retVal;
}

template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP = 0>
using T_inputIF = fir::T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP>;
template <bool TP_CASC_IN, typename TT_DATA>
using T_outputIF = fir::T_outputIF<TP_CASC_IN, TT_DATA>;

// Overloaded function to write to window output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeWindow(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_outVal<TT_DATA, TT_COEFF> outVal) {
    // Do nothing if cascade output is present, but window output is not
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeWindow(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_outVal<TT_DATA, TT_COEFF> outVal) {
    window_writeincr(outInterface.outWindow, outVal.val);
    if
        constexpr(TP_NUM_OUTPUTS == 2) { window_writeincr(outInterface.outWindow2, outVal.val); }
}

// Overloaded function to write to window output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeWindow(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_outVal384<TT_DATA, TT_COEFF> outVal) {
    // Do nothing if cascade output is present, but window output is not
}
// Overloaded function to write to window output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeWindow(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_outVal384<TT_DATA, TT_COEFF> outVal) {
    window_writeincr(outInterface.outWindow, outVal.val);
    if
        constexpr(TP_NUM_OUTPUTS == 2) { window_writeincr(outInterface.outWindow2, outVal.val); }
}

// Overloaded function to write to stream output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeStream(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface,
                             T_outVal<TT_DATA, TT_COEFF> outVal,
                             int phase = 0) {
    // Do nothing if cascade output is present, but stream output is not
    static constexpr unsigned streamBitWidth = 128;
    static constexpr unsigned streamByteWidth = streamBitWidth / 8;
    int outPhase = 0;
    if
        constexpr(TP_NUM_OUTPUTS == 1) { writeincr(outInterface.outStream, outVal.val); }
    else if
        constexpr(TP_NUM_OUTPUTS == 2) {
            // Check if vector is a 256-bits and alternate between streams if 128-bits.
            static_assert(outVal.val.bits() % streamBitWidth == 0);
            if
                constexpr(outVal.val.bits() >= TP_NUM_OUTPUTS * streamBitWidth) {
                    // if outval length = 2 * streamBitWidth, write to both streams
                    // TODO: As per UG1076, Chapter 4, Using Streams in Parallel , use macro: WRITEINCR(MS_rsrc1, idx1,
                    // val) and WRITEINCR(MS_rsrc2, idx2, val)
                    put_wms(0, outVal.val.template extract<outVal.getLanes() / 2>(0));
                    put_wms(1, outVal.val.template extract<outVal.getLanes() / 2>(1));
                }
            else {
                if (phase == 0) {
                    put_wms(0, outVal.val);
                } else {
                    put_wms(1, outVal.val);
                }
            }
        }
}

// Overloaded function to write to stream output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeStream(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                             T_outVal<TT_DATA, TT_COEFF> outVal,
                             int phase = 0) {
    static constexpr unsigned streamBitWidth = 128;
    static constexpr unsigned streamByteWidth = streamBitWidth / 8;
    int outPhase = 0;
    if
        constexpr(TP_NUM_OUTPUTS == 1) { writeincr(outInterface.outStream, outVal.val); }
    else if
        constexpr(TP_NUM_OUTPUTS == 2) {
            // Check if vector is a 256-bits and alternate between streams if 128-bits.
            static_assert(outVal.val.bits() % streamBitWidth == 0);
            if
                constexpr(outVal.val.bits() >= TP_NUM_OUTPUTS * streamBitWidth) {
                    // if outval length = 2 * streamBitWidth, write to both streams
                    // TODO: As per UG1076, Chapter 4, Using Streams in Parallel , use macro: WRITEINCR(MS_rsrc1, idx1,
                    // val) and WRITEINCR(MS_rsrc2, idx2, val)
                    put_wms(0, outVal.val.template extract<outVal.getLanes() / 2>(0));
                    put_wms(1, outVal.val.template extract<outVal.getLanes() / 2>(1));
                }
            else {
                if (phase == 0) {
                    put_wms(0, outVal.val);
                } else {
                    put_wms(1, outVal.val);
                }
            }
        }
}

// Overloaded function to write to stream output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeStream(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface,
                             T_outVal384<TT_DATA, TT_COEFF> outVal,
                             int phase = 0) {
    // Do nothing if cascade output is present, but stream output is not
}

// Overloaded function to write to stream output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
INLINE_DECL void writeStream(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                             T_outVal384<TT_DATA, TT_COEFF> outVal,
                             int phase = 0) {
    static_assert(TP_NUM_OUTPUTS <= 2, "ERROR: Unsupported parameter range. Only up to 2 streams available.");

    static constexpr unsigned streamBitWidth = 128;
    static constexpr unsigned streamByteWidth = streamBitWidth / 8;
    int outPhase = 0;

    if
        constexpr(TP_NUM_OUTPUTS == 1) { writeincr(outInterface.outStream, outVal.val); }
    else if
        constexpr(TP_NUM_OUTPUTS == 2) {
            // To max HW performance, data is interleaved with 128-bit data blocks.

            // Check if vector is a 256-bits and alternate between streams if 128-bits.
            static_assert(outVal.val.bits() % streamBitWidth == 0);
            if
                constexpr(outVal.val.bits() >= TP_NUM_OUTPUTS * streamBitWidth) {
                    // if outval length = 2 * streamBitWidth, write to both streams
                    // TODO: As per UG1076, Chapter 4, Using Streams in Parallel , use macro: WRITEINCR(MS_rsrc1, idx1,
                    // val) and WRITEINCR(MS_rsrc2, idx2, val)
                    put_wms(0, outVal.val.template extract<outVal.getLanes() / 2>(0));
                    put_wms(1, outVal.val.template extract<outVal.getLanes() / 2>(1));
                }
            else {
                if (phase == 0) {
                    put_wms(0, outVal.val);
                } else {
                    put_wms(1, outVal.val);
                }
            }
        }
}

// Overloaded function to write to output.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = USE_WINDOW_API,
          unsigned int ENABLE_WRITE = 0>
INLINE_DECL void writeOutput(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface,
                             T_outVal<TT_DATA, TT_COEFF> outVal,
                             int phase = 0,
                             auto& outItr = 0) {
    // By default: Do nothing if cascade output is present, but window output is not.
    // However, SSR operation introduces a structure where both streams & cascades are used as outputs to increase
    // throughput.
    // In such case, a manual driver is added here.
    if
        constexpr(ENABLE_WRITE == 1) { writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, phase); }
}

// Overloaded function to write to output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1, unsigned int TP_API = USE_WINDOW_API>
INLINE_DECL void writeOutput(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                             T_outVal<TT_DATA, TT_COEFF> outVal,
                             int phase = 0,
                             auto& outItr = 0) {
    if
        constexpr(TP_API == USE_WINDOW_API) {
            // writeWindow<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal);
            *outItr++ = outVal.val;
        }
    else {
        writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, phase);
    }
}

// Overloaded function to write to output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1, unsigned int TP_API = USE_WINDOW_API>
INLINE_DECL void writeOutput(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface,
                             T_outVal384<TT_DATA, TT_COEFF> outVal,
                             int phase = 0,
                             auto& outItr = 0) {
    // By default: Do nothing if cascade output is present, but window output is not.
    // However, SSR operation may introduce a structure where both streams & cascades are used as outputs to increase
    // throughput.
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1, unsigned int TP_API = USE_WINDOW_API>
INLINE_DECL void writeOutput(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                             T_outVal384<TT_DATA, TT_COEFF> outVal,
                             int phase = 0,
                             auto& outItr = 0) {
    if
        constexpr(TP_API == USE_WINDOW_API) {
            // writeWindow<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal);
            *outItr++ = outVal.val;
        }
    else {
        writeStream<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS>(outInterface, outVal, phase);
    }
}

// Prevent the superfluous VSHUFFLEs on cascaded kernels,
// where complex and real data are zipped to a complex form,
// only to be unzipped on the other end.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr bool isShuffleNeeded() {
#ifdef __chess__
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;

    constexpr int isShuffleRequiredOnDev = (__SHUFFLE_CASCADE__ == 1) ? 1 : 0;
    constexpr int isShuffleRequiredDataType =
        ((std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int16>::value) ||
         (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint16>::value) ||
         (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int32>::value) ||
         // (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint32>::value) || - not complex coeffs.
         (std::is_same<TT_DATA, cint32>::value && std::is_same<TT_COEFF, int16>::value) ||
         // (std::is_same<TT_DATA, cint32>::value && std::is_same<TT_COEFF, cint32>::value) || - not complex coeffs.
         (std::is_same<TT_DATA, cint32>::value && std::is_same<TT_COEFF, int32>::value))
            ? 1
            : 0;
    if
        constexpr(isShuffleRequiredOnDev == 1 && isShuffleRequiredDataType == 1) { return true; }
    else {
        return false;
    }

#else
    return false;
#endif
};

// Overloaded function to skip writing to cascade output.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL void writeCascade(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_acc<TT_DATA, TT_COEFF> acc) {
    // Do nothing if window output is present, but cascade output is not
}

// Overloaded function to write to cascade output.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL void writeCascade(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_acc<TT_DATA, TT_COEFF> acc) {
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;

    auto accumulator = acc.val;
#if __SHUFFLE_CASCADE__ == 1
    if
        constexpr(isShuffleNeeded<TT_DATA, TT_COEFF>()) {
            auto[lo, hi] = ::aie::detail::unzip_complex(accumulator);

            constexpr unsigned N = decltype(lo)::size();
            auto tmp = lo.template grow<N * 2>();
            tmp.insert(1, hi);

            accumulator = tmp.template cast_to<cacc64>();
        }
#endif

    writeincr<accTag, fnNumLanes<TT_DATA, TT_COEFF>()>((output_cascade<accTag>*)outInterface.outCascade, accumulator);
}

// Overloaded function to write to cascade output.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL void writeCascade(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_acc384<TT_DATA, TT_COEFF> acc) {
    // Do nothing
}

// Overloaded function to write to cascade output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void writeCascade(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_acc384<TT_DATA, TT_COEFF> acc) {
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;
    auto accumulator = acc.val;

#if __SHUFFLE_CASCADE__ == 1
    if
        constexpr(isShuffleNeeded<TT_DATA, TT_COEFF>()) {
            auto[lo, hi] = ::aie::detail::unzip_complex(accumulator);

            constexpr unsigned N = decltype(lo)::size();
            auto tmp = lo.template grow<N * 2>();
            tmp.insert(1, hi);

            accumulator = tmp.template cast_to<cacc64>();
        }
#endif

    writeincr<accTag, fnNumLanes384<TT_DATA, TT_COEFF>()>((output_cascade<accTag>*)outInterface.outCascade,
                                                          accumulator);
}

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP = 0>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> readCascade(T_inputIF<false, TT_DATA, TP_DUAL_IP> inInterface,
                                                 T_acc<TT_DATA, TT_COEFF> acc) {
    // Do nothing
    T_acc<TT_DATA, TT_COEFF> ret;
    // using accTag = accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()>;
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;
    ret.val = ::aie::zeros<accTag, ret.val.size()>();
    return ret;
};

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP = 0>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> readCascade(T_inputIF<true, TT_DATA, TP_DUAL_IP> inInterface,
                                                 T_acc<TT_DATA, TT_COEFF> acc) {
    T_acc<TT_DATA, TT_COEFF> ret;
    // using accTag = accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()>;
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;
    auto accumulator =
        readincr_v<fnNumLanes<TT_DATA, TT_COEFF>(), accTag>((input_cascade<accTag>*)inInterface.inCascade);
#if __SHUFFLE_CASCADE__ == 1
    if
        constexpr(isShuffleNeeded<TT_DATA, TT_COEFF>()) {
            auto lo = accumulator.template extract<acc.val.size() / 2>(0).template cast_to<acc64>();
            auto hi = accumulator.template extract<acc.val.size() / 2>(1).template cast_to<acc64>();
            accumulator = ::aie::detail::combine_into_complex(lo, hi);
        }
#endif

    ret.val = accumulator;
    return ret;
};

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP = 0>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> readCascade(T_inputIF<false, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_acc384<TT_DATA, TT_COEFF> acc) {
    // Do nothing
    T_acc384<TT_DATA, TT_COEFF> ret;
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;
    ret.val = ::aie::zeros<accTag, ret.val.size()>();
    return ret;
};

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP = 0>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> readCascade(T_inputIF<true, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_acc384<TT_DATA, TT_COEFF> acc) {
    T_acc384<TT_DATA, TT_COEFF> ret;
    using accTag = tAccBaseType_t<TT_DATA, TT_COEFF>;

    auto accumulator =
        readincr_v<fnNumLanes384<TT_DATA, TT_COEFF>(), accTag>((input_cascade<accTag>*)inInterface.inCascade);
#if __SHUFFLE_CASCADE__ == 1
    if
        constexpr(isShuffleNeeded<TT_DATA, TT_COEFF>()) {
            auto lo = accumulator.template extract<acc.val.size() / 2>(0).template cast_to<acc64>();
            auto hi = accumulator.template extract<acc.val.size() / 2>(1).template cast_to<acc64>();
            accumulator = ::aie::detail::combine_into_complex(lo, hi);
        }
#endif
    ret.val = accumulator;
    return ret;
};

// Read 256-bits from a stream.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void readStream256(::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    xbuff.insert(index % 4, readincr_v<256 / 8 / sizeof(TT_DATA)>(inInterface.inStream));
};

// Read 256-bits from a stream.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void readStream256(T_buff_1024b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    xbuff.val.insert(index % 4, readincr_v<256 / 8 / sizeof(TT_DATA)>(inInterface.inStream));
};

// Read 256-bits from 2 streams. Read 128-bits from each.
template <bool TP_CASC_IN, typename TT_DATA>
INLINE_DECL void readStream256(T_buff_1024b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, DUAL_IP_DUAL> inInterface) {
    using wss_type = typename T_buff_128b<cint16>::v_type;
    wss_type data0, data1;
#ifdef __SUPPORTS_GETC_WSS__
    data0 = getc_wss(0);
    data1 = getc_wss(1);
#endif //__SUPPORTS_GETC_WSS__
    xbuff.val.insert(index % 4, concat_vector(data0.template cast_to<TT_DATA>(), data1.template cast_to<TT_DATA>()));
};

// Read 256-bits from a stream.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void readStream256(T_buff_512b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    xbuff.val.insert(index % 2, readincr_v<256 / 8 / sizeof(TT_DATA)>(inInterface.inStream));
};

// Read 256-bits from 2 streams. Read 128-bits from each.
template <bool TP_CASC_IN, typename TT_DATA>
INLINE_DECL void readStream256(T_buff_512b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, DUAL_IP_DUAL> inInterface) {
    using wss_type = typename T_buff_128b<cint16>::v_type;
    wss_type data0, data1;
#ifdef __SUPPORTS_GETC_WSS__
    data0 = getc_wss(0);
    data1 = getc_wss(1);
#endif //__SUPPORTS_GETC_WSS__
    xbuff.val.insert(index % 2, concat_vector(data0.template cast_to<TT_DATA>(), data1.template cast_to<TT_DATA>()));
};

// Read 256-bits from a stream.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void readStream256(T_buff_256b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    xbuff.val.insert(0, readincr_v<256 / 8 / sizeof(TT_DATA)>(inInterface.inStream));
};

// Read 256-bits from 2 streams. Read 128-bits from each.
template <bool TP_CASC_IN, typename TT_DATA>
INLINE_DECL void readStream256(T_buff_256b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, DUAL_IP_DUAL> inInterface) {
    using wss_type = typename T_buff_128b<cint16>::v_type;
    wss_type data0, data1;
#ifdef __SUPPORTS_GETC_WSS__
    data0 = getc_wss(0);
    data1 = getc_wss(1);
#endif //__SUPPORTS_GETC_WSS__
    xbuff.val.insert(0, concat_vector(data0.template cast_to<TT_DATA>(), data1.template cast_to<TT_DATA>()));
};

// Read 256-bits from 2 streams. Read 128-bits from each.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP>
INLINE_DECL void readStream256(::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)> xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface) {
    using wss_type = typename T_buff_128b<cint16>::v_type;
    wss_type data0, data1;
#ifdef __SUPPORTS_GETC_WSS__
    data0 = getc_wss(0);
    data1 = getc_wss(1);
    xbuff.insert(0, concat_vector(data0.template cast_to<TT_DATA>(), data1.template cast_to<TT_DATA>()));
#else
    xbuff.insert(0, readincr_v<256 / 8 / sizeof(TT_DATA)>(inInterface.inStream));
#endif //__SUPPORTS_GETC_WSS__
};

// Read 128-bits from a stream.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP = 0>
INLINE_DECL void readStream128(T_buff_1024b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                               int phase = 0) {
    xbuff.val.insert(index % 8, readincr_v<128 / 8 / sizeof(TT_DATA)>(inInterface.inStream));
};

// Read 128-bits from 2 streams sequencially.
template <bool TP_CASC_IN, typename TT_DATA>
INLINE_DECL void readStream128(T_buff_1024b<TT_DATA>& xbuff,
                               int index,
                               T_inputIF<TP_CASC_IN, TT_DATA, DUAL_IP_DUAL> inInterface,
                               const int phase = 0) {
    using wss_type = typename T_buff_128b<cint16>::v_type;
    wss_type data0;
#ifdef __SUPPORTS_GETC_WSS__
    if (phase % 2 == 0) {
        data0 = getc_wss(0);
    } else {
        data0 = getc_wss(1);
    }
#endif //__SUPPORTS_GETC_WSS__
    xbuff.val.insert(index % 8, data0.template cast_to<TT_DATA>());
};

template <bool COND, unsigned int NUM_LANES, typename PORT_TYPE>
INLINE_DECL auto cond_begin_vector_random_or_scalar_circular(PORT_TYPE& port) {
    if
        constexpr(NUM_LANES == 1) {
            if
                constexpr(COND) return ::aie::begin_random_circular(port);
            else
                return ::aie::random_circular_iterator<typename PORT_TYPE::value_type, 1024>(nullptr);
        }
    else {
        if
            constexpr(COND) return ::aie::begin_vector_random_circular<NUM_LANES>(port);
        else
            return ::aie::vector_random_circular_iterator<typename PORT_TYPE::value_type, NUM_LANES, 1024>(nullptr);
    }
}
}
}
}
} // namespaces

#include "kernel_coeff_reload.hpp"
#include "kernel_broadcast.hpp"

#endif // _DSPLIB_RELOAD_UTILS_HPP_
