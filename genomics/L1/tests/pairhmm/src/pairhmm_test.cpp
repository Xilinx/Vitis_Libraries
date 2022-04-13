/*
 * Copyright 2022 Xilinx, Inc.
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

#include "hls_stream.h"
#include <ap_int.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <math.h>
#include <cmath>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include "phmmmodules.hpp"
#include "pairHmm.hpp"
#include "gensynthdata.hpp"
#include "pairHmm.cpp"

void pairhmmComputeEngine(ap_uint<GMEM_DWIDTH>* input_data, float* output_data, int numRead, int numHap) {
    int iterNum = numHap * numRead;
#pragma HLS INTERFACE m_axi port = input_data offset = slave bundle = gmem depth = 50331
#pragma HLS INTERFACE m_axi port = output_data offset = slave bundle = gmem depth = 262144
#pragma HLS INTERFACE s_axilite port = input_data bundle = control
#pragma HLS INTERFACE s_axilite port = output_data bundle = control
#pragma HLS INTERFACE s_axilite port = numRead bundle = control
#pragma HLS INTERFACE s_axilite port = numHap bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    // Step-3: Call computePairhmm API
    xf::genomics::computePairhmm<PE_COUNT>(input_data, numRead, numHap, iterNum, output_data);
}

int xilPairHMM::computePairhmmxil(pairhmmInput* input, pairhmmOutput* output, bool& usedFPGA) {
    short maxCols = 0;

    for (int i = 0; (size_t)i < input->haps.size(); i++) {
        short hapLenTmp = input->haps[i].bases.size();
        if (hapLenTmp + 1 > maxCols) {
            maxCols = hapLenTmp + 1;
        }
    }
#if 0    
    if(!worthFPGA(input, maxCols, 0)){
        printf("Using AVX\n");
        usedFPGA = false;
        computePairhmmAVX(input, output, false);
        return 0;
    }
#endif
    output->likelihoodData.clear();

    printf("Trying to use FPGA, numRead = %ld, numHap = %ld\n", input->reads.size(), input->haps.size());

    int singleFP_violate_count = 0;
    int numRead = input->reads.size();
    int numHap = input->haps.size();
    host_raw_output.resize(numRead * numHap);
    int max_rsdata_num = MAX_RSDATA_NUM;
    int max_hapdata_num = MAX_HAPDATA_NUM;
    int numReadSeg = numRead / max_rsdata_num - (numRead % max_rsdata_num == 0) + 1;
    int numHapSeg = numHap / max_hapdata_num - (numHap % max_hapdata_num == 0) + 1;
    bool violate = false;
    long long base = 0;
    int read_base_index = 0;
    int hap_base_index = 0;

    for (int i = 0; i < numReadSeg; i++) {
        int cur_numRead = i + 1 < numReadSeg ? max_rsdata_num : numRead - i * max_rsdata_num;
        hap_base_index = 0;
        for (int j = 0; j < numHapSeg; j++) {
            base = read_base_index * numHap + hap_base_index;
            int cur_numHap = j + 1 < numHapSeg ? max_hapdata_num : numHap - j * max_hapdata_num;
            update_host_inputs_new(input, read_base_index, hap_base_index, cur_numRead, cur_numHap, violate);
            // if(violate){
            //  computePairhmmAVXSegment(input, read_base_index, hap_base_index, cur_numRead, cur_numHap,
            //  host_raw_output);
            //} else{
            usedFPGA = true;
            for (int k = 0; k < 1; k++) {
                pairhmmComputeEngine((ap_uint<GMEM_DWIDTH>*)&(host_input[0]->dataPack), host_output[0],
                                     host_input[0]->numRead, host_input[0]->numHap);
                printf("cur_numRead:%d\t cur_numHap:%d\t host_read:%d\n", cur_numRead, cur_numHap,
                       host_input[0]->numRead);
            }
            for (int k = 0; k < cur_numHap * cur_numRead; k++) {
                float cur_float_result;
                if (k < host_input[0]->numRead * cur_numHap)
                    cur_float_result = host_output[0][k];
                else if (k - host_input[0]->numRead * cur_numHap < host_input[1]->numRead * cur_numHap)
                    cur_float_result = host_output[1][k - host_input[0]->numRead * cur_numHap];
                else
                    cur_float_result =
                        host_output[2][k - host_input[0]->numRead * cur_numHap - host_input[1]->numRead * cur_numHap];
                host_raw_output[base + (k / cur_numHap) * numHap + (k % cur_numHap)] = cur_float_result;
            }
            //}
            hap_base_index += cur_numHap;
        }
        read_base_index += cur_numRead;
    }

    for (int k = 0; k < numRead * numHap; k++) {
#if 0
        if(host_raw_output[k] < MIN_ACCEPTED){
            singleFP_violate_count++;
            testcase testCase;
            int ii = k / numHap;
            int jj = k % numHap;
            testCase.rslen = input->reads[ii].bases.size();
            testCase.rs = input->reads[ii].bases.c_str();
            testCase.i = input->reads[ii]._i.c_str();
            testCase.d = input->reads[ii]._d.c_str();
            testCase.q = input->reads[ii]._q.c_str();
            testCase.c = input->reads[ii]._c.c_str();
            testCase.haplen = input->haps[jj].bases.size();
            testCase.hap = input->haps[jj].bases.c_str();
            double result_double = compute_fp_avxd(&testCase);
            output->likelihoodData.push_back(log10(result_double) - g_ctxd.LOG10_INITIAL_CONSTANT);  
        } else {
#endif
        output->likelihoodData.push_back(log10(host_raw_output[k]) - g_ctxf.LOG10_INITIAL_CONSTANT);
        //  }
    }

    printf("This FPGA run is done, %d of %d single FP is overlowed and recomputed using AVX double precsion\n",
           singleFP_violate_count, numRead * numHap);
    return 0;
}

int main(int argc, char* argv[]) {
    // Step-1: Generate Input/Golden Output using
    // L2 level modules
    int totalTestNum = 4;

    pairhmmInput* input;
    pairhmmOutput* golden_output;
    pairhmmOutput* target_output;
    input = new pairhmmInput();
    golden_output = new pairhmmOutput();
    target_output = new pairhmmOutput();
    xilPairHMM* accelPhmm = new xilPairHMM();

    for (int i = 0; i < totalTestNum; ++i) {
        GenInputs(input, i);
        // GenOutputs(input, golden_output);

        bool usedFPGA = true;
        double current_cells = countCells(input);
        double error_count = 0;
        double largest_error = 0;

        accelPhmm->computePairhmmxil(input, target_output, usedFPGA);

        input->reads.clear();
        input->haps.clear();
        golden_output->likelihoodData.clear();
        target_output->likelihoodData.clear();
    }

    delete accelPhmm;

    delete input;
    delete golden_output;
    delete target_output;

    return 0;
}
