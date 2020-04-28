/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef _XF_MERGE_DEBEVEC_HPP_
#define _XF_MERGE_DEBEVEC_HPP_
#include <iostream>
#include "hls_stream.h"
#include <cmath>
#include "common/xf_common.hpp"

namespace xf {
namespace cv {
template <typename PIXEL_SRC_T,
          typename PIXEL_DST_T,
          int SRC_T,
          int DST_T,
          int SRC_NUM,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernelPixel(PIXEL_SRC_T data[SRC_NUM],
                             PIXEL_DST_T& localbufferout,
                             const float _times[SRC_NUM][NPC],
                             const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC]
                            )
{
    ap_uint<log2<NPC>::cvalue+1> j;      //Pixel per clock cycle loop;
    for(j = 0; j < XF_NPIXPERCYCLE(NPC); j++)
    {
        // clang-format off
        #pragma HLS UNROLL
        // clang-format on
        float Num[CHANNELS] = {0.0f};
        float Den = 0.0f;
        XF_CTUNAME(SRC_T, NPC) pxla[CHANNELS];
        ap_uint<log2<SRC_NUM>::cvalue+1> i;  //Number of images [2-6]
        for(i = 0; i < SRC_NUM; i++)
        {
            // clang-format off
            #pragma HLS UNROLL
            // clang-format on
            float WeightAvg = 0.0f;
            ap_uint<XF_DTPIXELDEPTH(SRC_T,NPC) + 3> PxlWeightSum = 0;
            ap_uint<log2<CHANNELS>::cvalue+1> k; //Channel loop
            for(k = 0; k < CHANNELS; k++)
            {
                // clang-format off
                #pragma HLS UNROLL
                // clang-format on
                XF_CTUNAME(SRC_T, NPC) pxl = data[i].range((j*XF_PIXELWIDTH(SRC_T,NPC) + (k+1)*XF_DTPIXELDEPTH(SRC_T,NPC) - 1),
                                                           (j*XF_PIXELWIDTH(SRC_T,NPC) + k*XF_DTPIXELDEPTH(SRC_T,NPC)));
                pxla[k] = pxl;
                ap_uint<XF_DTPIXELDEPTH(SRC_T,NPC) + 1> PxlWeight = 0;
                if(pxla[k] < (LDR_SIZE/2))
                    PxlWeight += (1 + pxla[k]);
                else
                    PxlWeight += (LDR_SIZE - pxla[k]);
                PxlWeightSum += PxlWeight;
            }
            WeightAvg = PxlWeightSum;

            for(k = 0; k < CHANNELS; k++)
            {
                // clang-format off
                #pragma HLS UNROLL
                // clang-format on
                Num[k] += WeightAvg*(input_response[pxla[k]][k][i][j] - _times[i][j]);
            }
            Den += WeightAvg;
        }
        Den = 1.0f/Den;

        ap_uint<log2<CHANNELS>::cvalue+1> k; //Channel loop
        for(k = 0; k < CHANNELS; k++)
        {
            // clang-format off
            #pragma HLS UNROLL
            // clang-format on
            float o = expf(Num[k]*Den);
            localbufferout.range((j*XF_PIXELWIDTH(DST_T,NPC) + (k+1)*XF_DTPIXELDEPTH(DST_T,NPC) - 1),
                                 (j*XF_PIXELWIDTH(DST_T,NPC) + k*XF_DTPIXELDEPTH(DST_T,NPC))) = floatToRawBits(o);
        }
    }
    //]]
}

template <typename mmin,
          typename mmout,
          int SRC_NUM,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernelSS(mmin& srcin1,
                          mmin& srcin2,
                          mmin& srcin3,
                          mmin& srcin4,
                          mmout& dstout,
                          const float _times[SRC_NUM][NPC],
                          const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC]
                         )
{
    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on
    int i;
Loop1:
    for(i = 0; i < srcin1.loopbound(); i++)
    {
        // clang-format off
        #pragma HLS LOOP_FLATTEN off
        #pragma HLS LOOP_TRIPCOUNT min=mmin::LOOPBOUND max=mmin::LOOPBOUND
        #pragma HLS PIPELINE II=1
        // clang-format on

        //Read pixel data from images [[
        typename mmin::DATATYPE data[SRC_NUM];
        data[0] = srcin1.read(i);
        data[1] = srcin2.read(i);
        data[2] = srcin3.read(i);
        data[3] = srcin4.read(i);
        //]]

        //Process the data [[
        typename mmout::DATATYPE localbufferout = 0;
        MergeDebevecKernelPixel<typename mmin::DATATYPE,
                                typename mmout::DATATYPE,
                                SRC_T,
                                DST_T,
                                SRC_NUM,
                                CHANNELS,
                                NPC,
                                LDR_SIZE> (data,localbufferout,_times,input_response);
        //]]

        //Write data to output [[
        dstout.write(localbufferout,i);
        //]]
    }
}

