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
 *  $DateTime: 2019/06/14$
 */

#ifndef BLAS_GEN_BIN_H
#define BLAS_GEN_BIN_H

#include <cstring>
#include "blas_program.h"

using namespace std;
namespace xf {
namespace linear_algebra {
namespace blas {

  template<
    typename t_DataType,
    typename t_HandleType,
    unsigned int t_MemWidthBytes,
    unsigned int t_InstrSizeBytes=8,
    unsigned int t_PageSizeBytes=4096,
    unsigned int t_MaxNumInstrs=64,
    unsigned int t_InstrPageIdx=0,
    unsigned int t_ParamPageIdx=1,
    unsigned int t_StatsPageIdx=2,
    unsigned int t_DataPageIdx=3
  >
  class GenBin {
    public:
      typedef ParamB1<t_DataType> ParamB1Type;
    public:
      static const size_t ParamB1Bytes = 36 + 2*sizeof(t_DataType);
    public:
      GenBin() {}
      xfblasStatus_t addB1Instr(
        string p_opName,
        uint32_t p_n,
        t_DataType p_alpha,
        void* p_x,
        void* p_y,
        void* p_xRes,
        void* p_yRes,
        t_DataType p_res
      ) {
        uint32_t l_opCode32;
        xfblasStatus_t l_status = m_opFinder.getOpCode(p_opName, l_opCode32);
        uint16_t l_opClass, l_opCode; 
        if ((l_status == XFBLAS_STATUS_SUCCESS) && (l_opCode32 < B1_MaxOpCode)) { //BLAS L1 operations
          l_opClass = 0;
          l_opCode = l_opCode32;
          
          Instr l_instr;
          l_instr.m_opClass = l_opClass;
          l_instr.m_opCode = l_opCode;
          l_instr.m_paramOff = m_program.getCurrParamOff();

          ParamB1Type l_param;
          l_param.m_n = p_n;
          l_param.m_alpha = p_alpha;
          l_param.m_resScalar = p_res;
          l_param.m_xOff = 0;
          l_param.m_yOff = 0;
          l_param.m_xResOff = 0;
          l_param.m_yResOff = 0;

          unsigned long long l_dataBufSize;
          if (p_x != nullptr) {
            l_status = m_program.regDatMem(p_x, p_x, p_n*sizeof(t_DataType));
            if (l_status != XFBLAS_STATUS_SUCCESS) {
              return (l_status);
            }
            l_param.m_xOff = reinterpret_cast<uint64_t>(m_program.getDatMem(p_x, l_dataBufSize));
          }
          if (p_y != nullptr) {
            l_status = m_program.regDatMem(p_y, p_y, p_n*sizeof(t_DataType));
            if (l_status != XFBLAS_STATUS_SUCCESS) {
              return (l_status);
            }
            l_param.m_yOff = reinterpret_cast<uint64_t>(m_program.getDatMem(p_y, l_dataBufSize));
          }

          if (p_xRes != nullptr) {
            l_status = m_program.regDatMem(p_xRes, p_xRes, p_n*sizeof(t_DataType));
            if (l_status != XFBLAS_STATUS_SUCCESS) {
              return (l_status);
            }
            l_param.m_xResOff = reinterpret_cast<uint64_t>(m_program.getDatMem(p_xRes, l_dataBufSize));
          }
          if (p_yRes != nullptr) {
            l_status = m_program.regDatMem(p_yRes, p_yRes, p_n*sizeof(t_DataType));
            if (l_status != XFBLAS_STATUS_SUCCESS) {
              return (l_status);
            }
            l_param.m_yResOff = reinterpret_cast<uint64_t>(m_program.getDatMem(p_yRes, l_dataBufSize));
          }

          uint8_t* l_instrAddr;
          uint8_t* l_paramAddr;
          m_program.getCurrInstrAddr(ParamB1Bytes, l_instrAddr, l_paramAddr);
          uint8_t* l_instrVal = reinterpret_cast<uint8_t*> (&l_instr);
          uint8_t* l_paramVal = reinterpret_cast<uint8_t*> (&l_param);
        
          //store the instruction and its parameters into memory
          memcpy(l_instrAddr, l_instrVal, t_InstrSizeBytes);  
          memcpy(l_paramAddr, l_paramVal, ParamB1Bytes);

          return(XFBLAS_STATUS_SUCCESS);
        }
        else {
          return(XFBLAS_STATUS_INVALID_OP);
        }
      }

