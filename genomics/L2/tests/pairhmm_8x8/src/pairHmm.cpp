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
#include <vector>
#include <string>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "pairHmm.hpp"
#include "m2m.hpp"

uint8_t ConvertChar::conversionTable[255];
bool cmpReadInfo(struct readInfo a, struct readInfo b) {
    return (a.new_rows > b.new_rows);
}

struct timespec diff_time(struct timespec start, struct timespec end) {
    struct timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

/* int xilPairHMM::computePairhmmBaseline(pairhmmInput* input, pairhmmOutput* output, bool use_double) {
    // baseline pairhmm algorithm ported from GATK v3.7 gatk3/public/VectorPairHMM/src/main/c++/baseline.cc

    testcase testCase;
    output->likelihoodData.clear();
    for (int i = 0; (size_t)i < input->reads.size(); i++) {
        testCase.rslen = input->reads[i].bases.size();
        testCase.rs = input->reads[i].bases.c_str();
        testCase.i = input->reads[i]._i.c_str();
        testCase.d = input->reads[i]._d.c_str();
        testCase.c = input->reads[i]._c.c_str();
        testCase.q = input->reads[i]._q.c_str();
        for (int j = 0; (size_t)j < input->haps.size(); j++) {
            testCase.haplen = input->haps[j].bases.size();
            testCase.hap = input->haps[j].bases.c_str();
            double result_final = 0;
            float result_float = use_double ? 0.0f : compute_full_prob_baseline<float>(&testCase, NULL);
            if (result_float < MIN_ACCEPTED) {
                double result_double = compute_full_prob_baseline<double>(&testCase, NULL);
                result_final = log10(result_double) - g_ctxd.LOG10_INITIAL_CONSTANT;
            } else {
                result_final = (double)(log10f(result_float) - g_ctxf.LOG10_INITIAL_CONSTANT);
            }
            output->likelihoodData.push_back(result_final);
        }
    }
    return 0;
} */

/* int xilPairHMM::computePairhmmAVX(pairhmmInput* input, pairhmmOutput* output, bool use_double) {
    testcase testCase;
    output->likelihoodData.clear();
    for (int i = 0; (size_t)i < input->reads.size(); i++) {
        testCase.rslen = input->reads[i].bases.size();
        testCase.rs = input->reads[i].bases.c_str();
        testCase.i = input->reads[i]._i.c_str();
        testCase.d = input->reads[i]._d.c_str();
        testCase.c = input->reads[i]._c.c_str();
        testCase.q = input->reads[i]._q.c_str();
        for (int j = 0; (size_t)j < input->haps.size(); j++) {
            testCase.haplen = input->haps[j].bases.size();
            testCase.hap = input->haps[j].bases.c_str();
            double result_final = 0;
            float result_float = use_double ? 0.0f : compute_fp_avxs(&testCase);
            if (result_float < MIN_ACCEPTED) {
                double result_double = compute_fp_avxd(&testCase);
                result_final = log10(result_double) - g_ctxd.LOG10_INITIAL_CONSTANT;
            } else {
                result_final = (double)(log10f(result_float) - g_ctxf.LOG10_INITIAL_CONSTANT);
            }
            output->likelihoodData.push_back(result_final);
        }
    }
    return 0;
} */

double countCell(pairhmmInput* input, short maxCols, bool& violate) {
    double numCell = 0;
    for (int i = 0; (size_t)i < input->reads.size(); i++) {
        for (int j = 0; (size_t)j < input->haps.size(); j++) {
            int cur_numCell = input->reads[i].bases.size() * input->haps[j].bases.size();
            if (input->reads[i].bases.size() * (input->reads[i].bases.size() + maxCols) <=
                1 + READ_BLOCK_SIZE * MAX_READ_LEN) {
                violate = true;
                return numCell;
            }
            numCell += cur_numCell;
        }
    }
    return numCell;
}

bool worthFPGA(pairhmmInput* input, short maxCols, double cellNum) {
    if (input->reads.size() < DIE_NUM) return false;
    bool violate = false;
    if (cellNum <= 0) cellNum = countCell(input, maxCols, violate);
    if (violate) return false;
    double AVX_perf = AVX_PERF;
    double FPGA_perf = FPGA_PERF;

    double AVX_time = cellNum / AVX_perf; // unit nano seconds, 0.6 GCUPS
    double data_prepare_time = 5e5;
    ;
    double CPU_to_DRAM = 1 * 1e6; // DIE_NUM * 300000 + DIE_NUM * sizeof(FPGAInput) / 3.0;
    double DRAM_to_FPGA = 0;
    double DRAM_to_CPU = 5e5 + (input->reads.size() * input->haps.size() * 4) / 3.0;
    double recompute_estm = 1e6;
    double FPGA_compute = cellNum / FPGA_perf;
    if (AVX_time > (CPU_to_DRAM + DRAM_to_FPGA + DRAM_to_CPU + FPGA_compute + data_prepare_time + recompute_estm)) {
        return true;
    } else
        return false;
}

int xilPairHMM::get_max_rsdata_num() {
    int slr0_pe_num = SLR0_PE_NUM;
    int slr1_pe_num = SLR1_PE_NUM;
    int slr2_pe_num = SLR2_PE_NUM;
    int max_pe_num = slr0_pe_num;
    if (max_pe_num < slr1_pe_num) {
        max_pe_num = slr1_pe_num;
    }
    if (max_pe_num < slr2_pe_num) {
        max_pe_num = slr2_pe_num;
    }
    return (int)floor((float)MAX_RSDATA_NUM / (float)max_pe_num * (float)TOTAL_PE_NUM);
}

void xilPairHMM::convert_read_input(readDataPack* cur_host_read, Read* input, int idx) {
    uint8_t _rs = ConvertChar::get(input->bases[idx]);
    uint8_t _q = input->_q[idx];
    uint8_t _i = input->_i[idx];
    uint8_t _d = input->_d[idx];
    uint8_t _c = input->_c[idx];
    uint32_t score = ((uint32_t)(_rs & 0x7) << 28) | ((uint32_t)(_q & 127) << 21) | ((uint32_t)(_i & 127) << 14) |
                     ((uint32_t)(_d & 127) << 7) | (uint32_t)(_c & 127);
    float m2m_val = m2m_table[128 * _i + _d];
    cur_host_read->scores = score;
    cur_host_read->m2m = m2m_val;
}

void xilPairHMM::distributeReads(pairhmmInput* input,
                                 int read_base_index,
                                 int hap_base_index,
                                 int& numRead0,
                                 int& numRead1,
                                 int& numRead2,
                                 int& totalNumRead,
                                 int& totalNumHap,
                                 short maxCols,
                                 bool& violate) {
    // first get the total actual cells
    double numCell = 0;
    for (int i = read_base_index; i < read_base_index + totalNumRead; i++) {
        for (int j = hap_base_index; j < hap_base_index + totalNumHap; j++) {
            int amended_read_length = input->reads[i].bases.size() + 1;
            int new_rows = amended_read_length;
            if (new_rows < DEP_DIST) new_rows = DEP_DIST;
            int cur_numCell = (new_rows + 1) * (amended_read_length + maxCols);
            numCell += cur_numCell;
        }
    }
    curNumCell = numCell;
    //  if (!worthFPGA(input, maxCols, numCell)) {
    //      violate = true;
    //      return;
    //  }
    float SLR_numCells[3];
    if (DIE_NUM == 2) {
        SLR_numCells[0] = floor(numCell * SLR0_PE_NUM / (SLR0_PE_NUM + SLR1_PE_NUM));
        SLR_numCells[1] = numCell - SLR_numCells[0];
        SLR_numCells[2] = 0;
    } else {
        SLR_numCells[0] = floor(numCell * SLR0_PE_NUM / (SLR0_PE_NUM + SLR1_PE_NUM + SLR2_PE_NUM));
        SLR_numCells[1] = floor(numCell * SLR1_PE_NUM / (SLR0_PE_NUM + SLR1_PE_NUM + SLR2_PE_NUM));
        SLR_numCells[2] = numCell - SLR_numCells[0] - SLR_numCells[1];
    }
    float curCells = 0.0;
    int readCount = 0;
    int i = 0;
    for (i = read_base_index; i < read_base_index + totalNumRead; i++) {
        for (int j = hap_base_index; j < hap_base_index + totalNumHap; j++) {
            int amended_read_length = input->reads[i].bases.size() + 1;
            int new_rows = amended_read_length;
            if (new_rows < DEP_DIST) new_rows = DEP_DIST;
            curCells += (new_rows + 1) * (amended_read_length + maxCols);
            ;
        }
        readCount++;
        if (curCells >= SLR_numCells[0]) {
            numRead0 = readCount;
            break;
        }
    }
    if (numRead0 <= 0) {
        violate = true;
        return;
    }
    if (DIE_NUM == 2) {
        numRead1 = totalNumRead - numRead0;
        if (numRead1 <= 0) violate = true;
        numRead2 = 0;
        return;
    } else {
        curCells = 0;
        readCount = 0;
        i++;
        for (; i < read_base_index + totalNumRead; i++) {
            for (int j = hap_base_index; j < hap_base_index + totalNumHap; j++) {
                int amended_read_length = input->reads[i].bases.size() + 1;
                int new_rows = amended_read_length;
                if (new_rows < DEP_DIST) new_rows = DEP_DIST;
                curCells += (new_rows + 1) * (amended_read_length + maxCols);
                ;
            }
            readCount++;
            if (curCells >= SLR_numCells[1]) {
                numRead1 = readCount;
                break;
            }
        }
        numRead2 = totalNumRead - numRead0 - numRead1;
        if (numRead2 <= 0) violate = true;
    }
}

/*void xilPairHMM::computePairhmmAVXSegment(pairhmmInput* input,
                                          int read_base_index,
                                          int hap_base_index,
                                          int cur_numRead,
                                          int cur_numHap,
                                          vector<float>& output) {
    testcase testCase;
    for (int i = read_base_index; i < read_base_index + cur_numRead; i++) {
        testCase.rslen = input->reads[i].bases.size();
        testCase.rs = input->reads[i].bases.c_str();
        testCase.i = input->reads[i]._i.c_str();
        testCase.d = input->reads[i]._d.c_str();
        testCase.c = input->reads[i]._c.c_str();
        testCase.q = input->reads[i]._q.c_str();
        for (int j = hap_base_index; j < hap_base_index + cur_numHap; j++) {
            testCase.haplen = input->haps[j].bases.size();
            testCase.hap = input->haps[j].bases.c_str();
            output[i * input->haps.size() + j] = compute_fp_avxs(&testCase);
        }
    }
}
*/
void xilPairHMM::sortReads(pairhmmInput* input,
                           int read_base_index,
                           int cur_numRead,
                           int cur_numHap,
                           short maxCols,
                           int slr_pu_num[3],
                           bool& violate) {
    vector<readInfo> sortedReadInfo[3];
    int read_start_index = 0;
    for (int k = 0; k < DIE_NUM; k++) {
        for (int i = 0; i < host_input[k]->numRead; i += 2) {
            readInfo curInfo;
            int cur_read_len = input->reads[i + read_start_index + read_base_index].bases.size();
            //   if (cur_read_len > MAX_READ_LEN) {
            //       violate = true;
            //       return;
            //   }
            curInfo.readLen[0] = cur_read_len;
            curInfo.big_rows = cur_read_len + 1;
            curInfo.oneOrTwo = false;
            curInfo.resultOffset = cur_numHap * i;
            curInfo.readID[0] = i;
            if (i + 1 < host_input[k]->numRead) {
                cur_read_len = input->reads[i + 1 + read_start_index + read_base_index].bases.size();
                //   if (cur_read_len > MAX_READ_LEN) {
                //       violate = true;
                //       return;
                //   }
                curInfo.readLen[1] = cur_read_len;
                curInfo.oneOrTwo = true;
                if (cur_read_len + 1 > curInfo.big_rows) curInfo.big_rows = cur_read_len + 1;
                curInfo.readID[1] = i + 1;
            }
            if (curInfo.big_rows < DEP_DIST)
                curInfo.new_rows = DEP_DIST;
            else
                curInfo.new_rows = curInfo.big_rows;
            curInfo.curIterNum = (curInfo.new_rows + 1) * (curInfo.big_rows + maxCols);
            curInfo.infoPacked = curInfo.readLen[0] + (curInfo.readLen[1] << 8) +
                                 ((uint64_t)curInfo.resultOffset << 16) + (((uint64_t)curInfo.curIterNum - 1) << 37) +
                                 ((uint64_t)curInfo.oneOrTwo << 58);
            sortedReadInfo[k].push_back(curInfo);
        }

        sort(sortedReadInfo[k].begin(), sortedReadInfo[k].end(), cmpReadInfo);

        for (int j = 0; (size_t)j < sortedReadInfo[k].size(); j += slr_pu_num[k]) {
            int upper_bound = slr_pu_num[k];
            if ((size_t)(j + upper_bound) >= sortedReadInfo[k].size()) upper_bound = sortedReadInfo[k].size() - j;
            for (int m = 0; m < upper_bound / 2; m++) {
                swap(sortedReadInfo[k][j + m], sortedReadInfo[k][j + upper_bound - 1 - m]);
            }
        }
        read_start_index += host_input[k]->numRead;
    }
    read_start_index = 0;
    for (int slr_id = 0; slr_id < DIE_NUM; slr_id++) {
        for (int i = 0; (size_t)i < sortedReadInfo[slr_id].size(); i++) {
            for (int j = 0; j < sortedReadInfo[slr_id][i].readLen[0]; j++) {
                convert_read_input(
                    &(host_input[slr_id]->dataPack.readData[READ_BLOCK_SIZE * i][j]),
                    &(input->reads[sortedReadInfo[slr_id][i].readID[0] + read_base_index + read_start_index]), j);
            }
            if (sortedReadInfo[slr_id][i].oneOrTwo) {
                for (int j = 0; j < sortedReadInfo[slr_id][i].readLen[1]; j++) {
                    convert_read_input(
                        &(host_input[slr_id]->dataPack.readData[READ_BLOCK_SIZE * i + 1][j]),
                        &(input->reads[sortedReadInfo[slr_id][i].readID[1] + read_base_index + read_start_index]), j);
                }
            }
        }
        read_start_index += host_input[slr_id]->numRead;
    }
    if (violate) return;

    for (int slr_id = 0; slr_id < 3; slr_id++) {
        int PU_id = 0;
        for (int i = 0; (size_t)i < sortedReadInfo[slr_id].size(); i++) {
            host_input[slr_id]->dataPack.iterNum[PU_id] += sortedReadInfo[slr_id][i].curIterNum;
            host_input[slr_id]->dataPack.readInfo[i] = sortedReadInfo[slr_id][i].infoPacked;
            host_input[slr_id]->dataPack.numReadPU[PU_id] += (sortedReadInfo[slr_id][i].oneOrTwo) ? 2 : 1;
            PU_id = (PU_id == slr_pu_num[slr_id] - 1) ? 0 : PU_id + 1;
        }
    }

    int hap_batch_size = cur_numHap / HAP_BLOCK_SIZE + (cur_numHap % HAP_BLOCK_SIZE != 0);
    for (int slr_id = 0; slr_id < 3; slr_id++) {
        long long total_slr_itercount = 0;
        for (int k = 0; k < slr_pu_num[slr_id]; k++) {
            host_input[slr_id]->dataPack.iterNum[k] *= hap_batch_size;
            total_slr_itercount += host_input[slr_id]->dataPack.iterNum[k];
        }
    }
}

void xilPairHMM::update_host_inputs_new(
    pairhmmInput* input, int read_base_index, int hap_base_index, int cur_numRead, int cur_numHap, bool& violate) {
    struct timespec time1, time2, time_diff;
    clock_gettime(CLOCK_REALTIME, &time1);

    for (int i = 0; i < DIE_NUM; i++) {
        host_input[i]->numHap = cur_numHap;
    }

    short maxCols = 0;
    for (int i = 0; i < cur_numHap; i++) {
        //   if (input->haps[i + hap_base_index].bases.size() > MAX_HAP_LEN) {
        //      violate = true;
        //      return;
        //  }
        short hapLenTmp = input->haps[i + hap_base_index].bases.size();
        if (hapLenTmp + 1 > maxCols) {
            maxCols = hapLenTmp + 1;
        }
        for (int j = 0; j < DIE_NUM; j++) {
            host_input[j]->dataPack.hapDataLen[i].hapLen = hapLenTmp;
            host_input[j]->dataPack.hapDataLen[i].oneDivHapLen = g_ctxf.INITIAL_CONSTANT / (float)(hapLenTmp);
        }
    }

    for (int i = 0; i < MAX_HAPDATA_NUM / HAP_BLOCK_SIZE; i++) {
        for (int j = 0; j < MAX_HAP_LEN; j++) {
            uint16_t tmp = 0;
            for (int k = 0; k < HAP_BLOCK_SIZE; k++) {
                char cur_base;
                if (i * HAP_BLOCK_SIZE + k + hap_base_index >= (int)input->haps.size())
                    cur_base = 'N';
                else if ((size_t)j >= input->haps[i * HAP_BLOCK_SIZE + k + hap_base_index].bases.size())
                    cur_base = 'N';
                else {
                    cur_base = input->haps[i * HAP_BLOCK_SIZE + k + hap_base_index].bases[j];
                }
                tmp = tmp | ((uint16_t)(ConvertChar::get(cur_base)) << (4 * k));
            }
            for (int k = 0; k < DIE_NUM; k++) {
                host_input[k]->dataPack.hapData[i][j] = tmp;
            }
        }
    }

    // distribute reads to each PE
    int slr_pu_num[3];
    slr_pu_num[0] = SLR0_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE);
    slr_pu_num[1] = SLR1_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE);
    slr_pu_num[2] = SLR2_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE);

    for (int i = 0; i < DIE_NUM; i++) {
        for (int j = 0; j < 64; j++) {
            host_input[i]->dataPack.numReadPU[j] = 0;
            host_input[i]->dataPack.iterNum[j] = 0;
        }
    }

    distributeReads(input, read_base_index, hap_base_index, host_input[0]->numRead, host_input[1]->numRead,
                    host_input[2]->numRead, cur_numRead, cur_numHap, maxCols, violate);
    printf("numRead0 = %d, numRead1 = %d, numRead2 = %d, total read = %d\n", host_input[0]->numRead,
           host_input[1]->numRead, host_input[2]->numRead, cur_numRead);
    if (violate) {
        printf("Either one SLR has 0 reads or this segment is too small to be run on FPGA, switch back to AVX\n");
        return;
    }
    sortReads(input, read_base_index, cur_numRead, cur_numHap, maxCols, slr_pu_num, violate);
    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    data_prepare_time += (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);
}

