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
 * usage: ./gemm_bench.exe PATH_TO_XCLBIN/gemx.xclbin PATH_TO_XCLBIN/config_info.dat
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

void showTimeData(string p_Task, TimePointType& t1, TimePointType& t2, double* p_TimeMsOut = 0) {
    t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> l_durationSec = t2 - t1;
    double l_timeMs = l_durationSec.count() * 1e3;
    if (p_TimeMsOut) {
        *p_TimeMsOut = l_timeMs;
    }
    cout << p_Task << "  " << fixed << setprecision(6) << l_timeMs << " msec\n";
}

float getBoardFreqMHz(string xclbin) {
    string l_freqCmd = "xclbinutil --info --input " + xclbin;
    float l_freq = -1;
    char l_lineBuf[256];
    shared_ptr<FILE> l_pipe(popen(l_freqCmd.c_str(), "r"), pclose);
    // if (!l_pipe) throw std::runtime_error("ERROR: popen(" + l_freqCmd + ") failed");
    if (!l_pipe) cout << ("ERROR: popen(" + l_freqCmd + ") failed");
    bool l_nextLine_isFreq = false;
    while (l_pipe && fgets(l_lineBuf, 256, l_pipe.get())) {
        std::string l_line(l_lineBuf);
        // std::cout << "DEBUG: read line " << l_line << std::endl;
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
        // if xbutil does not work, user could put the XOCC achieved kernel frequcy here
        l_freq = 250;
        std::cout << "INFO: Failed to get board frequency by xclbinutil. This is normal for cpu and hw emulation, "
                     "using 250 MHz for reporting.\n";
    }
    return (l_freq);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << " usage: \n"
             << " gemm_bench.exe gemx.xclbin config_info.dat m k n data_dir iteration\n"
             << " gemm_bench.exe gemx.xclbin config_info.dat\n";
        return EXIT_FAILURE;
    }
    unsigned int l_argIdx = 1;
    string l_xclbinFile(argv[l_argIdx++]);
    string l_configFile(argv[l_argIdx++]);
    string l_logFile;

    ofstream logFile("xrt_report.txt");
    logFile.close();
    l_logFile = "xrt_report.txt";

    int m = 256;
    int k = 256;
    int n = 256;

    if (argc >= 6) {
        m = stoi(argv[l_argIdx++]);
        k = stoi(argv[l_argIdx++]);
        n = stoi(argv[l_argIdx++]);
        cout << "Read custom sizes of matrix: (" << m << ", " << k << ", " << n << ")\n";
    }

    string data_dir("./data/float/");
    if (argc >= 7) {
        data_dir = (string)argv[l_argIdx++];
        cout << "Read custom data directory: " << data_dir << endl;
    }

    int l_numKernel = 1;
    if (argc >= 8) {
        l_numKernel = stoi(argv[l_argIdx++]);
        cout << "Read custom kernel number: " << l_numKernel << endl;
#ifdef XFBLAS_LAUNCH_ASYNC
        cout << "[INFO] Enabled Asynchronous Concurrent Kernels: " << l_numKernel << endl;
#else
        cout << "[INFO] Asynchronous Concurrent Execution Is Disabled." << endl;
#endif
    }

    int iteration = 2;
    if (argc >= 9) {
        iteration = stoi(argv[l_argIdx++]);
        cout << "Read custom iteration: " << iteration << endl;
    }

    int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1
    XFBLAS_dataType **a, **b, **c, **goldenC;

    a = (XFBLAS_dataType**)malloc(l_numKernel * sizeof(XFBLAS_dataType*));
    b = (XFBLAS_dataType**)malloc(l_numKernel * sizeof(XFBLAS_dataType*));
    c = (XFBLAS_dataType**)malloc(l_numKernel * sizeof(XFBLAS_dataType*));
    goldenC = (XFBLAS_dataType**)malloc(l_numKernel * sizeof(XFBLAS_dataType*));

    for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
        posix_memalign((void**)&a[kernelIndex], 4096, m * k * sizeof(XFBLAS_dataType));
        posix_memalign((void**)&b[kernelIndex], 4096, k * n * sizeof(XFBLAS_dataType));
        posix_memalign((void**)&c[kernelIndex], 4096, m * n * sizeof(XFBLAS_dataType));
        posix_memalign((void**)&goldenC[kernelIndex], 4096, m * n * sizeof(XFBLAS_dataType));
    }

    ifstream inFile;

    for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
        inFile.open(data_dir + "matA_in_" + to_string(m) + "_" + to_string(k) + ".bin", ifstream::binary);
        if (inFile.is_open()) {
            inFile.read((char*)a[kernelIndex], sizeof(XFBLAS_dataType) * m * k);
            inFile.close();
        } else {
            cerr << "Error opening " << (data_dir + "matA_in_" + to_string(m) + "_" + to_string(k) + ".bin") << endl;
            exit(1);
        }

        inFile.open(data_dir + "matB_in_" + to_string(k) + "_" + to_string(n) + ".bin", ifstream::binary);
        if (inFile.is_open()) {
            inFile.read((char*)b[kernelIndex], sizeof(XFBLAS_dataType) * k * n);
            inFile.close();
        } else {
            cerr << "Error opening " << (data_dir + "matB_in_" + to_string(k) + "_" + to_string(n) + ".bin") << endl;
            exit(1);
        }

        inFile.open(data_dir + "matC_in_" + to_string(m) + "_" + to_string(n) + ".bin", ifstream::binary);
        if (inFile.is_open()) {
            inFile.read((char*)c[kernelIndex], sizeof(XFBLAS_dataType) * m * n);
            inFile.close();
        } else {
            cerr << "Error opening " << (data_dir + "matC_in_" + to_string(m) + "_" + to_string(n) + ".bin") << endl;
            exit(1);
        }

        inFile.open(data_dir + "matC_out_" + to_string(m) + "_" + to_string(n) + ".bin", ifstream::binary);
        if (inFile.is_open()) {
            inFile.read((char*)goldenC[kernelIndex], sizeof(XFBLAS_dataType) * m * n);
            inFile.close();
        } else {
            cerr << "Error opening " << (data_dir + "matC_out_" + to_string(m) + "_" + to_string(n) + ".bin") << endl;
            exit(1);
        }
    }

    TimePointType l_tp_start_time;
    TimePointType l_tp_create_time;
    l_tp_start_time = chrono::high_resolution_clock::now();

    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status =
        xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);

    showTimeData("xfblasCreate", l_tp_start_time, l_tp_create_time);

    xfblasDestroy(l_numKernel);

    TimePointType l_tp_loop[3];
    chrono::duration<double> l_timeApiSum;

    for (int i = 0; i < iteration; i++) {
        xfblasStatus_t status =
            xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);
        unsigned int l_tpIdx = 0;
        l_tp_loop[l_tpIdx] = chrono::high_resolution_clock::now();
        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            status = xfblasMallocRestricted(m, k, sizeof(*a[kernelIndex]), a[kernelIndex], k, kernelIndex);
            status = xfblasMallocRestricted(k, n, sizeof(*b[kernelIndex]), b[kernelIndex], n, kernelIndex);
            status = xfblasMallocRestricted(m, n, sizeof(*c[kernelIndex]), c[kernelIndex], n, kernelIndex);
