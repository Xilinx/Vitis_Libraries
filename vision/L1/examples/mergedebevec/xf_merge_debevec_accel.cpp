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

#include "xf_merge_debevec_config.h"
static constexpr int DEPTH_IN  = (HEIGHT*WIDTH*XF_PIXELWIDTH(IN_TYPE,NPC))/PTR_IN_WIDTH;
static constexpr int DEPTH_OUT = (HEIGHT*WIDTH*XF_PIXELWIDTH(OUT_TYPE,NPC))/PTR_OUT_WIDTH;

template <int N, int NPC>
void readCaptureTimeInLocalBuffer(float _times[N][NPC], const float* times)
{
    ap_uint<(xf::cv::log2<N>::cvalue) + 1> i;
    ap_uint<(xf::cv::log2<NPC>::cvalue) + 1> j;
    //Read the response time in an array
    for(i = 0; i < N; i++) {
        // clang-format off
        #pragma HLS UNROLL
        // clang-format on
        float t = times[i];
        for(j = 0; j < NPC; j++) {
            // clang-format off
            #pragma HLS UNROLL
            // clang-format on
            _times[i][j] = t;
        }
    }
}

template <int N, int C, int S_NUM, int NPC>
void readCRFDataInLocalBuffer(float _input_response[N][C][S_NUM][NPC], const float* input_response)
{
    ap_uint<(xf::cv::log2<N>::cvalue) + 1>     i;
    ap_uint<(xf::cv::log2<C>::cvalue) + 1>     j;
    ap_uint<(xf::cv::log2<S_NUM>::cvalue) + 1> k;
    ap_uint<(xf::cv::log2<NPC>::cvalue) + 1>   l;
    //Read the response function in an array
    for(i = 0; i < N; i++) {
        for(j = 0; j < C; j++) {
            float f = input_response[C*i + j];
            for(k = 0; k < S_NUM; k++) {
                // clang-format off
                #pragma HLS UNROLL
                // clang-format on
                for(l = 0; l < NPC; l++) {
                    // clang-format off
                    #pragma HLS UNROLL
                    // clang-format on
                    _input_response[i][j][k][l] = f;
                }
            }
        }
    }
}