      xfblasStatus_t write2BinFile(string p_fileName){
        xfblasStatus_t l_status=XFBLAS_STATUS_SUCCESS;
        ofstream l_of(p_fileName.c_str(), ios::binary);
        if (l_of.is_open()) {
          //write instructions to file
          uint8_t* l_baseInstrAddr = m_program.getBaseInstrAddr();
          l_of.write(reinterpret_cast<char*>(l_baseInstrAddr), t_DataPageIdx*t_PageSizeBytes);
          //write data and fix x,y,xRes and yRes offset in the parameters
          unsigned int l_numInstrs = m_program.getNumInstrs();
          uint8_t *l_addr = l_baseInstrAddr;
          long l_ofPos = l_of.tellp();
          for (unsigned int i=0; i<l_numInstrs; ++i) {
            Instr l_instr;
            memcpy((uint8_t*) &l_instr, l_addr, t_InstrSizeBytes);
            uint8_t *l_paramAddr = l_baseInstrAddr + l_instr.m_paramOff;
            ParamB1Type l_param;
            memcpy((uint8_t*) &l_param, l_paramAddr, ParamB1Bytes);
            uint32_t l_n = l_param.m_n;
            size_t l_vecBytes = l_n * sizeof(t_DataType);
            uint8_t l_zeroConst = 0;
            unsigned int l_paddingBytes = (t_PageSizeBytes - (l_vecBytes%t_PageSizeBytes)) % t_PageSizeBytes;
            //write x vector
            if (l_param.m_xOff != 0) {
              uint8_t* l_xAddr = reinterpret_cast<uint8_t*>(l_param.m_xOff);
              l_param.m_xOff = l_ofPos;
              l_of.write((char*)l_xAddr, l_vecBytes);
              for (unsigned int b=0; b<l_paddingBytes; ++b) {
                l_of.write((char*)&l_zeroConst, 1);
              }
              l_ofPos = l_of.tellp();
            }
            //write y vector
            if (l_param.m_yOff != 0) {
              uint8_t* l_yAddr = reinterpret_cast<uint8_t*>(l_param.m_yOff);
              l_param.m_yOff = l_ofPos;
              l_of.write((char*)l_yAddr, l_vecBytes);
              for (unsigned int b=0; b<l_paddingBytes; ++b) {
                l_of.write((char*)&l_zeroConst, 1);
              }
              l_ofPos = l_of.tellp();
            }
            //write xRes vector 
            if (l_param.m_xResOff != 0) {
              uint8_t* l_xResAddr = reinterpret_cast<uint8_t*>(l_param.m_xResOff);
              l_param.m_xResOff = l_ofPos;
              l_of.write((char*)l_xResAddr, l_vecBytes);
              for (unsigned int b=0; b<l_paddingBytes; ++b) {
                l_of.write((char*)&l_zeroConst, 1);
              }
              l_ofPos = l_of.tellp();
            }
            //write yRes vector
            if (l_param.m_yResOff != 0) {
              uint8_t* l_yResAddr = reinterpret_cast<uint8_t*>(l_param.m_yResOff);
              l_param.m_yResOff = l_ofPos;
              l_of.write((char*)l_yResAddr, l_vecBytes);
              for (unsigned int b=0; b<l_paddingBytes; ++b) {
                l_of.write((char*)&l_zeroConst, 1);
              }
              l_ofPos = l_of.tellp();
            }
            size_t l_paramOff = l_instr.m_paramOff + 4 + sizeof(t_DataType);
            if (l_param.m_xOff !=0) {
              l_of.seekp(l_paramOff);
              l_of.write((char*) &(l_param.m_xOff), 8);
            }
            l_paramOff += 8;
            if (l_param.m_yOff !=0) {
              l_of.seekp(l_paramOff);
              l_of.write((char*) &(l_param.m_yOff), 8);
            }
            l_paramOff += 8;
            if (l_param.m_xResOff !=0) {
              l_of.seekp(l_paramOff);
              l_of.write((char*) &(l_param.m_xResOff), 8);
            }
            l_paramOff += 8;
            if (l_param.m_yResOff !=0) {
              l_of.seekp(l_paramOff);
              l_of.write((char*) &(l_param.m_yResOff), 8);
            }
            l_of.seekp(l_ofPos);
            l_addr += t_InstrSizeBytes;
          }

          l_of.close();
        }
        else {
          l_status = XFBLAS_STATUS_INVALID_FILE;
        }
        return (l_status);
      }
      void clearProgram() {
        m_program.clear();
      }

