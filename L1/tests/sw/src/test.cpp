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
#include <cmath>
#include "blas_def.h"
#include "uut_top.h"

using namespace xf::linear_algebra::blas;
#include <exception>

//provide same functionality as numpy.isclose
template <typename T>
bool isClose(
  float p_tolRel, 
  float p_tolAbs, 
  T p_vRef, 
  T p_v,
  bool &p_exactMatch
  ) {
  float l_diffAbs = abs(p_v - p_vRef);
  p_exactMatch = (p_vRef == p_v);
  bool l_status = (l_diffAbs <= (p_tolAbs + p_tolRel*l_diffAbs));
  return(l_status);
}
template<typename T>
bool compare(T x, T ref){
  return x == ref;
}

template<>
bool compare<double>(double x, double ref){
  bool l_exactMatch;
  return isClose<double>(1e-3, 3e-6, x, ref, l_exactMatch);
}
template<>
bool compare<float>(float x, float ref){
  bool l_exactMatch;
  return isClose<float>(1e-3, 3e-6, x, ref, l_exactMatch);
}


template<typename T>
bool compare(unsigned int n, T *x, T *ref){
  bool l_ret = true;
  try{
    if(ref == nullptr){
      if(x == nullptr)
        return true;
      for(int i=0;i<n;i++)
        l_ret = l_ret && compare(x[i], (T)0);
    } else {
      for(int i=0;i<n;i++)
        l_ret = l_ret && compare(x[i], ref[i]);
    }
  } catch (exception &e){
    std::cout << "Exception happend: " <<e.what() << std::endl;
    return false;
  }
  return l_ret;
}

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
  BLAS_resDataType l_resGolden=0, l_res=0;
  BLAS_dataType *l_x = nullptr, *l_y= nullptr;
  BLAS_dataType *l_xRes= nullptr, *l_yRes= nullptr;
  BLAS_dataType *l_xResRef= nullptr, *l_yResRef= nullptr;

  vector<Instr> l_instrs;
  bool l_return = true;
  xfblasStatus_t l_status = l_gen.readInstrs(l_binFile, l_instrs);
  for (unsigned int i=0; i<l_instrs.size(); ++i) {
    Instr l_curInstr=l_instrs[i];
    if (l_curInstr.m_opClass == B1_OP_CLASS) {
      l_gen.decodeB1Instr(l_curInstr, l_n, l_alpha, l_x, l_y, l_xResRef, l_yResRef, l_resGolden);
      l_xRes = new BLAS_dataType[l_n];
      l_yRes = new BLAS_dataType[l_n];
      for(int l=0;l<l_n;l++){
        l_xRes[l] = 0;
        l_yRes[l] = 0;
      }
      uut_top(l_n, l_alpha, l_x, l_y, l_xRes, l_yRes, l_res);
      l_return = l_return && compare(l_n, l_xRes, l_xResRef);
      l_return = l_return && compare(l_n, l_yRes, l_yResRef);
      l_return = l_return && compare(l_res, l_resGolden);
      delete []l_xRes;
      delete []l_yRes;
      if(!l_return)
        break;
    }
  }
  //compute
  if(l_return)
    return 0;
  else
    return -1;
};
