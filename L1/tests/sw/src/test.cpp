/*
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
 */
#include <iostream>
#include <string>
#include <vector>
#include "blas_def.h"
#include "uut_top.h"

using namespace xf::linear_algebra::blas;

int main(int argc, char** argv){
  if (argc < 2) {
    std::cout << "ERROR: passed %d arguments instead of %d, exiting" << argc << 2 << std::endl;
    std::cout << " Usage:" << std::endl;
    std::cout << "    test.exe testfile.bin" << std::endl;
    std::cout << " Example Usage:" << std::endl;
    std::cout << "    test.exe ./data/test_amax.bin" << std::endl;
    return EXIT_FAILURE;
  }
  std::string l_binFile(argv[1]);
  GenBinType l_gen; 
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

  vector<Instr> l_instrs;
  int l_returnRes = 0;
  xfblasStatus_t l_status = l_gen.readInstrs(l_binFile, l_instrs);
  for (unsigned int i=0; i<l_instrs.size(); ++i) {
    Instr l_curInstr=l_instrs[i];
    if (l_curInstr.m_opClass == B1_OP_CLASS) {
      l_gen.decodeB1Instr(l_curInstr, l_n, l_alpha, l_x, l_y, l_xRes, l_yRes, l_resGolden);
      l_returnRes = uut_top(l_n, l_alpha, l_x, l_y, l_xRes, l_yRes, l_resGolden);
      if (l_returnRes != 0) {
        break;
      }
    }
  }
  //compute
  return l_returnRes;
};