void xilPairHMM::update_host_inputs(
    pairhmmInput* input, int read_base_index, int hap_base_index, int cur_numRead, int cur_numHap, bool& violate) {
    short maxCols = 0;
    for (int i = 0; i < cur_numHap; i++) {
        if (input->haps[i + hap_base_index].bases.size() > MAX_HAP_LEN) {
            violate = true;
            return;
        }
        short hapLenTmp = input->haps[i + hap_base_index].bases.size();
        if (hapLenTmp + 1 > maxCols) {
            maxCols = hapLenTmp + 1;
        }
        for (int j = 0; j < DIE_NUM; j++) {
            host_input[j]->dataPack.hapDataLen[i].hapLen = hapLenTmp;
            host_input[j]->dataPack.hapDataLen[i].oneDivHapLen = g_ctxf.INITIAL_CONSTANT / (float)(hapLenTmp);
        }
    }
    distributeReads(input, read_base_index, hap_base_index, host_input[0]->numRead, host_input[1]->numRead,
                    host_input[2]->numRead, cur_numRead, cur_numHap, maxCols, violate);
    printf("numRead0 = %d, numRead1 = %d, numRead2 = %d, total read = %d\n", host_input[0]->numRead,
           host_input[1]->numRead, host_input[2]->numRead, cur_numRead);
    if (violate) {
        printf("Either one SLR has 0 reads or this segment is too small to be run on FPGA, switch back to AVX\n");
        return;
    }
    for (int i = 0; i < DIE_NUM; i++) {
        host_input[i]->numHap = cur_numHap;
    }
    uint8_t readDataLen[3][MAX_RSDATA_NUM];

    for (int i = 0; i < cur_numRead; i++) {
        int cur_read_len = input->reads[i + read_base_index].bases.size();
        if (cur_read_len > MAX_READ_LEN) {
            violate = true;
            return;
        }
        if (i < host_input[0]->numRead) {
            readDataLen[0][i] = cur_read_len;
            for (int j = 0; j < cur_read_len; j++) {
                convert_read_input(&(host_input[0]->dataPack.readData[i][j]), &(input->reads[i + read_base_index]), j);
            }
        } else if (i - host_input[0]->numRead < host_input[1]->numRead) {
            int offset = i - host_input[0]->numRead;
            readDataLen[1][offset] = cur_read_len;
            for (int j = 0; j < cur_read_len; j++) {
                convert_read_input(&(host_input[1]->dataPack.readData[offset][j]), &(input->reads[i + read_base_index]),
                                   j);
            }
        } else if (DIE_NUM > 2) {
            int offset = i - host_input[0]->numRead - host_input[1]->numRead;
            readDataLen[2][offset] = cur_read_len;
            for (int j = 0; j < cur_read_len; j++) {
                convert_read_input(&(host_input[2]->dataPack.readData[offset][j]), &(input->reads[i + read_base_index]),
                                   j);
            }
        }
    }

    for (int i = 0; i < MAX_HAPDATA_NUM / HAP_BLOCK_SIZE; i++) {
        for (int j = 0; j < MAX_HAP_LEN; j++) {
            uint32_t tmp = 0;
            for (int k = 0; k < HAP_BLOCK_SIZE; k++) {
                char cur_base;
                if (i * HAP_BLOCK_SIZE + k + hap_base_index >= (int)input->haps.size())
                    cur_base = 'N';
                else if ((size_t)j >= input->haps[i * HAP_BLOCK_SIZE + k + hap_base_index].bases.size())
                    cur_base = 'N';
                else
                    cur_base = input->haps[i * HAP_BLOCK_SIZE + k + hap_base_index].bases[j];
                tmp = tmp | ((uint16_t)(ConvertChar::get(cur_base)) << (4 * k));
            }
            for (int k = 0; k < DIE_NUM; k++) {
                host_input[k]->dataPack.hapData[i][j] = tmp;
            }
        }
    }

    // distribute reads to each PE
    int slr_pu_num[3];
    slr_pu_num[0] = SLR0_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE);
    slr_pu_num[1] = SLR1_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE);
    slr_pu_num[2] = SLR2_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE);
    for (int i = 0; i < DIE_NUM; i++) {
        for (int j = 0; j < 64; j++) {
            host_input[i]->dataPack.numReadPU[j] = 0;
            host_input[i]->dataPack.iterNum[j] = 0;
        }
    }

    for (int slr_id = 0; slr_id < 3; slr_id++) {
        uint64_t resultOffset = 0;
        int PU_id = 0;
        for (int i = 0; i < host_input[slr_id]->numRead; i += READ_BLOCK_SIZE) {
            uint64_t curInfo = (resultOffset << 16) + (readDataLen[slr_id][i + 1] << 8) + readDataLen[slr_id][i];
            bool oneOrTwo = (i + 1 < host_input[slr_id]->numRead);
            uint8_t big_rows = readDataLen[slr_id][i] + 1;
            if (oneOrTwo && big_rows < readDataLen[slr_id][i + 1] + 1) {
                big_rows = readDataLen[slr_id][i + 1] + 1;
            }
            uint8_t new_rows = big_rows;
            if (big_rows < DEP_DIST) new_rows = DEP_DIST;
            uint64_t curIterNum = (new_rows + 1) * (big_rows + maxCols);
            host_input[slr_id]->dataPack.iterNum[PU_id] += curIterNum;
            curInfo += (((curIterNum - 1) << 37) + ((uint64_t)oneOrTwo << 58));
            host_input[slr_id]->dataPack.readInfo[i / READ_BLOCK_SIZE] = curInfo;
            host_input[slr_id]->dataPack.numReadPU[PU_id] += (i + 1 < host_input[slr_id]->numRead) ? 2 : 1;
            PU_id = (PU_id == slr_pu_num[slr_id] - 1) ? 0 : PU_id + 1;
            resultOffset += READ_BLOCK_SIZE * cur_numHap;
        }
    }
    int hap_batch_size = cur_numHap / HAP_BLOCK_SIZE + (cur_numHap % HAP_BLOCK_SIZE != 0);
    for (int slr_id = 0; slr_id < 3; slr_id++) {
        long long total_slr_itercount = 0;
        for (int k = 0; k < slr_pu_num[slr_id]; k++) {
            host_input[slr_id]->dataPack.iterNum[k] *= hap_batch_size;
            printf("slr %d, PU %d has %ld iterations in total\n", slr_id, k, host_input[slr_id]->dataPack.iterNum[k]);
            total_slr_itercount += host_input[slr_id]->dataPack.iterNum[k];
        }
    }
}

