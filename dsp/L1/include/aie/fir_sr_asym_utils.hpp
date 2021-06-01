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

#ifndef _DSPLIB_FIR_AIE_LLI_API_DEBUG_

template <typename TT_DATA, typename TT_COEFF>
inline constexpr unsigned int fnAccSizeSrAsym() {
    return fnAccSize<TT_DATA, TT_COEFF>();
};

// template for mulSrAsym - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF>
inline T_acc<TT_DATA, TT_COEFF> mulSrAsym(T_buff_1024b<TT_DATA> xbuff,
                                          unsigned int xstart,
                                          T_buff_256b<TT_COEFF> zbuff,
                                          unsigned int zstart) {
    T_acc<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mul<fnNumLanesSrAsym<TT_DATA, TT_COEFF>(), fnNumColumnsSrAsym<TT_DATA, TT_COEFF>(), 1,
                                    1, 1, accClassTag_t<fnAccClass<TT_DATA>(), fnAccSizeSrAsym<TT_DATA, TT_COEFF>()> >(
        zbuff.val, zstart, xbuff.val, xstart);
    return retVal;
}

// template for macSrAsym - uses ::aie::api HLI
template <typename TT_DATA, typename TT_COEFF>
inline T_acc<TT_DATA, TT_COEFF> macSrAsym(T_acc<TT_DATA, TT_COEFF> acc,
                                          T_buff_1024b<TT_DATA> xbuff,
                                          unsigned int xstart,
                                          T_buff_256b<TT_COEFF> zbuff,
                                          unsigned int zstart) {
    T_acc<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mac<fnNumLanesSrAsym<TT_DATA, TT_COEFF>(), fnNumColumnsSrAsym<TT_DATA, TT_COEFF>()>(
        acc.val, zbuff.val, zstart, xbuff.val, xstart);
    return retVal;
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF>
inline T_acc<TT_DATA, TT_COEFF> initMacSrAsym(T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface,
                                              T_acc<TT_DATA, TT_COEFF> acc,
                                              T_buff_1024b<TT_DATA> xbuff,
                                              unsigned int xstart,
                                              T_buff_256b<TT_COEFF> zbuff,
                                              unsigned int zstart) {
    return mulSrAsym<TT_DATA, TT_COEFF>(xbuff, xstart, zbuff, zstart);
};
template <typename TT_DATA, typename TT_COEFF>
inline T_acc<TT_DATA, TT_COEFF> initMacSrAsym(T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface,
                                              T_acc<TT_DATA, TT_COEFF> acc,
                                              T_buff_1024b<TT_DATA> xbuff,
                                              unsigned int xstart,
                                              T_buff_256b<TT_COEFF> zbuff,
                                              unsigned int zstart) {
    return macSrAsym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
};

#endif // _DSPLIB_FIR_AIE_LLI_API_DEBUG_
}
}
}
}
}

#endif // _DSPLIB_FIR_SR_ASYM_UTILS_HPP_

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
