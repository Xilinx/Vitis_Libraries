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
 * usage: ./gemm_per_test.exe PATH_TO_XCLBIN/gemx.xclbin PATH_TO_XCLBIN/config_info.dat
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
#include "gemm_helper.hpp"

#define IDX2R(i, j, ld) (((i) * (ld)) + (j))

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePointType;

using namespace std;

void showTimeData(string p_Task, TimePointType &t1, TimePointType &t2, double *p_TimeMsOut = 0)
{
  t2 = chrono::high_resolution_clock::now();    
  chrono::duration<double> l_durationSec = t2 - t1;
  double l_timeMs = l_durationSec.count() * 1e3;
  if (p_TimeMsOut) {
    *p_TimeMsOut = l_timeMs;
  }
 cout << p_Task
      << "  " << fixed << setprecision(6)
      << l_timeMs << " msec\n";
}

float getBoardFreqMHz(string xclbin) {
  string l_freqCmd = "xclbinutil --info --input " + xclbin;;
  float l_freq = -1;
  char l_lineBuf[256];
  shared_ptr<FILE> l_pipe(popen(l_freqCmd.c_str(), "r"), pclose);
  //if (!l_pipe) throw std::runtime_error("ERROR: popen(" + l_freqCmd + ") failed");
  if (!l_pipe) cout << ("ERROR: popen(" + l_freqCmd + ") failed");
  bool l_nextLine_isFreq = false;
  while (l_pipe && fgets(l_lineBuf, 256, l_pipe.get()) ) {
      std::string l_line(l_lineBuf);
      //std::cout << "DEBUG: read line " << l_line << std::endl;
      if (l_nextLine_isFreq) {
        std::string l_prefix, l_val, l_mhz;
        std::stringstream l_ss(l_line);
        l_ss >> l_prefix >> l_val >> l_mhz;
        l_freq = std::stof(l_val);
        assert(l_mhz == "MHz");
        break;
      } else if (l_line.find("Type:      DATA") != std::string::npos) {
        l_nextLine_isFreq = true;
      }
  }
  if (l_freq == -1) {
      //if xbutil does not work, user could put the XOCC achieved kernel frequcy here
      l_freq = 250;
      std::cout << "INFO: Failed to get board frequency by xclbinutil. This is normal for cpu and hw emulation, using 250 MHz for reporting.\n";
  }
  return(l_freq);
}

int main(int argc, char **argv) {
  
  if (argc < 3){
    cerr << " usage: \n"
         << " gemx_perf_test.exe gemx.xclbin config_info.dat m k n\n"
         << " gemx_perf_test.exe gemx.xclbin config_info.dat\n";
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
  int m = 256;
  int k = 256;
  int n = 256;
  
  if (argc > 3){
    cout<<"read custom sizes of matrix\n";
    m = stoi(argv[l_argIdx++]);
    k = stoi(argv[l_argIdx++]);
    n = stoi(argv[l_argIdx++]);
  }
  
  int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1
  XFBLAS_dataType * a, * b, * c, * goldenC;
  
  posix_memalign((void** )&a, 4096, m*k* sizeof ( XFBLAS_dataType ));
  posix_memalign((void** )&b, 4096, k*n* sizeof ( XFBLAS_dataType ));
  posix_memalign((void** )&c, 4096, m*n* sizeof ( XFBLAS_dataType ));
  posix_memalign((void** )&goldenC, 4096, m*n* sizeof ( XFBLAS_dataType ));
  
  ifstream inFile;
  string data_dir("./data/");

  inFile.open(data_dir+"matA_in_"+to_string(m)+"_"+to_string(k)+".bin", ifstream::binary);
  inFile.read( (char*) a, sizeof(XFBLAS_dataType)*m*k );
  inFile.close();
  
  inFile.open(data_dir+"matB_in_"+to_string(k)+"_"+to_string(n)+".bin", ifstream::binary);
  inFile.read( (char*) b, sizeof(XFBLAS_dataType)*k*n );
  inFile.close();
  
  inFile.open(data_dir+"matC_in_"+to_string(m)+"_"+to_string(n)+".bin", ifstream::binary);
  inFile.read( (char*) c, sizeof(XFBLAS_dataType)*m*n );
  inFile.close();
  
  inFile.open(data_dir+"matC_out_"+to_string(m)+"_"+to_string(n)+".bin", ifstream::binary);
  inFile.read( (char*) goldenC, sizeof(XFBLAS_dataType)*m*n );
  inFile.close();
  
  TimePointType l_tp[4];
  unsigned int l_tpIdx = 0;
  l_tp[l_tpIdx] = chrono::high_resolution_clock::now(); 
  
  xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
  xfblasStatus_t status = xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
  
  showTimeData("xfblasCreate", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
  
  status = xfblasMallocRestricted(m,k,sizeof(*a),a,k, l_numKernel-1);
  status = xfblasMallocRestricted(k,n,sizeof(*b),b,n, l_numKernel-1);
  status = xfblasMallocRestricted(m,m,sizeof(*c),c,n, l_numKernel-1);
  
  status = xfblasSetMatrixRestricted(a, l_numKernel-1);
  status = xfblasSetMatrixRestricted(b, l_numKernel-1);
  status = xfblasSetMatrixRestricted(c, l_numKernel-1);
  
  showTimeData("copyToFpga", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
  
  
  status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, k, n, 1, a, k, b, n, 1, c, n, l_numKernel-1);
  status = xfblasGetMatrixRestricted(c, l_numKernel-1);
  
  showTimeData("copyFromFpga", l_tp[l_tpIdx], l_tp[l_tpIdx+1]); l_tpIdx++;
  
  chrono::duration<double> l_timeApi = l_tp[l_tpIdx] - l_tp[1];
  double l_timeMs = l_timeApi.count() * 1e3;
  
  
  cout << "Api time is " << fixed << setprecision(6) << l_timeMs << " msec\n";
  
  unordered_map<string, string> l_configDict;
  
  readConfigDict(l_configFile,&l_configDict);
  
  float l_freq = getBoardFreqMHz(l_xclbinFile);
  int GEMX_ddrWidth = stoi(l_configDict["GEMX_ddrWidth"]); 
  unsigned long int l_Ops = 2ull * m * k * n + m * n * 3;
  unsigned long int l_Parallel_Ops = 2ull * m * k * n;
  
  double l_perfApiInTops = l_Ops / (l_timeMs * 1e-3) / 1e12;
  double l_timeMsAt100pctEff = l_Parallel_Ops / 2 / GEMX_ddrWidth / GEMX_ddrWidth / (l_freq * 1e6) * 1e3;
  double l_effApiPct = 100 * l_timeMsAt100pctEff / l_timeMs;
  
  cout << std::string("DATA_CSV:,Freq,M,K,N,")
    + "TimeApiMs,"
    + "EffApiPct,PerfApiTops\n";
  cout << "DATA_CSV:,"<<l_freq<<","<<m<<","<<k<<","<<n<<","
       << l_timeMs <<","<<l_effApiPct<<","<<l_perfApiInTops<<"\n";
  
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
