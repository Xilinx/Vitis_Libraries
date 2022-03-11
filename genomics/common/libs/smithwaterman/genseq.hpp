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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define _COMPUTE_FULL_MATRIX 1
#include "matcharray.h"
#include "sw.h"

float insProb = 0.10;
float delProb = 0.10;
float mutProb = 0.10;

typedef enum { BP, MUTATE, INSERT, DELETE } command_e;

command_e command() {
    int val = rand() % 100;
    static int insVal = insProb * 100;
    static int delVal = delProb * 100;
    static int mutVal = mutProb * 100;
    static int insLevel = 0 + insVal;
    static int delLevel = insLevel + delVal;
    static int mutLevel = delLevel + mutVal;

    if (val < insLevel) {
        return INSERT;
    } else if (val < delLevel) {
        return DELETE;
    } else if (val < mutLevel) {
        return MUTATE;
    }
    return BP;
}

void genSeq(int readSize, int refSize, int readLoc, short* readSeq, short* refSeq) {
    int readCount = 0;
    int i;
    for (i = 0; i < refSize; ++i) {
        int bpid = rand() % 4;
        refSeq[i] = bpid;
        if (i >= readLoc && readCount < readSize) {
            switch (command()) {
                case INSERT: {
                    readSeq[readCount++] = rand() % 4;
                } break;
                // fallthrough...
                case BP: {
                    readSeq[readCount++] = bpid;
                } break;
                case DELETE:
                case MUTATE: {
                    readSeq[readCount++] = (bpid + rand() % 3) % 4;
                } break;
            }
        }
    }
}

void printSeq(int sz, short* d) {
    int i;
    for (i = 0; i < sz; ++i) {
        printf("%c", bases[d[i]]);
    }
    printf("\n");
}

void makeSeq(int readSize, int refSize, short* readSeq, short* refSeq) {
    int loc = rand() % (refSize - readSize + 1);
    genSeq(readSize, refSize, loc, readSeq, refSeq);
}

void printMatrix(int readSize, int refSize, short** mat, char* msg) {
    printf("Weight Matrix: %s\n", msg);
    int row, col;
    for (row = 0; row < readSize; ++row) {
        for (col = 0; col < refSize; col++) {
            printf("%d ", mat[row][col]);
        }
        printf("\n");
    }
}

void computeMatrix(
    int readSize, int refSize, short* readSeq, short* refSeq, short** mat, short* maxr, short* maxc, short* maxv) {
    *maxv = MINVAL;
    int row, col;
    for (col = 0; col < refSize; col++) {
        short d = refSeq[col];
        for (row = 0; row < readSize; ++row) {
            short n, nw, w;
            if (row == 0) {
                n = 0;
            } else {
                n = mat[row - 1][col];
            }
            if (col == 0) {
                w = 0;
            } else {
                w = mat[row][col - 1];
            }

            if (row > 0 && col > 0) {
                nw = mat[row - 1][col - 1];
            } else {
                nw = 0;
            }

            short q = readSeq[row];
            short max = 0;
            short match = (d == q) ? MATCH : MISS_MATCH;
            short t1 = (nw + match > max) ? nw + match : max;
            short t2 = (n + GAP > w + GAP) ? n + GAP : w + GAP;
            max = t1 > t2 ? t1 : t2;
            mat[row][col] = max;
            if (max > *maxv) {
                *maxv = max;
                *maxr = row;
                *maxc = col;
            }
        }
    }
}

void compareMatrix(int readSize, int refSize, short** matRef, short** matComp) {
    int error = 0;
    int row, col;
    for (row = 0; row < readSize; ++row) {
        for (col = 0; col < refSize; col++) {
            if (matRef[row][col] != matComp[row][col]) {
                printf("Difference at (%d, %d). Ref=%d, Computed=%d\n", row, col, matRef[row][col], matComp[row][col]);
                error++;
            }
        }
    }
    if (error) {
        printf("FAIL: %d values do not match\n", error);
    } else {
        printf("PASS: All values match\n");
    }
}

// A-0, C-1, G-2, T-3
short** buildMat(int readSize, int refSize) {
    short** mat;
    mat = new short*[readSize];
    int r;
    for (r = 0; r < readSize; ++r) {
        mat[r] = new short[refSize];
    }
    return mat;
}

void deleteMat(int readSize, int refSize, short*** mat) {
    int r;
    for (r = 0; r < readSize; ++r) {
        delete[](*mat)[r];
    }

    delete[] * mat;
}

