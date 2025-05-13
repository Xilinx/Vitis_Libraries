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
#ifndef _DSPLIB_FIR_SR_ASYM_UTILS_HPP_
#define _DSPLIB_FIR_SR_ASYM_UTILS_HPP_

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "fir_sr_asym.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnAccSizeSrAsym() {
    return fnAccSize<TT_DATA, TT_COEFF>();
};

// LONG ACCs

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> mulSrAsym(T_acc<TT_DATA, TT_COEFF> acc,
                                               T_buff_1024b<TT_DATA> xbuff,
                                               unsigned int xstart,
                                               T_buff_256b<TT_COEFF> zbuff,
                                               unsigned int zstart) {
    T_acc<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mul<fnNumLanes<TT_DATA, TT_COEFF>(), fnNumCols<TT_DATA, TT_COEFF>(), 1, 1, 1,
                                    tAccBaseType_t<TT_DATA, TT_COEFF> >(zbuff.val, (zstart), xbuff.val, (xstart));
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> macSrAsym(T_acc<TT_DATA, TT_COEFF> acc,
                                               T_buff_1024b<TT_DATA> xbuff,
                                               unsigned int xstart,
                                               T_buff_256b<TT_COEFF> zbuff,
                                               unsigned int zstart) {
    T_acc<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mac<fnNumLanes<TT_DATA, TT_COEFF>(), fnNumCols<TT_DATA, TT_COEFF>()>(
        acc.val, zbuff.val, (zstart), xbuff.val, (xstart));
    return retVal;
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> initMacSrAsym(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                   T_acc<TT_DATA, TT_COEFF> acc,
                                                   T_buff_1024b<TT_DATA> xbuff,
                                                   unsigned int xstart,
                                                   T_buff_256b<TT_COEFF> zbuff,
                                                   unsigned int zstart) {
    return mulSrAsym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
};
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> initMacSrAsym(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                   T_acc<TT_DATA, TT_COEFF> acc,
                                                   T_buff_1024b<TT_DATA> xbuff,
                                                   unsigned int xstart,
                                                   T_buff_256b<TT_COEFF> zbuff,
                                                   unsigned int zstart) {
    return macSrAsym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
};

// SHORT ACCs

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> mulSrAsym(T_acc384<TT_DATA, TT_COEFF> acc,
                                                  T_buff_1024b<TT_DATA> xbuff,
                                                  unsigned int xstart,
                                                  T_buff_256b<TT_COEFF> zbuff,
                                                  unsigned int zstart) {
    T_acc384<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mul<fnNumLanes384<TT_DATA, TT_COEFF>(), fnNumCols384<TT_DATA, TT_COEFF>(), 1, 1, 1,
                                    tAccBaseType_t<TT_DATA, TT_COEFF> >(zbuff.val, zstart, xbuff.val, xstart);
    return retVal;
}

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> macSrAsym(T_acc384<TT_DATA, TT_COEFF> acc,
                                                  T_buff_1024b<TT_DATA> xbuff,
                                                  unsigned int xstart,
                                                  T_buff_256b<TT_COEFF> zbuff,
                                                  unsigned int zstart) {
    T_acc384<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mac<fnNumLanes384<TT_DATA, TT_COEFF>(), fnNumCols384<TT_DATA, TT_COEFF>()>(
        acc.val, zbuff.val, zstart, xbuff.val, xstart);
    return retVal;
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> initMacSrAsym(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                      T_acc384<TT_DATA, TT_COEFF> acc,
                                                      T_buff_1024b<TT_DATA> xbuff,
                                                      unsigned int xstart,
                                                      T_buff_256b<TT_COEFF> zbuff,
                                                      unsigned int zstart) {
    return mulSrAsym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
};
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> initMacSrAsym(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                      T_acc384<TT_DATA, TT_COEFF> acc,
                                                      T_buff_1024b<TT_DATA> xbuff,
                                                      unsigned int xstart,
                                                      T_buff_256b<TT_COEFF> zbuff,
                                                      unsigned int zstart) {
    return macSrAsym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_SR_ASYM_UTILS_HPP_
