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

#include <string>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <fstream>

#include "xf_blas.hpp"

#define IDX2R(i, j, ld) (((i) * (ld)) + (j))

using namespace std;


bool compareGemm(XFBLAS_dataType* c, XFBLAS_dataType* goldenC, int m, int k, int n, float p_TolRel=1e-3, float p_TolAbs=1e-5){
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
         << " gemx_test.exe gemx.xclbin config_info.dat m k n\n"
         << " gemx_test.exe gemx.xclbin config_info.dat\n";
    return EXIT_FAILURE; 
  }
  int l_argIdx = 0;
  string l_xclbinFile, l_configFile;
  if (argc > ++l_argIdx) {l_xclbinFile = argv[l_argIdx];cout<<l_xclbinFile<<"\n"; }
  if (argc > ++l_argIdx) {l_configFile = argv[l_argIdx];}
  string l_logFile;

  ofstream logFile("xrt_report.txt");
  logFile.close();
  l_logFile = "xrt_report.txt";
  
  int l_numKernel = 1;
  int m = 256;
  int k = 256;
  int n = 256;
  
  if (argc > ++l_argIdx) {m = stoi(argv[l_argIdx]);}
  if (argc > ++l_argIdx) {k = stoi(argv[l_argIdx]);}
  if (argc > ++l_argIdx) {n = stoi(argv[l_argIdx]);}
  
  bool test_common;
  
  if (argc > ++l_argIdx) {test_common = stoi(argv[l_argIdx]);}

  
  int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1
  XFBLAS_dataType * a, * b, * c, * goldenC;
  
  posix_memalign((void** )&a, 4096, m*k* sizeof ( XFBLAS_dataType ));
  posix_memalign((void** )&b, 4096, k*n* sizeof ( XFBLAS_dataType ));
  posix_memalign((void** )&c, 4096, m*n* sizeof ( XFBLAS_dataType ));
  posix_memalign((void** )&goldenC, 4096, m*n* sizeof ( XFBLAS_dataType ));
  
  ifstream inFile;
  string data_dir("./data/");

  inFile.open(data_dir+"matA_in_"+to_string(m)+"_"+to_string(k)+".bin", ifstream::binary);
  if( inFile.is_open() ){
    inFile.read( (char*) a, sizeof(XFBLAS_dataType)*m*k );
    inFile.close();
  } else {
    cerr << "Error opening "<<(data_dir+"matA_in_"+to_string(m)+"_"+to_string(k)+".bin")<<endl;
    exit(1);
  }
  
  inFile.open(data_dir+"matB_in_"+to_string(k)+"_"+to_string(n)+".bin", ifstream::binary);
  if( inFile.is_open() ){
    inFile.read( (char*) b, sizeof(XFBLAS_dataType)*k*n );
    inFile.close();
  } else {
    cerr << "Error opening "<<(data_dir+"matB_in_"+to_string(k)+"_"+to_string(n)+".bin")<<endl;
    exit(1);
  }

  inFile.open(data_dir+"matC_in_"+to_string(m)+"_"+to_string(n)+".bin", ifstream::binary);
  if( inFile.is_open() ){
    inFile.read( (char*) c, sizeof(XFBLAS_dataType)*m*n );
    inFile.close();
  } else {
    cerr << "Error opening "<<(data_dir+"matC_in_"+to_string(m)+"_"+to_string(n)+".bin")<<endl;
    exit(1);
  }
  
  inFile.open(data_dir+"matC_out_"+to_string(m)+"_"+to_string(n)+".bin", ifstream::binary);
  if( inFile.is_open() ){
    inFile.read( (char*) goldenC, sizeof(XFBLAS_dataType)*m*n );
    inFile.close();
  } else {
    cerr << "Error opening "<<(data_dir+"matC_out_"+to_string(m)+"_"+to_string(n)+".bin")<<endl;
    exit(1);
  }
  
  xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
  xfblasStatus_t status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
  if (status != XFBLAS_STATUS_SUCCESS) {
    cout<<"Create Handle failed with error code: "<< status << "\n"; 
    return EXIT_FAILURE;   
  }
  
  if (test_common){
    cout<<"test default version\n";
    XFBLAS_dataType * d_a, * d_b, * d_c;
    status = xfblasMalloc(&d_a, m,k,sizeof(*a), l_numKernel-1);
    status = xfblasMalloc(&d_b, k,n,sizeof(*b), l_numKernel-1);
    status = xfblasMalloc(&d_c, m,n,sizeof(*c), l_numKernel-1);
    status = xfblasSetMatrix(m,k,sizeof(*a),a,k,d_a, l_numKernel-1);
    status = xfblasSetMatrix(k,n,sizeof(*b),b,n,d_b, l_numKernel-1);
    status = xfblasSetMatrix(m,n,sizeof(*c),c,n,d_c, l_numKernel-1);
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, d_a, k, d_b, n, 1, d_c, n, l_numKernel-1);
    status = xfblasGetMatrix(m,n,sizeof(*c),d_c,c,n, l_numKernel-1);   
  } else {
    cout<<"test restricted version\n";
    status = xfblasMallocRestricted(m,k,sizeof(*a),a,k, l_numKernel-1);
    status = xfblasMallocRestricted(k,n,sizeof(*b),b,n, l_numKernel-1);
    status = xfblasMallocRestricted(m,m,sizeof(*c),c,n, l_numKernel-1);
    status = xfblasSetMatrixRestricted(a, l_numKernel-1);
    status = xfblasSetMatrixRestricted(b, l_numKernel-1);
    status = xfblasSetMatrixRestricted(c, l_numKernel-1);
    status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, a, k, b, n, 1, c, n, l_numKernel-1);
    status = xfblasGetMatrixRestricted(c, l_numKernel-1);
  }
  
  for ( i = 0; i < 10; i ++){
    for ( j = 0; j < 10; j ++){
      cout<< (c[ IDX2R (i,j, k )])<<" ";
    }
    cout<<"\n";
  }
  
  if (compareGemm(c,goldenC,m,k,n)){
    cout<<"Test passed!\n";
  }else{
    cout<<"Test failed!\n";
  }
  
  xfblasFree(a, l_numKernel-1);
  xfblasFree(b, l_numKernel-1);
  xfblasFree(c, l_numKernel-1);
  free(a);
  free(b);
  free(c);
    
  xfblasDestory(l_numKernel);

  return EXIT_SUCCESS;
}
