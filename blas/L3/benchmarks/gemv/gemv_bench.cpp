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
 * usage: ./gemv_bench.exe PATH_TO_XCLBIN/gemx.xclbin PATH_TO_XCLBIN/config_info.dat
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
#include "../bench_helper.hpp"
#include "gemv_helper.hpp"

using namespace std;

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << " usage: \n"
             << " gemv_bench.exe gemx.xclbin config_info.dat m n\n"
             << " gemv_bench.exe gemx.xclbin config_info.dat\n";
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
    int n = 256;
    /*  if (argc == 4){
        cout<<"read custom number of kernels\n";
        l_numKernel = stoi(argv[l_argIdx++]);
      }
    */

    if (argc >= 5) {
        m = stoi(argv[l_argIdx++]);
        n = stoi(argv[l_argIdx++]);
        cout << "Read custom sizes of matrix/vector: (" << m << ", " << n << ", 1)" << endl;
    }

    string data_dir("./data/float/");
    if (argc >= 6) {
        data_dir = (string)argv[l_argIdx++];
        cout << "Read custom data directory: " << data_dir << endl;
    }

    int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1
    XFBLAS_dataType *a, *x, *y, *goldenY;

    posix_memalign((void**)&a, 4096, m * n * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&x, 4096, n * 1 * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&y, 4096, m * 1 * sizeof(XFBLAS_dataType));
    posix_memalign((void**)&goldenY, 4096, m * 1 * sizeof(XFBLAS_dataType));

    ifstream inFile;

    inFile.open(data_dir + "matA_in_" + to_string(m) + "_" + to_string(n) + ".bin", ifstream::binary);
    if (inFile.is_open()) {
        inFile.read((char*)a, sizeof(XFBLAS_dataType) * m * n);
        inFile.close();
    } else {
        cerr << "Error opening " << (data_dir + "matA_in_" + to_string(m) + "_" + to_string(n) + ".bin") << endl;
        exit(1);
    }

    inFile.open(data_dir + "vecX_in_" + to_string(n) + "_1.bin", ifstream::binary);
    if (inFile.is_open()) {
        inFile.read((char*)x, sizeof(XFBLAS_dataType) * n * 1);
        inFile.close();
    } else {
        cerr << "Error opening " << (data_dir + "vecX_in_" + to_string(n) + "_1.bin") << endl;
        exit(1);
    }

    inFile.open(data_dir + "vecY_in_" + to_string(m) + "_1.bin", ifstream::binary);
    if (inFile.is_open()) {
        inFile.read((char*)y, sizeof(XFBLAS_dataType) * m * 1);
        inFile.close();
    } else {
        cerr << "Error opening " << (data_dir + "vecY_in_" + to_string(m) + "_1.bin") << endl;
        exit(1);
    }

    inFile.open(data_dir + "vecY_out_" + to_string(m) + "_1.bin", ifstream::binary);
    if (inFile.is_open()) {
        inFile.read((char*)goldenY, sizeof(XFBLAS_dataType) * m * 1);
        inFile.close();
    } else {
        cerr << "Error opening " << (data_dir + "vecY_out_" + to_string(m) + "_1.bin") << endl;
        exit(1);
    }

    TimePointType l_tp[4];
    unsigned int l_tpIdx = 0;
    l_tp[l_tpIdx] = chrono::high_resolution_clock::now();

    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMV;
    xfblasStatus_t status =
        xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);

    showTimeData("xfblasCreate", l_tp[l_tpIdx], l_tp[l_tpIdx + 1]);
    l_tpIdx++;

    status = xfblasMallocRestricted(m, n, sizeof(*a), a, n, l_numKernel - 1);
    status = xfblasMallocRestricted(n, 1, sizeof(*x), x, 1, l_numKernel - 1);
    status = xfblasMallocRestricted(m, 1, sizeof(*y), y, 1, l_numKernel - 1);

    status = xfblasSetMatrixRestricted(a, l_numKernel - 1);
    status = xfblasSetVectorRestricted(x, l_numKernel - 1);
    status = xfblasSetVectorRestricted(y, l_numKernel - 1);

    showTimeData("copyToFpga", l_tp[l_tpIdx], l_tp[l_tpIdx + 1]);
    l_tpIdx++;

    status = xfblasGemv(XFBLAS_OP_N, m, n, 1, a, n, x, 1, 1, y, 1, l_numKernel - 1);
    status = xfblasGetVectorRestricted(y, l_numKernel - 1);

    showTimeData("copyFromFpga", l_tp[l_tpIdx], l_tp[l_tpIdx + 1]);
    l_tpIdx++;

    chrono::duration<double> l_timeApi = l_tp[l_tpIdx] - l_tp[1];
    double l_timeMs = l_timeApi.count() * 1e3;

    cout << "Api time is " << fixed << setprecision(6) << l_timeMs << " msec\n";

    unordered_map<string, string> l_configDict;

    readConfigDict(l_configFile, &l_configDict);

    float l_freq = getBoardFreqMHz(l_xclbinFile);
    int GEMX_ddrWidth = stoi(l_configDict["GEMX_ddrWidth"]);
    unsigned long int l_Ops = 2ull * m * n + m;
    unsigned long int l_Parallel_Ops = 2ull * m * n;

    double l_perfApiInTops = l_Ops / (l_timeMs * 1e-3) / 1e12;
    double l_timeMsAt100pctEff = l_Parallel_Ops / 2 / GEMX_ddrWidth / GEMX_ddrWidth / (l_freq * 1e6) * 1e3;
    double l_effApiPct = 100 * l_timeMsAt100pctEff / l_timeMs;

    cout << std::string("DATA_CSV:,Freq,M,N,") + "TimeApiMs," + "EffApiPct,PerfApiTops\n";
    cout << "DATA_CSV:," << l_freq << "," << m << "," << n << "," << l_timeMs << "," << l_effApiPct << ","
         << l_perfApiInTops << "\n";

    for (i = 0; i < 10; i++) {
        cout << (y[i]) << " ";
        cout << "\n";
    }

    if (compareGemv(y, goldenY, m)) {
        cout << "Test passed!\n";
    } else {
        cout << "Test failed!\n";
    }

    xfblasFree(a, l_numKernel - 1);
    xfblasFree(x, l_numKernel - 1);
    xfblasFree(y, l_numKernel - 1);
    free(a);
    free(x);
    free(y);
    free(goldenY);

    xfblasDestroy(l_numKernel);

    return EXIT_SUCCESS;
}
