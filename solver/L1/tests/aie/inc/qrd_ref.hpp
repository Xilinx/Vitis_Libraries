/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_QRD_REF_HPP_
#define _SOLVERLIB_QRD_REF_HPP_

/*
QRD Product reference model
*/

#include <adf.h>
#include <limits>
#include <array>

#include "qrd_ref_traits.hpp"
using namespace adf;

//#define _SOLVERLIB_QRD_REF_DEBUG_

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

//-----------------------------------------------------------------------------------------------------
// QRD - default/base 'specialization'
template <typename TT_DATA,
          size_t TP_DIM_ROWS,
          size_t TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class qrd_ref {
   private:

   public:
    // using out_t = outTypeMult_t<TT_DATA, TT_DATA>;

    // Constructor
    qrd_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(qrd_ref::qrd_main); }

    void qrd_main(input_buffer<TT_DATA>& inWindowA,
                  output_buffer<TT_DATA>& outWindowQ,
                  output_buffer<TT_DATA>& outWindowR);
};
}
}
}
}

#endif // _SOLVERLIB_QRD_REF_HPP_
