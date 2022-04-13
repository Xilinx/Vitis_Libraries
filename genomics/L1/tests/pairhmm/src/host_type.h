#ifndef HOST_TYPE_H
#define HOST_TYPE_H
#if 0
#if defined(_MSC_VER)
#include <intrin.h> // SIMD intrinsics for Windows
#else
#include <x86intrin.h> // SIMD intrinsics for GCC
#endif
#endif
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <string>

#include "headers.h"
#include "common.hpp"

#define CAT(X, Y) X##Y
#define CONCAT(X, Y) CAT(X, Y)

#define MIN_ACCEPTED 1e-28f
#define NUM_DISTINCT_CHARS 5
#define AMBIG_CHAR 4

#define TRANS_PROB_ARRAY_LENGTH 6

#define TRANSITION_matchToMatch 0
#define TRANSITION_indelToMatch 1
#define TRANSITION_matchToInsertion 2
#define TRANSITION_insertionToInsertion 3
#define TRANSITION_matchToDeletion 4
#define TRANSITION_deletionToDeletion 5

#define MM 0
#define GapM 1
#define MX 2
#define XX 3
#define MY 4
#define YY 5

#define MAX_QUAL 254
#define MAX_JACOBIAN_TOLERANCE 8.0
#define JACOBIAN_LOG_TABLE_STEP 0.0001
#define JACOBIAN_LOG_TABLE_INV_STEP (1.0 / JACOBIAN_LOG_TABLE_STEP)
#define MAXN 70000
#define LOG10_CACHE_SIZE (4 * MAXN) // we need to be able to go up to 2*(2N) when calculating some of the coefficients
#define JACOBIAN_LOG_TABLE_SIZE ((int)(MAX_JACOBIAN_TOLERANCE / JACOBIAN_LOG_TABLE_STEP) + 1)

#define SET_MATCH_TO_MATCH_PROB(output, insQual, delQual)                                                              \
    {                                                                                                                  \
        int minQual = delQual;                                                                                         \
        int maxQual = insQual;                                                                                         \
        if (insQual <= delQual) {                                                                                      \
            minQual = insQual;                                                                                         \
            maxQual = delQual;                                                                                         \
        }                                                                                                              \
        (output) = (MAX_QUAL < maxQual)                                                                                \
                       ? ((NUMBER)1.0) -                                                                               \
                             ctx.POW(((NUMBER)10),                                                                     \
                                     ctx.approximateLog10SumLog10(((NUMBER)-0.1) * minQual, ((NUMBER)-0.1) * maxQual)) \
                       : ctx.matchToMatchProb[((maxQual * (maxQual + 1)) >> 1) + minQual];                             \
    }

using namespace std;

struct timespec diff_time(struct timespec start, struct timespec end);

typedef struct {
    int rslen, haplen;
    const char *q, *i, *d, *c;
    const char *hap, *rs;
} testcase;

typedef struct {
    int hapLen;
    float oneDivHapLen;
} hapLenPack;

typedef struct {
    uint32_t scores; // 31:28 readBases, 27:21 readQuals, 20:14 insertionGOP, 13:7 deletionGOP, 6:0 overallGCP
    float m2m;
} readDataPack;

typedef struct {
    uint64_t readInfo[MAX_RSDATA_NUM / READ_BLOCK_SIZE];
    readDataPack readData[MAX_RSDATA_NUM][MAX_READ_LEN];
    hapLenPack hapDataLen[MAX_HAPDATA_NUM];
    uint16_t hapData[MAX_HAPDATA_NUM / HAP_BLOCK_SIZE][MAX_HAP_LEN];
    uint16_t numReadPU[64]; // numRead per PU
    uint64_t iterNum[64];   // iterNum per PU
} InputDataPackOpt;

typedef struct {
    InputDataPackOpt dataPack;
    int numRead;
    int numHap;
} FPGAInput;

typedef struct {
    string bases;
    string _q;
    string _i;
    string _d;
    string _c;
} Read;

typedef struct { string bases; } Hap;

typedef struct {
    vector<Read> reads;
    vector<Hap> haps;
} pairhmmInput;

typedef struct { vector<double> likelihoodData; } pairhmmOutput;

class ConvertChar {
    static uint8_t conversionTable[255];

   public:
    static void init() {
        assert(NUM_DISTINCT_CHARS == 5);
        assert(AMBIG_CHAR == 4);

        conversionTable['A'] = 0;
        conversionTable['C'] = 1;
        conversionTable['T'] = 2;
        conversionTable['G'] = 3;
        conversionTable['N'] = 4;
    }

    static inline uint8_t get(uint8_t input) { return conversionTable[input]; }
};

#endif // PAIRHMM_COMMON_H
