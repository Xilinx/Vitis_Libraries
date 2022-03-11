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
#ifndef PAIRHMM_HPP_
#define PAIRHMM_HPP_
//#include <CL/opencl.h>
//#include <CL/cl_ext.h>
//#include "avx_impl.h"
//#include "baseline_impl.h"

#include "Context.h"
#include "host_type.h"
#define KERNEL_NUM 1

#define SLR0_PE_NUM 64
#define SLR1_PE_NUM 64
#define SLR2_PE_NUM 64
#define TOTAL_PE_NUM (SLR0_PE_NUM + SLR1_PE_NUM + SLR2_PE_NUM)
#define DIE_NUM 1
using namespace std;

class xilPairHMM {
   public:
    xilPairHMM();
    xilPairHMM(char* bitstream, bool seq = false);
    void computePairhmm(pairhmmInput* input, pairhmmOutput* output, bool& usedFPGA);
    // int computePairhmmBaseline(pairhmmInput* input, pairhmmOutput* output, bool use_double);
    // int computePairhmmAVX(pairhmmInput* input, pairhmmOutput* output, bool use_double);
    // void computePairhmmAVXSegment(pairhmmInput* input, int read_base_index, int hap_base_index, int cur_numRead, int
    // cur_numHap, vector<float>& output);
    int computePairhmmxil(pairhmmInput* input, pairhmmOutput* output, bool& usedFPGA);
    double get_kernel_time();
    ~xilPairHMM();

   private:
    bool is_seq(void) { return m_seq; }
    bool init_FPGA(char* bitstream);
    bool init_FPGA_buffer();
    bool destroy_FPGA_buffer();
    int get_max_rsdata_num();
    void convert_read_input(readDataPack* cur_host_read, Read* input, int idx);
    void distributeReads(pairhmmInput* input,
                         int read_base_index,
                         int hap_base_index,
                         int& numRead0,
                         int& numRead1,
                         int& numRead2,
                         int& totalNumRead,
                         int& totalNumHap,
                         short maxCols,
                         bool& violate);
    void sortReads(pairhmmInput* input,
                   int read_base_index,
                   int cur_numRead,
                   int cur_numHap,
                   short maxCols,
                   int slr_pu_num[3],
                   bool& violate);
    void update_host_inputs(
        pairhmmInput* input, int read_base_index, int hap_base_index, int cur_numRead, int cur_numHap, bool& violate);
    void update_host_inputs_new(
        pairhmmInput* input, int read_base_index, int hap_base_index, int cur_numRead, int cur_numHap, bool& violate);
    bool useFPGA;
    FPGAInput* host_input[3];
    float* host_output[3];
    vector<float> host_raw_output;
    Context<float> g_ctxf;
    Context<double> g_ctxd;
    bool m_seq;
    float* m2m_table;
    double data_prepare_time;
    double cl_write_time;
    double kernel_time;
    double peak_kernel_gcups;
    double curNumCell;
    double cl_read_time;
    double recompute_time;
};
double countCell(pairhmmInput* input, short maxCols);
bool worthFPGA(pairhmmInput* input, short maxCols, double cellNum);
int printM2MPROB();
struct timespec diff_time(struct timespec start, struct timespec end);

struct readInfo {
    uint8_t readLen[READ_BLOCK_SIZE];
    uint8_t big_rows;
    uint8_t new_rows;
    bool oneOrTwo;
    int resultOffset;
    int curIterNum;
    short readID[READ_BLOCK_SIZE];
    uint64_t infoPacked;
};

#endif
