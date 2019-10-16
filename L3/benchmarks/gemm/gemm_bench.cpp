
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
#include "../bench_helper.hpp"
#include "gemm_helper.hpp"

using namespace std;

void readBin(char* mat, unsigned int row, unsigned int col, string dataDir, string name, unsigned int eleSize) {
    ifstream inFile;
    inFile.open(dataDir + name + to_string(row) + "_" + to_string(col) + ".bin", ifstream::binary);
    if (inFile.is_open()) {
        inFile.read((char*)mat, eleSize * row * col);
        inFile.close();
    } else {
        cerr << "Could not find " << (dataDir + name + to_string(row) + "_" + to_string(col) + ".bin") << endl;
        exit(1);
    }
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

    int iteration = 5;
    if (argc >= 9) {
        iteration = stoi(argv[l_argIdx++]);
        cout << "Read custom iteration: " << iteration << endl;
    }

    int i, j; // i-row l_numKernel -1 ,j- column l_numKernel -1

    vector<XFBLAS_dataType*> goldenC;

    for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
        XFBLAS_dataType* tmp_goldenC;
        posix_memalign((void**)&tmp_goldenC, 4096, m * n * sizeof(XFBLAS_dataType));
        readBin((char*)tmp_goldenC, m, n, data_dir, "matC_out_", sizeof(XFBLAS_dataType));
        goldenC.push_back(tmp_goldenC);
    }

    TimePointType l_tp_start_time;
    TimePointType l_tp_create_time;
    l_tp_start_time = chrono::high_resolution_clock::now();
    xfblasEngine_t engineName = XFBLAS_ENGINE_GEMM;
    xfblasStatus_t status =
        xfblasCreate(l_xclbinFile.c_str(), l_configFile, l_logFile.c_str(), engineName, l_numKernel);

    showTimeData("xfblasCreate", l_tp_start_time, l_tp_create_time);

    vector<XFBLAS_dataType*> resultC;

    TimePointType l_tp_loop[3];
    chrono::duration<double> l_timeApiSum = chrono::seconds(0);

    for (int i = 0; i < iteration; i++) {
        vector<XFBLAS_dataType *> a, b, c;

        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            XFBLAS_dataType *tmp_a, *tmp_b, *tmp_c;
            posix_memalign((void**)&tmp_a, 4096, m * k * sizeof(XFBLAS_dataType));
            memset(tmp_a, 0, m * k);
            posix_memalign((void**)&tmp_b, 4096, k * n * sizeof(XFBLAS_dataType));
            memset(tmp_b, 0, k * n);
            posix_memalign((void**)&tmp_c, 4096, m * n * sizeof(XFBLAS_dataType));
            memset(tmp_c, 0, m * n);
            readBin((char*)tmp_a, m, k, data_dir, "matA_in_", sizeof(XFBLAS_dataType));
            readBin((char*)tmp_b, k, n, data_dir, "matB_in_", sizeof(XFBLAS_dataType));
            readBin((char*)tmp_c, m, n, data_dir, "matC_in_", sizeof(XFBLAS_dataType));
            a.push_back(tmp_a);
            b.push_back(tmp_b);
            c.push_back(tmp_c);
        }

        unsigned int l_tpIdx = 0;
        l_tp_loop[l_tpIdx] = chrono::high_resolution_clock::now();
        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            status = xfblasMallocRestricted(m, k, sizeof(XFBLAS_dataType), a[kernelIndex], k, kernelIndex);
            status = xfblasMallocRestricted(k, n, sizeof(XFBLAS_dataType), b[kernelIndex], n, kernelIndex);
            status = xfblasMallocRestricted(m, n, sizeof(XFBLAS_dataType), c[kernelIndex], n, kernelIndex);
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

        if (i == iteration - 1) {
            for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
                XFBLAS_dataType* tmp_c;
                posix_memalign((void**)&tmp_c, 4096, m * n * sizeof(XFBLAS_dataType));
                memcpy(tmp_c, c[kernelIndex], m * n * sizeof(XFBLAS_dataType));
                resultC.push_back(tmp_c);
            }
        }

        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            xfblasFree(a[kernelIndex], kernelIndex);
            xfblasFree(b[kernelIndex], kernelIndex);
            xfblasFree(c[kernelIndex], kernelIndex);
        }
        xfblasFreeInstr();
        chrono::duration<double> l_timeApiLoop = l_tp_loop[l_tpIdx] - l_tp_loop[0];
        l_timeApiSum = l_timeApiSum + l_timeApiLoop;

        for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
            free(a[kernelIndex]);
            free(b[kernelIndex]);
            free(c[kernelIndex]);
        }
        a.clear();
        b.clear();
        c.clear();
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
        if (compareGemm(resultC[kernelIndex], goldenC[kernelIndex], m, n)) {
            cout << "Test passed!\n";
        } else {
            cout << "Test failed!\n";
        }
        for (i = 0; i < 10; i++) {
            for (j = 0; j < 10; j++) {
                cout << (resultC[kernelIndex][IDX2R(i, j, k)]) << " ";
            }
            cout << "\n";
        }
    }

    for (int kernelIndex = 0; kernelIndex < l_numKernel; kernelIndex++) {
        free(resultC[kernelIndex]);
        free(goldenC[kernelIndex]);
    }

    xfblasDestroy(l_numKernel);

    return EXIT_SUCCESS;
}