extern "C" {
void mergedebevec_accel(ap_uint<PTR_IN_WIDTH>* src1,
                        ap_uint<PTR_IN_WIDTH>* src2,
                        ap_uint<PTR_OUT_WIDTH>* dst,
                        const float* times,
                        const float* input_response,
                        int height,
                        int width
                       )
{
    //clang-format off
    #pragma HLS INTERFACE m_axi port=src1            offset=slave bundle=gmem_in1      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=src2            offset=slave bundle=gmem_in2      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=dst             offset=slave bundle=gmem_out1     depth=DEPTH_OUT
    #pragma HLS INTERFACE m_axi port=times           offset=slave bundle=gmem_times    depth=4
    #pragma HLS INTERFACE m_axi port=input_response  offset=slave bundle=gmem_response depth=768
    #pragma HLS INTERFACE s_axilite  port=height
    #pragma HLS INTERFACE s_axilite  port=width
    #pragma HLS INTERFACE s_axilite  port=return
    //clang-format on

    assert(SRC_NUM == 2);
    static constexpr int CHANNELS = (XF_CHANNELS(IN_TYPE,NPC));

    float _times[SRC_NUM][NPC];
    //clang-format off
    #pragma HLS ARRAY_PARTITION variable=_times complete dim=1
    #pragma HLS ARRAY_PARTITION variable=_times complete dim=2
    //clang-format on

    float _input_response[(1 << XF_DTPIXELDEPTH(IN_TYPE,NPC))][CHANNELS][SRC_NUM][NPC];
    //clang-format off
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=2
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=3
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=4
    //clang-format on

    //Read the response time in an array
    readCaptureTimeInLocalBuffer<SRC_NUM>(_times, times);

    //Read the response function in an array
    readCRFDataInLocalBuffer<(1 << XF_DTPIXELDEPTH(IN_TYPE,NPC)), (XF_CHANNELS(IN_TYPE,NPC)), SRC_NUM, NPC>(_input_response, input_response);

    //clang-format off
    #pragma HLS DATAFLOW
    //clang-format off
    xf::cv::MergeDebevecKernel<PTR_IN_WIDTH,
                               PTR_OUT_WIDTH,
                               SRC_NUM,
                               IN_TYPE,
                               OUT_TYPE,
                               HEIGHT,
                               WIDTH,
                               XF_CHANNELS(IN_TYPE,NPC),
                               NPC,
                               (1 << XF_DTPIXELDEPTH(IN_TYPE,NPC))> (src1, src2, dst, _times, _input_response, height, width);
    return;
}

void mergedebevec_accel_3(ap_uint<PTR_IN_WIDTH>* src1,
                          ap_uint<PTR_IN_WIDTH>* src2,
                          ap_uint<PTR_IN_WIDTH>* src3,
                          ap_uint<PTR_OUT_WIDTH>* dst,
                          const float* times,
                          const float* input_response,
                          int height,
                          int width
                         )
{
    //clang-format off
    #pragma HLS INTERFACE m_axi port=src1            offset=slave bundle=gmem_in1      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=src2            offset=slave bundle=gmem_in2      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=src3            offset=slave bundle=gmem_in3      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=dst             offset=slave bundle=gmem_out1     depth=DEPTH_OUT
    #pragma HLS INTERFACE m_axi port=times           offset=slave bundle=gmem_times    depth=4
    #pragma HLS INTERFACE m_axi port=input_response  offset=slave bundle=gmem_response depth=768
    #pragma HLS INTERFACE s_axilite  port=height
    #pragma HLS INTERFACE s_axilite  port=width
    #pragma HLS INTERFACE s_axilite  port=return
    //clang-format on

    assert(SRC_NUM == 3);
    static constexpr int CHANNELS = (XF_CHANNELS(IN_TYPE,NPC));

    float _times[SRC_NUM][NPC];
    //clang-format off
    #pragma HLS ARRAY_PARTITION variable=_times complete dim=1
    #pragma HLS ARRAY_PARTITION variable=_times complete dim=2
    //clang-format on

    float _input_response[(1 << XF_DTPIXELDEPTH(IN_TYPE,NPC))][CHANNELS][SRC_NUM][NPC];
    //clang-format off
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=2
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=3
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=4
    //clang-format on

    //Read the response time in an array
    readCaptureTimeInLocalBuffer<SRC_NUM>(_times, times);

    //Read the response function in an array
    readCRFDataInLocalBuffer<(1 << XF_DTPIXELDEPTH(IN_TYPE,NPC)), (XF_CHANNELS(IN_TYPE,NPC)), SRC_NUM, NPC>(_input_response, input_response);

    //clang-format off
    #pragma HLS DATAFLOW
    //clang-format off
    xf::cv::MergeDebevecKernel<PTR_IN_WIDTH,
                               PTR_OUT_WIDTH,
                               SRC_NUM,
                               IN_TYPE,
                               OUT_TYPE,
                               HEIGHT,
                               WIDTH,
                               XF_CHANNELS(IN_TYPE,NPC),
                               NPC,
                               (1 << XF_DTPIXELDEPTH(IN_TYPE,NPC))> (src1, src2, src3, dst, _times, _input_response, height, width);
    return;
}

void mergedebevec_accel_4(ap_uint<PTR_IN_WIDTH>* src1,
                          ap_uint<PTR_IN_WIDTH>* src2,
                          ap_uint<PTR_IN_WIDTH>* src3,
                          ap_uint<PTR_IN_WIDTH>* src4,
                          ap_uint<PTR_OUT_WIDTH>* dst,
                          const float* times,
                          const float* input_response,
                          int height,
                          int width
                         )
{
    //clang-format off
    #pragma HLS INTERFACE m_axi port=src1            offset=slave bundle=gmem_in1      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=src2            offset=slave bundle=gmem_in2      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=src3            offset=slave bundle=gmem_in3      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=src4            offset=slave bundle=gmem_in4      depth=DEPTH_IN
    #pragma HLS INTERFACE m_axi port=dst             offset=slave bundle=gmem_out1     depth=DEPTH_OUT
    #pragma HLS INTERFACE m_axi port=times           offset=slave bundle=gmem_times    depth=4
    #pragma HLS INTERFACE m_axi port=input_response  offset=slave bundle=gmem_response depth=768
    #pragma HLS INTERFACE s_axilite  port=height
    #pragma HLS INTERFACE s_axilite  port=width
    #pragma HLS INTERFACE s_axilite  port=return
    //clang-format on

    assert(SRC_NUM == 4);
    static constexpr int CHANNELS = (XF_CHANNELS(IN_TYPE,NPC));

    float _times[SRC_NUM][NPC];
    //clang-format off
    #pragma HLS ARRAY_PARTITION variable=_times complete dim=1
    #pragma HLS ARRAY_PARTITION variable=_times complete dim=2
    //clang-format on

    float _input_response[(1 << XF_DTPIXELDEPTH(IN_TYPE,NPC))][CHANNELS][SRC_NUM][NPC];
    //clang-format off
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=2
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=3
    #pragma HLS ARRAY_PARTITION variable=_input_response complete dim=4
    //clang-format on

    //Read the response time in an array
    readCaptureTimeInLocalBuffer<SRC_NUM>(_times, times);

    //Read the response function in an array
    readCRFDataInLocalBuffer<(1 << XF_DTPIXELDEPTH(IN_TYPE,NPC)), (XF_CHANNELS(IN_TYPE,NPC)), SRC_NUM, NPC>(_input_response, input_response);

    xf::cv::MergeDebevecKernel<PTR_IN_WIDTH,
                               PTR_OUT_WIDTH,
                               SRC_NUM,
                               IN_TYPE,
                               OUT_TYPE,
                               HEIGHT,
                               WIDTH,
                               XF_CHANNELS(IN_TYPE,NPC),
                               NPC,
                               (1 << XF_DTPIXELDEPTH(IN_TYPE,NPC))> (src1, src2, src3, src4, dst, _times, _input_response, height, width);
    return;
}

}