#ifdef XFBLAS_LAUNCH_ASYNC
            xfblasSetMatrixRestrictedAsync(a[kernelIndex], kernelIndex);
            xfblasSetMatrixRestrictedAsync(b[kernelIndex], kernelIndex);
            xfblasSetMatrixRestrictedAsync(c[kernelIndex], kernelIndex);
#else
            status = xfblasSetMatrixRestricted(a[kernelIndex], kernelIndex);
            status = xfblasSetMatrixRestricted(b[kernelIndex], kernelIndex);
            status = xfblasSetMatrixRestricted(c[kernelIndex], kernelIndex);
#endif
        }
#ifdef XFBLAS_LAUNCH_ASYNC
        xfblasKernelSynchronize();
#endif
        showTimeData("copyToFpga", l_tp_loop[l_tpIdx], l_tp_loop[l_tpIdx + 1]);
        l_tpIdx++;

        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            status = xfblasGemm(XFBLAS_OP_N, XFBLAS_OP_N, m, n, k, 1, a[kernelIndex], k, b[kernelIndex], n, 1,
                                c[kernelIndex], n, kernelIndex);
        }

        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
#ifdef XFBLAS_LAUNCH_ASYNC
            xfblasGetMatrixRestrictedAsync(c[kernelIndex], kernelIndex);
