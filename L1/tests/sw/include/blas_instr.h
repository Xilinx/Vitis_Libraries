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

#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include <vector>
#include "L3/include/sw/utility/utility.h"

using namespace std;

#define B1_MaxOpCode 14
#define NULL_OP 0
#define B1_OP_CLASS 0
#define OUTPUT_WIDTH 7 
#define ENTRIES_PER_LINE 16

namespace xf {
namespace linear_algebra {
namespace blas {
  class FindOpCode {
    public:
      FindOpCode() {
        m_opMap = {
          {"null_op", 0},
          {"amax", 1},
          {"amin", 2},
          {"asum", 3},
          {"axpy", 4},
          {"copy", 5},
          {"dot", 6},
          {"nrm2", 7},
          {"scal", 8},
          {"swap", 9}
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

  //all offsets are defined as byte offsets
  template<typename t_DataType, typename t_ResDataType> 
  class ParamB1 {
    public:
      ParamB1(){}
    public:
      void getData(
        uint64_t p_addr, 
        uint32_t p_n, 
        vector<t_DataType> &p_data
      ){
        p_data.clear();
        if (p_addr == 0) {
          return;
        }
        size_t l_dataBytes = p_n * sizeof(t_DataType);
        p_data.resize(m_n);
        memcpy((char*)&(p_data[0]), reinterpret_cast<char*>(p_addr), l_dataBytes);
        return;
      }
      void printData(ostream &os, const vector<t_DataType> &p_data, uint32_t p_n) {
        for (unsigned int i=0; i<p_n; ++i){
          if ((i % ENTRIES_PER_LINE) == 0) {
            os << "\n";
          }
          os << setw(OUTPUT_WIDTH) << p_data[i];
        } 
        os << "\n";
      }
      void print(ostream &os) {
        os << "n=" << m_n
           << " alpha="
           << setw(OUTPUT_WIDTH) << m_alpha
           << " resGolden="
           << setw(OUTPUT_WIDTH) << m_resScalar << "\n";

        vector<t_DataType> l_data;
        getData(m_xAddr, m_n, l_data);
        if (l_data.size() != 0) {
          os << "x:" << "\n";
          printData(os, l_data, m_n);
        }   
        getData(m_yAddr, m_n, l_data);
        if (l_data.size() != 0) {
          os << "y:" << "\n";
          printData(os, l_data, m_n);
        }   
        getData(m_xResAddr, m_n, l_data);
        if (l_data.size() != 0) {
          os << "xRes:" << "\n";
          printData(os, l_data, m_n);
        }   
        getData(m_yResAddr, m_n, l_data);
        if (l_data.size() != 0) {
          os << "yRes:" << "\n";
          printData(os, l_data, m_n);
        }   
      }
    public:
      uint32_t m_n;
      t_DataType m_alpha;
      uint64_t m_xAddr;
      uint64_t m_yAddr;
      uint64_t m_xResAddr;
      uint64_t m_yResAddr;
      t_ResDataType m_resScalar;
  };

  template<typename T1, typename T2>
  ostream& operator<<(ostream &os, ParamB1<T1, T2> &p_val) {
    p_val.print(os);
    return (os);
  }

  class Instr {
    public:
      Instr() {}
    public:
      void print(ostream &os) {
        FindOpCode l_opFinder;
        string l_opName;
        xfblasStatus_t l_status = l_opFinder.getOpName(m_opCode, l_opName);
        assert(l_status == XFBLAS_STATUS_SUCCESS);
        os << "Operation: " << l_opName << "\n";
      }
    public:
      uint16_t m_opClass;
      uint16_t m_opCode;
      int32_t m_paramOff;
  };

  ostream& operator<<(ostream &os, Instr &p_instr) {
    p_instr.print(os);
    return(os);
  }

} //end namespace blas
} //end namespace linear_algebra
} //end namespace xf

#endif
