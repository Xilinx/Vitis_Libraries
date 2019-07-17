.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _example_gemm_l3:

=====================
L3 API GEMM example
=====================


1. xfblasGemm - matrix-matrix multiplication 
---------------------------------------------

.. code-block:: c++

  #include "xf_blas.hpp"
  
  # define IDX2R(i,j,ld) (((i)*( ld ))+(j))
  # define m 5 // a - mxk matrix
  # define n 5 // b - kxn matrix
  # define k 5 // c - mxn matrix
  
  using namespace std;
  
  int main(int argc, char **argv) {
    
    if (argc < 3){
      cerr << " usage: \n"
           << " gemx_common_test.exe gemx.xclbin config_info.dat\n";
      return EXIT_FAILURE; 
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    string l_logFile;
    ofstream logFile("log.txt");
    logFile.close();
    l_logFile = "log.txt";
    
    int i, j; // i-row index ,j- column index
  
    short * a, * b, * c;
    a = ( short *) malloc (m*k* sizeof ( short )); 
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
    
    status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), XFBLAS_ENGINE_GEMM);
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
    
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, d_a, k, d_b, n, 1, d_c, n);
    
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

    xfblasFree(d_a);
    xfblasFree(d_b);
    xfblasFree(d_c);
    xfblasDestory();
    free(a);
    free(b);
    free(c);
    
  }
  
2. xfblasGemm - restricted memory version
-------------------------------------------

.. code-block:: c++

  #include "xf_blas.hpp"
  
  # define IDX2R(i,j,ld) (((i)*( ld ))+(j))
  # define m 128 // a - mxk matrix
  # define n 128 // b - kxn matrix
  # define k 128 // c - mxn matrix
  
  using namespace std;
  
  int main(int argc, char **argv) {
    
    if (argc < 3){
      cerr << " usage: \n"
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
    xfblasStatus_t status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), XFBLAS_ENGINE_GEMM);
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
    
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, a, k, b, n, 1, c, n);
    
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
    
    xfblasFree(a);
    xfblasFree(b);
    xfblasFree(c);
    xfblasDestory();
    free(a);
    free(b);
    free(c);
    
    return EXIT_SUCCESS;
  }

3. xfblasGemm - pre-allocated memory version
---------------------------------------------

.. code-block:: c++
  
  #include <string>
  #include <cmath>
  #include <iomanip>
  #include "xf_blas.hpp"

  # define IDX2R(i,j,ld) (((i)*( ld ))+(j))
  # define m 5 // a - mxk matrix
  # define n 5 // b - kxn matrix
  # define k 5 // c - mxn matrix
  
  int main(int argc, char **argv) {
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    string l_logFile(argv[l_argIdx++]);
    
    int i, j; // i-row index ,j- column index
  
    short * a, * b, * c;
    
    int padded_lda, padded_ldb, padded_ldc;
    
    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status = XFBLAS_STATUS_SUCCESS;
    
    status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), XFBLAS_ENGINE_GEMM);
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Create Handle failed with error code: "<< status << "\n"; 
      return EXIT_FAILURE;   
    }
    
    status = xfblasMallocManaged(&a, &padded_lda, m,k,sizeof(*a));
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Malloc memory for matrix A failed with error code: "<< status << "\n"; 
      return EXIT_FAILURE;   
    }
    status = xfblasMallocManaged(&b, &padded_ldb, k,n,sizeof(*b));
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Malloc memory for matrix B failed with error code: "<< status << "\n"; 
      return EXIT_FAILURE;   
    }
    
    status = xfblasMallocManaged(&c, &padded_ldc, m,n,sizeof(*c));
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Malloc memory for matrix C failed with error code: "<< status << "\n"; 
      return EXIT_FAILURE;   
    }
    
    
    for( i = 0; i<  m; i ++){ 
        for( j = 0; j < k; j ++){ 
            a[ IDX2R (i,j,padded_lda)]=( short ) 1; 
        } 
    } 
    
    for( i = 0; i<  k; i ++){ 
        for( j = 0; j < n; j ++){ 
            b[ IDX2R (i,j,padded_ldb )]=( short ) 1; 
        } 
    } 
  
    for( i = 0; i<  m; i ++){ 
        for( j = 0; j < n; j ++){ 
            c[ IDX2R (i,j,padded_ldc )]= 1; 
        } 
    } 
    
    cout<< "C before running GEMM\n";
    
    for ( i = 0; i < m; i ++){
          for ( j = 0; j < n; j ++){
              cout<< (c[ IDX2R (i,j,padded_ldc)])<<" ";
          }
          cout<<"\n";
    }
        
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, a, k, b, n, 1, c, n);
    
    status = xfblasDeviceSynchronize();
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
      return EXIT_FAILURE;   
    }
    
    cout<<"C after running GEMM\n";
    
    for ( i = 0; i < m; i ++){
          for ( j = 0; j < n; j ++){
              cout<< (c[ IDX2R (i,j, padded_ldc)])<<" ";
          }
          cout<<"\n";
    }
      
    if (compareGemm(c, goldenC, padded_ldc)){
      cout<<"Test passed!\n";
    }else{
      cout<<"Test failed!\n";
    }
    
    xfblasFree(a);
    xfblasFree(b);
    xfblasFree(c);
    xfblasDestory();
    
  }
  
