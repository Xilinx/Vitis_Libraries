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
 */

#include <iostream>
#include <fstream>
#include <string>
#include "xf_blas.h"

# define IDX2R(i,j,ld) (((i)*( ld ))+(j))
# define m 128 // a - mxk matrix
# define n 128 // b - kxn matrix
# define k 128 // c - mxn matrix

using namespace std;

bool compareGemm(short* a, short* b, short* c){
  short * goldenC;
  goldenC = (short*) malloc(m * n * sizeof (short));
  bool l_check = true;
  for(int row = 0; row < m; row++){ 
      for(int col = 0; col < n; col++){ 
          short l_val = 0;
          for (int i = 0; i < k; i ++) {
            l_val += a[IDX2R(row,i,k)] * b[IDX2R(i,col,n)];
          }
          goldenC[IDX2R(row,col,n)] = l_val;
      } 
  }
  for(int row = 0; row < m; row++){ 
    for(int col = 0; col < n; col++){
      if (goldenC[IDX2R(row,col,n)]!=c[IDX2R(row,col,n)]){
        cout<<"golden result "<<goldenC[IDX2R(row,col,n)]<<" is not equal to fpga result "<<c[IDX2R(row,col,n)]<<"\n";
        l_check = false;
      }
    }
  }
  return l_check;
}

int main(int argc, char **argv) {
  
  if (argc < 3){
    cerr << " usage: \n"
         << " gemx_test.exe gemx.xclbin config_info.dat log.txt 0\n"
         << " gemx_test.exe gemx.xclbin config_info.dat log.txt\n"
         << " gemx_test.exe gemx.xclbin config_info.dat\n";
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
  
  int l_kernelID = 0;
  
  if (argc == 5){
    cout<<"read custom kernel ID\n";
    l_kernelID = stoi(argv[l_argIdx++]); 
  }
  
  int i, j; // i-row index ,j- column index
  short * a, * b, * c;
  a = ( short *) malloc (m*k* sizeof ( short )); // host memory for a
  b = ( short *) malloc (k*n* sizeof ( short )); 
  c = ( short *) malloc (m*n* sizeof ( short )); 

  int ind = 1;
  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < k; j ++){ 
      a[ IDX2R (i,j,k )]= (short) ind++; 
    } 
  } 
  ind = 1;
  for( i = 0; i<  k; i ++){ 
    for( j = 0; j < n; j ++){ 
      b[ IDX2R (i,j,n )]= (short) ind++; 
    } 
  } 

  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < n; j ++){ 
      c[ IDX2R (i,j,n )]= 0; 
    } 
  } 

  xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
  xfblasStatus_t status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), XFBLAS_ENGINE_GEMM, l_kernelID);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Create Handle failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  status = xfblasMallocRestricted(m,k,sizeof(*a),a,k);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix A failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  
  status = xfblasMallocRestricted(k,n,sizeof(*b),b,n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix B failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  status = xfblasMallocRestricted(m,n,sizeof(*c),c,n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix C failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  
  status = xfblasSetMatrixRestricted(a);
  status = xfblasSetMatrixRestricted(b);
  status = xfblasSetMatrixRestricted(c);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Set Matrix failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  
  status = xfblasSgemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, a, k, b, n, 1, c, n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  
  status = xfblasGetMatrixRestricted(c);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Get Matirx failed with error code: "<< status << "\n"; 
    xfblasDestory();
    return EXIT_FAILURE;   
  }
  
  for ( i = 0; i < 10; i ++){
    for ( j = 0; j < 10; j ++){
      cout<< (c[ IDX2R (i,j, k )])<<" ";
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