int xilPairHMM::computePairhmmxil(pairhmmInput* input, pairhmmOutput* output, bool& usedFPGA) {
    struct timespec time1, time2, time_diff;
    short maxCols = 0;

    for (int i = 0; (size_t)i < input->haps.size(); i++) {
        short hapLenTmp = input->haps[i].bases.size();
        if (hapLenTmp + 1 > maxCols) {
            maxCols = hapLenTmp + 1;
        }
    }

#if 0   
if (!worthFPGA(input, maxCols, 0)) {
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
            // if (violate) {
            // computePairhmmAVXSegment(input, read_base_index, hap_base_index, cur_numRead, cur_numHap,
            //                          host_raw_output);
            //} else {
            usedFPGA = true;
            if (this->is_seq()) {
                computePairhmmFPGASeq((char*)&(host_input[0]->dataPack), host_input[0]->numRead, host_input[0]->numHap,
                                      host_output[0], (char*)&(host_input[1]->dataPack), host_input[1]->numRead,
                                      host_input[1]->numHap, host_output[1], (char*)&(host_input[2]->dataPack),
                                      host_input[2]->numRead, host_input[2]->numHap, host_output[2]);
            } else {
                computePairhmmFPGAOverlap(
                    (char*)&(host_input[0]->dataPack), host_input[0]->numRead, host_input[0]->numHap, host_output[0],
                    (char*)&(host_input[1]->dataPack), host_input[1]->numRead, host_input[1]->numHap, host_output[1],
                    (char*)&(host_input[2]->dataPack), host_input[2]->numRead, host_input[2]->numHap, host_output[2]);
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
            // }
            hap_base_index += cur_numHap;
        }
        read_base_index += cur_numRead;
    }

    clock_gettime(CLOCK_REALTIME, &time1);

    for (int k = 0; k < numRead * numHap; k++) {
        //      if (host_raw_output[k] < MIN_ACCEPTED) {
        //          singleFP_violate_count++;
        //          testcase testCase;
        //          int ii = k / numHap;
        //          int jj = k % numHap;
        //          testCase.rslen = input->reads[ii].bases.size();
        //        testCase.rs = input->reads[ii].bases.c_str();
        //       testCase.i = input->reads[ii]._i.c_str();
        //       testCase.d = input->reads[ii]._d.c_str();
        //       testCase.q = input->reads[ii]._q.c_str();
        //       testCase.c = input->reads[ii]._c.c_str();
        //       testCase.haplen = input->haps[jj].bases.size();
        //       testCase.hap = input->haps[jj].bases.c_str();
        //       double result_double = compute_fp_avxd(&testCase);
        //       output->likelihoodData.push_back(log10(result_double) - g_ctxd.LOG10_INITIAL_CONSTANT);
        //   } else {

        output->likelihoodData.push_back(log10(host_raw_output[k]) - g_ctxf.LOG10_INITIAL_CONSTANT);
        //  }
    }

    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    recompute_time += (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);
    printf("This FPGA run is done, %d of %d single FP is overlowed and recomputed using AVX double precsion\n",
           singleFP_violate_count, numRead * numHap);
    return 0;
}

