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
 *  @brief xf_blas compiler 
 *
 *  $DateTime: 2019/06/18 $
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "blas_def.h"

using namespace std;
using namespace xf::linear_algebra::blas;

void to_upper(string &p_str) {
  for_each(p_str.begin(), p_str.end(), [](char &c) {
    c = toupper(c);
  });
}

int main(int argc, char** argv)
{
  if (argc < 3 ){
    cout << "ERROR: passed " << argc <<" arguments, less than least arguments number " << 3 << ", exiting" << endl;
    cout << "  Usage:\n    blas_gen_bin.exe  <-write | -read> app.bin [op1 arg arg ...] [op2 arg arg ...] ..."
              << "    Ops:\n"
              << "      opName n alpha handleX handleY handleResX handleResY resScalar\n"
              << "    Examples:\n"
              << "      blas_gen_bin.exe -write app.bin amin 8092 0 x null null null 0\n"
              << "      blas_gen_bin.exe -read app.bin\n"
              << "\n";
    return EXIT_FAILURE;
  }
  
  string l_mode(argv[1]);
  bool l_write = l_mode == "-write";
  bool l_read = l_mode == "-read";
  bool l_print = l_mode == "-print";
  
  string l_binFile;
  
  if (l_read || l_write || l_print) {
    l_binFile = argv[2];

    printf("XFBLAS:  %s %s %s\n",
           argv[0], l_mode.c_str(), l_binFile.c_str());
  } else {
    assert(0);
  }
  
  // Early assert for proper instruction length setting
  assert(BLAS_instrSizeBytes * BLAS_argInstrWidth == BLAS_memWidthBytes); 
  
  ////////////////////////  TEST PROGRAM STARTS HERE  ////////////////////////
  GenBinType l_gen;
  if (l_write) {
    unsigned int l_argIdx = 3;
    unsigned int l_instrCount = 0;
    
    vector<BLAS_dataType> l_x, l_y, l_xRes, l_yRes;
    unsigned int l_xIdx=0, l_yIdx=0, l_xResIdx=0, l_yResIdx=0;
    while (l_argIdx < argc) {
      string l_opName(argv[l_argIdx++]);
      uint32_t l_n = stoi(argv[l_argIdx++]);
      BLAS_dataType l_alpha = static_cast<BLAS_dataType>(atof(argv[l_argIdx++]));
      string l_handleX(argv[l_argIdx++]);
      string l_handleY(argv[l_argIdx++]);
      string l_handleXres(argv[l_argIdx++]);
      string l_handleYres(argv[l_argIdx++]);
      BLAS_resDataType l_resScalar = static_cast<BLAS_resDataType>(atof(argv[l_argIdx++]));
      uint8_t* l_xPtr=nullptr;
      uint8_t* l_yPtr=nullptr;
      uint8_t* l_xResPtr=nullptr;
      uint8_t* l_yResPtr=nullptr;
      to_upper(l_handleX);
      to_upper(l_handleY);
      to_upper(l_handleXres);
      to_upper(l_handleYres);
      
      if (l_handleX != "NULL") {
        l_x.resize(l_xIdx+l_n);
        for (unsigned int d=0; d<l_n; ++d) {
          l_x[l_xIdx+d] = (BLAS_dataType)d/10;
        }
        l_xPtr = reinterpret_cast<uint8_t*>(&(l_x[l_xIdx]));
      }
      if (l_handleY != "NULL") {
        l_y.resize(l_yIdx+l_n);
        for (unsigned int d=0; d<l_n; ++d) {
          l_y[l_yIdx+d] = (BLAS_dataType)d;
        }
        l_yPtr = reinterpret_cast<uint8_t*>(&(l_y[l_yIdx]));
      }
      if (l_handleXres != "NULL") {
        l_xRes.resize(l_xResIdx+l_n);
        for (unsigned int d=0; d<l_n; ++d) {
          l_xRes[l_xResIdx+d] = (BLAS_dataType)d;
        }
        l_xResPtr = reinterpret_cast<uint8_t*>(&(l_xResPtr[l_xResIdx]));
      }
      if (l_handleYres != "NULL") {
        l_yRes.resize(l_yResIdx+l_n);
        for (unsigned int d=0; d<l_n; ++d) {
          l_yRes[l_yResIdx+d] = (BLAS_dataType)d;
        }
        l_yResPtr = reinterpret_cast<uint8_t*>(&(l_yRes[l_yResIdx]));
      }
      xfblasStatus_t l_status= l_gen.addB1Instr(
                        l_opName, l_n, l_alpha, 
                        l_xPtr, l_yPtr, l_xResPtr, l_yResPtr, l_resScalar);
      assert(l_status == XFBLAS_STATUS_SUCCESS);
    }
    xfblasStatus_t l_status = l_gen.write2BinFile(l_binFile);
    assert(l_status == XFBLAS_STATUS_SUCCESS);
  }
  else if (l_print) {
    xfblasStatus_t l_status = l_gen.readFromBinFile(l_binFile);
    assert(l_status == XFBLAS_STATUS_SUCCESS);
    l_gen.printProgram();
  } 
  else if (l_read) {
    vector<Instr> l_instrs;
    uint32_t l_n;
    BLAS_dataType l_alpha;
    BLAS_resDataType l_resGolden;
    BLAS_dataType *l_x, *l_y, *l_xRes, *l_yRes;
    BLAS_dataType l_xVal=0;
    BLAS_dataType l_yVal=0;
    BLAS_dataType l_xResVal=0;
    BLAS_dataType l_yResVal=0;
    l_x = &l_xVal;
    l_y = &l_yVal;
    l_xRes = &l_xResVal;
    l_yRes = &l_yResVal;

    xfblasStatus_t l_status = l_gen.readInstrs(l_binFile, l_instrs);
    for (unsigned int i=0; i<l_instrs.size(); ++i) {
      Instr l_curInstr=l_instrs[i];
      if (l_curInstr.m_opClass == B1_OP_CLASS) {
        l_gen.decodeB1Instr(l_curInstr, l_n, l_alpha, l_x, l_y, l_xRes, l_yRes, l_resGolden);
        cout <<"  n=" << l_n << "  alpha=" << l_alpha << "  resScalar=" << l_resGolden << endl;
        if (l_x != nullptr) {
          cout << "  x:" << endl;
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_x[i] << endl;
          }
        }
        if (l_y != nullptr) {
          cout << "  y:" << endl;
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_y[i];
          }
        }
        if (l_xRes != nullptr) {
          cout << "  xRes:" << endl;
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_xRes[i];
          }
        }
        if (l_yRes != nullptr) {
          cout << "  yRes:" << endl;
          for (unsigned int i=0; i<l_n; ++i) {
            cout << l_yRes[i];
          }
        }
      }
    }
  } else {
    assert(0); // Unknown user command
  }
  
  return EXIT_SUCCESS;
}

  
