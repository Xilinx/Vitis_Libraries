#pragma once

#ifndef _DSPLIB_KERNEL_UTILS_HPP_
#define _DSPLIB_KERNEL_UTILS_HPP_

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
#include "fir_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {

template <typename T>
struct T_buff_128b {};
template <>
struct T_buff_128b<int16> {
    using v_type = v8int16;
    v_type val;
};
template <>
struct T_buff_128b<cint16> {
    using v_type = v4cint16;
    v_type val;
};
template <>
struct T_buff_128b<int32> {
    using v_type = v4int32;
    v_type val;
};
template <>
struct T_buff_128b<cint32> {
    using v_type = v2cint32;
    v_type val;
};
template <>
struct T_buff_128b<float> {
    using v_type = v4float;
    v_type val;
};
template <>
struct T_buff_128b<cfloat> {
    using v_type = v2cfloat;
    v_type val;
};

template <typename T>
struct T_buff_256b {};
template <>
struct T_buff_256b<int16> {
    using v_type = v16int16;
    v_type val;
};
template <>
struct T_buff_256b<cint16> {
    using v_type = v8cint16;
    v_type val;
};
template <>
struct T_buff_256b<int32> {
    using v_type = v8int32;
    v_type val;
};
template <>
struct T_buff_256b<cint32> {
    using v_type = v4cint32;
    v_type val;
};
template <>
struct T_buff_256b<float> {
    using v_type = v8float;
    v_type val;
};
template <>
struct T_buff_256b<cfloat> {
    using v_type = v4cfloat;
    v_type val;
};

template <typename T>
struct T_buff_512b {};
template <>
struct T_buff_512b<int16> {
    using v_type = v32int16;
    v_type val;
};
template <>
struct T_buff_512b<cint16> {
    using v_type = v16cint16;
    v_type val;
};
template <>
struct T_buff_512b<int32> {
    using v_type = v16int32;
    v_type val;
};
template <>
struct T_buff_512b<cint32> {
    using v_type = v8cint32;
    v_type val;
};
template <>
struct T_buff_512b<float> {
    using v_type = v16float;
    v_type val;
};
template <>
struct T_buff_512b<cfloat> {
    using v_type = v8cfloat;
    v_type val;
};

template <typename T>
struct T_buff_1024b {};
template <>
struct T_buff_1024b<int16> {
    using v_type = v64int16;
    v_type val;
};
template <>
struct T_buff_1024b<cint16> {
    using v_type = v32cint16;
    v_type val;
};
template <>
struct T_buff_1024b<int32> {
    using v_type = v32int32;
    v_type val;
};
template <>
struct T_buff_1024b<cint32> {
    using v_type = v16cint32;
    v_type val;
};
template <>
struct T_buff_1024b<float> {
    using v_type = v32float;
    v_type val;
};
template <>
struct T_buff_1024b<cfloat> {
    using v_type = v16cfloat;
    v_type val;
};

template <typename T_D, typename T_C>
struct T_acc {};
template <>
struct T_acc<int16, int16> {
    v16acc48 val = null_v16acc48();
    v16acc48 uval = null_v16acc48();
};
template <>
struct T_acc<cint16, int16> {
    v8cacc48 val = null_v8cacc48();
    v8cacc48 uval = null_v8cacc48();
};
template <>
struct T_acc<cint16, cint16> {
    v8cacc48 val = null_v8cacc48();
    v8cacc48 uval = null_v8cacc48();
};
template <>
struct T_acc<int32, int16> {
    v8acc80 val = null_v8acc80();
    v8acc80 uval = null_v8acc80();
};
template <>
struct T_acc<int32, int32> {
    v8acc80 val = null_v8acc80();
    v8acc80 uval = null_v8acc80();
};
template <>
struct T_acc<cint32, int16> {
    v4cacc80 val = null_v4cacc80();
    v4cacc80 uval = null_v4cacc80();
};
template <>
struct T_acc<cint32, int32> {
    v4cacc80 val = null_v4cacc80();
    v4cacc80 uval = null_v4cacc80();
};
template <>
struct T_acc<cint32, cint16> {
    v4cacc80 val = null_v4cacc80();
    v4cacc80 uval = null_v4cacc80();
};
template <>
struct T_acc<cint32, cint32> {
    v2cacc80 val = null_v2cacc80();
    v2cacc80 uval = null_v2cacc80();
};
template <>
struct T_acc<float, float> {
    v8float val = null_v8float();
    v8float uval = null_v8float();
};
template <>
struct T_acc<cfloat, float> {
    v4cfloat val = null_v4cfloat();
    v4cfloat uval = null_v4cfloat();
};
template <>
struct T_acc<cfloat, cfloat> {
    v4cfloat val = null_v4cfloat();
    v4cfloat uval = null_v4cfloat();
};