typedef union {
    long long u;
    double f;
} ieee754;

typedef union {
    int u;
    float f;
} ieee754s;
#define LN10_HEX 0x40135d8e

#define MAX_QUAL 254
#define MAX_JACOBIAN_TOLERANCE 8.0
#define JACOBIAN_LOG_TABLE_STEP 0.0001
#define JACOBIAN_LOG_TABLE_INV_STEP (1.0 / JACOBIAN_LOG_TABLE_STEP)

static int fastRoundMerlin(float d) {
    int rose_temp;
    if (d > ((float)0.0)) {
        rose_temp = ((int)(d + ((float)0.5)));
    } else {
        rose_temp = ((int)(d - ((float)0.5)));
    }
    return rose_temp;
}

float approximateLog10SumLog10Merlin(float small, float big, float* jacobianLogTable) {
    if (small > big) {
        float t = big;
        big = small;
        small = t;
    }
    float ret;
    if (isinf(small) || isinf(big)) {
        ret = big;
    } else {
        float diff = big - small;
        if (diff >= ((float)MAX_JACOBIAN_TOLERANCE))
            ret = big;
        else {
            int ind = fastRoundMerlin((float)(diff * ((float)JACOBIAN_LOG_TABLE_INV_STEP))); // hard rounding
            ret = big + jacobianLogTable[ind];
        }
    }
    return ret;
}

