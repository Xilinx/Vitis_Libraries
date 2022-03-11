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
#include <vector>
#include <string>
#include <string.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <cmath>
#include <random>

double avg_numReads;
double avg_numHaps;
int case_counter;

char GenRandBase() {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 3);
    int number = distribution(generator);
    if (number == 0)
        return 'A';
    else if (number == 1)
        return 'T';
    else if (number == 2)
        return 'C';
    else
        return 'G';
}

int GenQuals() {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(30.0, 5.0);
    int quals = distribution(generator);
    if (quals < 6) quals = 6;
    return quals;
}

int GenInDel() {
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(40.0, 1.0);
    int quals = distribution(generator);
    if (quals < 1) quals = 1;
    return quals;
}

int GenLen(int limit) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(limit / 4, limit);
    int number = distribution(generator);
    return number;
}

int GenInputs(pairhmmInput* in, int size) {
    in->reads.clear();
    in->haps.clear();
    in->reads.resize(16 * (size + 1));
    in->haps.resize((size + 1));
    printf("%s - readsize %d \n", __FUNCTION__, in->reads.size());
    printf("%s - readsize %d \n", __FUNCTION__, in->haps.size());
    for (int i = 0; (size_t)i < in->reads.size(); i++) {
        Read& curRead = in->reads[i];
        for (int j = 0; j < GenLen(MAX_READ_LEN); j++) {
            curRead.bases.push_back(GenRandBase());
            curRead._q.push_back(GenQuals());
            curRead._i.push_back(GenInDel());
            curRead._d.push_back(GenInDel());
            curRead._c.push_back(10);
        }
    }
    for (int i = 0; (size_t)i < in->haps.size(); i++) {
        Hap& curHap = in->haps[i];
        for (int j = 0; j < GenLen(MAX_HAP_LEN); j++) {
            curHap.bases.push_back(GenRandBase());
        }
    }
    return 0;
}

#if 0
int GenOutputs(pairhmmInput* in, pairhmmOutput* out){
    xilPairHMM* accelPhmm = new xilPairHMM();
    accelPhmm->computePairhmmAVX(in, out, false);
    delete accelPhmm;
    return 0;
}
#endif

int GetInputs(pairhmmInput* in, std::string filename) {
    std::ifstream ifs(filename.c_str(), std::ifstream::in);
    int numReads;
    int numHaplotypes;
    // thie first line is number of reads and number of haplotypes
    char lineBuf[1024];
    if (!ifs.good()) printf("bad file name %s\n", filename.c_str());
    ifs.getline(lineBuf, 1024);
    char* token;
    token = strtok(lineBuf, " ");
    token = strtok(NULL, " ");
    numReads = atoi(token);
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    numHaplotypes = atoi(token);
    // start from the second line are all the reads
    for (int i = 0; i < numReads; ++i) {
        int curReadLen;
        Read curRead;
        ifs.getline(lineBuf, 1024);
        curReadLen = atoi(lineBuf);
        ifs.getline(lineBuf, 1024);
        ifs.getline(lineBuf, 1024);
        for (int j = 0; j < curReadLen; j++) {
            if (j == 0)
                token = strtok(lineBuf, " ");
            else
                token = strtok(NULL, " ");
            curRead.bases.push_back((char)atoi(token));
        }
        ifs.getline(lineBuf, 1024);
        ifs.getline(lineBuf, 1024);
        for (int j = 0; j < curReadLen; j++) {
            if (j == 0)
                token = strtok(lineBuf, " ");
            else
                token = strtok(NULL, " ");
            curRead._q.push_back((char)atoi(token));
        }
        ifs.getline(lineBuf, 1024);
        ifs.getline(lineBuf, 1024);
        for (int j = 0; j < curReadLen; j++) {
            if (j == 0)
                token = strtok(lineBuf, " ");
            else
                token = strtok(NULL, " ");
            curRead._i.push_back((char)atoi(token));
        }
        ifs.getline(lineBuf, 1024);
        ifs.getline(lineBuf, 1024);
        for (int j = 0; j < curReadLen; j++) {
            if (j == 0)
                token = strtok(lineBuf, " ");
            else
                token = strtok(NULL, " ");
            curRead._d.push_back((char)atoi(token));
        }
        ifs.getline(lineBuf, 1024);
        ifs.getline(lineBuf, 1024);
        for (int j = 0; j < curReadLen; j++) {
            if (j == 0)
                token = strtok(lineBuf, " ");
            else
                token = strtok(NULL, " ");
            curRead._c.push_back((char)atoi(token));
        }
        in->reads.push_back(curRead);
    }
    ifs.getline(lineBuf, 1024);
    for (int i = 0; i < numHaplotypes; ++i) {
        int curHapLen;
        Hap curHap;
        ifs.getline(lineBuf, 1024);
        curHapLen = atoi(lineBuf);
        ifs.getline(lineBuf, 1024);
        ifs.getline(lineBuf, 1024);
        for (int j = 0; j < curHapLen; j++) {
            curHap.bases.push_back(lineBuf[j]);
        }
        in->haps.push_back(curHap);
    }
    ifs.close();
    return 0;
}

int GetOutputs(pairhmmOutput* out, int outputSize, std::string filename) {
    std::ifstream ifs(filename.c_str(), std::ifstream::in);
    if (!ifs.good()) printf("bad file name %s\n", filename.c_str());
    for (int i = 0; i < outputSize; ++i) {
        double ref;
        ifs >> ref;
        union {
            long long i;
            double d;
        } value;
        ifs >> value.i;
        out->likelihoodData.push_back(value.d);
    }
    return 0;
}

int cmp(pairhmmOutput* target,
        pairhmmOutput* golden,
        int size,
        int test_id,
        bool exact_match,
        double& total_error_count,
        double& largest_error) {
    int error_count = 0;
    for (int i = 0; i < size; i++) {
        if (exact_match) {
            if (target->likelihoodData[i] != golden->likelihoodData[i]) {
                printf("errors in %d th data of %d test, target is %f, golden is %f\n", i, test_id,
                       target->likelihoodData[i], golden->likelihoodData[i]);
                error_count++;
            }
        } else {
            if (std::isnan(target->likelihoodData[i])) {
                printf("error, target is nan\n");
                error_count++;
            }
            double cur_error =
                fabs((target->likelihoodData[i] - golden->likelihoodData[i]) / golden->likelihoodData[i]);
            if (cur_error > largest_error) {
                largest_error = cur_error;
            }
            if (cur_error > 5e-3) {
                printf("%dth test: %dth result has significant error, golden=%f, target=%f\n", test_id, i,
                       golden->likelihoodData[i], target->likelihoodData[i]);
                error_count++;
            }
        }
    }
    if (error_count > 0) {
        printf("%d out of %d have significant error\n", error_count, size);
    }

    total_error_count = total_error_count + error_count;
    return 0;
}

double countCells(pairhmmInput* input) {
    double numCell = 0;
    for (int i = 0; (size_t)i < input->reads.size(); i++) {
        for (int j = 0; (size_t)j < input->haps.size(); j++) {
            double cur_numCell = input->reads[i].bases.size() * input->haps[j].bases.size();
            numCell += cur_numCell;
        }
    }
    return numCell;
}

void printHelp() {
    printf("./test_bin -v or ./test_bin --version: get the compatible platform for current host binary\n");
    printf("./test_bin -h or ./test_bin --help: print the help information\n");
    printf("./test_bin [bitstream filename] --real [real cases folder]: run real tests\n");
    printf("./test_bin [bitstream filename] --syn [syn cases number]: run synthetic tests\n");
}
