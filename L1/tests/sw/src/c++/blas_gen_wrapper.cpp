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

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "blas_def.h"

using namespace std;
using namespace xf::linear_algebra::blas;

extern "C" {
  GenBinType* genBinNew(){
    return new GenBinType();
  }

  void genBinDel(GenBinType* genBin){
    delete genBin;
  }

  xfblasStatus_t addB1Instr(GenBinType* genBin,
      const char * p_opName,
      uint32_t p_n,
      BLAS_dataType p_alpha,
      void* p_x,
      void* p_y,
      void* p_xRes,
      void* p_yRes,
      BLAS_resDataType p_res
      ) {
    return  genBin -> addB1Instr(p_opName, p_n, p_alpha, p_x, p_y, p_xRes, p_yRes, p_res);
  }
  xfblasStatus_t write2BinFile(GenBinType* genBin, const char * p_fileName){
    return  genBin -> write2BinFile(p_fileName);
  }
  xfblasStatus_t readFromBinFile(GenBinType* genBin, const char * p_fileName) {
    return  genBin -> readFromBinFile(p_fileName);
  }
  void printProgram(GenBinType* genBin) {
    genBin -> printProgram();
  }
}
