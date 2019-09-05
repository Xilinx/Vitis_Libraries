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
  
  # define IDX2R(i, j, ld) (((i) * (ld)) + (j))
  # define m 5 // a - mxk matrix
  # define n 5 // b - kxn matrix
  # define k 5 // c - mxn matrix
  
  using namespace std;
  
  int main(int argc, char **argv) {
    
    if (argc < 3){
      cerr << " usage: \n"
           << " gemm_common_test.exe gemx.xclbin config_info.dat 1\n"
           << " gemm_common_test.exe gemx.xclbin config_info.dat\n";
      return EXIT_FAILURE; 
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    string l_logFile;
     
    ofstream logFile("xrt_report.txt");
    logFile.close();
    l_logFile = "xrt_report.txt";
    
    int l_numKernel = 1;
    
    if (argc == 4){
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
      
    XFBLAS_dataType * d_a, * d_b, * d_c;
    
   
    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status = XFBLAS_STATUS_SUCCESS;
    
    status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
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
    
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, n, k, 1, d_a, k, d_b, n, 1, d_c, n, l_numKernel-1);
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
      return EXIT_FAILURE;   
    }
    status = xfblasGetMatrix(m,n,sizeof(*c),d_c,c,n, l_numKernel-1);
    
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
    
    // 590 605 620 635 650 
    // 1490 1530 1570 1610 1650 
    // 2390 2455 2520 2585 2650 
    // 3290 3380 3470 3560 3650 
    // 4190 4305 4420 4535 4650 
  
    xfblasFree(d_a, l_numKernel-1);
    xfblasFree(d_b, l_numKernel-1);
    xfblasFree(d_c, l_numKernel-1);
    xfblasDestroy(l_numKernel);
    free(a);
    free(b);
    free(c);
    
    
  }
  
2. xfblasGemm - restricted memory version
-------------------------------------------

.. code-block:: c++
  
  #include <iomanip>
  #include "xf_blas.hpp"

  # define IDX2R(i, j, ld) (((i) * (ld)) + (j))
  # define m 128 // a - mxk matrix
  # define n 128 // b - kxn matrix
  # define k 128 // c - mxn matrix

  using namespace std;
  
  int main(int argc, char **argv) {
    
    if (argc < 3){
      cerr << " usage: \n"
           << " gemm_test.exe gemx.xclbin config_info.dat 1\n"
           << " gemm_test.exe gemx.xclbin config_info.dat\n";
      return EXIT_FAILURE; 
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);  
    string l_logFile;
    ofstream logFile("xrt_report.txt");
    logFile.close();
    l_logFile = "xrt_report.txt";
    int l_numKernel = 1;
    
    if (argc == 4){
      cout<<"read custom number of kernels\n";
      l_numKernel = stoi(argv[l_argIdx++]); 
    }
    
    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Create Handle failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
      
    int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1
    XFBLAS_dataType * a, * b, * c;
    
    posix_memalign((void** )&a, 4096, m*k* sizeof ( XFBLAS_dataType ));
    posix_memalign((void** )&b, 4096, k*n* sizeof ( XFBLAS_dataType ));
    posix_memalign((void** )&c, 4096, m*n* sizeof ( XFBLAS_dataType ));
    
    int ind = 1;
    for( i = 0; i<  m; i ++){ 
      for( j = 0; j < k; j ++){ 
        a[ IDX2R (i,j,k )]= (XFBLAS_dataType) ind++; 
      } 
    } 
    ind = 1;
    for( i = 0; i<  k; i ++){ 
      for( j = 0; j < n; j ++){ 
        b[ IDX2R (i,j,n )]= (XFBLAS_dataType) ind++; 
      } 
    } 
  
    for( i = 0; i<  m; i ++){ 
      for( j = 0; j < n; j ++){ 
        c[ IDX2R (i,j,n )]= 0; 
      } 
    } 
        
    status = xfblasMallocRestricted(m,k,sizeof(*a),a,k, l_numKernel-1);
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Malloc memory for matrix A failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
    
    status = xfblasMallocRestricted(k,n,sizeof(*b),b,n, l_numKernel-1);
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Malloc memory for matrix B failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
    status = xfblasMallocRestricted(m,n,sizeof(*c),c,n, l_numKernel-1);
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Malloc memory for matrix C failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
    
    status = xfblasSetMatrixRestricted(a, l_numKernel-1);
    status = xfblasSetMatrixRestricted(b, l_numKernel-1);
    status = xfblasSetMatrixRestricted(c, l_numKernel-1);
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Set Matrix failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
    
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, n, k, 1, a, k, b, n, 1, c, n, l_numKernel-1);
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Matrix Multiplication failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
    
    status = xfblasGetMatrixRestricted(c, l_numKernel-1);
    
    if (status != XFBLAS_STATUS_SUCCESS) {
      cout<<"Get Matirx failed with error code: "<< status << "\n"; 
      xfblasDestroy();
      return EXIT_FAILURE;   
    }
    
    for ( i = 0; i < m; i ++){
      for ( j = 0; j < n; j ++){
        cout<< (c[ IDX2R (i,j, k )])<<" ";
      }
      cout<<"\n";
    }
    
    
    xfblasFree(a, l_numKernel-1);
    xfblasFree(b, l_numKernel-1);
    xfblasFree(c, l_numKernel-1);
    free(a);
    free(b);
    free(c);
    
    xfblasDestroy(l_numKernel);
  
    return EXIT_SUCCESS;
  }

3. xfblasGemm - pre-allocated memory version
---------------------------------------------

.. code-block:: c++
  
  #include "xf_blas.hpp"
  
  # define IDX2R(i, j, ld) (((i) * (ld)) + (j))
  # define m 5 // a - mxk matrix
  # define n 5 // b - kxn matrix
  # define k 5 // c - mxn matrix
  
  using namespace std;
  
  int main(int argc, char **argv) {
    
    if (argc < 3){
      cerr << " usage: \n"
           << " gemm_pre_allocated_test.exe gemx.xclbin config_info.dat\n";
      return EXIT_FAILURE; 
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    string l_logFile;
    
    ofstream logFile("xrt_report.txt");
    logFile.close();
    l_logFile = "xrt_report.txt";
    
    int i, j; // i-row index ,j- column index
  
    XFBLAS_dataType * a, * b, * c;
    
    int padded_lda, padded_ldb, padded_ldc;
    
    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status = XFBLAS_STATUS_SUCCESS;
    
    status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName);
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
    
    int ind = 1;
  
    for( i = 0; i<  m; i ++){ 
        for( j = 0; j < k; j ++){ 
            a[ IDX2R (i,j,padded_lda)]=( XFBLAS_dataType ) ind++; 
        } 
    } 
    
    for( i = 0; i<  k; i ++){ 
        for( j = 0; j < n; j ++){ 
            b[ IDX2R (i,j,padded_ldb )]=( XFBLAS_dataType ) ind++; 
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
      
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, n, k, 1, a, k, b, n, 1, c, n);
    
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
    
    //  591 606 621 636 651 
    // 1491 1531 1571 1611 1651 
    // 2391 2456 2521 2586 2651 
    // 3291 3381 3471 3561 3651 
    // 4191 4306 4421 4536 4651 
  
    
    xfblasFree(a);
    xfblasFree(b);
    xfblasFree(c);
    xfblasDestroy();
    
  }
  