template <int PTR_IN_WIDTH,
          int PTR_OUT_WIDTH,
          int SRC_NUM,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernel(ap_uint<PTR_IN_WIDTH>* src1,
                        ap_uint<PTR_IN_WIDTH>* src2,
                        ap_uint<PTR_IN_WIDTH>* src3,
                        ap_uint<PTR_IN_WIDTH>* src4,
                        ap_uint<PTR_OUT_WIDTH>* dst,
                        const float _times[SRC_NUM][NPC],
                        const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC],
                        int height,
                        int width
                       )
{
    assert(SRC_NUM == 4);
    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on
    
    using mmin = typename xf::cv::MMIterIn<PTR_IN_WIDTH, SRC_T, ROWS, COLS, NPC>;
    using mmout = typename xf::cv::MMIterOut<PTR_OUT_WIDTH, DST_T, ROWS, COLS, NPC>;

    mmin srcin1(src1,height,width);
    mmin srcin2(src2,height,width);
    mmin srcin3(src3,height,width);
    mmin srcin4(src4,height,width);
    mmout dstout;

    MergeDebevecKernelSS<mmin,mmout,SRC_NUM,SRC_T,DST_T,ROWS,COLS,CHANNELS,NPC,LDR_SIZE> (srcin1,
                                                                                          srcin2,
                                                                                          srcin3,
                                                                                          srcin4,
                                                                                          dstout,
                                                                                          _times,
                                                                                          input_response
                                                                                         );
    
    dstout.transfer(dst,height,width);
}

template <typename mmin,
          typename mmout,
          int SRC_NUM,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernelSS(mmin& srcin1,
                          mmin& srcin2,
                          mmin& srcin3,
                          mmout& dstout,
                          const float _times[SRC_NUM][NPC],
                          const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC]
                         )
{
    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on
    int i;
Loop1:
    for(i = 0; i < srcin1.loopbound(); i++)
    {
        // clang-format off
        #pragma HLS LOOP_FLATTEN off
        #pragma HLS LOOP_TRIPCOUNT min=mmin::LOOPBOUND max=mmin::LOOPBOUND
        #pragma HLS PIPELINE II=1
        // clang-format on

        //Read pixel data from images [[
        typename mmin::DATATYPE data[SRC_NUM];
        data[0] = srcin1.read(i);
        data[1] = srcin2.read(i);
        data[2] = srcin3.read(i);
        //]]

        //Process the data [[
        typename mmout::DATATYPE localbufferout = 0;
        MergeDebevecKernelPixel<typename mmin::DATATYPE,
                                typename mmout::DATATYPE,
                                SRC_T,
                                DST_T,
                                SRC_NUM,
                                CHANNELS,
                                NPC,
                                LDR_SIZE> (data,localbufferout,_times,input_response);
        //]]

        //Write data to output [[
        dstout.write(localbufferout,i);
        //]]
    }
}

