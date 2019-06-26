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

#ifndef XF_BLAS_UTILITY_H
#define XF_BLAS_UTILITY_H

#include <fstream>
#include <string>
#include <unordered_map>

using namespace std;

namespace xf {
namespace linear_algebra {
namespace blas {

typedef enum {
    XFBLAS_STATUS_SUCCESS,          //0
    XFBLAS_STATUS_NOT_INITIALIZED,  //1
    XFBLAS_STATUS_INVALID_VALUE,    //2
    XFBLAS_STATUS_ALLOC_FAILED,     //3
    XFBLAS_STATUS_NOT_SUPPORTED,    //4
    XFBLAS_STATUS_NOT_PADDED,       //5
    XFBLAS_STATUS_MEM_ALLOCATED,    //6
    XFBLAS_STATUS_INVALID_OP,       //7
    XFBLAS_STATUS_INVALID_FILE,     //8
    XFBLAS_STATUS_INVALID_PROGRAM   //9
} xfblasStatus_t;
  

typedef enum {
    XFBLAS_ENGINE_GEMM,
    XFBLAS_ENGINE_GEMV
} xfblasEngine_t;  
  

typedef enum {
  XFBLAS_OP_N,
  XFBLAS_OP_T,
  XFBLAS_OP_C
} xfblasOperation_t;


xfblasStatus_t buildConfigDict(string p_configFile, xfblasEngine_t p_engineName, unordered_map<string, string>* p_configDict){
  unordered_map<string,string> l_configDict;
  ifstream l_configInfo(p_configFile);
  bool l_good = l_configInfo.good();
  if (!l_good){
      return XFBLAS_STATUS_NOT_INITIALIZED;
  }
  if (l_configInfo.is_open()){
      string line;
      string key;
      string value;
      string equalSign = "=";
      while (getline(l_configInfo,line)) {
          int index = line.find(equalSign);
          if (index == 0) continue;
          key = line.substr(0,index);
          value = line.substr(index+1);
          l_configDict[key]=value;
      }
  }
  
  l_configInfo.close();
  
  // Additional limit for different engines
  if (p_engineName == XFBLAS_ENGINE_GEMM){
      if (l_configDict.find("GEMX_gemmMBlocks")!=l_configDict.end()){
        int l_mBlock = stoi(l_configDict["GEMX_gemmMBlocks"]);
        int l_kBlock = stoi(l_configDict["GEMX_gemmKBlocks"]);
        int l_nBlock = stoi(l_configDict["GEMX_gemmNBlocks"]);
        int l_ddrWidth = stoi(l_configDict["GEMX_ddrWidth"]);
        int l_maxBlock = max(l_mBlock, max(l_kBlock, l_nBlock));
        int l_minSize = l_ddrWidth * l_maxBlock;
        l_configDict["minSize"] = to_string(l_minSize);  
      } else {
        return XFBLAS_STATUS_NOT_INITIALIZED;
      }
  } 
  
  *p_configDict = l_configDict;
  
  return XFBLAS_STATUS_SUCCESS;
}

  
  
}
}
}


#endif
