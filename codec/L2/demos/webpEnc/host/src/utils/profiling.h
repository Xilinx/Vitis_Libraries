#ifndef WEBP_PROFILING_H_
#define WEBP_PROFILING_H_

#include "webp/types.h"

// Decode profiling
// lossy
extern double timeWebPDecode;
extern int countWebPDecode;
extern double timeGetCoeffs;
extern int countGetCoeffs;
extern double timeDecodeInto;
extern int countDecodeInto;
extern double timeVP8ProcessRow;
extern int countVP8ProcessRow;
extern double timeFinishRow;
extern int countFinishRow;
extern double timeParseFrame;
extern int countParseFrame;
extern double timeVP8DecodeMB;
extern int countVP8DecodeMB;
extern double timeVP8ParseIntraModeRow;
extern int countVP8ParseIntraModeRow;
extern double timeReconstructRow;
extern int countReconstructRow;
extern double timeParseResiduals;
extern int countParseResiduals;
extern double timeVP8TransformWHT;
extern int countVP8TransformWHT;
extern double timeParseResidualsIf1;
extern int countParseResidualsIf1;
extern double timeParseResidualsLoop1;
extern int countParseResidualsLoop1;
extern double timeParseResidualsLoop2;
extern int countParseResidualsLoop2;

// lossless
extern double timeVP8LDecodeImage;
extern int countVP8LDecodeImage;
extern double timeDecodeImageData;
extern int countDecodeImageData;
extern double timeProcessRows;
extern int countProcessRows;
extern double timeApplyInverseTransforms;
extern int countApplyInverseTransforms;
extern double timeProcessRowsCopy1;
extern int countProcessRowsCopy1;
extern double timeProcessRowsCopy2;
extern int countProcessRowsCopy2;
extern double timeVP8LAddGreenToBlueAndRed;
extern int countVP8LAddGreenToBlueAndRed;
extern double timeColorSpaceInverseTransform;
extern int countColorSpaceInverseTransform;
extern double timePredictorInverseTransform;
extern int countPredictorInverseTransform;
extern double timeColorIndexInverseTransform;
extern int countColorIndexInverseTransform;

// Encode profiling
// lossless
extern double timeApplyTransforms;
extern int countApplyTransforms;
extern double timeEncode;
extern int countEncode;
extern double timeVP8ApplyNearLossless;
extern int countVP8ApplyNearLossless;
extern double timeVP8ApplyNearLosslessOcl;
extern int countVP8ApplyNearLosslessOcl;
extern double timeApplySubtractGreen;
extern int countApplySubtractGreen;
extern double timeWebPEncode;
extern int countWebPEncode;
extern double timeEncPredict;
extern int countEncPredict;
extern double timeVP8LResidualImage;
extern int countVP8LResidualImage;
extern double timeGetPredModesResiduleForTile;
extern int countGetPredModesResiduleForTile;
extern double timeGetBestPredictorForTile2;
extern int countGetBestPredictorForTile2;
extern double timeBestPredict;
extern int countBestPredict;
extern double timeEncColorFilt;
extern int countEncColorFilt;
extern double timeBestColor;
extern int countBestColor;
extern double timeGetBackRef;
extern int countGetBackRef;
extern double timeBackwardRefLz77;
extern int countBackwardRefLz77;
extern double timeBackwardRefRle;
extern int countBackwardRefRle;
extern double timeGetHistoImg;
extern int countGetHistoImg;
extern double timeHistogramCombineStochastic;
extern int countHistogramCombineStochastic;
extern double timeHistogramCombineStochastic_forloop;
extern int countHistogramCombineStochastic_forloop;
extern double timeHistogramAddEval;
extern int countHistogramAddEval;
extern double timeHistogramRemap;
extern int countHistogramRemap;

// lossy
extern double timeEncAnalyze;
extern int countEncAnalyze;
extern double timeEncAnalyzeOcl;
extern int countEncAnalyzeOcl;
extern double timeEncTokenLoop;
extern int countEncTokenLoop;
extern double timeEncLoop;
extern int countEncLoop;
extern double timeEncLoopOcl;
extern int countEncLoopOcl;
extern int StatLoopFlag;
extern double timeStatLoop;
extern int countStatLoop;
extern double timeVP8Decimate;
extern int countVP8Decimate;
extern double timeVP8EmitTokens;
extern int countVP8EmitTokens;
extern double timeCodeResiduals;
extern int countCodeResiduals;
extern double timeVP8Decimate_2;
extern int countVP8Decimate_2;
extern double timeStoreFilterSts;
extern int countStoreFilterSts;
extern double timeVP8Decimate_BestIntra;
extern int countVP8Decimate_BestIntra;
extern double timeBestIntra16;
extern int countBestIntra16;
extern double timeBestIntra4;
extern int countBestIntra4;
extern double timeBestUV;
extern int countBestUV;
extern double timeRefineUsingDist;
extern int countRefineUsingDist;
extern double timeRefineUsingDist_2;
extern int countRefineUsingDist_2;
#if defined _WIN32 && !defined __GNUC__
#include <windows.h>

typedef LARGE_INTEGER StopProfilingWatch;

static WEBP_INLINE void StartProfiling(StopProfilingWatch* watch) {
    QueryPerformanceCounter(watch);
}

static WEBP_INLINE double StopProfiling(StopProfilingWatch* watch) {
    const LARGE_INTEGER old_value = *watch;
    LARGE_INTEGER freq;
    if (!QueryPerformanceCounter(watch)) return 0.0;
    if (!QueryPerformanceFrequency(&freq)) return 0.0;
    if (freq.QuadPart == 0) return 0.0;
    return (watch->QuadPart - old_value.QuadPart) / (double)freq.QuadPart;
}

#else               /* !_WIN32 */
#include <string.h> // memcpy
#include <sys/time.h>

typedef struct timeval StopProfilingWatch;

static WEBP_INLINE void StartProfiling(StopProfilingWatch* watch) {
    gettimeofday(watch, NULL);
}

static WEBP_INLINE double StopProfiling(StopProfilingWatch* watch, double* total_time, int* count) {
    struct timeval old_value;
    double delta_sec, delta_usec;
#if 1
    old_value.tv_sec = watch->tv_sec;
    old_value.tv_usec = watch->tv_usec;
#else
    memcpy(&old_value, watch, sizeof(old_value));
#endif
    gettimeofday(watch, NULL);
    delta_sec = (double)watch->tv_sec - old_value.tv_sec;
    delta_usec = (double)watch->tv_usec - old_value.tv_usec;
    double cur_time = delta_sec + delta_usec / 1000000.0;
    cur_time *= 1000;
    *total_time += cur_time;
    (*count)++;

    return cur_time;
}

void DisplayDecodeProfilingResult();
void DisplayEncodeProfilingResult();
void ResetDecodeProfilingData();
void ResetEncodeProfilingData();

#endif /* _WIN32 */

#endif /* WEBP_PROFILING_H_ */
