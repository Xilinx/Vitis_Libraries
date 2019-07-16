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

#include <string>
#include <cmath>
#include <iomanip>
#include "xf_blas.h"

# define IDX2R(i,j,ld) (((i)*( ld ))+(j))
# define m 5 // a - mxk matrix
# define n 5 // b - kxn matrix
# define k 5 // c - mxn matrix

using namespace std;

XFBLAS_dataType* getGoldenMat(XFBLAS_dataType* a, XFBLAS_dataType* b, XFBLAS_dataType* c){
  XFBLAS_dataType * goldenC;
  goldenC = (XFBLAS_dataType*) malloc(m * n * sizeof (XFBLAS_dataType));
  for(int row = 0; row < m; row++){ 
      for(int col = 0; col < n; col++){ 
          XFBLAS_dataType l_val = 0;
          for (int i = 0; i < k; i ++) {
            l_val += a[IDX2R(row,i,k)] * b[IDX2R(i,col,n)];
          }
          goldenC[IDX2R(row,col,n)] = l_val + c[IDX2R(row,col,n)];
      } 
  }
  return goldenC;
}

bool compareGemm(XFBLAS_dataType* c, XFBLAS_dataType* goldenC, float p_TolRel=1e-3, float p_TolAbs=1e-5){
  bool l_check = true;
  for(int row = 0; row < m; row++){ 
    for(int col = 0; col < n; col++){
      XFBLAS_dataType l_ref = goldenC[IDX2R(row,col,n)];
      XFBLAS_dataType l_result = c[IDX2R(row,col,n)];
      XFBLAS_dataType l_diffAbs = abs(l_ref-l_result);
      XFBLAS_dataType l_diffRel = l_diffAbs;
      if (goldenC[IDX2R(row,col,n)] != 0 ){
        l_diffRel /= abs(l_ref);
      }
      bool check = (l_diffRel <= p_TolRel) || (l_diffAbs <= p_TolAbs);
      if (!check){
        cout<<"golden result"<< setprecision(10) <<goldenC[IDX2R(row,col,n)]<<" is not equal to fpga result "<< setprecision(10) <<c[IDX2R(row,col,n)]<<"\n";
        l_check = false;
      }
    }
  }
  return l_check;
}

int main(int argc, char **argv) {
  
  if (argc < 3){
    cerr << " usage: \n"
         << " gemx_common_test.exe gemx.xclbin config_info.dat log.txt\n"
         << " gemx_common_test.exe gemx.xclbin config_info.dat\n";
    return EXIT_FAILURE; 
  }
  unsigned int l_argIdx = 1;
  string l_xclbinFile(argv[l_argIdx++]);
  string l_configFile(argv[l_argIdx++]);
  string l_logFile;
  
  if (argc == 3){
    ofstream logFile("log.txt");
    logFile.close();
    l_logFile = "log.txt";
  } else {
    l_logFile = argv[l_argIdx++];
  }
  
  
  int l_numKernel = 1;
  
  if (argc == 5){
    cout<<"read custom number of kernels\n";
    l_numKernel = stoi(argv[l_argIdx++]); 
  }
  
  
  int i, j; // i-row index ,j- column index

  XFBLAS_dataType * a, * b, * c;
  a = ( XFBLAS_dataType *) malloc (m*k* sizeof ( XFBLAS_dataType )); // host memory for a
  b = ( XFBLAS_dataType *) malloc (k*n* sizeof ( XFBLAS_dataType )); 
  c = ( XFBLAS_dataType *) malloc (m*n* sizeof ( XFBLAS_dataType )); 
  
  int ind = 1;
  
  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < k; j ++){ 
      a[ IDX2R (i,j,k )]=( XFBLAS_dataType ) ind++; 
    } 
  } 

  for( i = 0; i<  k; i ++){ 
    for( j = 0; j < n; j ++){ 
      b[ IDX2R (i,j,n )]=( XFBLAS_dataType ) ind++; 
    } 
  } 

  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < n; j ++){ 
      c[ IDX2R (i,j,n )]= 0; 
    } 
  } 
  
  XFBLAS_dataType* goldenC = getGoldenMat(a,b,c);
  
  XFBLAS_dataType * d_a, * d_b, * d_c;
  
 
  xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
  xfblasStatus_t status = XFBLAS_STATUS_SUCCESS;
  
  status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), XFBLAS_ENGINE_GEMM, l_numKernel);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Create Handle failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasMalloc(&d_a, m,k,sizeof(*a), l_numKernel-1);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix A failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  status = xfblasMalloc(&d_b, k,n,sizeof(*b), l_numKernel-1);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix B failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasMalloc(&d_c, m,n,sizeof(*c), l_numKernel-1);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix C failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasSetMatrix(m,k,sizeof(*a),a,k,d_a, l_numKernel-1);
  status = xfblasSetMatrix(k,n,sizeof(*b),b,n,d_b, l_numKernel-1);
  status = xfblasSetMatrix(m,n,sizeof(*c),c,n,d_c, l_numKernel-1);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Set Matrix failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, d_a, k, d_b, n, 1, d_c, n, l_numKernel-1);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  status = xfblasGetMatrix(m,n,sizeof(*c),d_c,c,m, l_numKernel-1);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Get Matirx failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  for ( i = 0; i < m; i ++){
    for ( j = 0; j < n; j ++){
      cout<< (c[ IDX2R (i,j, k )])<<" ";
    }
    cout<<"\n";
  }
  
  if (compareGemm(c, goldenC)){
    cout<<"Test passed!\n";
  }else{
    cout<<"Test failed!\n";
  }
  
  xfblasFree(d_a, l_numKernel-1);
  xfblasFree(d_b, l_numKernel-1);
  xfblasFree(d_c, l_numKernel-1);
  xfblasDestory(l_numKernel);
  free(a);
  free(b);
  free(c);
  
  
}
