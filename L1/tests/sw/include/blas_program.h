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
#include <fstream>
#include <iostream>
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
    typename t_DataType,
    typename t_ResDataType,
    unsigned int t_MemWidthBytes,
    unsigned int t_InstrSizeBytes,
    unsigned int t_PageSizeBytes,
    unsigned int t_MaxNumInstrs,
    unsigned int t_InstrPageIdx,
    unsigned int t_ParamPageIdx,
    unsigned int t_StatsPageIdx
  >
  class Program {
    public:
      typedef Page<uint8_t, t_PageSizeBytes> PageType;
      typedef vector<PageType> PageVectorType;
      typedef ParamB1<t_DataType, t_ResDataType> ParamB1Type; 
    public:
      static const unsigned int ParamStartOff = t_ParamPageIdx * t_PageSizeBytes;
      static const size_t ParamB1Bytes = sizeof(ParamB1Type);
    public:
      Program() : 
        m_numInstrs(0), 
        m_currParamOff(t_ParamPageIdx*t_PageSizeBytes) {
          assert((t_PageSizeBytes % t_MemWidthBytes) == 0);
          m_pages.resize(t_StatsPageIdx+1);
          PageType l_page0 = m_pages[0];
          //initialize instruction page with 0s
          for (unsigned int i=0; i<t_PageSizeBytes; ++i) {
            l_page0[i] = 0;
          }
        }

      void clear() {
        m_numInstrs = 0;
        m_currParamOff = t_ParamPageIdx*t_PageSizeBytes;
        m_pages.clear();
        m_datMem.clear();
        m_datMemSize.clear();
      }
      
      void reset(){
        m_numInstrs = 0;
        m_currParamOff = t_ParamPageIdx*t_PageSizeBytes;
      }

      void allocPages(size_t p_numPages) {
        m_pages.resize(p_numPages);
      }

      unsigned int& getNumInstrs() {
        return m_numInstrs;
      }
      
      uint32_t getCurrParamOff(){
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

      void addInstr(
        uint8_t* p_instr,
        uint8_t* p_param,
        unsigned int p_paramBytes
      ) {
        assert(m_numInstrs < t_MaxNumInstrs);
        assert((m_currParamOff + p_paramBytes) < (t_StatsPageIdx*t_PageSizeBytes));
        uint8_t* l_baseInstrAddr = getBaseInstrAddr();
        uint8_t* l_instr = (uint8_t*) &(l_baseInstrAddr[m_numInstrs*t_InstrSizeBytes]);
        memcpy(l_instr, p_instr, t_InstrSizeBytes);
        uint8_t* l_param = (uint8_t*) &(l_baseInstrAddr[m_currParamOff]);
        memcpy(l_param, p_param, p_paramBytes);

        m_numInstrs++;
        m_currParamOff += p_paramBytes;
        while (m_currParamOff % t_MemWidthBytes != 0) {
          m_currParamOff++;
        }
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
      xfblasStatus_t getInstrs(vector<Instr> &p_instrs) {
        reset();
        xfblasStatus_t l_status = XFBLAS_STATUS_INVALID_PROGRAM;
        uint8_t* l_baseInstrAddr = getBaseInstrAddr();
        uint8_t* l_addr = l_baseInstrAddr;
        Instr l_instr;
        memcpy((uint8_t*) &l_instr, l_addr, t_InstrSizeBytes);
        while (l_instr.m_opCode != NULL_OP) {
          p_instrs.push_back(l_instr);
          m_numInstrs++;
          l_addr += t_InstrSizeBytes;
          memcpy((uint8_t*) &l_instr, l_addr, t_InstrSizeBytes);
        }
        if (p_instrs.size() > 0) {
          l_status = XFBLAS_STATUS_SUCCESS;
        }
        return(l_status);
      }

      ParamB1Type getB1Param() {
        ParamB1Type l_param;
        uint8_t* l_baseInstrAddr = getBaseInstrAddr();
        memcpy((uint8_t*)&l_param, l_baseInstrAddr+m_currParamOff, ParamB1Bytes);
        m_currParamOff += ParamB1Bytes;
        while (m_currParamOff % t_MemWidthBytes != 0) {
          m_currParamOff++;
        }
        return (l_param);
      }

      void writePaddingBytes(size_t p_bytes, ofstream &p_of) {
        uint8_t l_zeroConst=0;
        for (unsigned int b=0; b<p_bytes; ++b) {
          p_of.write((char*)&l_zeroConst, 1);
        }
      }
      void writeData(
        size_t p_dataBytes, 
        size_t p_paddingBytes,
        uint64_t &p_addr, 
        ifstream::pos_type &p_ofPos,
        ofstream &p_of) {
          p_of.seekp(p_ofPos);
          char* l_addr = reinterpret_cast<char*>(p_addr);
          p_addr = (uint64_t)p_ofPos;
          p_of.write(l_addr, p_dataBytes);
          writePaddingBytes(p_paddingBytes, p_of);
          p_ofPos += (p_dataBytes+p_paddingBytes);
      }
      void writeB1Param(
        ofstream &p_of,
        ParamB1Type &p_param,
        ifstream::pos_type &p_ofParamPos,
        ifstream::pos_type &p_ofDataPos) {
        
        uint32_t l_n = p_param.m_n;
        size_t l_dataBytes = l_n*sizeof(t_DataType);
        size_t l_paddingBytes = t_PageSizeBytes - (l_dataBytes%t_PageSizeBytes);
        uint8_t l_zeroConst=0;

        ifstream::pos_type l_ofPos = p_ofDataPos;
        if (p_param.m_xAddr != 0) {
          writeData(l_dataBytes, l_paddingBytes, p_param.m_xAddr, l_ofPos, p_of);
        }
        if (p_param.m_yAddr != 0) {
          writeData(l_dataBytes, l_paddingBytes, p_param.m_yAddr, l_ofPos, p_of);
        }
        if (p_param.m_xResAddr != 0) {
          writeData(l_dataBytes, l_paddingBytes, p_param.m_xResAddr, l_ofPos, p_of);
        }
        if (p_param.m_yResAddr != 0) {
          writeData(l_dataBytes, l_paddingBytes, p_param.m_yResAddr, l_ofPos, p_of);
        }
        p_ofDataPos = l_ofPos;

        p_of.seekp(p_ofParamPos);
        p_of.write((char*)&p_param, ParamB1Bytes);
        l_paddingBytes = t_MemWidthBytes - (ParamB1Bytes % t_MemWidthBytes);
        writePaddingBytes(l_paddingBytes, p_of);
        p_ofParamPos += (ParamB1Bytes + l_paddingBytes);
      }

      xfblasStatus_t write2BinFile(const string &p_fileName) {
        xfblasStatus_t l_status=XFBLAS_STATUS_SUCCESS;
        ofstream l_of(p_fileName.c_str(), ios::binary);
        if (l_of.is_open()) {
          uint8_t* l_baseInstrAddr = getBaseInstrAddr();
          l_of.write(reinterpret_cast<char*>(l_baseInstrAddr), (t_StatsPageIdx+1)*t_PageSizeBytes);
          reset();
          vector<Instr> l_instrs;
          xfblasStatus_t l_status=getInstrs(l_instrs);
          ifstream::pos_type l_ofDataPos = (t_StatsPageIdx+1)*t_PageSizeBytes;
          ifstream::pos_type l_ofParamPos = m_currParamOff;
          
          for (unsigned int i=0; i<m_numInstrs; ++i) {
            if (l_instrs[i].m_opClass == B1_OP_CLASS) {
              ParamB1Type l_param = getB1Param();
              writeB1Param(l_of, l_param, l_ofParamPos, l_ofDataPos);
            }
          }
          l_of.close();
        }
        else {
          l_status = XFBLAS_STATUS_INVALID_FILE;
        }
        return(l_status);
      }
     
      xfblasStatus_t readFromBinFile(string p_fileName) {
        xfblasStatus_t l_status=XFBLAS_STATUS_INVALID_FILE;
        ifstream l_if(p_fileName.c_str(), ios::binary);
        if (l_if.is_open()) {
          size_t l_fileBytes = getFileSize(p_fileName);
          cout << "INFO: loading " << p_fileName << " of size " << l_fileBytes <<endl;
          size_t l_fileSizeInPages = l_fileBytes / t_PageSizeBytes;
          assert(l_fileBytes % t_PageSizeBytes == 0);
          clear();
          allocPages(l_fileSizeInPages);
          l_if.read((char*) (getBaseInstrAddr()), l_fileBytes);
          if (l_if) {
            cout << "INFO: loaded " << l_fileBytes << " bytes from " << p_fileName << endl;
            l_status = XFBLAS_STATUS_SUCCESS;
          }
          else {
            clear();
            cout << "ERROR: loaded only " << l_if.gcount() << " bytes from " << p_fileName << endl;
          }
          l_if.close();
        }
        vector<Instr> l_instrs;
        l_status = getInstrs(l_instrs);
        assert(l_status == XFBLAS_STATUS_SUCCESS);
        uint8_t* l_baseInstrAddr=getBaseInstrAddr();
        for (unsigned int i=0; i<l_instrs.size(); ++i) {
          if (l_instrs[i].m_opClass == B1_OP_CLASS) {
            ParamB1Type l_param;
            memcpy((uint8_t*)&l_param, l_baseInstrAddr+m_currParamOff, ParamB1Bytes);
            l_param.m_xAddr = (l_param.m_xAddr != 0)? reinterpret_cast<uint64_t> (l_baseInstrAddr+l_param.m_xAddr):0;
            l_param.m_yAddr = (l_param.m_yAddr != 0)? reinterpret_cast<uint64_t> (l_baseInstrAddr+l_param.m_yAddr):0;
            l_param.m_xResAddr = (l_param.m_xResAddr != 0)? reinterpret_cast<uint64_t> (l_baseInstrAddr+l_param.m_xResAddr):0;
            l_param.m_yResAddr = (l_param.m_yResAddr != 0)? reinterpret_cast<uint64_t> (l_baseInstrAddr+l_param.m_yResAddr):0;
            memcpy(l_baseInstrAddr+m_currParamOff, (uint8_t*)&l_param, ParamB1Bytes); 
            m_currParamOff += ParamB1Bytes;
            while (m_currParamOff % t_MemWidthBytes != 0) {
              m_currParamOff++;
            }
          }
        }
        return (l_status);
      }
      
      xfblasStatus_t readInstrsFromBinFile(
        string p_fileName,
        vector<Instr> &p_instrs
      ) {
        xfblasStatus_t l_status=readFromBinFile(p_fileName);
        assert (l_status == XFBLAS_STATUS_SUCCESS);
        l_status = getInstrs(p_instrs);
        return(l_status);
      }
      
      void decodeB1Instr(
        const Instr &p_instr,
        uint32_t &p_n, 
        t_DataType &p_alpha, 
        t_DataType* &p_x,
        t_DataType* &p_y,
        t_DataType* &p_xRes,
        t_DataType* &p_yRes,
        t_ResDataType &p_resScalar
      ) {
        uint8_t* l_baseAddr = getBaseInstrAddr();
        uint8_t* l_paramAddr = l_baseAddr + p_instr.m_paramOff;
        ParamB1Type l_param;
        memcpy((uint8_t*)&l_param, l_paramAddr, ParamB1Bytes);
        p_n = l_param.m_n;
        p_alpha = l_param.m_alpha;
        p_resScalar = l_param.m_resScalar;
        p_x = reinterpret_cast<t_DataType*>(l_param.m_xAddr);
        p_y = reinterpret_cast<t_DataType*>(l_param.m_yAddr);
        p_xRes = reinterpret_cast<t_DataType*>(l_param.m_xResAddr);
        p_yRes = reinterpret_cast<t_DataType*>(l_param.m_yResAddr); 
      }

    void print(ostream &os) {
      reset();
      vector<Instr> l_instrs;
      xfblasStatus_t l_status = getInstrs(l_instrs);
      assert(l_status == XFBLAS_STATUS_SUCCESS);
      uint8_t* l_baseInstrAddr=getBaseInstrAddr();
      for (unsigned int i=0; i<l_instrs.size(); ++i) {
        os << l_instrs[i];
        if (l_instrs[i].m_opClass == B1_OP_CLASS) {
          ParamB1Type l_param = getB1Param();
          os << l_param;
        }
      }
    }

    private:
      unsigned int m_numInstrs;
      uint32_t m_currParamOff;
      PageVectorType m_pages;
      unordered_map<t_HandleType, void*> m_datMem;
      unordered_map<t_HandleType, unsigned long long> m_datMemSize;
    private:  
      ifstream::pos_type getFileSize(string p_fileName) {
        ifstream in(p_fileName.c_str(), ifstream::ate | ifstream::binary);
        return in.tellg();
      }
  };

  template <
    typename T1,
    typename T2,
    typename T3,
    unsigned int T4,
    unsigned int T5,
    unsigned int T6,
    unsigned int T7,
    unsigned int T8,
    unsigned int T9,
    unsigned int T10
  >
  ostream& operator<<(ostream &os, Program<T1,T2,T3,T4,T5,T6,T7,T8,T9,T10> &p_val) {
    p_val.print(os);
    return(os);
  }

} //end namespace blas
} //end namespace linear_algebra
} //end namespace xf
#endif
