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
      BLAS_dataType p_res
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