//#define LN10_HEX 0x40026bb1bbb55516 //double
float setMatchToMatchProbMerlin(char insQual, char delQual, float* jacobianLogTable) {
    unsigned char minQual = (unsigned char)delQual;
    unsigned char maxQual = (unsigned char)insQual;
    if (insQual <= delQual) {
        minQual = ((unsigned char)insQual);
        maxQual = ((unsigned char)delQual);
    }
    float LN10 = log(10.0);
    float output_data;
    float temp0 = exp((approximateLog10SumLog10Merlin(((float)(-0.1)) * ((float)((int)minQual)),
                                                      ((float)(-0.1)) * ((float)((int)maxQual)), jacobianLogTable)) *
                      LN10);
    float temp1 = exp(log1p(-fmin((float)1.0, temp0)));
    output_data = (MAX_QUAL >= maxQual) ? temp1 : 1.0 - temp0;
    return output_data;
}

int printPH2PR() {
    float LN10 = log(10.0);
    FILE* fp0 = fopen("ph2pr.h", "w");
    FILE* fp1 = fopen("ph2pr_sub1.h", "w");
    FILE* fp2 = fopen("ph2pr_div3.h", "w");
    fprintf(fp0, "#define PH2PR_INIT {");
    fprintf(fp1, "#define PH2PR_SUB1_INIT {");
    fprintf(fp2, "#define PH2PR_DIV3_INIT {");
    for (int x = 0; x < 128; x++) {
        ieee754s c;
        c.f = exp(-((float)x) / 10.0 * LN10);
        fprintf(fp0, "%10.60f", c.f);
        fprintf(fp1, "%10.60f", 1.0 - c.f);
        fprintf(fp2, "%10.60f", c.f / 3.0);
        if (x != 127) {
            fprintf(fp0, ", ");
            fprintf(fp1, ", ");
            fprintf(fp2, ", ");
        }
    }
    fprintf(fp0, "};");
    fprintf(fp1, "};");
    fprintf(fp2, "};");
    fclose(fp0);
    fclose(fp1);
    fclose(fp2);
    return 0;
}

