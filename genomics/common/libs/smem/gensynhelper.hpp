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
#pragma once
#include "baseline.hpp"
#include "ocl.hpp"
#include <malloc.h>
#include <stdio.h>
#include <dirent.h>
#include <cstdlib>
#include <ctime>
#include <fstream>

#define BWT_SIZE 775451201

using namespace xf::genomics;
uint64_t time_total = 0;
bool first_time = true;

unsigned char nst_nt4_table[256] = {
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5 /*'-'*/, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,         4, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

timespec diff(timespec& start, timespec& end) {
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

timespec tic() {
    timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    return start_time;
}

double toc(timespec& start_time) {
    timespec current_time, diff_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    diff_time = diff(start_time, current_time);
    return diff_time.tv_sec * 1e9 + diff_time.tv_nsec;
}

uint8_t genRandBase() {
    int randNum = std::rand() % 102;
    uint8_t ret;
    if (randNum < 25)
        ret = 0;
    else if (randNum < 50)
        ret = 1;
    else if (randNum < 75)
        ret = 2;
    else if (randNum < 100)
        ret = 3;
    else if (randNum < 101)
        ret = 4;
    else
        ret = 5;
    return ret;
}

void genRandSeq(uint8_t* seq, uint8_t* seq_len) {
    // generate a random short read, assume seq is already malloced
    *seq_len = std::rand() % (c_seq_length - 1) + 1;
    for (int i = 0; i < *seq_len; ++i) {
        seq[i] = genRandBase();
    }
}

void genBatch(uint8_t** seq, uint8_t** seq_len, int batch_size) {
    for (int i = 0; i < batch_size; i++) {
        genRandSeq(((*seq) + i * c_seq_length), (*seq_len) + i);
    }
}

void getBatch(char* filename, uint8_t* seq, uint8_t* seq_len, int batch_size) {
    int test_count = 0;
    FILE* finput = fopen(filename, "r");
    if (finput == NULL) {
        printf("failed to open file %s\n", filename);
        exit(-1);
    }
    printf("reading file %s with batch_size %d\n", filename, batch_size);
    int read_count = 0;
    while (read_count < batch_size) {
        char curRead[256];
        fgets(curRead, c_seq_length + 1, finput);
        int len;
        len = strlen(curRead) - 1;
        // printf("seq %d in batch %d has len %d\n", read_count, test_count, len);
        *(seq_len + read_count) = len;
        for (int i = 0; i < len; i++) {
            *(seq + read_count * c_seq_length + i) = nst_nt4_table[curRead[i]];
        }
        read_count++;
    }
    fclose(finput);
}

void swap(bwtintv_t* a, bwtintv_t* b) {
    bwtintv_t tmp = *a;
    *a = *b;
    *b = tmp;
}

bool cmp_mem(bwtintv_t mem, bwtintv_t pivot) {
    if (mem.x[0] < pivot.x[0]) {
        return true;
    } else if (mem.x[0] == pivot.x[0]) {
        if (mem.x[1] < pivot.x[1])
            return true;
        else if (mem.x[1] == pivot.x[1]) {
            if (mem.x[2] < pivot.x[2])
                return true;
            else if (mem.x[2] == pivot.x[2]) {
                if (mem.info <= pivot.info) {
                    return true;
                } else
                    return false;
            } else
                return false;
        } else
            return false;
    } else
        return false;
}

int partition(bwtintv_t* mem, int low, int high) {
    bwtintv_t pivot = mem[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (cmp_mem(mem[j], pivot)) {
            i++;
            swap(&mem[i], &mem[j]);
        }
    }
    swap(&mem[i + 1], &mem[high]);
    return i + 1;
}

void sort_mem(bwtintv_t* mem, int start, int end) {
    if (start < end) {
        int pivot = partition(mem, start, end);
        sort_mem(mem, start, pivot - 1);
        sort_mem(mem, pivot + 1, end);
    }
}

void print_mem(bwtintv_t* mem, int mem_n) {
    for (int i = 0; i < mem_n; i++) {
        printf("mem[%d].info = %lx, mem[%d].x2 = %lx\n", i, mem[i].info, i, mem[i].x[2]);
    }
}

void cmp(bwtintv_t* mem,
         int* mem_n,
         bwtintv_t* mem_golden,
         int* mem_n_golden,
         int batch_size,
         int test_id,
         double& overflow_count,
         double& total_count) {
    bool error = false;
    for (int i = 0; i < batch_size; ++i) {
        total_count = total_count + 1;
        if (mem_n[i] > c_max_intv_alloc) {
            printf(
                "This batch id %d of test id %d needs to be rerun on CPU, because mem_n %d exceeds the MAX_INT_ALLOC "
                "%d\n",
                i, test_id, mem_n[i], c_max_intv_alloc);
            overflow_count = overflow_count + 1;
            return;
        }
        if (mem_n[i] != mem_n_golden[i]) {
            printf("Error: in test id %d, batch %d, intv number is %d while golden is %d\n", test_id, i, mem_n[i],
                   mem_n_golden[i]);
            error = true;
            printf("golden mem are\n");
            print_mem(mem_golden + i * c_max_intv_alloc, mem_n_golden[i]);
            printf("target mem are\n");
            print_mem(mem + i * c_max_intv_alloc, mem_n[i]);
            fflush(stdout);
            exit(-1);
        }
        int cur_mem_n = mem_n[i];
        sort_mem(mem, i * c_max_intv_alloc, i * c_max_intv_alloc + cur_mem_n - 1);
        sort_mem(mem_golden, i * c_max_intv_alloc, i * c_max_intv_alloc + cur_mem_n - 1);
        for (int j = 0; j < cur_mem_n; j++) {
            if (mem_golden[i * c_max_intv_alloc + j].x[0] != mem[i * c_max_intv_alloc + j].x[0]) {
                printf("Error: in test id %d, batch %d, intv %d in total %d intvs, x0 is %lx while golden is %lx\n",
                       test_id, i, j, cur_mem_n, mem[i * c_max_intv_alloc + j].x[0],
                       mem_golden[i * c_max_intv_alloc + j].x[0]);
                error = true;
                exit(-1);
            }
            if (mem_golden[i * c_max_intv_alloc + j].x[1] != mem[i * c_max_intv_alloc + j].x[1]) {
                printf("Error: in test id %d, batch %d, intv %d, x1 is %lx while golden is %lx\n", test_id, i, j,
                       mem[i * c_max_intv_alloc + j].x[1], mem_golden[i * c_max_intv_alloc + j].x[1]);
                error = true;
                exit(-1);
            }
            if (mem_golden[i * c_max_intv_alloc + j].x[2] != mem[i * c_max_intv_alloc + j].x[2]) {
                printf("Error: in test id %d, batch %d, intv %d, x2 is %lx while golden is %lx\n", test_id, i, j,
                       mem[i * c_max_intv_alloc + j].x[2], mem_golden[i * c_max_intv_alloc + j].x[2]);
                error = true;
                exit(-1);
            }
            if (mem_golden[i * c_max_intv_alloc + j].info != mem[i * c_max_intv_alloc + j].info) {
                printf("Error: in test id %d, batch %d, intv %d, info is %lx while golden is %lx\n", test_id, i, j,
                       mem[i * c_max_intv_alloc + j].info, mem_golden[i * c_max_intv_alloc + j].info);
                error = true;
                exit(-1);
            }
        }
    }
    if (error) {
        printf("error in test %d, exiting\n", test_id);
        fflush(stdout);
        exit(-1);
    } else
        printf("test id %d passed \n", test_id);
}

void randomTest(char* btsm, char* dirname, int test_num, int batch_size) {
    uint8_t* seq;
    uint8_t* seq_len;

    if (!(seq = (uint8_t*)malloc(BATCH_SIZE * c_seq_length * sizeof(uint8_t)))) {
        printf("Fail to alloc batch sequence\n");
        exit(-1);
    }

    if (!(seq_len = (uint8_t*)malloc(BATCH_SIZE * sizeof(uint8_t)))) {
        printf("Failed to alloc seq length \n");
        exit(-1);
    }
    bwtintv_t* mem_output;
    int* mem_num;
    if (!(mem_output = (bwtintv_t*)malloc(BATCH_SIZE * c_max_intv_alloc * sizeof(bwtintv_t)))) {
        printf("Failed to alloc mem_output\n");
        exit(-1);
    }
    if (!(mem_num = (int*)malloc(BATCH_SIZE * sizeof(int)))) {
        printf("Failed to alloc mem_num\n");
        exit(-1);
    }

    bwtintv_t* mem_output_golden;
    int* mem_num_golden;
    if (!(mem_output_golden = (bwtintv_t*)malloc(BATCH_SIZE * c_max_intv_alloc * sizeof(bwtintv_t)))) {
        printf("Failed to alloc mem_output_golden\n");
        exit(-1);
    }
    if (!(mem_num_golden = (int*)malloc(BATCH_SIZE * sizeof(int)))) {
        printf("Failed to alloc mem_num_golden\n");
        exit(-1);
    }
    printf("Start doing init FPGA platform\n");
    timespec ocl_init_start = tic();

    std::ifstream fin;
    fin.open("bwt_struct.file");
    std::string line;
    std::vector<uint32_t> bwt_input;
    std::vector<uint64_t> bwt_para_input;
    uint64_t bwt_size_input = 0;
    getline(fin, line);
    bwt_size_input = atoi(line.c_str());
    for (int i = 0; i < bwt_size_input; i++) {
        getline(fin, line);
        bwt_input.push_back(atoi(line.c_str()));
    }
    for (int i = 0; i < 7; i++) {
        getline(fin, line);
        bwt_para_input.push_back(atoi(line.c_str()));
    }
    fin.close();

    xilSmem* oclObj = new xilSmem(bwt_input.data(), bwt_para_input.data(), bwt_size_input, batch_size, btsm);
    oclObj->ocl_init();

    double ocl_init_elap = toc(ocl_init_start);
    printf("Done init FPGA platform, using %.2f sec\n", ocl_init_elap * 1e-9);
    fflush(stdout);
    double overflow_count = 0;
    double total_count = 0;
    double peak_cpu_bandwidth = 0;
    double peak_fpga_bandwidth = 0;
    double peak_fpga_bandwidth_w_openCL_overhead = 0;
    double total_cpu_mem_request_size = 0;
    double total_cpu_elapse = 0;
    double total_fpga_mem_request_size[COMPUTE_UNIT];
    double total_fpga_elapse;
    double total_read_num[COMPUTE_UNIT];
    double total_kernel_elapse[COMPUTE_UNIT];
    char filename[200];
    DIR* d = opendir(dirname);
    struct dirent* dir;

    int test_count = 0;
    for (int i = 0; i < COMPUTE_UNIT; i++) {
        total_fpga_mem_request_size[i] = 0;
        total_kernel_elapse[i] = 0;
        total_read_num[i] = 0;
    }
    if (d) {
        while ((dir = readdir(d)) != NULL && test_count < test_num) {
            if (dir->d_name[0] == '.') {
                continue;
            }
            sprintf(filename, "%s%s", dirname, dir->d_name);
            getBatch(filename, seq, seq_len, batch_size);
            double mem_request_size[COMPUTE_UNIT];
            timespec baseline_start = tic();

            // CPU Baseline implementation
            smem_baseline(bwt_input.data(), bwt_para_input.data(), seq, seq_len, batch_size, mem_output_golden,
                          mem_num_golden, mem_request_size);

            double baseline_elap = toc(baseline_start);
            double mem_request_size_sum = 0;

            for (int ii = 0; ii < COMPUTE_UNIT; ii++) {
                mem_request_size_sum += mem_request_size[ii];
            }
            printf(
                "Done baseline test %d, request DRAM size %e bytes, elapse time is %e ns, bandwidth is %lf GBps, "
                "thruput is %e reads per sec\n",
                test_count, mem_request_size_sum, baseline_elap, mem_request_size_sum / baseline_elap,
                batch_size / baseline_elap);

            if (mem_request_size_sum / baseline_elap > peak_cpu_bandwidth) {
                peak_cpu_bandwidth = mem_request_size_sum / baseline_elap;
            }

            total_cpu_mem_request_size += mem_request_size_sum;
            total_cpu_elapse += baseline_elap;
            double kernel_time[COMPUTE_UNIT];
            timespec fpga_start = tic();

            // FPGA Implementation
            oclObj->smem_ocl(seq, seq_len, mem_output, mem_num, kernel_time);

            double fpga_elap = toc(fpga_start);
            double kernel_time_sum = 0;
            double cur_bandwidth[COMPUTE_UNIT];
            double cur_thruput[COMPUTE_UNIT];
            double agg_bandwidth = 0;
            double agg_thruput = 0;

            for (int ii = 0; ii < COMPUTE_UNIT; ii++) {
                total_fpga_mem_request_size[ii] += mem_request_size[ii];
                total_kernel_elapse[ii] += kernel_time[ii];
                total_read_num[ii] += batch_size / COMPUTE_UNIT;
                cur_bandwidth[ii] = mem_request_size[ii] / kernel_time[ii];
                cur_thruput[ii] = batch_size / COMPUTE_UNIT / kernel_time[ii];
                if (cur_bandwidth[ii] > peak_fpga_bandwidth) {
                    peak_fpga_bandwidth = cur_bandwidth[ii];
                }
                agg_bandwidth += cur_bandwidth[ii];
                agg_thruput += cur_thruput[ii];
            }
            printf(
                "Done kernel test %d, request DRAM size %e bytes, elapse time is %e ns, overall bandwidth is %lf "
                "GBps\n",
                test_count, mem_request_size_sum, fpga_elap, mem_request_size_sum / fpga_elap);

            printf("Agg bandwidth is %lf GBps, ", agg_bandwidth);
            printf("Agg thruput is %e read/s, ", agg_thruput * 1e9);
            for (int idx = 0; idx < COMPUTE_UNIT; idx++) {
#if COMPUTE_UNIT == 4
                int bank_id = idx;
#else
                int bank_id = idx < 1 ? idx : idx + 1;
#endif
                printf("Avg bank%d bandwidth is %f GBps, ", bank_id, cur_bandwidth[idx]);
                printf("Avg bank%d thruput is %e read/s, ", bank_id, cur_thruput[idx] * 1e9);
            }
            printf("\n");

            total_fpga_elapse += fpga_elap;
            if (mem_request_size_sum / fpga_elap > peak_fpga_bandwidth_w_openCL_overhead) {
                peak_fpga_bandwidth_w_openCL_overhead = mem_request_size_sum / fpga_elap;
            }

            cmp(mem_output, mem_num, mem_output_golden, mem_num_golden, batch_size, test_count, overflow_count,
                total_count);
            fflush(stdout);
            test_count++;
        }
    } else {
        printf("Directory %s does not exist \n", dirname);
        exit(-1);
    }

    printf("%d out of %d test cases passed\n", (int)(total_count - overflow_count), (int)total_count);
    printf("%d out of %d test cases exceed fpga capablity, needs recompute on cpu\n", (int)(overflow_count),
           (int)total_count);
    printf("%d out of %d test cases failed\n", 0, (int)total_count);
    printf("peak cpu bandwidth is %f GBps, avg cpu bandwidth is %f GBps\n", peak_cpu_bandwidth,
           total_cpu_mem_request_size / total_cpu_elapse);
    printf(
        "bandwidth measurements w/ openCL transfer overheads: peak fpga bandwidth is %f GBps, avg fpga bandwidth is %f "
        "GBps\n",
        peak_fpga_bandwidth_w_openCL_overhead, total_cpu_mem_request_size / total_fpga_elapse);
    printf("bandwidth measurments w/o openCL transfer overheads: peak fpga bandwidth is %f GBps\n ",
           peak_fpga_bandwidth * COMPUTE_UNIT);

    for (int idx = 0; idx < COMPUTE_UNIT; idx++) {
#if COMPUTE_UNIT == 4
        int bank_id = idx;
#else
        int bank_id = idx < 1 ? idx : idx + 1;
#endif
        printf("Avg bank%d bandwidth is %f GBps, thruput is %e read/s\n", bank_id,
               total_fpga_mem_request_size[idx] / total_kernel_elapse[idx],
               total_read_num[idx] / total_kernel_elapse[idx] * 1e9);
    }
    free(seq);
    free(seq_len);
    free(mem_output);
    free(mem_num);
    free(mem_output_golden);
    free(mem_num_golden);
}
