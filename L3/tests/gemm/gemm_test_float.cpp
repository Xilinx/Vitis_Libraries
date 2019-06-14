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


/*
 * usage: ./gemm_test.exe PATH_TO_XCLBIN/gemx.xclbin PATH_TO_XCLBIN/config_info.dat
 * 
 * compareGemm will compare the golden reference result with the FPGA result, 
 * if either the absolute difference or the relative difference is within the tolerance, 
 * the FPGA result will be considered as correct.
 */

#include <string>
#include <cmath>
#include <iomanip>
#include "xf_blas.h"

# define IDX2R(i,j,ld) (((i)*( ld ))+(j))
# define m 128 // a - mxk matrix
# define n 128 // b - kxn matrix
# define k 128 // c - mxn matrix

using namespace std;

bool compareGemm(float* a, float* b, float* c, float p_TolRel=1e-3, float p_TolAbs=1e-5){
  float * goldenC;
  goldenC = (float*) malloc(m * n * sizeof (float));
  bool l_check = true;
  for(int row = 0; row < m; row++){ 
      for(int col = 0; col < n; col++){ 
          float l_val = 0;
          for (int i = 0; i < k; i ++) {
            l_val += a[IDX2R(row,i,k)] * b[IDX2R(i,col,n)];
          }
          goldenC[IDX2R(row,col,n)] = l_val;
      } 
  }
  for(int row = 0; row < m; row++){ 
    for(int col = 0; col < n; col++){
      float l_ref = goldenC[IDX2R(row,col,n)];
      float l_result = c[IDX2R(row,col,n)];
      float l_diffAbs = abs(l_ref-l_result);
      float l_diffRel = l_diffAbs;
      if (goldenC[IDX2R(row,col,n)] != 0 ){
        l_diffRel /= abs(l_ref);
      }
      bool check = (l_diffRel <= p_TolRel) || (l_diffAbs <= p_TolAbs);
      if (!check){
        cout<<"golden result "<< setprecision(10) <<goldenC[IDX2R(row,col,n)]<<" is not equal to fpga result "<< setprecision(10) <<c[IDX2R(row,col,n)]<<"\n";
        l_check = false;
      }
    }
  }
  return l_check;
}

int main(int argc, char **argv) {
  unsigned int l_argIdx = 1;
  string l_xclbinFile(argv[l_argIdx++]);
  string l_configFile(argv[l_argIdx++]);
  int i, j; // i-row index ,j- column index
  float * a, * b, * c;
  a = ( float *) malloc (m*k* sizeof ( float )); // host memory for a
  b = ( float *) malloc (k*n* sizeof ( float )); 
  c = ( float *) malloc (m*n* sizeof ( float )); 

  float ind = 1;
  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < k; j ++){ 
      a[ IDX2R (i,j,k )]=( float ) ind++;
    } 
  } 
  
  for( i = 0; i<  k; i ++){ 
    for( j = 0; j < n; j ++){ 
      b[ IDX2R (i,j,n )]=( float ) ind++;
    } 
  } 

  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < n; j ++){ 
      c[ IDX2R (i,j,n )]= 0; 
    } 
  } 

  xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
  xfblasStatus_t status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, XFBLAS_ENGINE_GEMM);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Create Handle failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  status = xfblasMalloc(m,k,sizeof(*a),a,k);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix A failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasMalloc(k,n,sizeof(*b),b,n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix B failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  status = xfblasMalloc(m,n,sizeof(*c),c,n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix C failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasSetMatrix(a);
  status = xfblasSetMatrix(b);
  status = xfblasSetMatrix(c);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Set Matrix failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasSgemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, a, k, b, n, 1, c, n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasGetMatrix(c);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Get Matirx failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  for ( i = 0; i < 10; i ++){
    for ( j = 0; j < 10; j ++){
      cout<< setprecision(10) <<(c[ IDX2R (i,j, k )])<<" ";
    }
    cout<<"\n";
  }
  
  if (compareGemm(a, b, c)){
    cout<<"Test passed!\n";
  }else{
    cout<<"Test failed!\n";
  }
  
  xfblasFree(a);
  xfblasFree(b);
  xfblasFree(c);
  xfblasDestory();
  free(a);
  free(b);
  free(c);
  
  return EXIT_SUCCESS;
}