int printM2MPROB() {
    float jacobianLogTable[JACOBIAN_LOG_TABLE_SIZE];
    float LN10 = log(10.0);

    for (int k = 0; k < JACOBIAN_LOG_TABLE_SIZE; k++) {
        jacobianLogTable[k] = (float)(log10(1.0 + exp(-((float)k) * JACOBIAN_LOG_TABLE_STEP * LN10)));
    }
    FILE* fp = fopen("m2m.h", "w");
    fprintf(fp, "#define M2M_INIT {");
    int table_count = 0;
    for (unsigned char x = 0; x < 128; x++) {
        for (unsigned char y = 0; y < 128; y++) {
            ieee754s c;
            c.f = setMatchToMatchProbMerlin(x, y, jacobianLogTable);
            fprintf(fp, "%10.60f", c.f);

            if (y != 127 || x != 127) fprintf(fp, ", \\\n");
            table_count++;
        }
    }
    fprintf(fp, "}");
    fclose(fp);
    printf("table size = %d\n", table_count);
    return 0;
}

int load_file_to_memory(const char* filename, char** result) {
    size_t size = 0;
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        printf("ERROR : Kernel binary %s not exist!\n", filename);
        *result = NULL;
        return -1; // -1 means file opening fail
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = (char*)malloc(size + 1);
    if ((int)size != (int)fread(*result, sizeof(char), size, f)) {
        free(*result);
        return -2; // -2 means file reading fail
    }
    fclose(f);
    (*result)[size] = 0;
    return size;
}

xilPairHMM::xilPairHMM() {
    useFPGA = true;
    ConvertChar::init();
    // _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
}