#else
            status = xfblasGetMatrixRestricted(c[kernelIndex], kernelIndex);
#endif
        }
#ifdef XFBLAS_LAUNCH_ASYNC
        xfblasKernelSynchronize();
#endif

        showTimeData("copyFromFpga", l_tp_loop[l_tpIdx], l_tp_loop[l_tpIdx + 1]);
        l_tpIdx++;

        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            xfblasFree(a[kernelIndex], kernelIndex);
            xfblasFree(b[kernelIndex], kernelIndex);
            xfblasFree(c[kernelIndex], kernelIndex);
        }
        xfblasDestroy(l_numKernel);
        chrono::duration<double> l_timeApiLoop = l_tp_loop[l_tpIdx] - l_tp_loop[0];
        l_timeApiSum = l_timeApiSum + l_timeApiLoop;

        if (i != iteration - 1) {
            for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
                inFile.open(data_dir + "matC_in_" + to_string(m) + "_" + to_string(n) + ".bin", ifstream::binary);
                if (inFile.is_open()) {
                    inFile.read((char*)c[kernelIndex], sizeof(XFBLAS_dataType) * m * n);
                    inFile.close();
                }
            }
        }
    }

    chrono::duration<double> l_timeApi = l_timeApiSum / iteration;
    double l_timeMs = l_timeApi.count() * 1e3;

    cout << "Api time is " << fixed << setprecision(6) << l_timeMs << " msec\n";

    unordered_map<string, string> l_configDict;

    readConfigDict(l_configFile, &l_configDict);

    float l_freq = getBoardFreqMHz(l_xclbinFile);
    int GEMX_ddrWidth = stoi(l_configDict["GEMX_ddrWidth"]);
    unsigned long int l_Ops = l_numKernel * (2ull * m * k * n + m * n * 3);
    unsigned long int l_Parallel_Ops = 2ull * m * k * n;

    double l_perfApiInTops = l_Ops / (l_timeMs * 1e-3) / 1e12;
    double l_timeMsAt100pctEff = l_Parallel_Ops / 2 / GEMX_ddrWidth / GEMX_ddrWidth / (l_freq * 1e6) * 1e3;
    double l_effApiPct = 100 * l_timeMsAt100pctEff / l_timeMs;

    cout << std::string("DATA_CSV:,Freq,M,K,N,") + "TimeApiMs," + "EffApiPct,PerfApiTops\n";
    cout << "DATA_CSV:," << l_freq << "," << m << "," << k << "," << n << "," << l_timeMs << "," << l_effApiPct << ","
         << l_perfApiInTops << "\n";

    for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
        cout << ">> Kernel #" << kernelIndex << " << ";
        if (compareGemm(c[kernelIndex], goldenC[kernelIndex], m, n)) {
            cout << "Test passed!\n";
        } else {
            cout << "Test failed!\n";
        }
        for (i = 0; i < 10; i++) {
            for (j = 0; j < 10; j++) {
                cout << (c[kernelIndex][IDX2R(i, j, k)]) << " ";
            }
            cout << "\n";
        }
    }
    for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
        free(a[kernelIndex]);
        free(b[kernelIndex]);
        free(c[kernelIndex]);
    }
    free(a);
    free(b);
    free(c);

    // xfblasDestroy(l_numKernel);

    return EXIT_SUCCESS;
}