      xfblasStatus_t readFromBinFile(string p_fileName) {
        xfblasStatus_t l_status=XFBLAS_STATUS_INVALID_FILE;
        ifstream l_if(p_fileName.c_str(), ios::binary);
        if (l_if.is_open()) {
          size_t l_fileBytes = getFileSize(p_fileName);
          cout << "INFO: loading " << p_fileName << " of size " << l_fileBytes <<endl;
          size_t l_fileSizeInPages = l_fileBytes / t_PageSizeBytes;
          assert(l_fileBytes % t_PageSizeBytes == 0);
          m_program.clear();
          m_program.allocPages(l_fileSizeInPages);
          l_if.read((char*) (m_program.getBaseInstrAddr()), l_fileBytes);
          if (l_if) { 
            cout << "INFO: loaded " << l_fileBytes << " bytes from " << p_fileName << endl;
            l_status = XFBLAS_STATUS_SUCCESS;
          }
          else {
            m_program.clear();
            cout << "ERROR: loaded only " << l_if.gcount() << " bytes from " << p_fileName << endl;
          }
          l_if.close();
        }
        else {
          cout << "ERROR: failed to open file " << p_fileName << endl;
        }
        return (l_status);
      }
      void printB1Param(ParamB1Type &p_param, uint8_t* p_baseAddr){
        cout <<"  n=" << p_param.m_n << "  alpha=" << p_param.m_alpha << "  resScalar=" << p_param.m_resScalar << endl;
        uint32_t l_n = p_param.m_n;
        size_t l_dataBytes = l_n * sizeof(t_DataType);
        vector<t_DataType> l_data;
        l_data.resize(l_n);
        if (p_param.m_xOff !=0) {
          cout << "  x:" << endl;
          memcpy((uint8_t*)&(l_data[0]), reinterpret_cast<uint8_t*>(p_baseAddr+p_param.m_xOff), l_dataBytes);
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_data[i] << endl;
          }
        }
        if (p_param.m_yOff !=0) {
          cout << "  y:" << endl;
          memcpy((uint8_t*)&(l_data[0]), reinterpret_cast<uint8_t*>(p_baseAddr+p_param.m_yOff), l_dataBytes);
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_data[i] << endl;
          }
        }
        if (p_param.m_xResOff !=0) {
          cout << "  xRes:" << endl;
          memcpy((uint8_t*)&(l_data[0]), reinterpret_cast<uint8_t*>(p_baseAddr+p_param.m_xResOff), l_dataBytes);
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_data[i] << endl;
          }
        }
        if (p_param.m_yResOff !=0) {
          cout << "  yRes:" << endl;
          memcpy((uint8_t*)&(l_data[0]), reinterpret_cast<uint8_t*>(p_baseAddr+p_param.m_yResOff), l_dataBytes);
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_data[i] << endl;
          }
        }
      }
      void printProgram() {
        uint8_t* l_baseInstrAddr = m_program.getBaseInstrAddr();
        uint8_t* l_addr = l_baseInstrAddr;
        Instr l_instr;
        memcpy((uint8_t*) &l_instr, l_addr, t_InstrSizeBytes);
        while (l_instr.m_opCode != 0) {
          uint8_t* l_paramAddr = l_baseInstrAddr + l_instr.m_paramOff;
          string l_opName;
          xfblasStatus_t l_status = m_opFinder.getOpName(l_instr.m_opCode, l_opName);
          assert(l_status == XFBLAS_STATUS_SUCCESS);
          cout << "Operation: " << l_opName << endl;
          if (l_instr.m_opClass == 0) { //BLAS L1 function parameters
            ParamB1Type l_param;
            uint8_t* l_paramAddr = l_baseInstrAddr + l_instr.m_paramOff;
            memcpy((uint8_t*)&l_param, l_paramAddr, ParamB1Bytes);
            printB1Param(l_param, l_baseInstrAddr);
          }
          l_addr += t_InstrSizeBytes; 
          memcpy((uint8_t*) &l_instr, l_addr, t_InstrSizeBytes);
        }
      }
    private:
      Program<
        t_HandleType,
        t_MemWidthBytes,
        t_InstrSizeBytes,
        t_PageSizeBytes,
        t_MaxNumInstrs,
        t_InstrPageIdx,
        t_ParamPageIdx,
        t_StatsPageIdx,
        t_DataPageIdx
      > m_program;

      FindOpCodeB1 m_opFinder;
    private:
      ifstream::pos_type getFileSize(string p_fileName) {
        ifstream in(p_fileName.c_str(), ifstream::ate | ifstream::binary);
        return in.tellg();
      }
  };
    
} //end namespace blas
} //end namespace linear_algebra
} //end namespace xf
#endif