bool xilPairHMM::init_FPGA(char* bitstream) {
    // get platform info
    char cl_platform_vendor[1001];
    char cl_platform_name[1001];
    cl_platform_vendor[0] = 0;
    cl_platform_name[0] = 0;
    int err;

    err = clGetPlatformIDs(1, &platform_id, NULL);
    if (err != CL_SUCCESS) {
        printf("Warning: Failed to find an OpenCL platform!Code %i\n", err);
        return false;
    }
    printf("Successfully create platform\n");
    err = clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, 1000, (void*)cl_platform_vendor, NULL);
    if (err != CL_SUCCESS) {
        printf("Warning: clGetPlatformInfo(CL_PLATFORM_VENDOR) failed! Code %i\n", err);
        return false;
    }
    printf("CL_PLATFORM_VENDOR %s\n", cl_platform_vendor);
    err = clGetPlatformInfo(platform_id, CL_PLATFORM_NAME, 1000, (void*)cl_platform_name, NULL);
    if (err != CL_SUCCESS) {
        printf("Warning: clGetPlatformInfo(CL_PLATFORM_NAME) failed! Code %i\n", err);
        return false;
    }
    printf("CL_PLATFORM_NAME %s\n", cl_platform_name);

    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ACCELERATOR, 1, &device_id, NULL);
    if (err != CL_SUCCESS) {
        printf("Warning: Failed to create a device group! Code %i\n", err);
        return false;
    }
    printf("Successfully create device\n");

    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context) {
        printf("Warning: Failed to create a compute context! Code %i\n", err);
        return false;
    }
    printf("Successfully create context \n");
    for (int i = 0; i < KERNEL_NUM; i++) {
        commands[i] = clCreateCommandQueue(context, device_id, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
        if (!commands[i]) {
            printf("Warning: Failed to create a command queue commands[%d]! Code %i\n", i, err);
            return false;
        }
    }
    printf("Successfully create command queue \n");
    unsigned char* kernelbinary;

    int n_i = 0;
    n_i = load_file_to_memory(bitstream, (char**)&kernelbinary);
    if (n_i < 0) {
        printf("Warning : failed to load kernel from binary: %s\n", bitstream);
        return false;
    }
    printf("Successfully load kernel from binary: %s\n", bitstream);

    int status;
    size_t n = n_i;
    program =
        clCreateProgramWithBinary(context, 1, &device_id, &n, (const unsigned char**)&kernelbinary, &status, &err);
    if ((!program) || (err != CL_SUCCESS)) {
        printf("Warning: Failed to create compute program from binary! Code %d\n", err);
        return false;
    }
    printf("Success to create compute program from binary! \n");

    // Build the program executable
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        printf("Warning: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        return false;
    }
    printf("Sucess to build program executable!\n");
    std::string kname = "pairhmm";
    std::string kernelName = kname + ":{" + kname + "_" + "1" + "}";
    _pmm_kernel[0] = clCreateKernel(program, kernelName.c_str(), &err);
    if ((!_pmm_kernel[0]) || err != CL_SUCCESS) {
        printf("Warning: Failed to create compute kernel for pairhmm core0\n");
        return false;
    }
    if (!this->is_seq()) {
        kernelName = kname + ":{" + kname + "_" + "2" + "}";
        _pmm_kernel[1] = clCreateKernel(program, kernelName.c_str(), &err);
        if ((!_pmm_kernel[1]) || err != CL_SUCCESS) {
            printf("Warning: Failed to create compute kernel for pairhmm core1\n");
            return false;
        }
        kernelName = kname + ":{" + kname + "_" + "3" + "}";
        _pmm_kernel[2] = clCreateKernel(program, kernelName.c_str(), &err);
        if ((!_pmm_kernel[2]) || err != CL_SUCCESS) {
            printf("Warning: Failed to create compute kernel for pairhmm core2\n");
            return false;
        }
    }
    return true;
}

bool xilPairHMM::init_FPGA_buffer() {
    int err;
    for (int i = 0; i < 3; i++) {
        OCL_CHECK(err, _input_buffer[i] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                                         sizeof(InputDataPackOpt), &(host_input[i]->dataPack), &err));
        OCL_CHECK(err, _output_buffer[i] =
                           clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
                                          sizeof(float) * MAX_RSDATA_NUM * MAX_HAPDATA_NUM, host_output[i], &err));
    }
    return true;
}

bool xilPairHMM::destroy_FPGA_buffer() {
    for (int i = 0; i < 3; i++) {
        clReleaseMemObject(_input_buffer[i]);
        clReleaseMemObject(_output_buffer[i]);
    }
    return true;
}

xilPairHMM::xilPairHMM(char* bitstream, bool seq) {
    ConvertChar::init();
    m_seq = seq;
    //  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    float m2m[128 * 128] = M2M_INIT;
    m2m_table = (float*)malloc(128 * 128 * sizeof(float));
    for (int i = 0; i < 128 * 128; i++) {
        m2m_table[i] = m2m[i];
    }
    data_prepare_time = 0;
    cl_write_time = 0;
    peak_kernel_gcups = 0;
    kernel_time = 0;
    cl_read_time = 0;
    recompute_time = 0;
    printf("Start the constructor\n");
    if ((useFPGA = init_FPGA(bitstream))) {
        for (int i = 0; i < 3; i++) {
            host_input[i] = (FPGAInput*)memalign(4096, sizeof(FPGAInput));
            host_output[i] = (float*)memalign(4096, sizeof(float) * MAX_RSDATA_NUM * MAX_HAPDATA_NUM);
        }
        useFPGA = init_FPGA_buffer();
        printf("init FPGA buffer succeeds\n");
    }
    printf("depth of input is %ld 512-bit, %ld bytes \n", sizeof(InputDataPackOpt) / 64, sizeof(InputDataPackOpt));
}

bool xilPairHMM::computePairhmmFPGASeq(char* input0,
                                       int numRead0,
                                       int numHap0,
                                       float* output0,
                                       char* input1,
                                       int numRead1,
                                       int numHap1,
                                       float* output1,
                                       char* input2,
                                       int numRead2,
                                       int numHap2,
                                       float* output2) {
    int err;
    struct timespec time1, time2, time_diff;
    clock_gettime(CLOCK_REALTIME, &time1);

    int narg = 0;
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_input_buffer[0]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_output_buffer[0]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numRead0);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numHap0);

    err = clEnqueueMigrateMemObjects(commands[0], DIE_NUM, _input_buffer, 0, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to migrate write to device buffer! Code %d\n", err);
        return false;
    }
    clFinish(commands[0]);

    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    cl_write_time += (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);

    clock_gettime(CLOCK_REALTIME, &time1);
    err = clEnqueueTask(commands[0], _pmm_kernel[0], 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel0! Code %d\n", err);
        return false;
    }

    narg = 0;
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_input_buffer[1]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_output_buffer[1]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numRead1);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numHap1);

    err = clEnqueueTask(commands[0], _pmm_kernel[0], 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel1! Code %d\n", err);
        return false;
    }
    clFinish(commands[0]);

    narg = 0;
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_input_buffer[2]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_output_buffer[2]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numRead2);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numHap2);

    err = clEnqueueTask(commands[0], _pmm_kernel[0], 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel2! Code %d\n", err);
        return false;
    }
    clFinish(commands[0]);

    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    double cur_kernel_time = (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);
    if (curNumCell / cur_kernel_time > peak_kernel_gcups) peak_kernel_gcups = curNumCell / cur_kernel_time;
    kernel_time += cur_kernel_time;

    clock_gettime(CLOCK_REALTIME, &time1);
    err = clEnqueueMigrateMemObjects(commands[0], DIE_NUM, _output_buffer, CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                     nullptr);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to migrate read from device buffer! Code %d\n", err);
        return false;
    }
    clFinish(commands[0]);

    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    cl_read_time += (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);

    clFlush(commands[0]);
    clFinish(commands[0]);
    return true;
}