template <typename T_D, typename T_C>
struct T_acc384 {};
template <>
struct T_acc384<int16, int16> {
    v8acc48 val = null_v8acc48();
    v8acc48 uval = null_v8acc48();
};
template <>
struct T_acc384<cint16, int16> {
    v4cacc48 val = null_v4cacc48();
    v4cacc48 uval = null_v4cacc48();
};
template <>
struct T_acc384<cint16, cint16> {
    v4cacc48 val = null_v4cacc48();
    v4cacc48 uval = null_v4cacc48();
};
template <>
struct T_acc384<int32, int16> {
    v4acc80 val = null_v4acc80();
    v4acc80 uval = null_v4acc80();
};
template <>
struct T_acc384<int32, int32> {
    v4acc80 val = null_v4acc80();
    v4acc80 uval = null_v4acc80();
};
template <>
struct T_acc384<cint32, int16> {
    v2cacc80 val = null_v2cacc80();
    v2cacc80 uval = null_v2cacc80();
};
template <>
struct T_acc384<cint32, int32> {
    v2cacc80 val = null_v2cacc80();
    v2cacc80 uval = null_v2cacc80();
};
template <>
struct T_acc384<cint32, cint16> {
    v2cacc80 val = null_v2cacc80();
    v2cacc80 uval = null_v2cacc80();
};
template <>
struct T_acc384<cint32, cint32> {
    v2cacc80 val = null_v2cacc80();
    v2cacc80 uval = null_v2cacc80();
};
template <>
struct T_acc384<float, float> : T_acc<float, float> {
    using T_acc<float, float>::operator=;
};
template <>
struct T_acc384<cfloat, float> : T_acc<cfloat, float> {
    using T_acc<cfloat, float>::operator=;
};
template <>
struct T_acc384<cfloat, cfloat> : T_acc<cfloat, cfloat> {
    using T_acc<cfloat, cfloat>::operator=;
};

template <typename T_D, typename T_C>
struct T_outVal {};
template <>
struct T_outVal<int16, int16> {
    using v_type = v16int16;
    v_type val;
};
template <>
struct T_outVal<cint16, int16> {
    using v_type = v8cint16;
    v_type val;
};
template <>
struct T_outVal<cint16, cint16> {
    using v_type = v8cint16;
    v_type val;
};
template <>
struct T_outVal<int32, int16> {
    using v_type = v8int32;
    v_type val;
};
template <>
struct T_outVal<int32, int32> {
    using v_type = v8int32;
    v_type val;
};
template <>
struct T_outVal<cint32, int16> {
    using v_type = v4cint32;
    v_type val;
};
template <>
struct T_outVal<cint32, int32> {
    using v_type = v4cint32;
    v_type val;
};
template <>
struct T_outVal<cint32, cint16> {
    using v_type = v4cint32;
    v_type val;
};
template <>
struct T_outVal<cint32, cint32> {
    using v_type = v4cint32;
    v_type val;
};
template <>
struct T_outVal<float, float> {
    using v_type = v8float;
    v_type val;
};
template <>
struct T_outVal<cfloat, float> {
    using v_type = v4cfloat;
    v_type val;
};
template <>
struct T_outVal<cfloat, cfloat> {
    using v_type = v4cfloat;
    v_type val;
};

template <typename T_D, typename T_C>
struct T_outVal384 {};
template <>
struct T_outVal384<int16, int16> {
    using v_type = v8int16;
    v_type val;
};
template <>
struct T_outVal384<cint16, int16> {
    using v_type = v4cint16;
    v_type val;
};
template <>
struct T_outVal384<cint16, cint16> {
    using v_type = v4cint16;
    v_type val;
};
template <>
struct T_outVal384<int32, int16> {
    using v_type = v4int32;
    v_type val;
};
template <>
struct T_outVal384<int32, int32> {
    using v_type = v4int32;
    v_type val;
};
template <>
struct T_outVal384<cint32, int16> {
    using v_type = v2cint32;
    v_type val;
};
template <>
struct T_outVal384<cint32, int32> {
    using v_type = v2cint32;
    v_type val;
};
template <>
struct T_outVal384<cint32, cint16> {
    using v_type = v2cint32;
    v_type val;
};
template <>
struct T_outVal384<cint32, cint32> {
    using v_type = v2cint32;
    v_type val;
};
template <>
struct T_outVal384<float, float> : T_outVal<float, float> {
    using T_outVal<float, float>::operator=;
};
template <>
struct T_outVal384<cfloat, float> : T_outVal<cfloat, float> {
    using T_outVal<cfloat, float>::operator=;
};
template <>
struct T_outVal384<cfloat, cfloat> : T_outVal<cfloat, cfloat> {
    using T_outVal<cfloat, cfloat>::operator=;
};