void uintTouint2Array(int bufferSz, unsigned int* buffer, short* buffer2b) {
    int i, j;
    for (i = 0; i < bufferSz * 16; ++i) {
        buffer2b[i] = 0;
    }
    for (i = 0; i < bufferSz; ++i) {
        unsigned int packedV = buffer[i];
        for (j = 0; j < 16; ++j) {
            buffer2b[16 * i + j] = packedV & 3;
            packedV = packedV >> 2;
        }
    }
}

void uint2TouintArray(int buffer2bSz, short* buffer2b, unsigned int* buffer) {
    int i, j;
    for (i = 0; i < buffer2bSz / 16; ++i) {
        buffer[i] = 0;
    }
    for (i = 0; i < buffer2bSz / 16; ++i) {
        unsigned int packedV = 0;
        for (j = 0; j < 16; ++j) {
            unsigned int tmp = buffer2b[16 * i + j];
            tmp = tmp << j * 2;
            packedV |= tmp;
        }
        buffer[i] = packedV;
    }
}

void compareSeq(int sz, short* seq, short* seqT, char* s) {
    int fail = 0;
    int i;
    for (i = 0; i < sz; ++i) {
        if (seq[i] != seqT[i]) {
            fail = 1;
            printf("%s:FAIL:%d:%c:%c\n", s, i, bases[seq[i]], bases[seqT[i]]);
        }
    }
    if (fail == 0) {
        printf("%s:PASS\n", s);
    }
}

unsigned int* generatePackedNReadRefPair(
    int N, int readSize, int refSize, unsigned int** maxVal, int computeOutput = 1) {
    int numInt = READREFUINTSZ(readSize, refSize);
    unsigned int* pairs = new unsigned int[N * numInt];
    short* readSeq = new short[readSize];
    short* refSeq = new short[refSize];
    short* readSeqT = new short[readSize];
    short* refSeqT = new short[refSize];
    unsigned int* readSeqP = new unsigned int[readSize / UINTNUMBP];
    unsigned int* refSeqP = new unsigned int[refSize / UINTNUMBP];
    short** matRef = buildMat(readSize, refSize);
    *maxVal = new unsigned int[N * 3];
    int i;
    for (i = 0; i < N; ++i) {
        short maxv, maxr, maxc;
        maxv = 0;
        maxc = 0;
        maxr = 0;
        int offset = numInt * i;
        // make seq
        makeSeq(readSize, refSize, readSeq, refSeq);
        // compute max ref value
        if (computeOutput) {
            computeMatrix(readSize, refSize, readSeq, refSeq, matRef, &maxr, &maxc, &maxv);
            (*maxVal)[3 * i + 0] = maxr;
            (*maxVal)[3 * i + 1] = maxc;
            (*maxVal)[3 * i + 2] = maxv;
        }
        // convert to packed
        uint2TouintArray(readSize, readSeq, readSeqP);
        uint2TouintArray(refSize, refSeq, refSeqP);
        memcpy((pairs + offset), readSeqP, sizeof(unsigned int) * readSize / UINTNUMBP);
        uintTouint2Array(readSize / UINTNUMBP, (pairs + offset), readSeqT);
        memcpy((pairs + offset + readSize / UINTNUMBP), refSeqP, sizeof(unsigned int) * refSize / UINTNUMBP);
        uintTouint2Array(refSize / UINTNUMBP, (pairs + offset + readSize / UINTNUMBP), refSeqT);
    }
    delete[] readSeqP;
    delete[] refSeqP;
    delete[] readSeq;
    delete[] refSeq;
    delete[] readSeqT;
    delete[] refSeqT;
    deleteMat(readSize, refSize, &matRef);
    return pairs;
}

void writeReadRefFile(char* fname, unsigned int* pairs, unsigned int* maxVals, int N) {
    FILE* fp = fopen(fname, "w");
    fprintf(fp, "rdsz,%d\n", MAXROW);
    fprintf(fp, "refsz,%d\n", MAXCOL);
    fprintf(fp, "samples,%d\n", N);
    for (size_t i = 0; i < (size_t)N; ++i) {
        fprintf(fp, "S%lu,", i);
        for (size_t j = 0; j < PACKEDSZ; ++j) {
            fprintf(fp, "%u,", pairs[i * PACKEDSZ + j]);
        }
        for (size_t j = 0; j < 3; ++j) {
            if (j == 2) {
                fprintf(fp, "%u\n", maxVals[i * 3 + j]);
            } else {
                fprintf(fp, "%u,", maxVals[i * 3 + j]);
            }
        }
    }
    fclose(fp);
}

