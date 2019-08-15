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

#include "gemv_mkl_helper.hpp"

#include <fstream>
#include <string>
using namespace std;


int main(int argc, char **argv) {

  if (argc < 4) {
    printf("Usage: gemv_mkl m n dir\n");
    return EXIT_FAILURE;
  }

  int m=atoi(argv[1]), n=atoi(argv[2]);
  XFBLAS_dataType *a, *x, *y, alpha = 1., beta = 1.;

  // Generating Random Input
  a = createMat(m, n);
  x = createMat(n, 1);
  y = createMat(m, 1, true);

  ofstream outFile;
  string data_dir(argv[3]);

  outFile.open(data_dir+"matA_in_"+to_string(m)+"_"+to_string(n)+".bin", ofstream::binary);
  outFile.write( (char*) a, sizeof(XFBLAS_dataType)*m*n );
  outFile.close();

  outFile.open(data_dir+"vecX_in_"+to_string(n)+"_"+to_string(1)+".bin", ofstream::binary);
  outFile.write( (char*) x, sizeof(XFBLAS_dataType)*n*1 );
  outFile.close();

  outFile.open(data_dir+"vecY_in_"+to_string(m)+"_"+to_string(1)+".bin", ofstream::binary);
  outFile.write( (char*) y, sizeof(XFBLAS_dataType)*m*1 );
  outFile.close();
  
  // Generating Golden Output
  GEMV_MKL(m, n, alpha, beta, a, x, y);

  outFile.open(data_dir+"vecY_out_"+to_string(m)+"_"+to_string(1)+".bin", ofstream::binary);
  outFile.write( (char*) y, sizeof(XFBLAS_dataType)*m*1 );
  outFile.close();

  free(a);
  free(x);
  free(y);

  return 0;
}