template <int PTR_IN_WIDTH,
          int PTR_OUT_WIDTH,
          int SRC_NUM,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernel(ap_uint<PTR_IN_WIDTH>* src1,
                        ap_uint<PTR_IN_WIDTH>* src2,
                        ap_uint<PTR_IN_WIDTH>* src3,
                        ap_uint<PTR_OUT_WIDTH>* dst,
                        const float _times[SRC_NUM][NPC],
                        const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC],
                        int height,
                        int width
                       )
{
    assert(SRC_NUM == 3);
    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    using mmin = typename xf::cv::MMIterIn<PTR_IN_WIDTH, SRC_T, ROWS, COLS, NPC>;
    using mmout = typename xf::cv::MMIterOut<PTR_OUT_WIDTH, DST_T, ROWS, COLS, NPC>;

    mmin srcin1(src1,height,width);
    mmin srcin2(src2,height,width);
    mmin srcin3(src3,height,width);
    mmout dstout;

    MergeDebevecKernelSS<mmin,mmout,SRC_NUM,SRC_T,DST_T,ROWS,COLS,CHANNELS,NPC,LDR_SIZE> (srcin1,
                                                                                          srcin2,
                                                                                          srcin3,
                                                                                          dstout,
                                                                                          _times,
                                                                                          input_response
                                                                                         );

    dstout.transfer(dst,height,width);
}

template <typename mmin,
          typename mmout,
          int SRC_NUM,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernelSS(mmin& srcin1,
                          mmin& srcin2,
                          mmout& dstout,
                          const float _times[SRC_NUM][NPC],
                          const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC]
                         )
{
    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on
    int i;
Loop1:
    for(i = 0; i < srcin1.loopbound(); i++)
    {
        // clang-format off
        #pragma HLS LOOP_FLATTEN off
        #pragma HLS LOOP_TRIPCOUNT min=mmin::LOOPBOUND max=mmin::LOOPBOUND
        #pragma HLS PIPELINE II=1
        // clang-format on

        //Read pixel data from images [[
        typename mmin::DATATYPE data[SRC_NUM];
        data[0] = srcin1.read(i);
        data[1] = srcin2.read(i);
        //]]

        //Process the data [[
        typename mmout::DATATYPE localbufferout = 0;
        MergeDebevecKernelPixel<typename mmin::DATATYPE,
                                typename mmout::DATATYPE,
                                SRC_T,
                                DST_T,
                                SRC_NUM,
                                CHANNELS,
                                NPC,
                                LDR_SIZE> (data,localbufferout,_times,input_response);
        //]]

        //Write data to output [[
        dstout.write(localbufferout,i);
        //]]
    }
}


template <int PTR_IN_WIDTH,
          int PTR_OUT_WIDTH,
          int SRC_NUM,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int CHANNELS,
          int NPC,
          int LDR_SIZE>
void MergeDebevecKernel(ap_uint<PTR_IN_WIDTH>* src1,
                        ap_uint<PTR_IN_WIDTH>* src2,
                        ap_uint<PTR_OUT_WIDTH>* dst,
                        const float _times[SRC_NUM][NPC],
                        const float input_response[LDR_SIZE][CHANNELS][SRC_NUM][NPC],
                        int height,
                        int width
                       )
{
    assert(SRC_NUM == 2);
    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    using mmin = typename xf::cv::MMIterIn<PTR_IN_WIDTH, SRC_T, ROWS, COLS, NPC>;
    using mmout = typename xf::cv::MMIterOut<PTR_OUT_WIDTH, DST_T, ROWS, COLS, NPC>;

    mmin srcin1(src1,height,width);
    mmin srcin2(src2,height,width);
    mmout dstout;

    MergeDebevecKernelSS<mmin,mmout,SRC_NUM,SRC_T,DST_T,ROWS,COLS,CHANNELS,NPC,LDR_SIZE> (srcin1,
                                                                                          srcin2,
                                                                                          dstout,
                                                                                          _times,
                                                                                          input_response
                                                                                         );

    dstout.transfer(dst,height,width);
}

} // namespace cv
} // namespace xf
#endif //_XF_ACCUMULATE_IMAGE_HPP_
