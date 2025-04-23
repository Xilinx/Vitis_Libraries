/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef __FLEXIBLE_CHOLESKY_COMPLEX_HPP__
#define __FLEXIBLE_CHOLESKY_COMPLEX_HPP__

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include <adf.h>

static constexpr unsigned VecSize = 4;

namespace xf {
namespace solver {

template <unsigned Dim, unsigned CoreNum, unsigned BlkNum>
class CholeskyComplexFlexible {
   public:
    unsigned columnId;
    CholeskyComplexFlexible(unsigned current_column) { columnId = current_column; }
    CholeskyComplexFlexible(void){};
    void run(adf::input_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict in,
             adf::output_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict out);
    static void registerKernelClass(void) { REGISTER_FUNCTION(CholeskyComplexFlexible::run); };

   private:
    static constexpr unsigned vecNum = Dim / VecSize;
    void calV(const unsigned k, const unsigned baseId);
    void updA(const unsigned j, const unsigned k, const unsigned baseId);
    alignas(32) aie::vector<cfloat, VecSize> ColA[BlkNum + 1][vecNum];
    alignas(32) unsigned curColId = 0;
};

} // namespace solver
} // namespace xf
#endif
