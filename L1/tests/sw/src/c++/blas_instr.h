/**********
 * Copyright 2019 Xilinx, Inc.
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
 * **********/
/**
 *  @brief instruction format for BLAS L1 functions 
 *
 *  $DateTime: 2019/06/13 $
 */

#ifndef BLAS_INSTR_H
#define BLAS_INSTR_H

#include <string>
#include <unordered_map>
#include "L3/include/sw/utility/utility.h"
#include "blas_instr.h"

using namespace std;

#define B1_MaxOpCode 14
#define NULL_OP 0
#define B1_OP_CLASS 0

namespace xf {
namespace linear_algebra {
namespace blas {

  //all offsets are defined as byte offsets
  template<typename t_DataType> 
  struct ParamB1 {
    uint32_t m_n;
    t_DataType m_alpha;
    uint64_t m_xOff;
    uint64_t m_yOff;
    uint64_t m_xResOff;
    uint64_t m_yResOff;
    t_DataType m_resScalar;
  };

  struct Instr {
    uint16_t m_opClass;
    uint16_t m_opCode;
    int32_t m_paramOff;
  };

  class FindOpCodeB1 {
    public:
      FindOpCodeB1() {
        m_opMap = {
          {"null_op", 0},
          {"amax", 1},
          {"amin", 2},
          {"asum", 3},
          {"axpy", 4},
          {"copy", 5},
          {"dot", 6},
          {"scal", 7},
          {"swap", 8}
        };
      }
      xfblasStatus_t getOpCode(const string &p_opName, uint32_t &p_opCode) {
        if (m_opMap.find(p_opName) == m_opMap.end()) {
          return XFBLAS_STATUS_INVALID_VALUE;
        } 
        else {
          p_opCode = m_opMap[p_opName];
          return XFBLAS_STATUS_SUCCESS;
        }
      }
      xfblasStatus_t getOpName(uint32_t p_opCode, string &p_opName) {
        xfblasStatus_t l_status = XFBLAS_STATUS_INVALID_VALUE;
        p_opName = "no_op";
        for (auto it = m_opMap.begin(); it != m_opMap.end(); ++it) {
          if (it->second == p_opCode) {
            p_opName = it->first;
            l_status = XFBLAS_STATUS_SUCCESS;
            break;
          }
        }
        return (l_status);
      }
    private:
      unordered_map<string, uint32_t> m_opMap;
  };

} //end namespace blas
} //end namespace linear_algebra
} //end namespace xf

#endif
