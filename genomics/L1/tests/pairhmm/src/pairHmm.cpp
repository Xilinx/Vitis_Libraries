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
            curInfo.readLen[0] = cur_read_len;
            curInfo.big_rows = cur_read_len + 1;
            curInfo.oneOrTwo = false;
            curInfo.resultOffset = cur_numHap * i;
            curInfo.readID[0] = i;
            if (i + 1 < host_input[k]->numRead) {
                cur_read_len = input->reads[i + 1 + read_start_index + read_base_index].bases.size();
#if 0
                if(cur_read_len > MAX_READ_LEN){
                    violate = true;
                    return;
                }
#endif
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
    for (int i = 0; i < DIE_NUM; i++) {
        host_input[i]->numHap = cur_numHap;
    }

    short maxCols = 0;
    for (int i = 0; i < cur_numHap; i++) {
#if 0
        if(input->haps[i + hap_base_index].bases.size() > MAX_HAP_LEN){
            violate = true;
            return;
        }
#endif
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

int printYROW1() {
    Context<float> g_ctxf;
    FILE* fp = fopen("yrow1.h", "w");
    fprintf(fp, "#define YROW1_INIT {");
    for (int i = 0; i < 512; i++) {
        if (i == 0)
            fprintf(fp, "INFINITY");
        else
            fprintf(fp, "%10.60f", g_ctxf.INITIAL_CONSTANT / (float)i);
        if (i != 511) fprintf(fp, ", ");
    }
    fprintf(fp, "};");
    fclose(fp);
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
    //_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    float m2m[128 * 128] = M2M_INIT;
    m2m_table = (float*)malloc(128 * 128 * sizeof(float));
    for (int i = 0; i < 128 * 128; i++) {
        m2m_table[i] = m2m[i];
    }
    for (int i = 0; i < 3; i++) {
        printf("sizeof FPGAInput:%d\n", sizeof(FPGAInput));
        host_input[i] = (FPGAInput*)memalign(4096, sizeof(FPGAInput));
        host_output[i] = (float*)memalign(4096, sizeof(float) * MAX_RSDATA_NUM * MAX_HAPDATA_NUM);
    }
    printf("depth of input is %ld 512-bit, %ld bytes \n", sizeof(InputDataPackOpt) / 64, sizeof(InputDataPackOpt));
}

xilPairHMM::~xilPairHMM() {
    if (useFPGA) {
        for (int i = 0; i < 3; i++) {
            free(host_input[i]);
            free(host_output[i]);
        }
        free(m2m_table);
        host_raw_output.clear();
    }
}
