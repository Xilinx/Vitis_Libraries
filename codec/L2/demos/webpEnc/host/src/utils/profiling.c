#include <stdio.h>
#include "profiling.h"

// Decode profiling

// lossy
double timeWebPDecode = 0;
int countWebPDecode = 0;
double timeGetCoeffs = 0;
int countGetCoeffs = 0;
double timeDecodeInto = 0;
int countDecodeInto = 0;
double timeVP8ProcessRow = 0;
int countVP8ProcessRow = 0;
double timeFinishRow = 0;
int countFinishRow = 0;
double timeParseFrame = 0;
int countParseFrame = 0;
double timeVP8DecodeMB = 0;
int countVP8DecodeMB = 0;
double timeVP8ParseIntraModeRow = 0;
int countVP8ParseIntraModeRow = 0;
double timeReconstructRow = 0;
int countReconstructRow = 0;
double timeParseResiduals = 0;
int countParseResiduals = 0;
double timeVP8TransformWHT = 0;
int countVP8TransformWHT = 0;
double timeParseResidualsIf1 = 0;
int countParseResidualsIf1 = 0;
double timeParseResidualsLoop1 = 0;
int countParseResidualsLoop1 = 0;
double timeParseResidualsLoop2 = 0;
int countParseResidualsLoop2 = 0;

// lossless
double timeVP8LDecodeImage = 0;
int countVP8LDecodeImage = 0;
double timeDecodeImageData = 0;
int countDecodeImageData = 0;
double timeProcessRows = 0;
int countProcessRows = 0;
double timeApplyInverseTransforms = 0;
int countApplyInverseTransforms = 0;
double timeProcessRowsCopy1 = 0;
int countProcessRowsCopy1 = 0;
double timeProcessRowsCopy2 = 0;
int countProcessRowsCopy2 = 0;
double timeVP8LAddGreenToBlueAndRed = 0;
int countVP8LAddGreenToBlueAndRed = 0;
double timeColorSpaceInverseTransform = 0;
int countColorSpaceInverseTransform = 0;
double timePredictorInverseTransform = 0;
int countPredictorInverseTransform = 0;
double timeColorIndexInverseTransform = 0;
int countColorIndexInverseTransform = 0;

void ResetDecodeProfilingData() {
    // lossy
    timeWebPDecode = 0;
    countWebPDecode = 0;
    timeGetCoeffs = 0;
    countGetCoeffs = 0;
    timeDecodeInto = 0;
    countDecodeInto = 0;
    timeVP8ProcessRow = 0;
    countVP8ProcessRow = 0;
    timeFinishRow = 0;
    countFinishRow = 0;
    timeParseFrame = 0;
    countParseFrame = 0;
    timeVP8DecodeMB = 0;
    countVP8DecodeMB = 0;
    timeVP8ParseIntraModeRow = 0;
    countVP8ParseIntraModeRow = 0;
    timeReconstructRow = 0;
    countReconstructRow = 0;
    timeParseResiduals = 0;
    countParseResiduals = 0;
    timeVP8TransformWHT = 0;
    countVP8TransformWHT = 0;
    timeParseResidualsIf1 = 0;
    countParseResidualsIf1 = 0;
    timeParseResidualsLoop1 = 0;
    countParseResidualsLoop1 = 0;
    timeParseResidualsLoop2 = 0;
    countParseResidualsLoop2 = 0;

    // lossless
    timeVP8LDecodeImage = 0;
    countVP8LDecodeImage = 0;
    timeDecodeImageData = 0;
    countDecodeImageData = 0;
    timeProcessRows = 0;
    countProcessRows = 0;
    timeApplyInverseTransforms = 0;
    countApplyInverseTransforms = 0;
    timeProcessRowsCopy1 = 0;
    countProcessRowsCopy1 = 0;
    timeProcessRowsCopy2 = 0;
    countProcessRowsCopy2 = 0;
    timeVP8LAddGreenToBlueAndRed = 0;
    countVP8LAddGreenToBlueAndRed = 0;
    timeColorSpaceInverseTransform = 0;
    countColorSpaceInverseTransform = 0;
    timePredictorInverseTransform = 0;
    countPredictorInverseTransform = 0;
    timeColorIndexInverseTransform = 0;
    countColorIndexInverseTransform = 0;
}

