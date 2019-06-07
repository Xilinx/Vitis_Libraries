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
#include "util.h"
#include "asum_top.h"

int main(int argc, char** argv){
  if (argc < 4) {
    std::cout << "ERROR: passed %d arguments instead of %d, exiting" << argc << 4 << std::endl;
    std::cout << " Usage:" << std::endl;
    std::cout << "    test.exe input_vector_file number_of_entries_in_the_vector golden_value" << std::endl;
    std::cout << " Example Usage:" << std::endl;
    std::cout << "    test.exe ./data/vec0.csv 1024 102" << std::endl;
    return EXIT_FAILURE;
  }
  std::string l_fileName(argv[1]);
  unsigned int l_n = atoi(argv[2]);
  
  BLAS_dataType l_vec[BLAS_size];
  int l_res = 0;
  l_res = readVector<BLAS_dataType, BLAS_size>(l_fileName, l_n, l_vec);
  if (l_res != 0) {
    std::cout << "ERROR: failed to read input vector file, exiting" << std::endl;
    return EXIT_FAILURE;
  }
 
  BLAS_dataType l_sumOut, l_sumRef = atof(argv[3]);
  //compute
  UUT_Top(l_vec, l_n, l_sumOut);
  //compute golden reference
  //aminRef<BLAS_dataType, BLAS_size, BLAS_indexType>(l_vec, l_n, l_sumRef);

  //compare
  double err_rel=abs(l_sumOut/(double) l_sumRef-1.0);
  BLAS_dataType err_abs=abs(l_sumOut-l_sumRef);
  std::cout << "Relative error is "<<err_rel<<", and absolute error is " << err_abs<<std::endl;
  if (err_rel>1e-3 || err_abs > 1e-3) {
    std::cout << "ERROR: output index != golden reference" << std::endl;
    std::cout << "outVal = " << l_sumOut << " refVal = " << l_sumRef << std::endl; 
    return -1;
  }
  return 0;
};
