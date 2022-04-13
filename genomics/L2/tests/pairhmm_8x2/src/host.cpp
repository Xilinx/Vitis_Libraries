/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#include "pairHmm.hpp"
#include "gensynthdata.hpp"

int main(int argc, char* argv[]) {
    if (argc == 2) {
        if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
            // printVersion();
            return 0;
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printHelp();
            return 0;
        } else {
            printf("Invalid argument list\n");
            printHelp();
            return EXIT_FAILURE;
        }
    }

    if (argc != 4) {
        printf("Invalid argument list\n");
        printHelp();
        return EXIT_FAILURE;
    }

    std::string input_common_prefix;
    std::string output_common_prefix;
    DIR* dir;
    struct dirent* ent;

    bool synthetic = false;
    int test_num = 0;

    if (strcmp(argv[2], "--real") == 0) {
        if ((dir = opendir(argv[3])) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                test_num++;
            }
            closedir(dir);
        } else {
            printf("cannot find dir %s\n", argv[3]);
            return EXIT_FAILURE;
        }
        printf("find %d files in dir %s\n", test_num, argv[3]);

        input_common_prefix = std::string(argv[3]) + std::string("input");
        output_common_prefix = std::string(argv[3]) + std::string("output");
        test_num = (test_num - 2) / 2;
    } else if (strcmp(argv[2], "--syn") == 0) {
        try {
            test_num = xcl::is_emulation() ? 4 : std::stoi(std::string(argv[3]));
        } catch (const std::invalid_argument& ia) {
            std::cout << "Invalid synthetic cases number " << ia.what() << '\n';
            return EXIT_FAILURE;
        }
        synthetic = true;
    } else {
        printHelp();
        return EXIT_FAILURE;
    }

    int totalTestNum = test_num;

    avg_numReads = 0;
    avg_numHaps = 0;
    int total_numHaps = 0;
    case_counter = 0;
    pairhmmInput* input;
    pairhmmOutput* golden_output;
    pairhmmOutput* target_output;
    input = new pairhmmInput();
    golden_output = new pairhmmOutput();
    target_output = new pairhmmOutput();
    struct timespec time1, time2, time_diff;
    double total_avx_count = 0;
    double total_fpga_count = 0;
    double total_avx_time = 0;
    double total_fpga_time = 0;
    double total_kernel_time = 0;
    double cur_time = 0;
    double total_avx_cells = 0;
    double total_fpga_cells = 0;
    double total_fpga_results = 0;
    double total_avx_results = 0;
    double current_cells = 0;
    double error_count = 0;
    double largest_error = 0;
    float peak_GCUPS = 0;

    xilPairHMM* accelPhmm = new xilPairHMM(argv[1], true);

    for (int i = 0; i < totalTestNum; ++i) {
        if (synthetic == false) {
            std::string test_id = std::to_string(i);
            std::string input_filename = input_common_prefix + test_id;
            GetInputs(input, input_filename);
            std::string output_filename = output_common_prefix + test_id;
            GetOutputs(golden_output, input->reads.size() * input->haps.size(), output_filename);
        } else {
            GenInputs(input, i);
            GenOutputs(input, golden_output);
        }

        bool usedFPGA = false;
        current_cells = countCells(input);
        clock_gettime(CLOCK_REALTIME, &time1);
        accelPhmm->computePairhmm(input, target_output, usedFPGA);
        clock_gettime(CLOCK_REALTIME, &time2);
        time_diff = diff_time(time1, time2);
        cur_time = (long)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);
        if (current_cells / cur_time > peak_GCUPS) peak_GCUPS = current_cells / cur_time;
        if (usedFPGA) {
            total_fpga_time += cur_time;
            total_fpga_cells += current_cells;
            total_fpga_results += input->reads.size() * input->haps.size();
            total_fpga_count += 1;
            total_numHaps += input->haps.size();
        } else {
            total_avx_time += cur_time;
            total_avx_cells += current_cells;
            total_avx_results += input->reads.size() * input->haps.size();
            total_avx_count += 1;
        }
        cmp(target_output, golden_output, input->reads.size() * input->haps.size(), i, false, error_count,
            largest_error);
        printf("%d test passed, overall time is %f secs, overall GCUPS is %f, current GCUPS is %f, peak GCUPS is %f\n",
               i, (total_avx_time + total_fpga_time) * (1e-9),
               (total_fpga_cells + total_avx_cells) / (total_fpga_time + total_avx_time), current_cells / cur_time,
               peak_GCUPS);
        input->reads.clear();
        input->haps.clear();
        golden_output->likelihoodData.clear();
        target_output->likelihoodData.clear();
    }
#ifdef FPGA
    total_kernel_time = accelPhmm->get_kernel_time();
    delete accelPhmm;
    printf("%f out of %d tests use FPGA, use %f secs, FPGA GCUPs is %f, avg numHaps = %f\n", total_fpga_count,
           totalTestNum, total_fpga_time * 1e-9, total_fpga_cells / total_fpga_time,
           (float)total_numHaps / (float)total_fpga_count);
    printf("pure kernel GCUPs is %f\n", total_fpga_cells / total_kernel_time);
#endif
    delete input;
    delete golden_output;
    delete target_output;
    printf("%f out of %d tests use AVX, use %f secs, AVX GCUPs is %f\n", total_avx_count, totalTestNum,
           total_avx_time * 1e-9, total_avx_cells / total_avx_time);
    printf(
        "%e out of %e FPGA run results have significant errors, total number of results is %e, error rate is %e, "
        "largest error is %e\n",
        error_count, total_fpga_results, (total_fpga_results + total_avx_results), error_count / total_fpga_results,
        largest_error);
    return 0;
}
