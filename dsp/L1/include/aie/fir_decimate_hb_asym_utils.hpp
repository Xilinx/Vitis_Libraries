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
#ifndef _DSPLIB_FIR_DEC_HB_ASYM_UTILS_HPP_
#define _DSPLIB_FIR_DEC_HB_ASYM_UTILS_HPP_

#include "fir_decimate_hb_asym.hpp"
#include "debug_utils.h"
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb {

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_acc384<TT_DATA, TT_COEFF> macDecHbAsym(T_acc384<TT_DATA, TT_COEFF> acc,
                                                     ::aie::vector<TT_DATA, 1024 / 8 / sizeof(TT_DATA)> dataBuff,
                                                     unsigned int xstart,
                                                     ::aie::vector<TT_COEFF, 256 / 8 / sizeof(TT_COEFF)> coeffBuff,
                                                     unsigned int zstart) {
    T_acc384<TT_DATA, TT_COEFF> retVal;
    retVal.val = ::aie::sliding_mac<fnNumLanes384<TT_DATA, TT_COEFF>(), fnNumCols384<TT_DATA, TT_COEFF>()>(
        acc.val, coeffBuff, zstart, dataBuff, xstart);
    return retVal;
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DEC_HB_ASYM_UTILS_HPP_
