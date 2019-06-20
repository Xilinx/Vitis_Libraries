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
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>
#include "blas_instr.h"

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
      typedef array<t_DataType, t_PageSize> PageType;
    public:
      Page() {
        for (int i=0; i<t_PageSize; ++i) {
          m_page[i] = 0;
        }
      }
      t_DataType& operator[](unsigned int p_idx) { return m_page[p_idx];}
    private:
      PageType m_page;
  };

  class PageHandleDescriptor {
    public:
      PageHandleDescriptor(): m_startPage(0), m_numPages(0);
      PageHandleDescriptor(unsigned int p_startPage, unsigned int p_numPages) :
        m_startPage(p_startPage),
        m_numPages(p_numPages)
      {}
      PageHandleDescriptor(const PageHandleDescriptor &p_pageDes) :
        m_startPage(p_pageDes.getStartPage()),
        m_numPages(p_pageDes.getNumPages())
      {}
      unsigned int& getStartPage() {return &m_startPage;}
      unsigned int& getNumPages() {return &m_numPages;}
      bool operator < (const PageHandleDescriptor &p_pageDes) {
        return {m_startPage < p_pageDes.getStartPage();}
      }
    private:
      unsigned int m_startPage;
      unsigned int m_numPages;
  };

  template <
    typename t_DataType,
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
      Program() : m_currParamOff(ParamStartOff) {}
      void init (
        unsigned int p_numInstrs, 
        size_t p_numPages) {
        m_numInstrs = p_numInstrs;
        m_pages.resize(p_numPages);
      }

      /**
       * @brief allocPage funtion that allocates number of pages for given number of bytes
       * @param p_handle matrix or vector name
       * @param p_newAlloc if the memory has already been allocated
       * @param p_numBytes number of bytes requested
       */ 
      unsigned int allocPages(
        string p_handle,
        size_t p_numBytes,
        bool &p_newAlloc
      ){
        assert(p_numBytes > 0);
        unsigned int l_numPages = (p_numBytes + t_PageSizeBytes - 1) / t_PageSizeBytes;
        unsigned int l_startPage = 0;
        if (m_handles.find(p_handle) == m_handles.end()) { //memory not allocated
          l_startPage = m_pages.size();
          m_pages.resize(l_startPage + l_numPages);
          m_handles[p_handle] = PageHandleDescriptor(l_startPage, l_numPages);
          p_newAlloc = true;
        }
        else {
          PageHandleDescriptor l_desc = m_pages[p_handle];
          asssert(l_numPages <= l_desc.getNumPages());
          l_startPage = l_des.getStartPage();
          p_newAlloc = false;
        }
        return (l_startPage);
      }

      uint8_t* getPageAddr(unsigned int p_pageIdx) {
        assert(p_pageIdx < m_pages.size());
        unit8_t* l_addr = (uint8_t*) &(m_pages[p_pageIdx])
        return(l_addr);
      }

      unit8_t* getBaseInstrAddr() {
        return (uint8_t*) &(m_pages[t_InstrPageIdx]);
      }
      
      uint8_t* getBaseParamAddr() {
        return (uint8_t*) &(m_pages[t_ParamPageIdx]);
      }

      uint32_t getCurrParamOff() {
        return m_currParamOff;
      }

      uint8_t* getBaseStatsAddr() {
        return (uint8_t*) &(m_pages[t_StatsPageIdx]);
      }

      uint8_t* getBaseDataAddr() {
        return (uint8_t*) &(m_pages[t_DataPageIdx]);
      }
    
      /**
       * @brief getCurrInstrAddr returns the current instrction and parameter sections' addresses
       * @param p_paramsBytes number of bytes in parameters
       * @param p_instr current instruction section address
       * @param p_params current parameter section address
       */ 
      void getCurrInstrAddr(
        unsigned int p_paramBytes,
        uint8_t *p_instr,
        uint8_t *p_params
      ) {
        assert(m_numInstrs < t_MaxNumInstrs);
        assert((m_currParamOff + p_paramBytes) < ((t_StatsPageIdx - t_InstrPageIdx)*t_PageSizeBytes));
        uint8_t* l_baseInstrAddr = getBaseInstrAddr();
        p_instr = (uint8_t*) &(l_baseInstrAddr[m_numInstrs*t_InstrSizeBytes]);
        uint8_t* l_baseParamAddr = getBaseParamAddr();
        p_params = (uint8_t*) &(l_baseInstrAddr[m_currParamOff]);
        m_numInstrs++;
        m_currParamOff += p_paramBytes;
      } 
       
    private:
      unsigned int m_numInstrs;
      uint32_t m_currParamOff;
      PageVectorType m_pages;
      //use m_handles to track data memory allocations for vectors or matrices
      unordered_map<string, PageHandleDescriptor> m_handles; 
  };

} //end namespace blas
} //end namespace linear_algebra
} //end namespace xf
#endif
