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
#include "matcharray.h"
#include "logger.h"
#include "sw.h"
#include "genseq.hpp"
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string.h>

using namespace sda;
using namespace std;

extern void uintTouint2Array(int bufferSz, unsigned int* buffer, short* buffer2b);

MatchArray::MatchArray(int nSamples, int rdSz, int rfSz) {
    readSize = rdSz;
    refSize = rfSz;
    numSamples = nSamples;

    readStrings = new char*[numSamples];
    for (int i = 0; i < numSamples; ++i) {
        readStrings[i] = new char[rdSz + 1];
    }
    refStrings = new char*[nSamples];
    for (int i = 0; i < numSamples; ++i) {
        refStrings[i] = new char[rfSz + 1];
    }
    maxv = new short[nSamples];
    maxr = new short[nSamples];
    maxc = new short[nSamples];

    m_counter = 0;
}

MatchArray::~MatchArray() {
    delete[] maxv;
    delete[] maxr;
    delete[] maxc;
}

void MatchArray::populateArray(unsigned int* pairs, unsigned int* maxval) {
    int numInt = READREFUINTSZ(readSize, refSize);
    short* readSeqT = new short[readSize];
    short* refSeqT = new short[refSize];
    for (int i = 0; i < numSamples; ++i) {
        unsigned int* onepair = pairs + numInt * i;
        uintTouint2Array(readSize / UINTNUMBP, onepair, readSeqT);
        uintTouint2Array(refSize / UINTNUMBP, onepair + readSize / UINTNUMBP, refSeqT);
        for (int j = 0; j < readSize; ++j) {
            readStrings[i][j] = bases[readSeqT[j]];
        }
        readStrings[i][readSize] = '\0';
        for (int j = 0; j < refSize; ++j) {
            refStrings[i][j] = bases[refSeqT[j]];
        }
        refStrings[i][refSize] = '\0';
        unsigned int* oneval = maxval + 3 * i;
        maxr[i] = oneval[0];
        maxc[i] = oneval[1];
        maxv[i] = oneval[2];
    }
    delete[] readSeqT;
    delete[] refSeqT;
}
