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

#include "gemm_mkl_helper.hpp"

#include <fstream>
#include <string>
using namespace std;

int main(int argc, char **argv) {

  if (argc < 4) {
    printf("Usage: gemm_mkl m k n\n");
    return EXIT_FAILURE;
  }

  int m=atoi(argv[1]), k=atoi(argv[2]), n=atoi(argv[3]);
  XFBLAS_dataType *a, *b, *c, alpha = 1., beta = 1.;

  // Generating Random Input
  a = createMat(m, k);
  b = createMat(k, n);
  c = createMat(m, n);

  ofstream outFile;
  string data_dir("../data/");

  outFile.open(data_dir+"matA_in_"+to_string(m)+"_"+to_string(k)+".bin", ofstream::binary);
  outFile.write( (char*) a, sizeof(XFBLAS_dataType)*m*k );
  outFile.close();

  outFile.open(data_dir+"matB_in_"+to_string(k)+"_"+to_string(n)+".bin", ofstream::binary);
  outFile.write( (char*) b, sizeof(XFBLAS_dataType)*k*n );
  outFile.close();

  outFile.open(data_dir+"matC_in_"+to_string(m)+"_"+to_string(n)+".bin", ofstream::binary);
  outFile.write( (char*) c, sizeof(XFBLAS_dataType)*m*n );
  outFile.close();
  
  // Generating Golden Output
  GEMM_MKL(m, k, n, alpha, beta, a, b, c);

  outFile.open(data_dir+"matC_out_"+to_string(m)+"_"+to_string(n)+".bin", ofstream::binary);
  outFile.write( (char*) c, sizeof(XFBLAS_dataType)*m*n );
  outFile.close();

  free(a);
  free(b);
  free(c);

  return 0;
}