bool xilPairHMM::computePairhmmFPGAOverlap(char* input0,
                                           int numRead0,
                                           int numHap0,
                                           float* output0,
                                           char* input1,
                                           int numRead1,
                                           int numHap1,
                                           float* output1,
                                           char* input2,
                                           int numRead2,
                                           int numHap2,
                                           float* output2) {
    printf("Overlap solution \n");
    int err;
    struct timespec time1, time2, time_diff;
    cl_event event_kernel[3];
    cl_event event_write;
    cl_event event_read;
    clock_gettime(CLOCK_REALTIME, &time1);

    int narg = 0;
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_input_buffer[0]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(cl_mem), &_output_buffer[0]);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numRead0);
    clSetKernelArg(_pmm_kernel[0], narg++, sizeof(int), &numHap0);

    narg = 0;
    clSetKernelArg(_pmm_kernel[1], narg++, sizeof(cl_mem), &_input_buffer[1]);
    clSetKernelArg(_pmm_kernel[1], narg++, sizeof(cl_mem), &_output_buffer[1]);
    clSetKernelArg(_pmm_kernel[1], narg++, sizeof(int), &numRead1);
    clSetKernelArg(_pmm_kernel[1], narg++, sizeof(int), &numHap1);

    narg = 0;
    clSetKernelArg(_pmm_kernel[2], narg++, sizeof(cl_mem), &_input_buffer[2]);
    clSetKernelArg(_pmm_kernel[2], narg++, sizeof(cl_mem), &_output_buffer[2]);
    clSetKernelArg(_pmm_kernel[2], narg++, sizeof(int), &numRead2);
    clSetKernelArg(_pmm_kernel[2], narg++, sizeof(int), &numHap2);

    err = clEnqueueMigrateMemObjects(commands[0], DIE_NUM, _input_buffer, 0, 0, NULL, &event_write);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to migrate write to device buffer! Code %d\n", err);
        return false;
    }
    clWaitForEvents(1, &event_write);

    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    cl_write_time += (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);

    clock_gettime(CLOCK_REALTIME, &time1);
    err = clEnqueueTask(commands[0], _pmm_kernel[0], 0, NULL, &event_kernel[0]);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel0! Code %d\n", err);
        return false;
    }
    err = clEnqueueTask(commands[0], _pmm_kernel[1], 0, NULL, &event_kernel[1]);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel1! Code %d\n", err);
        return false;
    }
    err = clEnqueueTask(commands[0], _pmm_kernel[2], 0, NULL, &event_kernel[2]);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to execute kernel2! Code %d\n", err);
        return false;
    }
    clWaitForEvents(3, event_kernel);
    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    double cur_kernel_time = (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);
    if (curNumCell / cur_kernel_time > peak_kernel_gcups) peak_kernel_gcups = curNumCell / cur_kernel_time;
    kernel_time += cur_kernel_time;

    clock_gettime(CLOCK_REALTIME, &time1);
    err = clEnqueueMigrateMemObjects(commands[0], DIE_NUM, _output_buffer, CL_MIGRATE_MEM_OBJECT_HOST, 0, NULL,
                                     &event_read);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to migrate read from device buffer! Code %d\n", err);
        return false;
    }

    clWaitForEvents(1, &event_read);
    clock_gettime(CLOCK_REALTIME, &time2);
    time_diff = diff_time(time1, time2);
    cl_read_time += (double)(time_diff.tv_sec * 1e9 + time_diff.tv_nsec);

    clFlush(commands[0]);
    clFinish(commands[0]);
    clReleaseEvent(event_write);
    clReleaseEvent(event_kernel[0]);
    clReleaseEvent(event_kernel[1]);
    clReleaseEvent(event_kernel[2]);
    return true;
}

double xilPairHMM::get_kernel_time() {
    return kernel_time;
}

void xilPairHMM::computePairhmm(pairhmmInput* input, pairhmmOutput* output, bool& usedFPGA) {
    if (useFPGA) computePairhmmxil(input, output, usedFPGA);
    //  else
    // computePairhmmAVX(input, output, false);
}

xilPairHMM::~xilPairHMM() {
    if (useFPGA) {
        for (int i = 0; i < 3; i++) {
//            clReleaseMemObject(_input_buffer[i]);
//            clReleaseMemObject(_output_buffer[i]);
//            clReleaseKernel(_pmm_kernel[i]);
            free(host_input[i]);
            free(host_output[i]);
        }
        free(m2m_table);
        host_raw_output.clear();
//        clReleaseProgram(program);
//        clReleaseCommandQueue(commands[0]);
//        clReleaseContext(context);
    }
//    printf("total data prepare time is %e secs\n", data_prepare_time * 1e-9);
//    printf("total OpenCL write buffer time is %e secs\n", cl_write_time * 1e-9);
//    printf("total kernel time is %e secs\n", kernel_time * 1e-9);
//    printf("total OpenCL read buffer time is %e secs\n", cl_read_time * 1e-9);
//    printf("total recompute time is %e secs\n", recompute_time * 1e-9);
//    printf("total time is %e secs\n",
//           (data_prepare_time + cl_write_time + kernel_time + cl_read_time + recompute_time) * 1e-9);
//    printf("peak kernel gcups is %2.1f\n", peak_kernel_gcups);
}
