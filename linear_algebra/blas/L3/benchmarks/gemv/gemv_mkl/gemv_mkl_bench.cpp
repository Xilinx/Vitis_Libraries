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

#include <cstdlib>
#include "gemv_mkl_helper.hpp"

#define LOOP 4
using namespace std;

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: gemv_mkl m n\n");
        return EXIT_FAILURE;
    }

    int m = atoi(argv[1]), n = atoi(argv[2]);
    XFBLAS_dataType *a, *x, *y, alpha = 1., beta = 1.;

    a = createMat(m, n);
    x = createMat(n, 1);
    y = createMat(m, 1);

    TimePointType l_tp[3];

    // Cold Start
    l_tp[0] = chrono::high_resolution_clock::now();
    GEMV_MKL(m, n, alpha, beta, a, x, y);
    l_tp[1] = chrono::high_resolution_clock::now();

    // Hot benchmarking
    for (int i = 0; i < LOOP; i++) GEMV_MKL(m, n, alpha, beta, a, x, y);
    l_tp[2] = chrono::high_resolution_clock::now();

    chrono::duration<double> l_durationSec_cold = l_tp[1] - l_tp[0];
    chrono::duration<double> l_durationSec_bench = l_tp[2] - l_tp[1];

    double flops = 2. * (double)m * (double)n;

    cout << std::string("DATA_CSV:,Type,Thread,Func,M,N,") + "TimeApiMs," + "EffApiPct,PerfApiTops\n";
    cout << "DATA_CSV:,"
         << "Cold Start," << getenv("OMP_NUM_THREADS") << "," << DISPLAY_GEMV_FUNC << "," << m << "," << n << ","
         << ((double)l_durationSec_cold.count() * 1e3) << ","
         << "N/A," << (flops / (double)l_durationSec_cold.count() * 1.e-12) << endl;
    cout << "DATA_CSV:,"
         << "Benchmark," << getenv("OMP_NUM_THREADS") << "," << DISPLAY_GEMV_FUNC << "," << m << "," << n << ","
         << ((double)l_durationSec_bench.count() / (double)LOOP * 1e3) << ","
         << "N/A," << (flops / (double)l_durationSec_bench.count() * (double)LOOP * 1.e-12) << endl;

    free(a);
    free(x);
    free(y);

    return 0;
}