// function to window_readincr a 128 bit vector
template <typename TT_DATA>
T_buff_128b<TT_DATA> inline window_read_128b(input_window<TT_DATA>* inWindow) {
    printf("Error: unknown data type\n");
};
template <>
T_buff_128b<int16> inline window_read_128b<int16>(input_window<int16>* inWindow) {
    T_buff_128b<int16> result;
    result.val = window_read_v8(inWindow);
    return result;
};
template <>
T_buff_128b<cint16> inline window_read_128b<cint16>(input_window<cint16>* inWindow) {
    T_buff_128b<cint16> result;
    result.val = window_read_v4(inWindow);
    return result;
};
template <>
T_buff_128b<int32> inline window_read_128b<int32>(input_window<int32>* inWindow) {
    T_buff_128b<int32> result;
    result.val = window_read_v4(inWindow);
    return result;
};
template <>
T_buff_128b<cint32> inline window_read_128b<cint32>(input_window<cint32>* inWindow) {
    // The following code is a workaround to the fact that 128b window reads are not yet supported for cint32.
    T_buff_128b<cint32> result;
    v4int32 castVector;
    input_window<int32>* castWindow;
    castWindow = (input_window<int32>*)inWindow;
    castVector = window_read_v4(castWindow);

    result.val = as_v2cint32(castVector);
    return result;
};
template <>
T_buff_128b<float> inline window_read_128b<float>(input_window<float>* inWindow) {
    T_buff_128b<float> result;
    result.val = window_read_v4(inWindow);
    return result;
};
template <>
T_buff_128b<cfloat> inline window_read_128b<cfloat>(input_window<cfloat>* inWindow) {
    T_buff_128b<cfloat> result;
    v4int32 castVector;
    input_window<int32>* castWindow;
    castWindow = (input_window<int32>*)inWindow;
    castVector = window_read_v4(castWindow);

    result.val = as_v2cfloat(castVector);
    return result;
};
// function to window_readincr a 128 bit vector
template <typename TT_DATA>
T_buff_128b<TT_DATA> inline window_readincr_128b(input_window<TT_DATA>* inWindow) {
    printf("Error: unknown data type\n");
};
template <>
T_buff_128b<int16> inline window_readincr_128b<int16>(input_window<int16>* inWindow) {
    T_buff_128b<int16> result;
    result.val = window_readincr_v8(inWindow);
    return result;
};
template <>
T_buff_128b<cint16> inline window_readincr_128b<cint16>(input_window<cint16>* inWindow) {
    T_buff_128b<cint16> result;
    result.val = window_readincr_v4(inWindow);
    return result;
};
template <>
T_buff_128b<int32> inline window_readincr_128b<int32>(input_window<int32>* inWindow) {
    T_buff_128b<int32> result;
    result.val = window_readincr_v4(inWindow);
    return result;
};
template <>
T_buff_128b<cint32> inline window_readincr_128b<cint32>(input_window<cint32>* inWindow) {
    // The following code is a workaround to the fact that 128b window reads are not yet supported for cint32.
    T_buff_128b<cint32> result;
    v4int32 castVector;
    input_window<int32>* castWindow;
    castWindow = (input_window<int32>*)inWindow;
    castVector = window_read_v4(castWindow);

    window_incr(inWindow, 2);
    result.val = as_v2cint32(castVector);
    return result;
};
template <>
T_buff_128b<float> inline window_readincr_128b<float>(input_window<float>* inWindow) {
    T_buff_128b<float> result;
    result.val = window_readincr_v4(inWindow);
    return result;
};
template <>
T_buff_128b<cfloat> inline window_readincr_128b<cfloat>(input_window<cfloat>* inWindow) {
    T_buff_128b<cfloat> result;
    v4int32 castVector;
    input_window<int32>* castWindow;
    castWindow = (input_window<int32>*)inWindow;
    castVector = window_read_v4(castWindow);

    window_incr(inWindow, 2);
    result.val = as_v2cfloat(castVector);
    return result;
};

// function to window_readincr a 256 bit vector
template <typename TT_DATA>
T_buff_256b<TT_DATA> inline window_readincr_256b(input_window<TT_DATA>* inWindow) {
    printf("Error: unknown data type\n");
};
template <>
T_buff_256b<int16> inline window_readincr_256b<int16>(input_window<int16>* inWindow) {
    T_buff_256b<int16> result;
    result.val = window_readincr_v16(inWindow);
    return result;
};
template <>
T_buff_256b<cint16> inline window_readincr_256b<cint16>(input_window<cint16>* inWindow) {
    T_buff_256b<cint16> result;
    result.val = window_readincr_v8(inWindow);
    return result;
};
template <>
T_buff_256b<int32> inline window_readincr_256b<int32>(input_window<int32>* inWindow) {
    T_buff_256b<int32> result;
    result.val = window_readincr_v8(inWindow);
    return result;
};
template <>
T_buff_256b<cint32> inline window_readincr_256b<cint32>(input_window<cint32>* inWindow) {
    T_buff_256b<cint32> result;
    result.val = window_readincr_v4(inWindow);
    return result;
};
template <>
T_buff_256b<float> inline window_readincr_256b<float>(input_window<float>* inWindow) {
    T_buff_256b<float> result;
    result.val = window_readincr_v8(inWindow);
    return result;
};
template <>
T_buff_256b<cfloat> inline window_readincr_256b<cfloat>(input_window<cfloat>* inWindow) {
    T_buff_256b<cfloat> result;
    result.val = window_readincr_v4(inWindow);
    return result;
};

