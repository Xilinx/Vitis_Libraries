/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced ROWicro Devices, Inc.
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
#ifndef __FLEXIBLE_QRD_HOUSEHOLDER_HPP__
#define __FLEXIBLE_QRD_HOUSEHOLDER_HPP__

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include <adf.h>

const float tol = 5.0e-4;
const float eta = 1.0e-3;
static constexpr unsigned VecSize = 4;

namespace xf {
namespace solver {

template <int ROW, int COL, int CoreNum, int BlkNum>
class QRDHouseholderComplexFlexible {
   public:
    int columnId;
    QRDHouseholderComplexFlexible(int current_column) { columnId = current_column; }
    static void registerKernelClass() { REGISTER_FUNCTION(QRDHouseholderComplexFlexible::run); }
    void run(adf::input_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict in,
             adf::output_buffer<cfloat, adf::extents<adf::inherited_extent> >& __restrict out);

   private:
    static constexpr unsigned vecNum = ROW / VecSize;
    alignas(32) unsigned curColId = 0;
    alignas(32) float norm2[BlkNum];
    alignas(32) cfloat sgn[BlkNum];
    alignas(32) float alphInv[BlkNum];
    alignas(32) aie::vector<cfloat, VecSize> A[BlkNum + 1][vecNum];
    alignas(32) aie::vector<cfloat, VecSize> U[BlkNum][vecNum];
    void calV(const unsigned k, const unsigned baseId);
    void updA(const unsigned j, const unsigned k, const unsigned baseId);
};

} // namespace solver
} // namespace xf
#endif
