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
 *  @brief xf_blas program (including instruction and data) memory manager
 *
 *  $DateTime: 2019/06/13$
 */

#ifndef BLAS_PROGRAM_H
#define BLAS_PROGRAM_H

#include <array>
#include <cassert>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>
#include "blas_instr.h"
#include "L3/include/sw/utility/utility.h"

using namespace std;
namespace xf {
namespace linear_algebra {
namespace blas {

  template <
    typename t_DataType,
    unsigned int t_PageSize
  >
  class Page {
    public:
      Page() {
        for (int i=0; i<t_PageSize; ++i) {
          m_page[i] = 0;
        }
      }
      t_DataType& operator[](unsigned int p_idx) { return m_page[p_idx];}
    private:
      array<t_DataType, t_PageSize> m_page;
  };

  /**
   * @brief program memory, including data and instruction memory 
   */ 
  template <
    typename t_HandleType,
    unsigned int t_MemWidthBytes,
    unsigned int t_InstrSizeBytes,
    unsigned int t_PageSizeBytes,
    unsigned int t_MaxNumInstrs,
    unsigned int t_InstrPageIdx,
    unsigned int t_ParamPageIdx,
    unsigned int t_StatsPageIdx,
    unsigned int t_DataPageIdx 
  >
  class Program {
    public:
      typedef Page<uint8_t, t_PageSizeBytes> PageType;
      typedef vector<PageType> PageVectorType; 
    public:
      static const unsigned int ParamStartOff = t_ParamPageIdx * t_PageSizeBytes;
      
    public:
      Program() : 
        m_numInstrs(0), 
        m_currParamOff(t_ParamPageIdx*t_PageSizeBytes) {
          assert((t_PageSizeBytes % t_MemWidthBytes) == 0);
          m_pages.resize(t_DataPageIdx);
          PageType l_page0 = m_pages[0];
          //initialize instruction page with 0s
          for (unsigned int i=0; i<t_PageSizeBytes; ++i) {
            l_page0[i] = 0;
          }
        }

      void clear() {
        m_numInstrs = 0;
        m_currParamOff = 0;
        m_pages.clear();
        m_datMem.clear();
        m_datMemSize.clear();
      }

      void allocPages(size_t p_numPages) {
        m_pages.resize(p_numPages);
      }

      unsigned int& getNumInstrs() {
        return m_numInstrs;
      }

      uint32_t& getCurrParamOff() {
        return m_currParamOff;
      }

      uint8_t* getPageAddr(size_t p_pageIdx) {
        assert(p_pageIdx < m_pages.size());
        uint8_t* l_addr = (uint8_t*) &(m_pages[p_pageIdx][0]);
        return(l_addr);
      }

      uint8_t* getBaseInstrAddr() {
        return (uint8_t*) &(m_pages[t_InstrPageIdx][0]);
      }
      
      uint8_t* getBaseParamAddr() {
        return (uint8_t*) &(m_pages[t_ParamPageIdx][0]);
      }

      uint8_t* getBaseStatsAddr() {
        return (uint8_t*) &(m_pages[t_StatsPageIdx][0]);
      }

      uint8_t* getBaseDataAddr() {
        return (uint8_t*) &(m_pages[t_DataPageIdx][0]);
      }
    
      /**
       * @brief getCurrInstrAddr returns the current instrction and parameter sections' addresses
       * @param p_paramsBytes number of bytes in parameters
       * @param p_instr current instruction section address
       * @param p_params current parameter section address
       */ 
      void getCurrInstrAddr(
        unsigned int p_paramBytes,
        uint8_t* &p_instr,
        uint8_t* &p_params
      ) {
        assert(m_numInstrs < t_MaxNumInstrs);
        assert((m_currParamOff + p_paramBytes) < (t_StatsPageIdx*t_PageSizeBytes));
        uint8_t* l_baseInstrAddr = getBaseInstrAddr();
        p_instr = (uint8_t*) &(l_baseInstrAddr[m_numInstrs*t_InstrSizeBytes]);
        uint8_t* l_baseParamAddr = getBaseParamAddr();
        p_params = (uint8_t*) &(l_baseInstrAddr[m_currParamOff]);
        m_numInstrs++;
        m_currParamOff += p_paramBytes;
        while (m_currParamOff % t_MemWidthBytes != 0) {
          m_currParamOff++;
        }
      }
      
      void* getInstrAddr(unsigned int p_instrIdx) {
      }

      xfblasStatus_t regDatMem(
        const t_HandleType &p_memHandle, 
        void* p_memPtr, 
        unsigned long long p_bufBytes
      ) {
        xfblasStatus_t l_resStats;
        if (m_datMem.find(p_memHandle) == m_datMem.end()) {
          m_datMem[p_memHandle] = p_memPtr;
          m_datMemSize[p_memHandle] = p_bufBytes;
          l_resStats = XFBLAS_STATUS_SUCCESS;
        }
        else if (m_datMemSize[p_memHandle] != p_bufBytes){
          m_datMem[p_memHandle] = p_memPtr;
          m_datMemSize[p_memHandle] = p_bufBytes;
          l_resStats = XFBLAS_STATUS_SUCCESS;
        }
        else {
          l_resStats = XFBLAS_STATUS_MEM_ALLOCATED;
        }
        return (l_resStats);
      }

      void* getDatMem(
        const t_HandleType &p_memHandle, 
        unsigned long long &p_bufSize
      ) {
        void* l_resPtr = nullptr;
        if (m_datMem.find(p_memHandle) != m_datMem.end()) {
          l_resPtr = m_datMem[p_memHandle];
          p_bufSize = m_datMemSize[p_memHandle];
        }
        return l_resPtr;
      } 
      xfblasStatus_t createProgramImage(
      ) {
      } 
    private:
      unsigned int m_numInstrs;
      uint32_t m_currParamOff;
      PageVectorType m_pages;
      unordered_map<t_HandleType, void*> m_datMem;
      unordered_map<t_HandleType, unsigned long long> m_datMemSize;
  };

} //end namespace blas
} //end namespace linear_algebra
} //end namespace xf
#endif
