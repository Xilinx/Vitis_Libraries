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
#ifndef _DSPLIB_FIR_INTERPOLATE_HB_UTILS_HPP_
#define _DSPLIB_FIR_INTERPOLATE_HB_UTILS_HPP_

#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb {
/*
Halfband interpolating FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/
// Specialised type for final accumulator. Concat of two polyphases
template <typename T_D, typename T_C>
struct T_accIntHb : T_acc<T_D, T_C> {
#if __HAS_SYM_PREADD__ == 0
    using v_type = ::aie::accum<typename tAccBaseType<T_D, T_C>::type, 2 * fnNumLanes384<T_D, T_C>()>;
    v_type val, uval;
    static constexpr unsigned getLanes() { return fnNumLanes384<T_D, T_C>(); };
    static constexpr unsigned getSize() { return fnAccSize<T_D, T_C>(); };
#else
    using T_acc<T_D, T_C>::operator=;
#endif
};

// for aie-ml
template <typename T_D, typename T_C, unsigned int T_API = 0>
struct T_accAsymIntHb : T_acc<T_D, T_C> {
    using T_acc<T_D, T_C>::operator=;
};
template <>
struct T_accAsymIntHb<int16, int16, 1> : T_acc384<int16, int16> {
    using T_acc384<int16, int16>::operator=;
};
template <>
struct T_accAsymIntHb<int16, int32, 1> : T_acc384<int16, int32> {
    using T_acc384<int16, int32>::operator=;
};
template <>
struct T_accAsymIntHb<int32, int16, 1> : T_acc384<int32, int16> {
    using T_acc384<int32, int16>::operator=;
};
template <>
struct T_accAsymIntHb<int32, int32, 1> : T_acc384<int32, int32> {
    using T_acc384<int32, int32>::operator=;
};
template <>
struct T_accAsymIntHb<cint16, int16, 1> : T_acc384<cint16, int16> {
    using T_acc384<cint16, int16>::operator=;
};
template <>
struct T_accAsymIntHb<cint16, int32, 1> : T_acc384<cint16, int32> {
    using T_acc384<cint16, int32>::operator=;
};
template <>
struct T_accAsymIntHb<cint16, cint16, 1> : T_acc384<cint16, cint16> {
    using T_acc384<cint16, cint16>::operator=;
};
template <>
struct T_accAsymIntHb<cint16, cint32, 1> : T_acc384<cint16, cint32> {
    using T_acc384<cint16, cint32>::operator=;
};
template <>
struct T_accAsymIntHb<cint32, int16, 1> : T_acc384<cint32, int16> {
    using T_acc384<cint32, int16>::operator=;
};
template <>
struct T_accAsymIntHb<cint32, cint16, 1> : T_acc384<cint32, cint16> {
    using T_acc384<cint32, cint16>::operator=;
};
template <>
struct T_accAsymIntHb<cint32, int32, 1> : T_acc384<cint32, int32> {
    using T_acc384<cint32, int32>::operator=;
};
template <>
struct T_accAsymIntHb<cint32, cint32, 1> : T_acc384<cint32, cint32> {
    using T_acc384<cint32, cint32>::operator=;
};
template <>
struct T_accAsymIntHb<float, float, 1> : T_acc384<float, float> {
    using T_acc384<float, float>::operator=;
};

template <typename T_D, typename T_C, unsigned int T_API = 0>
struct T_OutvalAsymIntHb : T_acc<T_D, T_C> {
    using T_acc<T_D, T_C>::operator=;
};
template <>
struct T_OutvalAsymIntHb<int16, int16, 1> : T_acc384<int16, int16> {
    using T_acc384<int16, int16>::operator=;
};
template <>
struct T_OutvalAsymIntHb<cint16, int16, 1> : T_acc384<cint16, int16> {
    using T_acc384<cint16, int16>::operator=;
};
template <>
struct T_OutvalAsymIntHb<cint16, cint16, 1> : T_acc384<cint16, cint16> {
    using T_acc384<cint16, cint16>::operator=;
};
// Smaller type for each polyphase
template <typename T_D, typename T_C, unsigned int T_UCT = 0>
struct T_accSymIntHb : T_acc384<T_D, T_C> {
    using T_acc384<T_D, T_C>::operator=;
};
// Upshift Center Tap - used with 16-bit integer data types - stores the upshifted ct in top lanes. No concat needed in
// such case.
template <>
struct T_accSymIntHb<int16, int16, 1> : T_acc<int16, int16> {
    using T_acc<int16, int16>::operator=;
};
template <>
struct T_accSymIntHb<cint16, int16, 1> : T_acc<cint16, int16> {
    using T_acc<cint16, int16>::operator=;
};
template <>
struct T_accSymIntHb<cint16, cint16, 1> : T_acc<cint16, cint16> {
    using T_acc<cint16, cint16>::operator=;
};

// Final output value type after shift and rounding
template <typename T_D, typename T_C>
struct T_outValIntHb : T_outVal<T_D, T_C> {
#if __HAS_SYM_PREADD__ == 0
    using v_type = ::aie::vector<T_D, 2 * fnNumLanes384<T_D, T_C>()>;
    using out_type = T_D;
    v_type val;
    static constexpr unsigned getLanes() { return 2 * fnNumLanes384<T_D, T_C>(); };
#else
    using T_outVal<T_D, T_C>::operator=;
#endif
};

// Final output value type after shift and rounding
template <typename T_D, typename T_C, unsigned int TP_API>
struct T_outValIntHbAsym {
    static constexpr unsigned int m_kLanes = TP_API == 0 ? fnNumLanes<T_D, T_C>() : fnNumLanes384<T_D, T_C>();
    using v_type = ::aie::vector<T_D, 2 * m_kLanes>;
    using out_type = T_D;
    v_type val;
    static constexpr unsigned getLanes() {
        if
            constexpr(TP_API == 0) return 2 * fnNumLanes<T_D, T_C>();
        else
            return 2 * fnNumLanes384<T_D, T_C>();
    }
};

// Overloaded function to write to output.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1, unsigned int TP_API = USE_WINDOW_API>
INLINE_DECL void writeOutputHbAsym(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                                   T_outValIntHbAsym<TT_DATA, TT_COEFF, TP_API> outVal,
                                   int phase = 0,
                                   auto& outItr = 0) {
    if
        constexpr(TP_API == USE_WINDOW_API) { *outItr++ = outVal.val; }
    else {
        writeincr(outInterface.outStream, outVal.val);
    }
}

// Halfband Output. This function takes the two polyphase lane sets, interleaves them and outputs them
// however for some types, the lane vector is too large for the interleave intrinsic, so they have to be split and for
// the splices to be interleaved.
// int16/int16
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1, unsigned int TP_API = USE_WINDOW_API>
INLINE_DECL void writeOutputIntHb(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                                  const T_accSymIntHb<TT_DATA, TT_COEFF> accLow,
                                  const T_accSymIntHb<TT_DATA, TT_COEFF> accHigh,
                                  const int shift,
                                  auto& outItr) {
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, 0>::v_type;
    using out_type = typename T_outValIntHb<TT_DATA, TT_COEFF>::out_type;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value && std::is_same<TT_COEFF, int32>::value, int32_t,
        std::conditional_t<
            std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int32>::value, cint32_t,
            std::conditional_t<std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint32>::value, cint32_t,
                               TT_DATA> > >
        T_OutType;
    T_outValIntHb<TT_DATA, TT_COEFF> outVal;
    T_accIntHb<TT_DATA, TT_COEFF> acc;
    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
#if __SUPPORTS_CFLOAT__ == 1
            v4cacc80 acc1, acc2, concatacc;
            v2cacc80 acc3, acc4;
            acc1 = lups(as_v4cint32(accLow.val), 0);
            acc2 = lups(as_v4cint32(accHigh.val), 0);
            acc3 = ext_lo(acc1);
            acc4 = ext_lo(acc2);
            concatacc = concat(acc3, acc4);
            outVal.val = as_v4cfloat(srs_ilv(concatacc, 0));
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
            acc3 = ext_hi(acc1);
            acc4 = ext_hi(acc2);
            concatacc = concat(acc3, acc4);
            outVal.val = as_v4cfloat(srs_ilv(concatacc, 0));
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
#endif
        }
    else if
        constexpr(std::is_same<TT_DATA, float>::value) {
            ::aie::vector<TT_DATA, accLow.getLanes() / 2> tmpExtract1 =
                accLow.val.template extract<accLow.getLanes() / 2>(0);
            ::aie::vector<TT_DATA, accLow.getLanes() / 2> tmpExtract2 =
                accHigh.val.template extract<accHigh.getLanes() / 2>(0);
            T_outValIntHb<TT_DATA, TT_COEFF> tempOutVal;
            auto[tmpIlv1, tmpIlv2] = ::aie::interleave_zip(tmpExtract1, tmpExtract2, 1);
            tempOutVal.val.insert(0, tmpIlv1);
            tempOutVal.val.insert(1, tmpIlv2);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, tempOutVal, 0, outItr);
            tmpExtract1 = accLow.val.template extract<accLow.getLanes() / 2>(1);
            tmpExtract2 = accHigh.val.template extract<accHigh.getLanes() / 2>(1);
            auto[tmpIlv3, tmpIlv4] = ::aie::interleave_zip(tmpExtract1, tmpExtract2, 1);
            tempOutVal.val.insert(0, tmpIlv3);
            tempOutVal.val.insert(1, tmpIlv4);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, tempOutVal, 0, outItr);
        }
    else if
        constexpr((std::is_same<TT_DATA, int16>::value && std::is_same<TT_COEFF, int16>::value) ||
                  (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int16>::value) ||
                  (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint16>::value) ||
                  (std::is_same<TT_DATA, int32>::value && std::is_same<TT_COEFF, int32>::value)) {
            // accLow & accHigh are short 384-bit accums, acc is an 768-bit long acc.
            acc.val = ::aie::concat<acc_type, acc_type>(accLow.val, accHigh.val);
            outVal.val = acc.val.template to_vector_zip<TT_DATA>(shift);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
        }
    else {
        if
            constexpr(std::is_same<TT_DATA, cint32>::value && std::is_same<TT_COEFF, cint32>::value) {
                // Special case for cint32 data and cint32 coeffs.
                // Concat of accs, shifted down is only 256-bits, instead of 512-bits like other combo types.
                // Need to split accs into 2 128-bit splices, when sending data to 2 128-bit stream outputs.
                auto concatVal1 = ::aie::concat((accLow.val), (accHigh.val));
                auto tmpAcc = concatVal1.template to_vector<TT_DATA>(shift); // ::aie::from_vector<cacc64>(conca);
                // interleave
                auto tmpExtract1 = tmpAcc.template extract<tmpAcc.size() / 2>(0);
                auto tmpExtract2 = tmpAcc.template extract<tmpAcc.size() / 2>(1);
                auto[tmpIlv1, tmpIlv2] = ::aie::interleave_zip(tmpExtract1, tmpExtract2, 1);
                outVal.val = tmpIlv1;
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
                outVal.val = tmpIlv2;
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 1, outItr);
            }
        else if
            constexpr(std::is_same<TT_DATA, T_OutType>::value) {
                // accLow & accHigh are long 768-bit accums, acc is an 768-bit long acc.
                acc.val = ::aie::concat(accLow.val.template extract<accLow.getLanes() / 2>(0),
                                        accHigh.val.template extract<accHigh.getLanes() / 2>(0));
                outVal.val = acc.val.template to_vector_zip<TT_DATA>(shift);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
                acc.val = ::aie::concat(accLow.val.template extract<accLow.getLanes() / 2>(1),
                                        accHigh.val.template extract<accHigh.getLanes() / 2>(1));
                outVal.val = acc.val.template to_vector_zip<TT_DATA>(shift);
                writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
            }
        else {
            // Special case for 16-bit data and 32-bit coeffs.
            // 80-bit accs shifted down to 16-bit output type require a 2-stage srs.
            // First, shift and interleave.
            // Then, shift down further to 16-bit output
            T_outValIntHb<T_OutType, int16> outIntermediateVal;
            T_accIntHb<T_OutType, int16> accIntermediate;
            acc.val = ::aie::concat(accLow.val.template extract<accLow.getLanes() / 2>(0),
                                    accHigh.val.template extract<accHigh.getLanes() / 2>(0));
            outIntermediateVal.val = acc.val.template to_vector_zip<T_OutType>(shift);
            accIntermediate.val.from_vector(outIntermediateVal.val, 0); // upshift back to 48-bit acc
            outVal.val = accIntermediate.val.template to_vector<TT_DATA>(0);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
            acc.val = ::aie::concat(accLow.val.template extract<accLow.getLanes() / 2>(1),
                                    accHigh.val.template extract<accHigh.getLanes() / 2>(01));
            outIntermediateVal.val = acc.val.template to_vector_zip<T_OutType>(shift);
            accIntermediate.val.from_vector(outIntermediateVal.val, 0); // upshift back to 48-bit acc
            outVal.val = accIntermediate.val.template to_vector<TT_DATA>(0);
            writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 1, outItr);
        }
    }
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_OUTPUTS = 1, unsigned int TP_API = USE_WINDOW_API>
INLINE_DECL void writeOutputIntHb(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                                  const T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accLow,
                                  const T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accHigh,
                                  const int shift,
                                  auto& outItr) {
    using acc_type = typename T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API>::v_type;
    using out_type = typename T_outValIntHbAsym<TT_DATA, TT_COEFF, TP_API>::out_type;
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value && std::is_same<TT_COEFF, int32>::value, int32_t,
        std::conditional_t<
            std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int32>::value, cint32_t,
            std::conditional_t<std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint32>::value, cint32_t,
                               TT_DATA> > >
        T_OutType;
    T_outValIntHbAsym<TT_DATA, TT_COEFF, TP_API> outVal;
    if
        constexpr((std::is_same<TT_DATA, int16>::value && std::is_same<TT_COEFF, int16>::value) ||
                  (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, int16>::value) ||
                  (std::is_same<TT_DATA, cint16>::value && std::is_same<TT_COEFF, cint16>::value) ||
                  (std::is_same<TT_DATA, int32>::value && std::is_same<TT_COEFF, int32>::value)) {
            // accLow & accHigh are short 384-bit accums, acc is an 768-bit long acc.
            auto tmp1 = accLow.val.template to_vector<TT_DATA>(shift);
            auto tmp2 = accHigh.val.template to_vector<TT_DATA>(shift);
            auto[tmpIlv1, tmpIlv2] = ::aie::interleave_zip(accLow.val.template to_vector<TT_DATA>(shift),
                                                           accHigh.val.template to_vector<TT_DATA>(shift), 1);
            outVal.val.insert(0, tmpIlv1);
            outVal.val.insert(1, tmpIlv2);
            writeOutputHbAsym<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
        }
    else {
        if
            constexpr(std::is_same<TT_DATA, cint32>::value && std::is_same<TT_COEFF, cint32>::value) {
                // Special case for cint32 data and cint32 coeffs.
                // Concat of accs, shifted down is only 256-bits, instead of 512-bits like other combo types.
                // Need to split accs into 2 128-bit splices, when sending data to 2 128-bit stream outputs.
                auto concatVal1 = ::aie::concat((accLow.val), (accHigh.val));
                auto tmpAcc = concatVal1.template to_vector<TT_DATA>(shift);
                // interleave
                auto tmpExtract1 = tmpAcc.template extract<tmpAcc.size() / 2>(0);
                auto tmpExtract2 = tmpAcc.template extract<tmpAcc.size() / 2>(1);
                auto[tmpIlv1, tmpIlv2] = ::aie::interleave_zip(tmpExtract1, tmpExtract2, 1);
                outVal.val = ::aie::concat(tmpIlv1, tmpIlv2);
                writeOutputHbAsym<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
            }
        else if
            constexpr(std::is_same<TT_DATA, T_OutType>::value) {
                // accLow & accHigh are long 768-bit accums, acc is an 768-bit long acc.
                auto accLowShifted = accLow.val.template to_vector<TT_DATA>(shift);
                auto accHighShifted = accHigh.val.template to_vector<TT_DATA>(shift);

                outVal.val = ::aie::concat(::aie::interleave_zip(accLowShifted, accHighShifted, 1).first,
                                           ::aie::interleave_zip(accLowShifted, accHighShifted, 1).second);
                writeOutputHbAsym<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
            }
        else {
            // Special case for 16-bit data and 32-bit coeffs.
            // 80-bit accs shifted down to 16-bit output type require a 2-stage srs.
            // First, shift and interleave.
            // Then, shift down further to 16-bit output
            auto accHighShifted = accHigh.val.template to_vector<TT_DATA>(shift);
            auto accLowShifted = accLow.val.template to_vector<TT_DATA>(shift);
            outVal.val = ::aie::concat(::aie::interleave_zip(accLowShifted, accHighShifted, 1).first,
                                       ::aie::interleave_zip(accLowShifted, accHighShifted, 1).second);
            writeOutputHbAsym<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
        }
    }
}

// Halfband Output. This function takes the two polyphase lanes passed in one acc argument, interleaves them and outputs
// them.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API>
INLINE_DECL void writeOutputIntHb(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                                  const T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> accHigh,
                                  const int shift,
                                  auto& outItr) {
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>::v_type;
    acc_type tmp;
    tmp = accHigh.val;
    T_outValIntHb<TT_DATA, TT_COEFF> outVal;
    outVal.val = tmp.template to_vector_zip<TT_DATA>(shift);
    writeOutput<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, outVal, 0, outItr);
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL int upshiftPos() {
    int retVal;
    retVal = TP_FIR_LEN;
    return retVal;
}

// Overloaded function to write to window output.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API>
INLINE_DECL void writeOutputSel(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface,
                                T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> accHP,
                                T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> accLP,
                                unsigned int shift,
                                auto& outItr) {
    // Do nothing
}

// Overloaded function to write to window output.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API>
INLINE_DECL void writeOutputSel(T_outputIF<CASC_OUT_TRUE, TT_DATA> outInterface,
                                T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accHP,
                                T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accLP,
                                unsigned int shift,
                                auto& outItr) {
    // Do nothing
}

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API>
INLINE_DECL void writeOutputSel(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                                T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> accHP,
                                T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> accLP,
                                unsigned int shift,
                                auto& outItr) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            writeOutputIntHb<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, accHP, accLP, shift, outItr);
        }
    else {
        writeOutputIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_NUM_OUTPUTS, TP_API>(outInterface, accHP, shift, outItr);
    }
}

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT = 0,
          unsigned int TP_API>
INLINE_DECL void writeOutputSel(T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface,
                                T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accHP,
                                T_accAsymIntHb<TT_DATA, TT_COEFF, TP_API> accLP,
                                unsigned int shift,
                                auto& outItr) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            writeOutputIntHb<TT_DATA, TT_COEFF, TP_NUM_OUTPUTS, TP_API>(outInterface, accHP, accLP, shift, outItr);
        }
    else {
        writeOutputIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT, TP_NUM_OUTPUTS, TP_API>(outInterface, accHP, shift, outItr);
    }
}
#ifdef __X86SIM__
// #define _DSPLIB_FIR_SR_SYM_DEBUG_ 1
#endif

// template for mulSlidingSym1buffUCT - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> mulSlidingSym1buffUCT(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart, // no ystart - API calculates ystart based on Points size
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int cfShift) {
    constexpr unsigned int Lanes = 2 * fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = (TP_FIR_LEN + 1) / 2;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStep = 1;
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>::v_type;
    // acc_type chess_storage(bm0) tmp;
    acc_type tmp;

    // #define _DSPLIB_FIR_INT_HB_UTILS_DEBUG_

    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
#if __HAS_SYM_PREADD__ == 1
    tmp = ::aie::sliding_mul_sym_uct<Lanes, Points, CoeffStep, DataStep>(zbuff.val, zstart, xbuff.val, xstart, cfShift);
#else
    tmp = ::aie::sliding_mul<Lanes, Points, CoeffStep, DataStep>(zbuff.val, zstart, xbuff.val, xstart);
#endif
    retVal.val = tmp;
    return retVal;
}
// template for macSlidingSymUCT1buff - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> macSlidingSymUCT1buff(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart, // no ystart - API calculates ystart based on Points size
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int cfShift) {
    constexpr unsigned int Lanes = 2 * fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = (TP_FIR_LEN + 1) / 2;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStep = 1;
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>::v_type;
    acc_type tmp;

    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
#if __HAS_SYM_PREADD__ == 1
    tmp = ::aie::sliding_mac_sym_uct<Lanes, Points, CoeffStep, DataStep>(acc.val, zbuff.val, zstart, xbuff.val, xstart,
                                                                         cfShift);
#else
    tmp = ::aie::sliding_mac<Lanes, Points, CoeffStep, DataStep>(acc.val, zbuff.val, zstart, xbuff.val, xstart);
#endif
    retVal.val = tmp;
    return retVal;
}

// template for mulSlidingSymUCT2buff - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> mulSlidingSymUCT2buff(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int cfShift) {
    constexpr unsigned int Lanes = 2 * fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
    // adjust Point size to where the center tap is in relation to number of columns in low-level intrinsic
    constexpr unsigned int Points = 2 * (((TP_FIR_LEN + 1) / 4 - 1) % fnNumSymColsIntHb<TT_DATA, TT_COEFF>() + 1);
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStep = 1;
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>::v_type;
    acc_type tmp;

    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
#if __HAS_SYM_PREADD__ == 1
    tmp = ::aie::sliding_mul_sym_uct<Lanes, Points, CoeffStep, DataStep>(zbuff.val, zstart, xbuff.val, xstart,
                                                                         ybuff.val, ystart, cfShift);
#else
    tmp = ::aie::sliding_mul<Lanes, Points, CoeffStep, DataStep>(zbuff.val, zstart, xbuff.val, xstart);
#endif
    retVal.val = tmp;
    return retVal;
}
// template for macSlidingSymUCT2buff - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> macSlidingSymUCT2buff(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int cfShift) {
    constexpr unsigned int Lanes = 2 * fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
    // adjust Point size to where the center tap is in relation to number of columns in low-level intrinsic
    constexpr unsigned int Points = 2 * (((TP_FIR_LEN + 1) / 4 - 1) % fnNumSymColsIntHb<TT_DATA, TT_COEFF>() + 1);
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStep = 1;
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>::v_type;
    acc_type tmp;

    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
#if __HAS_SYM_PREADD__ == 1
    tmp = ::aie::sliding_mac_sym_uct<Lanes, Points, CoeffStep, DataStep>(acc.val, zbuff.val, zstart, xbuff.val, xstart,
                                                                         ybuff.val, ystart, cfShift);
#else
    tmp = ::aie::sliding_mac<Lanes, Points, CoeffStep, DataStep>(acc.val, zbuff.val, zstart, xbuff.val, xstart);
#endif
    retVal.val = tmp;
    return retVal;
}

// template for macSlidingSym2buff - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> macSlidingSym2buff(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
    // adjust Point size to where the center tap is in relation to number of columns in low-level intrinsic
    constexpr unsigned int Points = 2 * (((TP_FIR_LEN + 1) / 4 - 1) % fnNumSymColsIntHb<TT_DATA, TT_COEFF>() + 1);
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStep = 1;
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, 0>::v_type;
    // acc_type chess_storage(aml0) tmp;  // lower part of bm0, used in UCT
    acc_type tmp; // lower part of bm0, used in UCT
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
    if
        constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
            retVal.val =
                ::aie::sliding_mac<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(), fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(
                    acc.val, zbuff.val, zstart, xbuff.val, xstart);
            retVal.val =
                ::aie::sliding_mac<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(), fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(
                    retVal.val, zbuff.val, zstart, ybuff.val, ystart);
        }
    else {
        retVal = acc;
#if __HAS_SYM_PREADD__ == 1
        tmp = ::aie::sliding_mac_sym<Lanes, Points, CoeffStep, DataStep>(acc.val.template extract<Lanes>(0), zbuff.val,
                                                                         zstart, xbuff.val, xstart, ybuff.val, ystart);
#else
        tmp = ::aie::sliding_mac<Lanes, Points, CoeffStep, DataStep>(acc.val, zbuff.val, zstart, xbuff.val, xstart);
#endif
        retVal.val = retVal.val.insert(0, tmp);
    }
    return retVal;
}

// template for mulSlidingSym2buff - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> mulSlidingSym2buff(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
    // adjust Point size to where the center tap is in relation to number of columns in low-level intrinsic
    constexpr unsigned int Points = 2 * (((TP_FIR_LEN + 1) / 4 - 1) % fnNumSymColsIntHb<TT_DATA, TT_COEFF>() + 1);
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = 1;
    using acc_type = typename T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>::v_type;
    // acc_type chess_storage(aml0) tmp;  // lower part of bm0, used in UCT
    acc_type tmp; // lower part of bm0, used in UCT
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
    if
        constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
            retVal.val =
                ::aie::sliding_mul<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(), fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(
                    zbuff.val, zstart, xbuff.val, xstart);
            retVal.val =
                ::aie::sliding_mac<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(), fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(
                    retVal.val, zbuff.val, zstart, ybuff.val, ystart);
        }
    else {
#if __HAS_SYM_PREADD__ == 1
        tmp = ::aie::sliding_mul_sym<Lanes, Points, CoeffStep, DataStep>(zbuff.val, zstart, xbuff.val, xstart,
                                                                         ybuff.val, ystart);
#else
        tmp = ::aie::sliding_mul<Lanes, Points, CoeffStep, DataStep>(zbuff.val, zstart, xbuff.val, xstart);
#endif
        retVal.val = retVal.val.insert(0, tmp);
    }
    return retVal;
}

// MAC operation for 1buff arch. Template function which also hides the struct contents.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT = 0>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> mulSym1buffIntHb(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;

    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded function which uses native vectors
            constexpr unsigned int Lanes = fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
            constexpr unsigned int Points = kSymmetryFactor * fnNumSymColsIntHb<TT_DATA, TT_COEFF>();
            constexpr unsigned int CoeffStep = 1;
            constexpr unsigned int DataStepX = 1;
            constexpr unsigned int DataStepY = 1;

            // floats datapath doesn't have a pre-adder, need to issue 2 x non-sym calls.
            if
                constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
                    retVal.val = ::aie::sliding_mul<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(),
                                                    fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
                    retVal.val = ::aie::sliding_mac<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(),
                                                    fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(retVal.val, zbuff.val,
                                                                                            zstart, xbuff.val, ystart);
                }
            else {
#if __HAS_SYM_PREADD__ == 1
                retVal.val =
                    ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                               tAccBaseType_t<TT_DATA, TT_COEFF> >::mul_sym(zbuff.val, zstart,
                                                                                            xbuff.val, xstart, ystart);
#else
                retVal.val =
                    ::aie::sliding_mul<Lanes, Points, CoeffStep, DataStepX>(zbuff.val, zstart, xbuff.val, xstart);
#endif
            }
        }
    else {
        // Do nothing, API _uct call already instantiated all the required calls.
        retVal = acc;
    }
    return retVal;
};

// MAC operation for 1buff arch. Template function which also hides the struct contents.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT = 0>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> macSym1buffIntHb(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;

    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded function which uses native vectors
            constexpr unsigned int Lanes = fnNumSymLanesIntHb<TT_DATA, TT_COEFF>();
            constexpr unsigned int Points = 2 * fnNumSymColsIntHb<TT_DATA, TT_COEFF>();
            constexpr unsigned int CoeffStep = 1;
            constexpr unsigned int DataStepX = 1;
            constexpr unsigned int DataStepY = 1;

            // floats datapath doesn't have a pre-adder, need to issue 2 x non-sym calls.
            if
                constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
                    retVal.val = ::aie::sliding_mac<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(),
                                                    fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(acc.val, zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
                    retVal.val = ::aie::sliding_mac<fnNumSymLanesIntHb<TT_DATA, TT_COEFF>(),
                                                    fnNumSymColsIntHb<TT_DATA, TT_COEFF>()>(retVal.val, zbuff.val,
                                                                                            zstart, xbuff.val, ystart);
                }
            else {
#if __HAS_SYM_PREADD__ == 1
                retVal.val = ::aie::sliding_mac_sym<Lanes, Points, CoeffStep, DataStepX, DataStepY>(
                    acc.val, zbuff.val, zstart, xbuff.val, xstart, ystart);
#else
                retVal.val = ::aie::sliding_mac<Lanes, Points, CoeffStep, DataStepX>(acc.val, zbuff.val, zstart,
                                                                                     xbuff.val, xstart);
#endif
            }
        }
    else {
        // Do nothing, API _uct call already instantiated all the required calls.
        retVal = acc;
    }
    return retVal;
};

// MAC operation for 2buff arch. Calls APIs sliding_mac.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> macSym2buffIntHb(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return macSlidingSym2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart, zbuff,
                                                                            zstart);
};

// MAC operation for 2buff arch. Calls APIs sliding_mac or sliding_mac_uct. Overloaded with extra argument to call UCT.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> macSym2buffIntHb(
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int ctShift) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            return macSlidingSym2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart,
                                                                                    zbuff, zstart);
        }
    else {
        // Call API _uct .
        return macSlidingSymUCT2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart,
                                                                                   zbuff, zstart, ctShift);
    }
};

// MAC operation for Low Polyphase 1buff arch. Template function which also hides the struct contents.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> mulCentreTap2buffIntHb(T_buff_512b<TT_DATA> xbuff,
                                                                                   unsigned int xstart,
                                                                                   T_buff_256b<TT_COEFF> zbuff) {
    unsigned int zstart = 0;
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> retVal;
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded low level function which uses native vectors
            retVal.val =
                ::aie::sliding_mul<fnNumLanes384<TT_DATA, TT_COEFF>(), fnNumCols384<TT_DATA, TT_COEFF>(), 1, 1, 1,
                                   tAccBaseType_t<TT_DATA, TT_COEFF> >(zbuff.val, zstart, xbuff.val, xstart);
        }
    else {
        // return undef vector. Nothing to do here.
    }

    return retVal;
};

// Initial MUL operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> initMacIntHb(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int ctShift) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded low level function which uses native vectors
            return mulSym1buffIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT>(acc, xbuff, xstart, ystart, zbuff, zstart);
        }
    else {
        // Call API  .
        return mulSlidingSym1buffUCT<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, zbuff, zstart,
                                                                                   ctShift);
    }
};
// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> initMacIntHb(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int ctShift) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded low level function which uses native vectors
            return macSym1buffIntHb(acc, xbuff, xstart, ystart, zbuff, zstart);
        }
    else {
        // Call API UCT MACs, API call unrolls all required operations, so subsequent calls to macSym1buffIntHb are not
        // required.
        // 1 Buff arch fits all required data in xbuff.
        // Note. API call to single MAC with a 1buff is not available.
        return macSlidingSymUCT1buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, zbuff, zstart,
                                                                                   ctShift);
    }
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_UPSHIFT_CT, unsigned int TP_API = 0>
INLINE_DECL auto mulCentreTap1buffIntHb(T_buff_1024b<TT_DATA> xbuff, unsigned int xstart, T_buff_256b<TT_COEFF> zbuff) {
    unsigned int zstart = 0;
#if __HAS_SYM_PREADD__ == 1
    static constexpr unsigned int lanes = fnNumLanes384<TT_DATA, TT_COEFF>();
#else
    static constexpr unsigned int lanes =
        TP_API == 0 ? fnNumLanes<TT_DATA, TT_COEFF>() : fnNumLanes384<TT_DATA, TT_COEFF>();
#endif
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded low level function which uses native vectors
            return ::aie::sliding_mul<lanes, fnNumCols384<TT_DATA, TT_COEFF>(), 1, 1, 1,
                                      tAccBaseType_t<TT_DATA, TT_COEFF> >(zbuff.val, zstart, xbuff.val, xstart);
        }
    else {
        // return undef vector. Nothing to do here.
    }

    // return retVal;
};

// Initial MAC operation for 2buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> initMacIntHb(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int ctShift) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded low level function which uses native vectors
            return macSlidingSym2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart,
                                                                                    zbuff, zstart);
        }
    else {
        return mulSlidingSymUCT2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart,
                                                                                   zbuff, zstart, ctShift);
    }
};
// Initial MAC operation for 2buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_UPSHIFT_CT,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> initMacIntHb(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accSymIntHb<TT_DATA, TT_COEFF, TP_UPSHIFT_CT> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    unsigned int ctShift) {
    if
        constexpr(TP_UPSHIFT_CT == 0) {
            // Call overloaded low level function which uses native vectors
            return macSlidingSym2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart,
                                                                                    zbuff, zstart);
        }
    else {
        return macSlidingSymUCT2buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_UPSHIFT_CT>(acc, xbuff, xstart, ybuff, ystart,
                                                                                   zbuff, zstart, ctShift);
    }
};
}
}
}
}
}
#endif // _DSPLIB_FIR_INTERPOLATE_HB_UTILS_HPP_
