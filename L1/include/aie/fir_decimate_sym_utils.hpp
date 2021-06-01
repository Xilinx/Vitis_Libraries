#ifndef _DSPLIB_FIR_DECIMATE_SYM_UTILS_HPP_
#define _DSPLIB_FIR_DECIMATE_SYM_UTILS_HPP_

/*
Symmetrical Decimation FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "fir_decimate_sym.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_sym {

template <typename TT_DATA, typename TT_COEFF>
struct T_accDecSym {};
template <>
struct T_accDecSym<int16, int16> : T_acc384<int16, int16> {
    using T_acc384<int16, int16>::operator=;
};
template <>
struct T_accDecSym<cint16, int16> : T_acc384<cint16, int16> {
    using T_acc384<cint16, int16>::operator=;
};
;
template <>
struct T_accDecSym<cint16, cint16> : T_acc384<cint16, cint16> {
    using T_acc384<cint16, cint16>::operator=;
};
template <>
struct T_accDecSym<int32, int16> : T_acc384<int32, int16> {
    using T_acc384<int32, int16>::operator=;
};
template <>
struct T_accDecSym<int32, int32> : T_acc384<int32, int32> {
    using T_acc384<int32, int32>::operator=;
};
template <>
struct T_accDecSym<cint32, int16> : T_acc384<cint32, int16> {
    using T_acc384<cint32, int16>::operator=;
};
template <>
struct T_accDecSym<cint32, int32> : T_acc<cint32, int32> {
    using T_acc<cint32, int32>::operator=;
}; // 4-lane acc
template <>
struct T_accDecSym<cint32, cint16> : T_acc384<cint32, cint16> {
    using T_acc384<cint32, cint16>::operator=;
};
template <>
struct T_accDecSym<cint32, cint32> : T_acc384<cint32, cint32> {
    using T_acc384<cint32, cint32>::operator=;
};
template <>
struct T_accDecSym<float, float> : T_acc384<float, float> {
    using T_acc384<float, float>::operator=;
};
template <>
struct T_accDecSym<cfloat, float> : T_acc384<cfloat, float> {
    using T_acc384<cfloat, float>::operator=;
};
template <>
struct T_accDecSym<cfloat, cfloat> : T_acc384<cfloat, cfloat> {
    using T_acc384<cfloat, cfloat>::operator=;
};

template <typename TT_DATA, typename TT_COEFF>
struct T_outValDecSym {};
template <>
struct T_outValDecSym<int16, int16> : T_outVal384<int16, int16> {
    using T_outVal384<int16, int16>::operator=;
};
template <>
struct T_outValDecSym<cint16, int16> : T_outVal384<cint16, int16> {
    using T_outVal384<cint16, int16>::operator=;
};
template <>
struct T_outValDecSym<cint16, cint16> : T_outVal384<cint16, cint16> {
    using T_outVal384<cint16, cint16>::operator=;
};
template <>
struct T_outValDecSym<int32, int16> : T_outVal384<int32, int16> {
    using T_outVal384<int32, int16>::operator=;
};
template <>
struct T_outValDecSym<int32, int32> : T_outVal384<int32, int32> {
    using T_outVal384<int32, int32>::operator=;
};
template <>
struct T_outValDecSym<cint32, int16> : T_outVal384<cint32, int16> {
    using T_outVal384<cint32, int16>::operator=;
};
template <>
struct T_outValDecSym<cint32, int32> : T_outVal<cint32, int32> {
    using T_outVal<cint32, int32>::operator=;
};
template <>
struct T_outValDecSym<cint32, cint16> : T_outVal384<cint32, cint16> {
    using T_outVal384<cint32, cint16>::operator=;
};
template <>
struct T_outValDecSym<cint32, cint32> : T_outVal384<cint32, cint32> {
    using T_outVal384<cint32, cint32>::operator=;
};
template <>
struct T_outValDecSym<float, float> : T_outVal384<float, float> {
    using T_outVal384<float, float>::operator=;
};
template <>
struct T_outValDecSym<cfloat, float> : T_outVal384<cfloat, float> {
    using T_outVal384<cfloat, float>::operator=;
};
template <>
struct T_outValDecSym<cfloat, cfloat> : T_outVal384<cfloat, cfloat> {
    using T_outVal384<cfloat, cfloat>::operator=;
};

#ifndef L_BUFFER
#define L_BUFFER xa
#endif
#ifndef R_BUFFER
#define R_BUFFER xb
#endif
#ifndef Y_BUFFER
#define Y_BUFFER ya
#endif
#ifndef Z_BUFFER
#define Z_BUFFER wc0
#endif

#define _DSPLIB_FIR_AIE_API_ 1

//---------------------------------------------------------------------------------------------------
// Functions

template <typename TT_DATA, typename TT_COEFF, unsigned int T_SIZE>
inline void fnLoadXIpData(T_buff_1024b<TT_DATA>& buff, const unsigned int splice, input_window<TT_DATA>* inWindow) {
    if
        constexpr(T_SIZE == 256) {
            T_buff_256b<TT_DATA> readData;
            const short kSpliceRange = 4;
            readData = window_readincr_256b<TT_DATA>(inWindow);
            buff.val = upd_w(buff.val, splice % kSpliceRange, readData.val);
        }
    else {
        T_buff_128b<TT_DATA> readData;
        const short kSpliceRange = 8;
        readData = window_readincr_128b<TT_DATA>(inWindow);
        buff.val = upd_v(buff.val, splice % kSpliceRange, readData.val);
    }
};

template <typename TT_DATA, typename TT_COEFF, unsigned int T_SIZE>
inline void fnLoadXIpData(T_buff_512b<TT_DATA>& buff, unsigned int splice, input_window<TT_DATA>* inWindow) {
    using buf_type = typename T_buff_512b<TT_DATA>::v_type;
    buf_type chess_storage(L_BUFFER) buffTmp;

    if
        constexpr(T_SIZE == 256) {
            T_buff_256b<TT_DATA> readData;
            const short kSpliceRange = 2;
            readData = window_readincr_256b<TT_DATA>(inWindow);
            buffTmp = upd_w(buff.val, splice % kSpliceRange, readData.val);
            buff.val = buffTmp;
        }
    else {
        T_buff_128b<TT_DATA> readData;
        const short kSpliceRange = 4;
        readData = window_readincr_128b<TT_DATA>(inWindow);
        buffTmp = upd_v(buff.val, splice % kSpliceRange, readData.val);
        buff.val = buffTmp;
    }
};

template <typename TT_DATA, typename TT_COEFF, unsigned int T_SIZE>
inline void fnLoadYIpData(T_buff_512b<TT_DATA>& buff, unsigned int splice, input_window<TT_DATA>* inWindow) {
    using buf_type = typename T_buff_512b<TT_DATA>::v_type;
    buf_type chess_storage(R_BUFFER) buffTmp;

    if
        constexpr(T_SIZE == 256) {
            T_buff_256b<TT_DATA> readData;
            const short kSpliceRange = 2;
            readData = window_readdecr_256b<TT_DATA>(inWindow);
            buffTmp = upd_w(buff.val, splice % kSpliceRange, readData.val);
            buff.val = buffTmp;
        }
    else {
        T_buff_128b<TT_DATA> readData;
        const short kSpliceRange = 4;
        readData = window_readdecr_128b<TT_DATA>(inWindow);
        buffTmp = upd_v(buff.val, splice % kSpliceRange, readData.val);
        buff.val = buffTmp;
    }
};

#if _DSPLIB_FIR_AIE_API_ == 0

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_1024b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     unsigned int ystart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart,
                                                     const unsigned int decimateOffsets) {
    T_accDecSym<TT_DATA, TT_COEFF> retVal;
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       unsigned int ystart,
                                                       unsigned int ct,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets) {
    T_accDecSym<TT_DATA, TT_COEFF> retVal;
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym2Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_512b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     T_buff_512b<TT_DATA> ybuff,
                                                     unsigned int ystart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart,
                                                     const unsigned int decimateOffsets) {
    T_accDecSym<TT_DATA, TT_COEFF> retVal;
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym2Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_512b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_512b<TT_DATA> ybuff,
                                                       unsigned int ystart,
                                                       unsigned int ct,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets) {
    T_accDecSym<TT_DATA, TT_COEFF> retVal;
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint32, int32> macDecSym1Buff(T_accDecSym<cint32, int32> acc,
                                                 T_buff_1024b<cint32> xbuff,
                                                 unsigned int xstart,
                                                 unsigned int ystart,
                                                 T_buff_256b<int32> zbuff,
                                                 unsigned int zstart,
                                                 const unsigned int decimateOffsets) {
    T_accDecSym<cint32, int32> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

#if NATIVE_LMAC2 == 1
    retVal.val = lmac2_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
#else
    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
#endif
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint32, int32> macDecSym1Buffct(T_accDecSym<cint32, int32> acc,
                                                   T_buff_1024b<cint32> xbuff,
                                                   unsigned int xstart,
                                                   unsigned int ystart,
                                                   unsigned int ct,
                                                   T_buff_256b<int32> zbuff,
                                                   unsigned int zstart,
                                                   const unsigned int decimateOffsets) {
    T_accDecSym<cint32, int32> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

#if NATIVE_LMAC2 == 1
    retVal.val =
        lmac2_sym_ct(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
#else
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
#endif
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint32, int32> macDecSym2Buff(T_accDecSym<cint32, int32> acc,
                                                 T_buff_512b<cint32> xbuff,
                                                 unsigned int xstart,
                                                 T_buff_512b<cint32> ybuff,
                                                 unsigned int ystart,
                                                 T_buff_256b<int32> zbuff,
                                                 unsigned int zstart,
                                                 const unsigned int decimateOffsets) {
    T_accDecSym<cint32, int32> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

#if NATIVE_LMAC2 == 1
    retVal.val =
        lmac2_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
#else
    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
#endif
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint32, int32> macDecSym2Buffct(T_accDecSym<cint32, int32> acc,
                                                   T_buff_512b<cint32> xbuff,
                                                   unsigned int xstart,
                                                   T_buff_512b<cint32> ybuff,
                                                   unsigned int ystart,
                                                   unsigned int ct,
                                                   T_buff_256b<int32> zbuff,
                                                   unsigned int zstart,
                                                   const unsigned int decimateOffsets) {
    T_accDecSym<cint32, int32> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

#if NATIVE_LMAC2 == 1
    retVal.val =
        lmac2_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
#else
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
#endif
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint16, int16> macDecSym1Buff(T_accDecSym<cint16, int16> acc,
                                                 T_buff_1024b<cint16> xbuff,
                                                 unsigned int xstart,
                                                 unsigned int ystart,
                                                 T_buff_256b<int16> zbuff,
                                                 unsigned int zstart,
                                                 const unsigned int decimateOffsets) {
    T_accDecSym<cint16, int16> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

    retVal.val = mac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint16, int16> macDecSym1Buffct(T_accDecSym<cint16, int16> acc,
                                                   T_buff_1024b<cint16> xbuff,
                                                   unsigned int xstart,
                                                   unsigned int ystart,
                                                   unsigned int ct,
                                                   T_buff_256b<int16> zbuff,
                                                   unsigned int zstart,
                                                   const unsigned int decimateOffsets) {
    T_accDecSym<cint16, int16> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

    retVal.val =
        mac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint16, int16> macDecSym2Buff(T_accDecSym<cint16, int16> acc,
                                                 T_buff_512b<cint16> xbuff,
                                                 unsigned int xstart,
                                                 T_buff_512b<cint16> ybuff,
                                                 unsigned int ystart,
                                                 T_buff_256b<int16> zbuff,
                                                 unsigned int zstart,
                                                 const unsigned int decimateOffsets) {
    T_accDecSym<cint16, int16> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

    retVal.val =
        mac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<cint16, int16> macDecSym2Buffct(T_accDecSym<cint16, int16> acc,
                                                   T_buff_512b<cint16> xbuff,
                                                   unsigned int xstart,
                                                   T_buff_512b<cint16> ybuff,
                                                   unsigned int ystart,
                                                   unsigned int ct,
                                                   T_buff_256b<int16> zbuff,
                                                   unsigned int zstart,
                                                   const unsigned int decimateOffsets) {
    T_accDecSym<cint16, int16> retVal;
    const unsigned int xoffsets = decimateOffsets;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;

    retVal.val = mac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, ct, zbuff.val, zstart,
                             zoffsets, zstep);
    return retVal;
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buff(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                         T_buff_1024b<TT_DATA> xbuff,
                                                         unsigned int xstart,
                                                         unsigned int ystart,
                                                         T_buff_256b<TT_COEFF> zbuff,
                                                         unsigned int zstart,
                                                         const unsigned int decimateOffsets) {
    return macDecSym1Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, zbuff, zstart,
                                                                 decimateOffsets);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buff(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                         T_buff_1024b<TT_DATA> xbuff,
                                                         unsigned int xstart,
                                                         unsigned int ystart,
                                                         T_buff_256b<TT_COEFF> zbuff,
                                                         unsigned int zstart,
                                                         const unsigned int decimateOffsets) {
    return macDecSym1Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, zbuff, zstart,
                                                                 decimateOffsets);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buffct(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           unsigned int ystart,
                                                           unsigned int ct,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets) {
    return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, ct, zbuff, zstart,
                                                                   decimateOffsets);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buffct(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           unsigned int ystart,
                                                           unsigned int ct,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets) {
    return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, ct, zbuff, zstart,
                                                                   decimateOffsets);
};

#endif // _DSPLIB_FIR_AIE_API_

#if _DSPLIB_FIR_AIE_API_ == 1

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_1024b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     unsigned int ystart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart,
                                                     const unsigned int decimateOffsets) {
    // API Sliding_mul_sym unrolls all the required MAC calls for the 1 buff arch.
    // There's nothing left to do when this function is called.
    // Not used, retained for ompatibilty with non-API low level intrinsic mode.

    // Do nothing here
    return acc;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       unsigned int ystart,
                                                       unsigned int ct,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets) {
    // API Sliding_mul_sym unrolls all the required MAC calls for the 1 buff arch.
    // There's nothing left to do when this function is called.
    // Not used, retained for ompatibilty with non-API low level intrinsic mode.

    // Do nothing here
    return acc;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_1024b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = TP_FIR_LEN;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
    T_accDecSym<TT_DATA, TT_COEFF> retVal;

    // #define FIR_DEC_SYM_DEBUG_ 1

    retVal.val = ::aie::sliding_mul_sym_ops<
        Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
        accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mul_sym(zbuff.val, zstart, xbuff.val,
                                                                                        xstart);
    // (acc.val, zbuff.val, zstart, xbuff.val, xstart);

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = TP_FIR_LEN;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
    T_accDecSym<TT_DATA, TT_COEFF> retVal;

    // #define FIR_DEC_SYM_DEBUG_ 1

    if
        constexpr(Points == 1) {
            retVal.val = ::aie::sliding_mul_ops<
                Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac(acc.val, zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<
            Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
            accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
    }

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym2Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_512b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     T_buff_512b<TT_DATA> ybuff,
                                                     unsigned int ystart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart,
                                                     const unsigned int decimateOffsets) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
    T_accDecSym<TT_DATA, TT_COEFF> retVal;

    retVal.val = ::aie::sliding_mul_sym_ops<
        Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
        accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                        xbuff.val, xstart, ybuff.val,
                                                                                        ystart);

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecSym<TT_DATA, TT_COEFF> macDecSym2Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_512b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_512b<TT_DATA> ybuff,
                                                       unsigned int ystart,
                                                       unsigned int ct,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsDecSym<TT_DATA, TT_COEFF>() - 1;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;

    T_accDecSym<TT_DATA, TT_COEFF> retVal;
    if
        constexpr(Points == 1) {
            retVal.val = ::aie::sliding_mul_ops<
                Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac(acc.val, zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<
            Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
            accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                            xbuff.val, xstart,
                                                                                            ybuff.val, ystart);
    }
    return retVal;
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buff(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                         T_buff_1024b<TT_DATA> xbuff,
                                                         unsigned int xstart,
                                                         unsigned int ystart,
                                                         T_buff_256b<TT_COEFF> zbuff,
                                                         unsigned int zstart,
                                                         const unsigned int decimateOffsets) {
    return macDecSym1Buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buff(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                         T_buff_1024b<TT_DATA> xbuff,
                                                         unsigned int xstart,
                                                         unsigned int ystart,
                                                         T_buff_256b<TT_COEFF> zbuff,
                                                         unsigned int zstart,
                                                         const unsigned int decimateOffsets) {
    return macDecSym1Buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buffct(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           unsigned int ystart,
                                                           unsigned int ct,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets) {
    return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buffct(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           unsigned int ystart,
                                                           unsigned int ct,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets) {
    return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
};
#endif // _DSPLIB_FIR_AIE_API_

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buff(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                         T_buff_512b<TT_DATA> xbuff,
                                                         unsigned int xstart,
                                                         T_buff_512b<TT_DATA> ybuff,
                                                         unsigned int ystart,
                                                         T_buff_256b<TT_COEFF> zbuff,
                                                         unsigned int zstart,
                                                         const unsigned int decimateOffsets) {
    return macDecSym2Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart,
                                                                 decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buff(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                         T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                         T_buff_512b<TT_DATA> xbuff,
                                                         unsigned int xstart,
                                                         T_buff_512b<TT_DATA> ybuff,
                                                         unsigned int ystart,
                                                         T_buff_256b<TT_COEFF> zbuff,
                                                         unsigned int zstart,
                                                         const unsigned int decimateOffsets) {
    return macDecSym2Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart,
                                                                 decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buffct(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_512b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           T_buff_512b<TT_DATA> ybuff,
                                                           unsigned int ystart,
                                                           unsigned int ct,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets) {
    return macDecSym2Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, ct, zbuff, zstart,
                                                                   decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
inline T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buffct(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_512b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           T_buff_512b<TT_DATA> ybuff,
                                                           unsigned int ystart,
                                                           unsigned int ct,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets) {
    return macDecSym2Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, ct, zbuff, zstart,
                                                                   decimateOffsets);
};

// Shift and Saturate Decimation Sym
template <typename TT_DATA, typename TT_COEFF>
inline T_outVal384<TT_DATA, TT_COEFF> shiftAndSaturateDecSym(T_acc384<TT_DATA, TT_COEFF> acc, const int shift) {
    return shiftAndSaturate(acc, shift);
};

// Shift and Saturate Decimation Sym
template <typename TT_DATA, typename TT_COEFF>
inline T_outVal<TT_DATA, TT_COEFF> shiftAndSaturateDecSym(T_acc<TT_DATA, TT_COEFF> acc, const int shift) {
    return shiftAndSaturate(acc, shift);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_SYM_UTILS_HPP_

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