// function to window_readdecr a 128 vector
template <typename TT_DATA>
T_buff_128b<TT_DATA> inline window_readdecr_128b(input_window<TT_DATA>* inWindow) {
    printf("Error: unknown data type\n");
};
template <>
T_buff_128b<int16> inline window_readdecr_128b<int16>(input_window<int16>* inWindow) {
    T_buff_128b<int16> result;
    result.val = window_readdecr_v8(inWindow);
    return result;
};
template <>
T_buff_128b<cint16> inline window_readdecr_128b<cint16>(input_window<cint16>* inWindow) {
    T_buff_128b<cint16> result;
    result.val = window_readdecr_v4(inWindow);
    return result;
};
template <>
T_buff_128b<int32> inline window_readdecr_128b<int32>(input_window<int32>* inWindow) {
    T_buff_128b<int32> result;
    result.val = window_readdecr_v4(inWindow);
    return result;
};
template <>
T_buff_128b<cint32> inline window_readdecr_128b<cint32>(input_window<cint32>* inWindow) {
    // The following code is a workaround to the fact that 128b window reads are not yet supported for cint32.
    T_buff_128b<cint32> result;
    v4int32 castVector;
    input_window<int32>* castWindow;
    castWindow = (input_window<int32>*)inWindow;
    castVector = window_read_v4(castWindow);

    window_decr(inWindow, 2);
    result.val = as_v2cint32(castVector);
    return result;
};
template <>
T_buff_128b<float> inline window_readdecr_128b<float>(input_window<float>* inWindow) {
    T_buff_128b<float> result;
    result.val = window_readdecr_v4(inWindow);
    return result;
};
template <>
T_buff_128b<cfloat> inline window_readdecr_128b<cfloat>(input_window<cfloat>* inWindow) {
    T_buff_128b<cfloat> result;
    v4int32 castVector;
    input_window<int32>* castWindow;
    castWindow = (input_window<int32>*)inWindow;
    castVector = window_read_v4(castWindow);

    window_decr(inWindow, 2);
    result.val = as_v2cfloat(castVector);
    return result;
};

// function to window_readdecr a vector
template <typename TT_DATA>
T_buff_256b<TT_DATA> inline window_readdecr_256b(input_window<TT_DATA>* inWindow) {
    printf("Error: unknown data type\n");
};
template <>
T_buff_256b<int16> inline window_readdecr_256b<int16>(input_window<int16>* inWindow) {
    T_buff_256b<int16> result;
    result.val = window_readdecr_v16(inWindow);
    return result;
};
template <>
T_buff_256b<cint16> inline window_readdecr_256b<cint16>(input_window<cint16>* inWindow) {
    T_buff_256b<cint16> result;
    result.val = window_readdecr_v8(inWindow);
    return result;
};
template <>
T_buff_256b<int32> inline window_readdecr_256b<int32>(input_window<int32>* inWindow) {
    T_buff_256b<int32> result;
    result.val = window_readdecr_v8(inWindow);
    return result;
};
template <>
T_buff_256b<cint32> inline window_readdecr_256b<cint32>(input_window<cint32>* inWindow) {
    T_buff_256b<cint32> result;
    result.val = window_readdecr_v4(inWindow);
    return result;
};
template <>
T_buff_256b<float> inline window_readdecr_256b<float>(input_window<float>* inWindow) {
    T_buff_256b<float> result;
    result.val = window_readdecr_v8(inWindow);
    return result;
};
template <>
T_buff_256b<cfloat> inline window_readdecr_256b<cfloat>(input_window<cfloat>* inWindow) {
    T_buff_256b<cfloat> result;
    result.val = window_readdecr_v4(inWindow);
    return result;
};

// function to window_readdecr a vector
template <typename TT_DATA>
T_buff_256b<TT_DATA> inline window_read_256b(input_window<TT_DATA>* inWindow) {
    printf("Error: unknown data type\n");
};
template <>
T_buff_256b<int16> inline window_read_256b<int16>(input_window<int16>* inWindow) {
    T_buff_256b<int16> result;
    result.val = window_read_v16(inWindow);
    return result;
};
template <>
T_buff_256b<cint16> inline window_read_256b<cint16>(input_window<cint16>* inWindow) {
    T_buff_256b<cint16> result;
    result.val = window_read_v8(inWindow);
    return result;
};
template <>
T_buff_256b<int32> inline window_read_256b<int32>(input_window<int32>* inWindow) {
    T_buff_256b<int32> result;
    result.val = window_read_v8(inWindow);
    return result;
};
template <>
T_buff_256b<cint32> inline window_read_256b<cint32>(input_window<cint32>* inWindow) {
    T_buff_256b<cint32> result;
    result.val = window_read_v4(inWindow);
    return result;
};
template <>
T_buff_256b<float> inline window_read_256b<float>(input_window<float>* inWindow) {
    T_buff_256b<float> result;
    result.val = window_read_v8(inWindow);
    return result;
};
template <>
T_buff_256b<cfloat> inline window_read_256b<cfloat>(input_window<cfloat>* inWindow) {
    T_buff_256b<cfloat> result;
    result.val = window_read_v4(inWindow);
    return result;
};

