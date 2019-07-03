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


#include "xf_blas.h"

# define IDX2R(i,j,ld) (((i)*( ld ))+(j))
# define m 5 // a - mxk matrix
# define n 5 // b - kxn matrix
# define k 5 // c - mxn matrix

using namespace std;

bool compareGemm(short* a, short* b, short* c){
  short * goldenC;
  goldenC = (short*) malloc(m*n* sizeof ( short ));
  bool l_check = true;
  for(int row = 0; row< m; row++){ 
      for(int col = 0; col < n; col++){ 
          short l_val = 0;
          for (int i = 0; i <k; i++ ) {
            l_val += a[IDX2R(row,i,k)]*b[IDX2R(i,col,n)];
          }
          goldenC[IDX2R(row,col,n)] = l_val;
      } 
  }
  for(int row = 0; row< m; row++){ 
    for(int col = 0; col < n; col++){
      if (goldenC[IDX2R(row,col,n)]!=c[IDX2R(row,col,n)]){
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
      a[ IDX2R (i,j,k )]=( short ) ind++; 
    } 
  } 

  for( i = 0; i<  k; i ++){ 
    for( j = 0; j < n; j ++){ 
      b[ IDX2R (i,j,n )]=( short ) ind++; 
    } 
  } 

  for( i = 0; i<  m; i ++){ 
    for( j = 0; j < n; j ++){ 
      c[ IDX2R (i,j,n )]= 0; 
    } 
  } 
  
  short * d_a, * d_b, * d_c;
  
 
  xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
  xfblasStatus_t status = XFBLAS_STATUS_SUCCESS;
  
  status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), XFBLAS_ENGINE_GEMM, l_kernelID);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Create Handle failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasMalloc(&d_a, m,k,sizeof(*a));
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix A failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  status = xfblasMalloc(&d_b, k,n,sizeof(*b));
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix B failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasMalloc(&d_c, m,n,sizeof(*c));
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Malloc memory for matrix C failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasSetMatrix(m,k,sizeof(*a),a,k,d_a);
  status = xfblasSetMatrix(k,n,sizeof(*b),b,n,d_b);
  status = xfblasSetMatrix(m,n,sizeof(*c),c,n,d_c);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Set Matrix failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  status = xfblasSgemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, d_a, k, d_b, n, 1, d_c, n);
  
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  status = xfblasGetMatrix(m,n,sizeof(*c),d_c,c,m);
  
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
  
  if (compareGemm(a, b, c)){
    cout<<"Test passed!\n";
  }else{
    cout<<"Test failed!\n";
  }
  
  xfblasFree(d_a);
  xfblasFree(d_b);
  xfblasFree(d_c);
  xfblasDestory();
  free(a);
  free(b);
  free(c);
  
  
}