// Display profiling result
void DisplayDecodeProfilingResult() {
    if (0 != countWebPDecode) {
        fprintf(stderr, "WebPDecode Total Time: %.6f ms, count:%d, average time: %.6f ms\n", timeWebPDecode,
                countWebPDecode, timeWebPDecode / countWebPDecode);
    }
    if (0 != countDecodeInto) {
        fprintf(stderr, "  DecodeInto Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeDecodeInto, countDecodeInto, timeDecodeInto / countDecodeInto,
                timeDecodeInto * 100 / timeWebPDecode);
    }

    // lossy start
    if (0 != countParseFrame) {
        fprintf(stderr, "    ParseFrame Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeParseFrame, countParseFrame, timeParseFrame / countParseFrame,
                timeParseFrame * 100 / timeWebPDecode);
    }
    if (0 != countVP8ParseIntraModeRow) {
        fprintf(stderr,
                "      VP8ParseIntraModeRow Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8ParseIntraModeRow, countVP8ParseIntraModeRow,
                timeVP8ParseIntraModeRow / countVP8ParseIntraModeRow, timeVP8ParseIntraModeRow * 100 / timeWebPDecode);
    }
    if (0 != countVP8DecodeMB) {
        fprintf(stderr, "      VP8DecodeMB Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8DecodeMB, countVP8DecodeMB, timeVP8DecodeMB / countVP8DecodeMB,
                timeVP8DecodeMB * 100 / timeWebPDecode);
    }
    if (0 != countParseResiduals) {
        fprintf(stderr,
                "        ParseResiduals Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeParseResiduals, countParseResiduals, timeParseResiduals / countParseResiduals,
                timeParseResiduals * 100 / timeWebPDecode);
    }
    if (0 != countVP8TransformWHT) {
        fprintf(stderr,
                "          VP8TransformWHT Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8TransformWHT, countVP8TransformWHT, timeVP8TransformWHT / countVP8TransformWHT,
                timeVP8TransformWHT * 100 / timeWebPDecode);
    }
    if (0 != countGetCoeffs) {
        fprintf(stderr, "          GetCoeffs Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeGetCoeffs, countGetCoeffs, timeGetCoeffs / countGetCoeffs, timeGetCoeffs * 100 / timeWebPDecode);
    }
    if (0 != countParseResidualsIf1) {
        fprintf(stderr,
                "          ParseResidualsIf1 Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeParseResidualsIf1, countParseResidualsIf1, timeParseResidualsIf1 / countParseResidualsIf1,
                timeParseResidualsIf1 * 100 / timeWebPDecode);
    }
    if (0 != countParseResidualsLoop1) {
        fprintf(
            stderr,
            "          ParseResidualsLoop1 Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
            timeParseResidualsLoop1, countParseResidualsLoop1, timeParseResidualsLoop1 / countParseResidualsLoop1,
            timeParseResidualsLoop1 * 100 / timeWebPDecode);
    }
    if (0 != countParseResidualsLoop2) {
        fprintf(
            stderr,
            "          ParseResidualsLoop2 Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
            timeParseResidualsLoop2, countParseResidualsLoop2, timeParseResidualsLoop2 / countParseResidualsLoop2,
            timeParseResidualsLoop2 * 100 / timeWebPDecode);
    }
    if (0 != countVP8ProcessRow) {
        fprintf(stderr, "      VP8ProcessRow Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8ProcessRow, countVP8ProcessRow, timeVP8ProcessRow / countVP8ProcessRow,
                timeVP8ProcessRow * 100 / timeWebPDecode);
    }
    if (0 != countReconstructRow) {
        fprintf(stderr,
                "        ReconstructRow Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeReconstructRow, countReconstructRow, timeReconstructRow / countReconstructRow,
                timeReconstructRow * 100 / timeWebPDecode);
    }
    if (0 != countFinishRow) {
        fprintf(stderr, "        FinishRow Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeFinishRow, countFinishRow, timeFinishRow / countFinishRow, timeFinishRow * 100 / timeWebPDecode);
    }
    // lossy end

    // lossless start
    // if (0 != countVP8LDecodeImage) {
    //   fprintf(stderr, "  VP8LDecodeImage Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
    //           timeVP8LDecodeImage, countVP8LDecodeImage, timeVP8LDecodeImage / countVP8LDecodeImage,
    //           timeVP8LDecodeImage * 100 / timeWebPDecode);
    // }
    if (0 != countDecodeImageData) {
        fprintf(stderr, "    DecodeImageData Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeDecodeImageData, countDecodeImageData, timeDecodeImageData / countDecodeImageData,
                timeDecodeImageData * 100 / timeWebPDecode);
    }
    if (0 != countProcessRows) {
        fprintf(stderr, "      ProcessRows Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeProcessRows, countProcessRows, timeProcessRows / countProcessRows,
                timeProcessRows * 100 / timeWebPDecode);
    }
    if (0 != countApplyInverseTransforms) {
        fprintf(
            stderr,
            "        ApplyInverseTransforms Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
            timeApplyInverseTransforms, countApplyInverseTransforms,
            timeApplyInverseTransforms / countApplyInverseTransforms,
            timeApplyInverseTransforms * 100 / timeWebPDecode);
    }
    if (0 != countProcessRowsCopy1) {
        fprintf(stderr,
                "          ProcessRowsCopy1 Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeProcessRowsCopy1, countProcessRowsCopy1, timeProcessRowsCopy1 / countProcessRowsCopy1,
                timeProcessRowsCopy1 * 100 / timeWebPDecode);
    }
    if (0 != countVP8LAddGreenToBlueAndRed) {
        fprintf(stderr,
                "          VP8LAddGreenToBlueAndRed Total Time: %.6f ms, count:%d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeVP8LAddGreenToBlueAndRed, countVP8LAddGreenToBlueAndRed,
                timeVP8LAddGreenToBlueAndRed / countVP8LAddGreenToBlueAndRed,
                timeVP8LAddGreenToBlueAndRed * 100 / timeWebPDecode);
    }
    if (0 != countColorSpaceInverseTransform) {
        fprintf(stderr,
                "          ColorSpaceInverseTransform Total Time: %.6f ms, count:%d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeColorSpaceInverseTransform, countColorSpaceInverseTransform,
                timeColorSpaceInverseTransform / countColorSpaceInverseTransform,
                timeColorSpaceInverseTransform * 100 / timeWebPDecode);
    }
    if (0 != countPredictorInverseTransform) {
        fprintf(stderr,
                "          PredictorInverseTransform Total Time: %.6f ms, count:%d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timePredictorInverseTransform, countPredictorInverseTransform,
                timePredictorInverseTransform / countPredictorInverseTransform,
                timePredictorInverseTransform * 100 / timeWebPDecode);
    }
    if (0 != countProcessRowsCopy2) {
        fprintf(stderr,
                "          ProcessRowsCopy2 Total Time: %.6f ms, count:%d, average time: %.6f ms, percentage:%.6f%\n",
                timeProcessRowsCopy2, countProcessRowsCopy2, timeProcessRowsCopy2 / countProcessRowsCopy2,
                timeProcessRowsCopy2 * 100 / timeWebPDecode);
    }
    if (0 != countColorIndexInverseTransform) {
        fprintf(stderr,
                "        ColorIndexInverseTransform Total Time: %.6f ms, count:%d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeColorIndexInverseTransform, countColorIndexInverseTransform,
                timeColorIndexInverseTransform / countColorIndexInverseTransform,
                timeColorIndexInverseTransform * 100 / timeWebPDecode);
    }
    // lossless end
}

// Encode profiling
double timeWebPEncode = 0;
int countWebPEncode = 0;

// lossless
double timeApplyTransforms = 0;
int countApplyTransforms = 0;
double timeEncode = 0;
int countEncode = 0;
double timeVP8ApplyNearLossless = 0;
int countVP8ApplyNearLossless = 0;
double timeVP8ApplyNearLosslessOcl = 0;
int countVP8ApplyNearLosslessOcl = 0;
double timeApplySubtractGreen = 0;
int countApplySubtractGreen = 0;
double timeEncPredict = 0;
int countEncPredict = 0;
double timeVP8LResidualImage = 0;
int countVP8LResidualImage = 0;
double timeGetPredModesResiduleForTile = 0;
int countGetPredModesResiduleForTile = 0;
double timeGetBestPredictorForTile2 = 0;
int countGetBestPredictorForTile2 = 0;
double timeBestPredict = 0;
int countBestPredict = 0;
double timeEncColorFilt = 0;
int countEncColorFilt = 0;
double timeBestColor = 0;
int countBestColor = 0;
double timeGetBackRef = 0;
int countGetBackRef = 0;
double timeBackwardRefLz77 = 0;
int countBackwardRefLz77 = 0;
double timeBackwardRefRle = 0;
int countBackwardRefRle = 0;
double timeGetHistoImg = 0;
int countGetHistoImg = 0;
double timeHistogramCombineStochastic = 0;
int countHistogramCombineStochastic = 0;
double timeHistogramCombineStochastic_forloop = 0;
int countHistogramCombineStochastic_forloop = 0;
double timeHistogramAddEval = 0;
int countHistogramAddEval = 0;
double timeHistogramRemap = 0;
int countHistogramRemap = 0;

// lossy
double timeEncAnalyze = 0;
int countEncAnalyze = 0;
double timeEncAnalyzeOcl = 0;
int countEncAnalyzeOcl = 0;
double timeEncTokenLoop = 0;
int countEncTokenLoop = 0;
double timeEncLoop = 0;
int countEncLoop = 0;
double timeEncLoopOcl = 0;
int countEncLoopOcl = 0;
int StatLoopFlag = 1;
double timeStatLoop = 0;
int countStatLoop = 0;
double timeVP8Decimate = 0;
int countVP8Decimate = 0;
double timeVP8EmitTokens = 0;
int countVP8EmitTokens = 0;
double timeCodeResiduals = 0;
int countCodeResiduals = 0;
double timeVP8Decimate_2 = 0;
int countVP8Decimate_2 = 0;
double timeStoreFilterSts = 0;
int countStoreFilterSts = 0;
double timeVP8Decimate_BestIntra = 0;
int countVP8Decimate_BestIntra = 0;
double timeBestIntra16 = 0;
int countBestIntra16 = 0;
double timeBestIntra4 = 0;
int countBestIntra4 = 0;
double timeBestUV = 0;
int countBestUV = 0;
double timeRefineUsingDist = 0;
int countRefineUsingDist = 0;
double timeRefineUsingDist_2 = 0;
int countRefineUsingDist_2 = 0;

void ResetEncodeProfilingData() {
    timeWebPEncode = 0;
    countWebPEncode = 0;

    // lossless
    timeApplyTransforms = 0;
    countApplyTransforms = 0;
    timeEncode = 0;
    countEncode = 0;
    timeVP8ApplyNearLossless = 0;
    countVP8ApplyNearLossless = 0;
    timeVP8ApplyNearLosslessOcl = 0;
    countVP8ApplyNearLosslessOcl = 0;
    timeApplySubtractGreen = 0;
    countApplySubtractGreen = 0;
    timeEncPredict = 0;
    countEncPredict = 0;
    timeVP8LResidualImage = 0;
    countVP8LResidualImage = 0;
    timeGetPredModesResiduleForTile = 0;
    countGetPredModesResiduleForTile = 0;
    timeGetBestPredictorForTile2 = 0;
    countGetBestPredictorForTile2 = 0;
    timeBestPredict = 0;
    countBestPredict = 0;
    timeEncColorFilt = 0;
    countEncColorFilt = 0;
    timeBestColor = 0;
    countBestColor = 0;
    timeGetBackRef = 0;
    countGetBackRef = 0;
    timeBackwardRefLz77 = 0;
    countBackwardRefLz77 = 0;
    timeBackwardRefRle = 0;
    countBackwardRefRle = 0;
    timeGetHistoImg = 0;
    countGetHistoImg = 0;
    timeHistogramCombineStochastic = 0;
    countHistogramCombineStochastic = 0;
    timeHistogramCombineStochastic_forloop = 0;
    countHistogramCombineStochastic_forloop = 0;
    timeHistogramAddEval = 0;
    countHistogramAddEval = 0;
    timeHistogramRemap = 0;
    countHistogramRemap = 0;

    // lossy
    timeEncAnalyze = 0;
    countEncAnalyze = 0;
    timeEncAnalyzeOcl = 0;
    countEncAnalyzeOcl = 0;
    timeEncTokenLoop = 0;
    countEncTokenLoop = 0;
    timeEncLoop = 0;
    countEncLoop = 0;
    timeEncLoopOcl = 0;
    countEncLoopOcl = 0;
    StatLoopFlag = 1;
    timeStatLoop = 0;
    countStatLoop = 0;
    timeVP8Decimate = 0;
    countVP8Decimate = 0;
    timeVP8EmitTokens = 0;
    countVP8EmitTokens = 0;
    timeCodeResiduals = 0;
    countCodeResiduals = 0;
    timeVP8Decimate_2 = 0;
    countVP8Decimate_2 = 0;
    timeStoreFilterSts = 0;
    countStoreFilterSts = 0;
    timeVP8Decimate_BestIntra = 0;
    countVP8Decimate_BestIntra = 0;
    timeBestIntra16 = 0;
    countBestIntra16 = 0;
    timeBestIntra4 = 0;
    countBestIntra4 = 0;
    timeBestUV = 0;
    countBestUV = 0;
    timeRefineUsingDist = 0;
    countRefineUsingDist = 0;
    timeRefineUsingDist_2 = 0;
    countRefineUsingDist_2 = 0;
}

// Display profiling result
void DisplayEncodeProfilingResult() {
    // display all profiling data
    if (0 != countWebPEncode) {
        fprintf(stderr, "WebPEncode Total Time: %.6f ms, count:%d, average time: %.6f ms\n", timeWebPEncode,
                countWebPEncode, timeWebPEncode / countWebPEncode);
    }
    if (0 != countApplyTransforms) {
        fprintf(stderr, "lossless encode.......\n");
        fprintf(stderr,
                "  ApplyTransforms\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeApplyTransforms, countApplyTransforms, timeApplyTransforms / countApplyTransforms,
                timeApplyTransforms * 100 / timeWebPEncode);
    }
    if (0 != timeVP8ApplyNearLossless) {
        fprintf(stderr,
                "    VP8ApplyNearLossless\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8ApplyNearLossless, countVP8ApplyNearLossless,
                timeVP8ApplyNearLossless / countVP8ApplyNearLossless, timeVP8ApplyNearLossless * 100 / timeWebPEncode);
    }
    if (0 != timeVP8ApplyNearLosslessOcl) {
        fprintf(
            stderr,
            "    VP8ApplyNearLosslessOcl\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeVP8ApplyNearLosslessOcl, countVP8ApplyNearLosslessOcl,
            timeVP8ApplyNearLosslessOcl / countVP8ApplyNearLosslessOcl,
            timeVP8ApplyNearLosslessOcl * 100 / timeWebPEncode);
    }
    if (0 != countApplySubtractGreen) {
        fprintf(stderr,
                "    ApplySubtractGreen\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeApplySubtractGreen, countApplySubtractGreen, timeApplySubtractGreen / countApplySubtractGreen,
                timeApplySubtractGreen * 100 / timeWebPEncode);
    }
    if (0 != countEncPredict) {
        fprintf(stderr,
                "    ApplyPredictFilter\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeEncPredict, countEncPredict, timeEncPredict / countEncPredict,
                timeEncPredict * 100 / timeWebPEncode);
    }
    if (0 != countVP8LResidualImage) {
        fprintf(stderr,
                "      VP8LResidualImage\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8LResidualImage, countVP8LResidualImage, timeVP8LResidualImage / countVP8LResidualImage,
                timeVP8LResidualImage * 100 / timeWebPEncode);
    }
    if (0 != countBestPredict) {
        fprintf(stderr,
                "        GetBestPredictorForTile\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeBestPredict, countBestPredict, timeBestPredict / countBestPredict,
                timeBestPredict * 100 / timeWebPEncode);
    }
    if (0 != countGetPredModesResiduleForTile) {
        fprintf(stderr,
                "        GetPredModesResiduleForTile Total Time: %.6f ms, count:%6d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeGetPredModesResiduleForTile, countGetPredModesResiduleForTile,
                timeGetPredModesResiduleForTile / countGetPredModesResiduleForTile,
                timeGetPredModesResiduleForTile * 100 / timeWebPEncode);
    }
    if (0 != countGetBestPredictorForTile2) {
        fprintf(stderr,
                "        GetBestPredictorForTile2\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeGetBestPredictorForTile2, countGetBestPredictorForTile2,
                timeGetBestPredictorForTile2 / countGetBestPredictorForTile2,
                timeGetBestPredictorForTile2 * 100 / timeWebPEncode);
    }
    if (0 != countEncColorFilt) {
        fprintf(stderr,
                "    ApplyCrossColorFilter\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeEncColorFilt, countEncColorFilt, timeEncColorFilt / countEncColorFilt,
                timeEncColorFilt * 100 / timeWebPEncode);
    }
    if (0 != countBestColor) {
        fprintf(stderr,
                "      GetBestColorTransformForTileTotal Time: %.6f ms, count:%6d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeBestColor, countBestColor, timeBestColor / countBestColor, timeBestColor * 100 / timeWebPEncode);
    }
    if (0 != countGetBackRef) {
        fprintf(stderr,
                "  GetBackwardReferences\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeGetBackRef, countGetBackRef, timeGetBackRef / countGetBackRef,
                timeGetBackRef * 100 / timeWebPEncode);
    }
    if (0 != countBackwardRefLz77) {
        fprintf(stderr,
                "    BackwardReferencesLz77\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeBackwardRefLz77, countBackwardRefLz77, timeBackwardRefLz77 / countBackwardRefLz77,
                timeBackwardRefLz77 * 100 / timeWebPEncode);
    }
    if (0 != countBackwardRefRle) {
        fprintf(stderr,
                "    BackwardReferencesRle\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeBackwardRefRle, countBackwardRefRle, timeBackwardRefRle / countBackwardRefRle,
                timeBackwardRefRle * 100 / timeWebPEncode);
    }
    if (0 != countGetHistoImg) {
        fprintf(stderr,
                "  VP8LGetHistoImageSymbols\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeGetHistoImg, countGetHistoImg, timeGetHistoImg / countGetHistoImg,
                timeGetHistoImg * 100 / timeWebPEncode);
    }
    if (0 != countHistogramCombineStochastic) {
        fprintf(
            stderr,
            "    HistogramCombineStochastic\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeHistogramCombineStochastic, countHistogramCombineStochastic,
            timeHistogramCombineStochastic / countHistogramCombineStochastic,
            timeHistogramCombineStochastic * 100 / timeWebPEncode);
    }
    if (0 != countHistogramCombineStochastic_forloop) {
        fprintf(stderr,
                "    HistogramCombineStochastic_forloop\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, "
                "percentage:%.6f%\n",
                timeHistogramCombineStochastic_forloop, countHistogramCombineStochastic_forloop,
                timeHistogramCombineStochastic_forloop / countHistogramCombineStochastic_forloop,
                timeHistogramCombineStochastic_forloop * 100 / timeWebPEncode);
    }
    if (0 != countHistogramAddEval) {
        fprintf(stderr,
                "      HistogramAddEval\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeHistogramAddEval, countHistogramAddEval, timeHistogramAddEval / countHistogramAddEval,
                timeHistogramAddEval * 100 / timeWebPEncode);
    }
    if (0 != countHistogramRemap) {
        fprintf(stderr,
                "    HistogramRemap\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeHistogramRemap, countHistogramRemap, timeHistogramRemap / countHistogramRemap,
                timeHistogramRemap * 100 / timeWebPEncode);
    }
    if (0 != countEncode) {
        fprintf(stderr, "  Encode\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeEncode, countEncode, timeEncode / countEncode, timeEncode * 100 / timeWebPEncode);
    }
    if (0 != countEncAnalyze) {
        fprintf(stderr, "lossy encode.......\n");
        fprintf(
            stderr, "  VP8EncAnalyze\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeEncAnalyze, countEncAnalyze, timeEncAnalyze / countEncAnalyze, timeEncAnalyze * 100 / timeWebPEncode);
    }
    if (0 != countEncAnalyzeOcl) {
        fprintf(stderr, "lossy encode.......\n");
        fprintf(stderr,
                "  VP8EncAnalyzeOcl\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeEncAnalyzeOcl, countEncAnalyzeOcl, timeEncAnalyzeOcl / countEncAnalyzeOcl,
                timeEncAnalyzeOcl * 100 / timeWebPEncode);
    }
    if (0 != countEncTokenLoop) {
        fprintf(stderr,
                "  VP8EncTokenLoop\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeEncTokenLoop, countEncTokenLoop, timeEncTokenLoop / countEncTokenLoop,
                timeEncTokenLoop * 100 / timeWebPEncode);
    }
    if (0 != countEncLoop) {
        fprintf(stderr, "  VP8EncLoop\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeEncLoop, countEncLoop, timeEncLoop / countEncLoop, timeEncLoop * 100 / timeWebPEncode);
    }
    if (0 != countEncLoopOcl) {
        fprintf(
            stderr, "  VP8EncLoopOcl\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeEncLoopOcl, countEncLoopOcl, timeEncLoopOcl / countEncLoopOcl, timeEncLoopOcl * 100 / timeWebPEncode);
    }
    if (0 != countStatLoop) {
        fprintf(stderr, "    StatLoop\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeStatLoop, countStatLoop, timeStatLoop / countStatLoop, timeStatLoop * 100 / timeWebPEncode);
    }
    if (0 != countVP8Decimate_2 && 0 != countEncLoop) {
        fprintf(stderr,
                "      VP8Decimate\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8Decimate_2, countVP8Decimate_2, timeVP8Decimate_2 / countVP8Decimate_2,
                timeVP8Decimate_2 * 100 / timeWebPEncode);
    } else if (0 != countVP8Decimate_2) {
        fprintf(stderr,
                "    VP8Decimate\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8Decimate_2, countVP8Decimate_2, timeVP8Decimate_2 / countVP8Decimate_2,
                timeVP8Decimate_2 * 100 / timeWebPEncode);
    }
    if (0 != countRefineUsingDist) {
        fprintf(
            stderr,
            "        RefineUsingDistortion\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeRefineUsingDist_2, countRefineUsingDist_2, timeRefineUsingDist_2 / countRefineUsingDist_2,
            timeRefineUsingDist_2 * 100 / timeWebPEncode);
    }
    if (0 != countStoreFilterSts) {
        fprintf(
            stderr,
            "    VP8StoreFilterStats\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeStoreFilterSts, countStoreFilterSts, timeStoreFilterSts / countStoreFilterSts,
            timeStoreFilterSts * 100 / timeWebPEncode);
    }
    if (0 != countCodeResiduals) {
        fprintf(stderr,
                "    CodeResiduals\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeCodeResiduals, countCodeResiduals, timeCodeResiduals / countCodeResiduals,
                timeCodeResiduals * 100 / timeWebPEncode);
    }
    if (0 != countVP8Decimate) {
        fprintf(stderr,
                "    VP8Decimate\t\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8Decimate, countVP8Decimate, timeVP8Decimate / countVP8Decimate,
                timeVP8Decimate * 100 / timeWebPEncode);
    }
    if (0 != countRefineUsingDist) {
        fprintf(
            stderr,
            "      RefineUsingDistortion\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeRefineUsingDist, countRefineUsingDist, timeRefineUsingDist / countRefineUsingDist,
            timeRefineUsingDist * 100 / timeWebPEncode);
    }
    if (0 != countBestIntra16) {
        fprintf(stderr,
                "      PickBestIntra16\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeBestIntra16, countBestIntra16, timeBestIntra16 / countBestIntra16,
                timeBestIntra16 * 100 / timeWebPEncode);
    }
    if (0 != countBestIntra4) {
        fprintf(
            stderr, "      PickBestIntra4\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
            timeBestIntra4, countBestIntra4, timeBestIntra4 / countBestIntra4, timeBestIntra4 * 100 / timeWebPEncode);
    }
    if (0 != countBestUV) {
        fprintf(stderr, "      PickBestUV\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeBestUV, countBestUV, timeBestUV / countBestUV, timeBestUV * 100 / timeWebPEncode);
    }
    if (0 != countVP8EmitTokens) {
        fprintf(stderr,
                "    VP8EmitTokens\t\tTotal Time: %.6f ms, count:%6d, average time: %.6f ms, percentage:%.6f%\n",
                timeVP8EmitTokens, countVP8EmitTokens, timeVP8EmitTokens / countVP8EmitTokens,
                timeVP8EmitTokens * 100 / timeWebPEncode);
    }
}