// function to clear buffer
template <typename TT_DATA>
T_buff_1024b<TT_DATA> inline null_buff_1024b() {
    printf("Error: unknown data type\n");
};
template <>
T_buff_1024b<int16> inline null_buff_1024b<int16>() {
    T_buff_1024b<int16> result;
    result.val = null_v64int16();
    return result;
};
template <>
T_buff_1024b<cint16> inline null_buff_1024b<cint16>() {
    T_buff_1024b<cint16> result;
    result.val = null_v32cint16();
    return result;
};
template <>
T_buff_1024b<int32> inline null_buff_1024b<int32>() {
    T_buff_1024b<int32> result;
    result.val = null_v32int32();
    return result;
};
template <>
T_buff_1024b<cint32> inline null_buff_1024b<cint32>() {
    T_buff_1024b<cint32> result;
    result.val = null_v16cint32();
    return result;
};
template <>
T_buff_1024b<float> inline null_buff_1024b<float>() {
    T_buff_1024b<float> result;
    result.val = null_v32float();
    return result;
};
template <>
T_buff_1024b<cfloat> inline null_buff_1024b<cfloat>() {
    T_buff_1024b<cfloat> result;
    result.val = null_v16cfloat();
    return result;
};

// function to clear 512b buffer
template <typename TT_DATA>
T_buff_512b<TT_DATA> inline null_buff_512b() {
    printf("Error: unknown data type\n");
};
template <>
T_buff_512b<int16> inline null_buff_512b<int16>() {
    T_buff_512b<int16> result;
    result.val = null_v32int16();
    return result;
};
template <>
T_buff_512b<cint16> inline null_buff_512b<cint16>() {
    T_buff_512b<cint16> result;
    result.val = null_v16cint16();
    return result;
};
template <>
T_buff_512b<int32> inline null_buff_512b<int32>() {
    T_buff_512b<int32> result;
    result.val = null_v16int32();
    return result;
};
template <>
T_buff_512b<cint32> inline null_buff_512b<cint32>() {
    T_buff_512b<cint32> result;
    result.val = null_v8cint32();
    return result;
};
template <>
T_buff_512b<float> inline null_buff_512b<float>() {
    T_buff_512b<float> result;
    result.val = null_v16float();
    return result;
};
template <>
T_buff_512b<cfloat> inline null_buff_512b<cfloat>() {
    T_buff_512b<cfloat> result;
    result.val = null_v8cfloat();
    return result;
};

// function to clear 256b buffer
template <typename TT_DATA>
T_buff_256b<TT_DATA> inline null_buff_256b() {
    printf("Error: unknown data type\n");
};
template <>
T_buff_256b<int16> inline null_buff_256b<int16>() {
    T_buff_256b<int16> result;
    result.val = null_v16int16();
    return result;
};
template <>
T_buff_256b<cint16> inline null_buff_256b<cint16>() {
    T_buff_256b<cint16> result;
    result.val = null_v8cint16();
    return result;
};
template <>
T_buff_256b<int32> inline null_buff_256b<int32>() {
    T_buff_256b<int32> result;
    result.val = null_v8int32();
    return result;
};
template <>
T_buff_256b<cint32> inline null_buff_256b<cint32>() {
    T_buff_256b<cint32> result;
    result.val = null_v4cint32();
    return result;
};
template <>
T_buff_256b<float> inline null_buff_256b<float>() {
    T_buff_256b<float> result;
    result.val = null_v8float();
    return result;
};
template <>
T_buff_256b<cfloat> inline null_buff_256b<cfloat>() {
    T_buff_256b<cfloat> result;
    result.val = null_v4cfloat();
    return result;
};

// Overloaded shift and saturate calls to allow null operation for float types
template <typename TT_DATA, typename TT_COEFF>
inline T_outVal<TT_DATA, TT_COEFF> shiftAndSaturate(const T_acc<TT_DATA, TT_COEFF> acc, const int shift) {
    T_outVal<TT_DATA, TT_COEFF> retVal;
    retVal.val = srs(acc.val, shift);
    return retVal;
};
template <>
inline T_outVal<cint32, cint32> shiftAndSaturate(const T_acc<cint32, cint32> acc, const int shift) {
    T_outVal<cint32, cint32> retVal;
    v2cint32 lower, upper;
    lower = srs(acc.val, shift);
    upper = srs(acc.uval, shift);
    retVal.val = upd_v(retVal.val, 0, lower);
    retVal.val = upd_v(retVal.val, 1, upper);
    return retVal;
};

template <>
inline T_outVal<float, float> shiftAndSaturate(const T_acc<float, float> acc, const int shift) {
    T_outVal<float, float> retVal;
    retVal.val = acc.val;
    return retVal; // Shift and saturate do not apply to float types
};

template <>
inline T_outVal<cfloat, float> shiftAndSaturate(const T_acc<cfloat, float> acc, const int shift) {
    T_outVal<cfloat, float> retVal;
    retVal.val = acc.val;
    return retVal; // Shift and saturate do not apply to float types
};

template <>
inline T_outVal<cfloat, cfloat> shiftAndSaturate(const T_acc<cfloat, cfloat> acc, const int shift) {
    T_outVal<cfloat, cfloat> retVal;
    retVal.val = acc.val;
    return retVal; // Shift and saturate do not apply to float types
};

// Overloaded shift and saturate calls to allow null operation for float types
template <typename TT_DATA, typename TT_COEFF>
inline T_outVal384<TT_DATA, TT_COEFF> shiftAndSaturate(const T_acc384<TT_DATA, TT_COEFF> acc, const int shift) {
    T_outVal384<TT_DATA, TT_COEFF> retVal;
    retVal.val = srs(acc.val, shift);
    return retVal;
};