int getToken(FILE* fp, char* tok) {
    int pos = 0;
    char ch;
    while ((ch = (char)(fgetc(fp)))) {
        if (ch == EOF) {
            return 0;
        }
        if (ch == ' ') {
            continue;
        }
        if (ch == ',' || ch == '\n') {
            tok[pos] = '\0';
            return 1;
        }
        tok[pos++] = ch;
    }
    return -1;
}

int readReadRefFile(char* fname, unsigned int** pairs, unsigned int** maxv, int N) {
    FILE* fp = fopen(fname, "r");
    char* string = new char[1024];
    int rdSz = 0;
    int refSz = 0;
    int sampleNum = 0;
    int numInt = 0;
    int numSamples;
    while ((sampleNum < N) && getToken(fp, string)) {
        if (!strcmp(string, "rdsz")) {
            getToken(fp, string);
            rdSz = atoi(string);
        }
        if (!strcmp(string, "refsz")) {
            getToken(fp, string);
            refSz = atoi(string);
        }
        if (!strcmp(string, "samples")) {
            getToken(fp, string);
            numSamples = atoi(string);
            assert(N <= numSamples);
            printf("Reading %d samples out of %d in the file\n", N, numSamples);
            numInt = READREFUINTSZ(rdSz, refSz);
            *pairs = new unsigned int[N * numInt];
            *maxv = new unsigned int[3 * N];
        }
        if (string[0] == 'S') {
            for (int p = 0; p < numInt; ++p) {
                getToken(fp, string);
                unsigned int val = (unsigned int)atoll(string);
                (*pairs)[sampleNum * numInt + p] = val;
            }
            for (int p = 0; p < 3; ++p) {
                getToken(fp, string);
                unsigned int val = (unsigned int)atoll(string);
                (*maxv)[sampleNum * 3 + p] = val;
            }
            sampleNum++;
        }
    }
    delete[] string;
    fclose(fp);
    return numSamples;
}

void printPackedNReadRefPair(unsigned int* pairs, int N, int readSize, int refSize) {
    int numInt = READREFUINTSZ(readSize, refSize);
    short* readSeq = new short[readSize];
    short* refSeq = new short[refSize];
    unsigned int* readSeqP = new unsigned int[readSize / UINTNUMBP];
    unsigned int* refSeqP = new unsigned int[refSize / UINTNUMBP];

    int i;
    for (i = 0; i < N; ++i) {
        int offset = i * numInt;
        int readSz = readSize / UINTNUMBP;
        int refSz = refSize / UINTNUMBP;
        memcpy(readSeqP, (pairs + offset), sizeof(unsigned int) * readSz);
        uintTouint2Array(readSize / UINTNUMBP, readSeqP, readSeq);
        printf("Read%d:", i);
        printSeq(readSize, readSeq);
        memcpy(refSeqP, (pairs + offset + readSz), sizeof(unsigned int) * refSz);
        uintTouint2Array(refSize / UINTNUMBP, refSeqP, refSeq);
        printf("Ref %d:", i);
        printSeq(refSize, refSeq);
    }
    delete[] readSeqP;
    delete[] refSeqP;
    delete[] readSeq;
    delete[] refSeq;
}

#define TESTSZ 64
void testuintConv() {
    short d[TESTSZ];
    short q[TESTSZ];
    short out[TESTSZ];
    unsigned int dI[TESTSZ / 16];
    int cnt, i;
    for (cnt = 0; cnt < 100; ++cnt) {
        makeSeq(TESTSZ / 2, TESTSZ, d, q);
        printSeq(TESTSZ, q);
        uint2TouintArray(TESTSZ, q, dI);
        uintTouint2Array(TESTSZ / 16, dI, out);
        int error = 0;
        for (i = 0; i < TESTSZ; ++i) {
            if (q[i] != out[i]) {
                printf("FAIL!!\n");
                printSeq(TESTSZ, q);
                printSeq(TESTSZ, out);
                error = 1;
            }
        }

        if (!error) {
            printf("PASS!!\n");
            printSeq(TESTSZ, q);
            printSeq(TESTSZ, out);
        }
    }
}