// Function to upload shifted accumulation to output vector. Usually this is trivial, with v2cint32 to v4cint32 being
// the exception
inline void upshift(v16int16& outVal, int pass, v16int16 shiftVal) {
    outVal = shiftVal;
}
inline void upshift(v8cint16& outVal, int pass, v8cint16 shiftVal) {
    outVal = shiftVal;
}
inline void upshift(v8int32& outVal, int pass, v8int32 shiftVal) {
    outVal = shiftVal;
}
inline void upshift(v4cint32& outVal, int pass, v4cint32 shiftVal) {
    outVal = shiftVal;
}
inline void upshift(v4cint32& outVal, int pass, v2cint32 shiftVal) {
    outVal = upd_v(outVal, pass, shiftVal);
}

#define CASC_IN_TRUE true
#define CASC_IN_FALSE false
#define CASC_OUT_TRUE true
#define CASC_OUT_FALSE false

// Overloaded function to write to cascade output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_acc<TT_DATA, TT_COEFF> acc) {
    put_mcd(ext_lo(acc.val));
    put_mcd(ext_hi(acc.val));
}
// specialized functions for cint32/cint32 type
template <>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, cint32> outInterface, T_acc<cint32, cint32> acc) {
    put_mcd(acc.val);
    put_mcd(acc.uval);
}
// specialized functions for float types
template <>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, float> outInterface, T_acc<float, float> acc) {
    put_mcd(ups(as_v8int32(acc.val), 0));
}
template <>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, cfloat> outInterface, T_acc<cfloat, float> acc) {
    put_mcd((ups(as_v4cint32(acc.val), 0)));
}
template <>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, cfloat> outInterface, T_acc<cfloat, cfloat> acc) {
    put_mcd((ups(as_v4cint32(acc.val), 0)));
}

// Overloaded function to skip writing to cascade output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeCascade(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_acc<TT_DATA, TT_COEFF> acc) {
    // Do nothing
}

// Overloaded function to write to cascade output.
template <typename TT_DATA, typename TT_COEFF>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_acc384<TT_DATA, TT_COEFF> acc) {
    put_mcd(acc.val);
}
// specialized functions for cint32/cint32 type
template <>
inline void writeCascade(T_outputIF<CASC_OUT_TRUE, cint32> outInterface, T_acc384<cint32, cint32> acc) {
    put_mcd(acc.val);
}
// Overloaded function to skip writing to cascade output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeCascade(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_acc384<TT_DATA, TT_COEFF> acc) {
    // Do nothing
}

// Overloaded function to write to window output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeWindow(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_outVal<TT_DATA, TT_COEFF> outVal) {
    // Do nothing
}
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeWindow(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_outVal<TT_DATA, TT_COEFF> outVal) {
    window_writeincr(outInterface.outWindow, outVal.val);
    if
        constexpr(TP_NUM_OUTPUTS == 2) { window_writeincr(outInterface.outWindow2, outVal.val); }
}

// Overloaded function to write to window output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeWindow(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface, T_outVal384<TT_DATA, TT_COEFF> outVal) {
    // Do nothing
}
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1>
inline void writeWindow(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface, T_outVal384<TT_DATA, TT_COEFF> outVal) {
    window_writeincr(outInterface.outWindow, outVal.val);
    if
        constexpr(TP_NUM_OUTPUTS == 2) { window_writeincr(outInterface.outWindow2, outVal.val); }
}

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
inline T_acc<TT_DATA, TT_COEFF> readCascade(T_inputIF<false, TT_DATA, TP_DUAL_IP> inInterface,
                                            T_acc<TT_DATA, TT_COEFF> acc) {
    T_acc<TT_DATA, TT_COEFF> null_ret;
    return null_ret;
};

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
inline T_acc<TT_DATA, TT_COEFF> readCascade(T_inputIF<true, TT_DATA, TP_DUAL_IP> inInterface,
                                            T_acc<TT_DATA, TT_COEFF> acc) {
    // Do nothing here. All cases covered below.
    return acc;
};
template <>
inline T_acc<int16, int16> readCascade(T_inputIF<true, int16, 0> inInterface, T_acc<int16, int16> acc) {
    T_acc<int16, int16> ret;
    ret.val = upd_lo(acc.val, get_scd());
    ret.val = upd_hi(ret.val, get_scd());
    return ret;
};
template <>
inline T_acc<cint16, int16> readCascade(T_inputIF<true, cint16, 0> inInterface, T_acc<cint16, int16> acc) {
    T_acc<cint16, int16> ret;
    ret.val = upd_lo(acc.val, getc_scd());
    ret.val = upd_hi(ret.val, getc_scd());
    return ret;
};
template <>
inline T_acc<cint16, cint16> readCascade(T_inputIF<true, cint16, 0> inInterface, T_acc<cint16, cint16> acc) {
    T_acc<cint16, cint16> ret;
    ret.val = upd_lo(acc.val, getc_scd());
    ret.val = upd_hi(ret.val, getc_scd());
    return ret;
};
template <>
inline T_acc<int32, int16> readCascade(T_inputIF<true, int32, 0> inInterface, T_acc<int32, int16> acc) {
    T_acc<int32, int16> ret;
    ret.val = upd_lo(acc.val, getl_scd());
    ret.val = upd_hi(ret.val, getl_scd());
    return ret;
};
template <>
inline T_acc<int32, int32> readCascade(T_inputIF<true, int32, 0> inInterface, T_acc<int32, int32> acc) {
    T_acc<int32, int32> ret;
    ret.val = upd_lo(acc.val, getl_scd());
    ret.val = upd_hi(ret.val, getl_scd());
    return ret;
};
template <>
inline T_acc<cint32, int16> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc<cint32, int16> acc) {
    T_acc<cint32, int16> ret;
    ret.val = upd_lo(acc.val, getlc_scd());
    ret.val = upd_hi(ret.val, getlc_scd());
    return ret;
};
template <>
inline T_acc<cint32, int32> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc<cint32, int32> acc) {
    T_acc<cint32, int32> ret;
    ret.val = upd_lo(acc.val, getlc_scd());
    ret.val = upd_hi(ret.val, getlc_scd());
    return ret;
};
template <>
inline T_acc<cint32, cint16> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc<cint32, cint16> acc) {
    T_acc<cint32, cint16> ret;
    ret.val = upd_lo(acc.val, getlc_scd());
    ret.val = upd_hi(ret.val, getlc_scd());
    return ret;
};
template <>
inline T_acc<cint32, cint32> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc<cint32, cint32> acc) {
    T_acc<cint32, cint32> ret;
    ret.val = getlc_scd();
    ret.uval = getlc_scd();
    return ret;
};
template <>
inline T_acc<float, float> readCascade(T_inputIF<true, float, 0> inInterface, T_acc<float, float> acc) {
    T_acc<float, float> ret;
    ret.val = as_v8float(lsrs(get_scd(), 0));
    return ret;
};
template <>
inline T_acc<cfloat, float> readCascade(T_inputIF<true, cfloat, 0> inInterface, T_acc<cfloat, float> acc) {
    T_acc<cfloat, float> ret;
    ret.val = as_v4cfloat(lsrs(getc_scd(), 0));
    return ret;
};
template <>
inline T_acc<cfloat, cfloat> readCascade(T_inputIF<true, cfloat, 0> inInterface, T_acc<cfloat, cfloat> acc) {
    T_acc<cfloat, cfloat> ret;
    ret.val = as_v4cfloat(lsrs(getc_scd(), 0));
    return ret;
};
template <>
inline T_acc<int16, int16> readCascade(T_inputIF<true, int16, 1> inInterface, T_acc<int16, int16> acc) {
    T_acc<int16, int16> ret;
    ret.val = upd_lo(acc.val, get_scd());
    ret.val = upd_hi(ret.val, get_scd());
    return ret;
};
template <>
inline T_acc<cint16, int16> readCascade(T_inputIF<true, cint16, 1> inInterface, T_acc<cint16, int16> acc) {
    T_acc<cint16, int16> ret;
    ret.val = upd_lo(acc.val, getc_scd());
    ret.val = upd_hi(ret.val, getc_scd());
    return ret;
};
template <>
inline T_acc<cint16, cint16> readCascade(T_inputIF<true, cint16, 1> inInterface, T_acc<cint16, cint16> acc) {
    T_acc<cint16, cint16> ret;
    ret.val = upd_lo(acc.val, getc_scd());
    ret.val = upd_hi(ret.val, getc_scd());
    return ret;
};
template <>
inline T_acc<int32, int16> readCascade(T_inputIF<true, int32, 1> inInterface, T_acc<int32, int16> acc) {
    T_acc<int32, int16> ret;
    ret.val = upd_lo(acc.val, getl_scd());
    ret.val = upd_hi(ret.val, getl_scd());
    return ret;
};
template <>
inline T_acc<int32, int32> readCascade(T_inputIF<true, int32, 1> inInterface, T_acc<int32, int32> acc) {
    T_acc<int32, int32> ret;
    ret.val = upd_lo(acc.val, getl_scd());
    ret.val = upd_hi(ret.val, getl_scd());
    return ret;
};
template <>
inline T_acc<cint32, int16> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc<cint32, int16> acc) {
    T_acc<cint32, int16> ret;
    ret.val = upd_lo(acc.val, getlc_scd());
    ret.val = upd_hi(ret.val, getlc_scd());
    return ret;
};
template <>
inline T_acc<cint32, int32> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc<cint32, int32> acc) {
    T_acc<cint32, int32> ret;
    ret.val = upd_lo(acc.val, getlc_scd());
    ret.val = upd_hi(ret.val, getlc_scd());
    return ret;
};
template <>
inline T_acc<cint32, cint16> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc<cint32, cint16> acc) {
    T_acc<cint32, cint16> ret;
    ret.val = upd_lo(acc.val, getlc_scd());
    ret.val = upd_hi(ret.val, getlc_scd());
    return ret;
};
template <>
inline T_acc<cint32, cint32> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc<cint32, cint32> acc) {
    T_acc<cint32, cint32> ret;
    ret.val = getlc_scd();
    ret.uval = getlc_scd();
    return ret;
};
template <>
inline T_acc<float, float> readCascade(T_inputIF<true, float, 1> inInterface, T_acc<float, float> acc) {
    T_acc<float, float> ret;
    ret.val = as_v8float(lsrs(get_scd(), 0));
    return ret;
};
template <>
inline T_acc<cfloat, float> readCascade(T_inputIF<true, cfloat, 1> inInterface, T_acc<cfloat, float> acc) {
    T_acc<cfloat, float> ret;
    ret.val = as_v4cfloat(lsrs(getc_scd(), 0));
    return ret;
};
template <>
inline T_acc<cfloat, cfloat> readCascade(T_inputIF<true, cfloat, 1> inInterface, T_acc<cfloat, cfloat> acc) {
    T_acc<cfloat, cfloat> ret;
    ret.val = as_v4cfloat(lsrs(getc_scd(), 0));
    return ret;
};

// Template to skip reading cascade when interface is not present
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
inline T_acc384<TT_DATA, TT_COEFF> readCascade(T_inputIF<false, TT_DATA, TP_DUAL_IP> inInterface,
                                               T_acc384<TT_DATA, TT_COEFF> acc) {
    T_acc384<TT_DATA, TT_COEFF> null_ret;
    return null_ret;
};
// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
inline T_acc384<TT_DATA, TT_COEFF> readCascade(T_inputIF<true, TT_DATA, TP_DUAL_IP> inInterface,
                                               T_acc384<TT_DATA, TT_COEFF> acc) {
    // Do nothing here. All cases covered below.
    return acc;
};
template <>
inline T_acc384<int16, int16> readCascade(T_inputIF<true, int16, 0> inInterface, T_acc384<int16, int16> acc) {
    T_acc384<int16, int16> ret;
    ret.val = get_scd();
    return ret;
};
template <>
inline T_acc384<cint16, int16> readCascade(T_inputIF<true, cint16, 0> inInterface, T_acc384<cint16, int16> acc) {
    T_acc384<cint16, int16> ret;
    ret.val = getc_scd();
    return ret;
};
template <>
inline T_acc384<cint16, cint16> readCascade(T_inputIF<true, cint16, 0> inInterface, T_acc384<cint16, cint16> acc) {
    T_acc384<cint16, cint16> ret;
    ret.val = getc_scd();
    return ret;
};
template <>
inline T_acc384<int32, int16> readCascade(T_inputIF<true, int32, 0> inInterface, T_acc384<int32, int16> acc) {
    T_acc384<int32, int16> ret;
    ret.val = getl_scd();
    return ret;
};
template <>
inline T_acc384<int32, int32> readCascade(T_inputIF<true, int32, 0> inInterface, T_acc384<int32, int32> acc) {
    T_acc384<int32, int32> ret;
    ret.val = getl_scd();
    return ret;
};
template <>
inline T_acc384<cint32, int16> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc384<cint32, int16> acc) {
    T_acc384<cint32, int16> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<cint32, int32> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc384<cint32, int32> acc) {
    T_acc384<cint32, int32> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<cint32, cint16> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc384<cint32, cint16> acc) {
    T_acc384<cint32, cint16> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<cint32, cint32> readCascade(T_inputIF<true, cint32, 0> inInterface, T_acc384<cint32, cint32> acc) {
    T_acc384<cint32, cint32> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<int16, int16> readCascade(T_inputIF<true, int16, 1> inInterface, T_acc384<int16, int16> acc) {
    T_acc384<int16, int16> ret;
    ret.val = get_scd();
    return ret;
};
template <>
inline T_acc384<cint16, int16> readCascade(T_inputIF<true, cint16, 1> inInterface, T_acc384<cint16, int16> acc) {
    T_acc384<cint16, int16> ret;
    ret.val = getc_scd();
    return ret;
};
template <>
inline T_acc384<cint16, cint16> readCascade(T_inputIF<true, cint16, 1> inInterface, T_acc384<cint16, cint16> acc) {
    T_acc384<cint16, cint16> ret;
    ret.val = getc_scd();
    return ret;
};
template <>
inline T_acc384<int32, int16> readCascade(T_inputIF<true, int32, 1> inInterface, T_acc384<int32, int16> acc) {
    T_acc384<int32, int16> ret;
    ret.val = getl_scd();
    return ret;
};
template <>
inline T_acc384<int32, int32> readCascade(T_inputIF<true, int32, 1> inInterface, T_acc384<int32, int32> acc) {
    T_acc384<int32, int32> ret;
    ret.val = getl_scd();
    return ret;
};
template <>
inline T_acc384<cint32, int16> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc384<cint32, int16> acc) {
    T_acc384<cint32, int16> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<cint32, int32> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc384<cint32, int32> acc) {
    T_acc384<cint32, int32> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<cint32, cint16> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc384<cint32, cint16> acc) {
    T_acc384<cint32, cint16> ret;
    ret.val = getlc_scd();
    return ret;
};
template <>
inline T_acc384<cint32, cint32> readCascade(T_inputIF<true, cint32, 1> inInterface, T_acc384<cint32, cint32> acc) {
    T_acc384<cint32, cint32> ret;
    ret.val = getlc_scd();
    return ret;
};
}
}
}
} // namespaces

#include "kernel_rtp_reload.hpp"
#include "kernel_broadcast.hpp"

#endif // _DSPLIB_KERNEL_UTILS_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

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
