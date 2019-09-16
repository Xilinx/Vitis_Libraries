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
/**
 * @file corrand.hpp
 * @brief This file contains the implementation of correlated random number
 * generator.
 */
#ifndef XF_FINTECH_CORRAND_H
#define XF_FINTECH_CORRAND_H
#include "ap_int.h"
#include "hls_stream.h"
#include "xf_fintech/utils.hpp"
namespace xf {
namespace fintech {
namespace internal {
template <typename DT, int ASSETS>
void martixMul(unsigned int loopNm,
               hls::stream<DT>& inStrm,
               hls::stream<DT> outStrm[8],
               DT corrMatrix[ASSETS][ASSETS]) {
    DT buff[ASSETS][8];
#pragma HLS array_partition variable = buff dim = 1
    // why we cannot add this pragma
    //#pragma HLS array_partition variable=buff dim=0
    ap_uint<3> cnt = 0;
    for (int l = 0; l < loopNm; ++l) {
#pragma HLS loop_tripcount min = 1024 max = 1024
        for (int i = 0; i < ASSETS; ++i) {
#pragma HLS pipeline II = 1
            DT dw = inStrm.read();
            DT outB[ASSETS];
#pragma HLS array_partition variable = outB
            for (int j = 0; j < ASSETS; ++j) {
#pragma HLS unroll
                DT nDw;
#pragma HLS resource variable = nDw core = DMul_meddsp
                nDw = corrMatrix[j][i] * dw;
                DT pre = buff[j][cnt];
                DT oldD;
                if (i == 0) {
                    oldD = 0;
                } else {
                    oldD = pre;
                }
                DT newD;
#pragma HLS resource variable = newD core = DAddSub_nodsp
                newD = oldD + nDw;
                buff[j][cnt] = newD;
                outB[j] = newD;
            }
            cnt++;
            for (int k = 0; k < 8; k++) {
#pragma HLS unroll
                DT out;
                if (k + i < ASSETS) {
                    out = outB[k + i];
                } else {
                    out = 0;
                }
                outStrm[k].write(out);
            }
        }
    }
}

template <typename DT, int ASSETS>
void mergeS(unsigned int loopNm, hls::stream<DT> inStrm[8], hls::stream<DT>& outStrm) {
    DT buff[8];
#pragma HLS array_partition variable = buff dim = 0
    for (int i = 0; i < loopNm; ++i) {
#pragma HLS loop_tripcount min = 1024 max = 1024
        for (int p = 0; p < ASSETS / 8; ++p) {
#pragma HLS pipeline II = 1
            DT in[8][8];
#pragma HLS array_partition variable = in dim = 0
            DT out[8];
#pragma HLS array_partition variable = out dim = 0
            for (int j = 0; j < 8; j++) {
                for (int k = 0; k < 8; k++) {
#pragma HLS unroll
                    in[k][j] = inStrm[k].read();
                }
            }
            DT pre[8];
#pragma HLS array_partition variable = pre dim = 0
            for (int k = 0; k < 8; k++) {
#pragma HLS unroll
                if (p == 0) {
                    pre[k] = 0;
                } else {
                    pre[k] = buff[0];
                }
            }
            out[0] = pre[0] + in[0][0];
            out[1] = pre[1] + in[0][1] + in[1][0];
            out[2] = pre[2] + in[0][2] + in[1][1] + in[2][0];
            out[3] = pre[3] + in[0][3] + in[1][2] + in[2][1] + in[3][0];
            out[4] = pre[4] + in[0][4] + in[1][3] + in[2][2] + in[3][1] + in[4][0];
            out[5] = pre[5] + in[0][5] + in[1][4] + in[2][3] + in[3][2] + in[4][1] + in[5][0];
            out[6] = pre[6] + in[0][6] + in[1][5] + in[2][4] + in[3][3] + in[4][2] + in[5][1] + in[6][0];
            out[7] = in[0][7] + in[1][6] + in[2][5] + in[3][4] + in[4][3] + in[5][2] + in[6][1] + in[7][0];

            buff[0] = in[1][7] + in[2][6] + in[3][5] + in[4][4] + in[5][3] + in[6][2] + in[7][1];
            buff[1] = in[2][7] + in[3][6] + in[4][5] + in[5][4] + in[6][3] + in[7][2];
            buff[2] = in[3][7] + in[4][6] + in[5][5] + in[6][4] + in[7][3];
            buff[3] = in[4][7] + in[5][6] + in[6][5] + in[7][4];
            buff[4] = in[5][7] + in[6][6] + in[7][5];
            buff[5] = in[6][7] + in[7][6];
            buff[6] = in[7][7];
            for (int k = 0; k < 8; k++) {
#pragma HLS unroll
                outStrm.write(out[k]);
            }
        }
    }
}

template <typename DT, int ASSETS>
void corrand(unsigned int loopNm, hls::stream<DT>& inStrm, hls::stream<DT>& outStrm, DT corrMatrix[ASSETS][ASSETS]) {
#pragma HLS dataflow
    hls::stream<DT> buffStrm[8];
#pragma HLS array_partition variable = buffStrm dim = 0
    martixMul(loopNm, inStrm, buffStrm, corrMatrix);
    mergeS<DT, ASSETS>(loopNm * ASSETS, buffStrm, outStrm);
}

/*
CORRAND_2 generates random number for MultiAssetEuropeanHestonEngine.
Option based on N underlying assets, 2 random numbers for each, 2*N in total.
2*N random numbers's correlation Matrix is
CORRAND_2 takes 2 independent random numbers in 1 cycle.
CORRAND_2 produce 2 correlated random number in 1 cycle, in the order of assets.
*/

template <typename DT, int PathNm, int ASSETS>
class CORRAND_2 {
   public:
    bool firstrun;
    ap_uint<1> rounds;

    DT buff_0[ASSETS * 2][PathNm];
    DT buff_1[ASSETS * 2][PathNm];
    DT corrMatrix[ASSETS * 2 + 1][ASSETS];

    CORRAND_2() : firstrun(true), rounds(0) {}

    void setup(DT inputMatrix[ASSETS * 2 + 1][ASSETS]) {
        for (int i = 0; i < ASSETS * 2 + 1; i++) {
            for (int j = 0; j < ASSETS; j++) {
#pragma HLS pipeline II = 1
                corrMatrix[i][j] = inputMatrix[i][j];
            }
        }
    }

    void corrPathCube_s(hls::stream<ap_uint<16> >& timesteps_strm,
                        hls::stream<ap_uint<16> >& paths_strm,
                        hls::stream<DT>& inStrm0,
                        hls::stream<DT>& inStrm1,
                        hls::stream<DT>& outStrm0,
                        hls::stream<DT>& outStrm1) {
#pragma HLS array_partition variable = buff_0 dim = 1
#pragma HLS array_partition variable = buff_1 dim = 1
#pragma HLS array_partition variable = corrMatrix dim = 1

        ap_uint<16> timesteps, paths;
        timesteps = timesteps_strm.read();
        paths = paths_strm.read();

        if (firstrun) {
            timesteps += 1;
        }

    TIME_LOOP:
        for (int t_itr = 0; t_itr < timesteps; t_itr++) {
        ASSETS_LOOP:
            for (int a_itr = 0; a_itr < ASSETS; a_itr++) {
                int L0 = ASSETS * 2 - a_itr;
                int L1 = a_itr;
                int L2 = L0 - 1;
                int O_A0 = 2 * a_itr;
                int O_A1 = O_A0 + 1;

                DT matrix_buff[ASSETS * 2 + 1];
#pragma HLS array_partition variable = matrix_buff dim = 0
                for (int k = 0; k < ASSETS * 2 + 1; k++) {
#pragma HLS unroll
                    matrix_buff[k] = corrMatrix[k][a_itr];
                }

            PATH_LOOP:
                for (int p_itr = 0; p_itr < paths; p_itr++) {
#pragma HLS PIPELINE II = 1
                    DT z0 = inStrm0.read();
                    DT z1 = inStrm1.read();

                    DT tmp_buff[ASSETS * 2];
#pragma HLS array_partition variable = tmp_buff dim = 0
                    DT tmp_mul[ASSETS * 2 + 1];
#pragma HLS array_partition variable = tmp_mul dim = 0
                    DT tmp_add[ASSETS * 2];
#pragma HLS array_partition variable = tmp_add dim = 0

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (a_itr == 0) {
                            tmp_buff[k] = 0;
                        } else {
                            if (rounds == 0) {
                                tmp_buff[k] = buff_0[k][p_itr];
                            } else {
                                tmp_buff[k] = buff_1[k][p_itr];
                            }
                        }
                    }

                    for (int k = 0; k < ASSETS * 2 + 1; k++) {
#pragma HLS unroll
                        DT tmp_mul_tmp;
                        if (k < L0) {
                            tmp_mul_tmp = FPTwoMul(matrix_buff[k], z0);
                        } else {
                            tmp_mul_tmp = FPTwoMul(matrix_buff[k], z1);
                        }
                        tmp_mul[k] = tmp_mul_tmp;
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (k < L1) {
                            tmp_add[k] = 0;
                        } else {
                            tmp_add[k] = tmp_mul[k - L1];
                        }
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        DT tmp_add_tmp = tmp_add[k];
                        if (k < L2) {
                            tmp_add_tmp = FPTwoAdd(tmp_add_tmp, (DT)0.0);
                        } else {
                            tmp_add_tmp = FPTwoAdd(tmp_add_tmp, tmp_mul[k + 1]);
                        }
                        tmp_add[k] = tmp_add_tmp;
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        DT tmp_buff_tmp = tmp_buff[k];
                        tmp_buff_tmp = FPTwoAdd(tmp_buff_tmp, tmp_add[k]);
                        tmp_buff[k] = tmp_buff_tmp;
                    }

                WRITE_BACK_LOOP:
                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (rounds == 0) {
                            buff_0[k][p_itr] = tmp_buff[k];
                        } else {
                            buff_1[k][p_itr] = tmp_buff[k];
                        }
                    }

                    if (firstrun && t_itr == 0) {
                        // If this is first run and t = 0, no ouput
                    } else {
                        DT r_0, r_1;
                        if (rounds == 0) {
                            r_0 = buff_1[O_A0][p_itr];
                            r_1 = buff_1[O_A1][p_itr];
                        } else {
                            r_0 = buff_0[O_A0][p_itr];
                            r_1 = buff_0[O_A1][p_itr];
                        }
                        outStrm0.write(r_0);
                        outStrm1.write(r_1);
                    }
                }
            }
            rounds += 1;
        }
        firstrun = false;
    }

    void corrPathCube(ap_uint<16> timesteps,
                      ap_uint<16> paths,
                      hls::stream<DT>& inStrm0,
                      hls::stream<DT>& inStrm1,
                      hls::stream<DT>& outStrm0,
                      hls::stream<DT>& outStrm1) {
#pragma HLS array_partition variable = buff_0 dim = 1
#pragma HLS array_partition variable = buff_1 dim = 1
#pragma HLS array_partition variable = corrMatrix dim = 1

        if (firstrun) {
            timesteps += 1;
        }

    TIME_LOOP:
        for (int t_itr = 0; t_itr < timesteps; t_itr++) {
        ASSETS_LOOP:
            for (int a_itr = 0; a_itr < ASSETS; a_itr++) {
                int L0 = ASSETS * 2 - a_itr;
                int L1 = a_itr;
                int L2 = L0 - 1;
                int O_A0 = 2 * a_itr;
                int O_A1 = O_A0 + 1;

                DT matrix_buff[ASSETS * 2 + 1];
#pragma HLS array_partition variable = matrix_buff dim = 0
                for (int k = 0; k < ASSETS * 2 + 1; k++) {
#pragma HLS unroll
                    matrix_buff[k] = corrMatrix[k][a_itr];
                }

            PATH_LOOP:
                for (int p_itr = 0; p_itr < paths; p_itr++) {
#pragma HLS PIPELINE II = 1
                    DT z0 = inStrm0.read();
                    DT z1 = inStrm1.read();

                    DT tmp_buff[ASSETS * 2];
#pragma HLS array_partition variable = tmp_buff dim = 0
                    DT tmp_mul[ASSETS * 2 + 1];
#pragma HLS array_partition variable = tmp_mul dim = 0
                    DT tmp_add[ASSETS * 2];
#pragma HLS array_partition variable = tmp_add dim = 0

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (a_itr == 0) {
                            tmp_buff[k] = 0;
                        } else {
                            if (rounds == 0) {
                                tmp_buff[k] = buff_0[k][p_itr];
                            } else {
                                tmp_buff[k] = buff_1[k][p_itr];
                            }
                        }
                    }

                    for (int k = 0; k < ASSETS * 2 + 1; k++) {
#pragma HLS unroll
                        DT tmp_mul_tmp;
                        if (k < L0) {
                            tmp_mul_tmp = FPTwoMul(matrix_buff[k], z0);
                        } else {
                            tmp_mul_tmp = FPTwoMul(matrix_buff[k], z1);
                        }
                        tmp_mul[k] = tmp_mul_tmp;
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (k < L1) {
                            tmp_add[k] = 0;
                        } else {
                            tmp_add[k] = tmp_mul[k - L1];
                        }
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        DT tmp_add_tmp = tmp_add[k];
                        if (k < L2) {
                            tmp_add_tmp = FPTwoAdd(tmp_add_tmp, (DT)0.0);
                        } else {
                            tmp_add_tmp = FPTwoAdd(tmp_add_tmp, tmp_mul[k + 1]);
                        }
                        tmp_add[k] = tmp_add_tmp;
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        DT tmp_buff_tmp = tmp_buff[k];
                        tmp_buff_tmp = FPTwoAdd(tmp_buff_tmp, tmp_add[k]);
                        tmp_buff[k] = tmp_buff_tmp;
                    }

                WRITE_BACK_LOOP:
                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (rounds == 0) {
                            buff_0[k][p_itr] = tmp_buff[k];
                        } else {
                            buff_1[k][p_itr] = tmp_buff[k];
                        }
                    }

                    if (firstrun && t_itr == 0) {
                        // If this is first run and t = 0, no ouput
                    } else {
                        DT r_0, r_1;
                        if (rounds == 0) {
                            r_0 = buff_1[O_A0][p_itr];
                            r_1 = buff_1[O_A1][p_itr];
                        } else {
                            r_0 = buff_0[O_A0][p_itr];
                            r_1 = buff_0[O_A1][p_itr];
                        }
                        outStrm0.write(r_0);
                        outStrm1.write(r_1);
                    }
                }
            }
            rounds += 1;
        }
        firstrun = false;
    }

    void corrPathCube(ap_uint<16> timesteps,
                      ap_uint<16> paths,
                      hls::stream<DT>& inStrm0,
                      hls::stream<DT>& inStrm1,
                      hls::stream<DT>& outStrm0,
                      hls::stream<DT>& outStrm1,
                      hls::stream<DT>& outStrm2,
                      hls::stream<DT>& outStrm3) {
#pragma HLS array_partition variable = buff_0 dim = 1
#pragma HLS array_partition variable = buff_1 dim = 1
#pragma HLS array_partition variable = corrMatrix dim = 1

        if (firstrun) {
            timesteps += 1;
        }

    TIME_LOOP:
        for (int t_itr = 0; t_itr < timesteps; t_itr++) {
        ASSETS_LOOP:
            for (int a_itr = 0; a_itr < ASSETS; a_itr++) {
                int L0 = ASSETS * 2 - a_itr;
                int L1 = a_itr;
                int L2 = L0 - 1;
                int O_A0 = 2 * a_itr;
                int O_A1 = O_A0 + 1;

                DT matrix_buff[ASSETS * 2 + 1];
#pragma HLS array_partition variable = matrix_buff dim = 0
                for (int k = 0; k < ASSETS * 2 + 1; k++) {
#pragma HLS unroll
                    matrix_buff[k] = corrMatrix[k][a_itr];
                }

            PATH_LOOP:
                for (int p_itr = 0; p_itr < paths; p_itr++) {
#pragma HLS PIPELINE II = 1
                    DT z0 = inStrm0.read();
                    DT z1 = inStrm1.read();

                    DT tmp_buff[ASSETS * 2];
#pragma HLS array_partition variable = tmp_buff dim = 0
                    DT tmp_mul[ASSETS * 2 + 1];
#pragma HLS array_partition variable = tmp_mul dim = 0
                    DT tmp_add[ASSETS * 2];
#pragma HLS array_partition variable = tmp_add dim = 0

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (a_itr == 0) {
                            tmp_buff[k] = 0;
                        } else {
                            if (rounds == 0) {
                                tmp_buff[k] = buff_0[k][p_itr];
                            } else {
                                tmp_buff[k] = buff_1[k][p_itr];
                            }
                        }
                    }

                    for (int k = 0; k < ASSETS * 2 + 1; k++) {
#pragma HLS unroll
                        DT tmp_mul_tmp;
                        if (k < L0) {
                            tmp_mul_tmp = FPTwoMul(matrix_buff[k], z0);
                        } else {
                            tmp_mul_tmp = FPTwoMul(matrix_buff[k], z1);
                        }
                        tmp_mul[k] = tmp_mul_tmp;
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (k < L1) {
                            tmp_add[k] = 0;
                        } else {
                            tmp_add[k] = tmp_mul[k - L1];
                        }
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        DT tmp_add_tmp = tmp_add[k];
                        if (k < L2) {
                            tmp_add_tmp = FPTwoAdd(tmp_add_tmp, (DT)0.0);
                        } else {
                            tmp_add_tmp = FPTwoAdd(tmp_add_tmp, tmp_mul[k + 1]);
                        }
                        tmp_add[k] = tmp_add_tmp;
                    }

                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        DT tmp_buff_tmp = tmp_buff[k];
                        tmp_buff_tmp = FPTwoAdd(tmp_buff_tmp, tmp_add[k]);
                        tmp_buff[k] = tmp_buff_tmp;
                    }

                WRITE_BACK_LOOP:
                    for (int k = 0; k < ASSETS * 2; k++) {
#pragma HLS unroll
                        if (rounds == 0) {
                            buff_0[k][p_itr] = tmp_buff[k];
                        } else {
                            buff_1[k][p_itr] = tmp_buff[k];
                        }
                    }

                    if (firstrun && t_itr == 0) {
                        // If this is first run and t = 0, no ouput
                    } else {
                        DT r_0, r_1;
                        if (rounds == 0) {
                            r_0 = buff_1[O_A0][p_itr];
                            r_1 = buff_1[O_A1][p_itr];
                        } else {
                            r_0 = buff_0[O_A0][p_itr];
                            r_1 = buff_0[O_A1][p_itr];
                        }
                        outStrm0.write(r_0);
                        outStrm1.write(r_1);
                        outStrm2.write(-r_0);
                        outStrm3.write(-r_1);
                    }
                }
            }
            rounds += 1;
        }
        firstrun = false;
    }
};

//*******************************************************************
// output order
// asset-0: path-0, p1, p2, p3, ......, p1023
// asset-1: path-0, p1, p2, p3, ......, p1023
// asset-2: path-0, p1, p2, p3, ......, p1023
//......
// asset-N: path-0, p1, p2, p3, ......, p1023
template <typename DT, int SampleNm, int ASSETS>
class CORRAND {
   public:
    CORRAND() {}
    void corrPathCube(unsigned int loopNm,
                      hls::stream<DT>& inStrm,
                      hls::stream<DT>& outStrm,
                      DT corrMatrix[ASSETS][ASSETS]) {
#ifndef __SYNTHESIS__
        std::cout << "----Correlated Matrix--" << std::endl;
        for (int i = 0; i < ASSETS; ++i) {
            for (int j = 0; j < ASSETS; ++j) {
                std::cout << corrMatrix[i][j] << ", ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------" << std::endl;
#endif
        DT buff[ASSETS][SampleNm];
#pragma HLS array_partition variable = buff dim = 1
        for (int n = 0; n < ASSETS; ++n) {
            for (int i = 0; i < loopNm; ++i) {
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount min = 1024 max = 1024
                DT dw = inStrm.read();
                DT pre[ASSETS];
#pragma HLS array_partition variable = pre dim = 0
                for (int k = 0; k < ASSETS; ++k) {
#pragma HLS unroll
                    if (i == 0)
                        pre[k] = 0;
                    else
                        pre[k] = buff[k][i];
                }
                DT newD[ASSETS];
#pragma HLS array_partition variable = newD dim = 0
                for (int m = 0; m < ASSETS; ++m) {
#pragma HLS unroll
                    DT mulDw;
                    mulDw = FPTwoMul(corrMatrix[m][n], dw);
                    DT addTmp;
                    addTmp = FPTwoAdd(pre[m], mulDw);
                    newD[m] = addTmp;
                }
                for (int k = 0; k < ASSETS; ++k) {
#pragma HLS unroll
                    buff[k][i] = newD[k];
                }
                outStrm.write(newD[n]);
            }
        }
    }
    void corrPathCube(ap_uint<8> assets,
                      ap_uint<16> timesteps,
                      ap_uint<16> paths,
                      hls::stream<DT>& inStrm,
                      hls::stream<DT>& outStrm,
                      DT corrMatrix[ASSETS][ASSETS]) {
#ifndef __SYNTHESIS__
        std::cout << "----Correlated Matrix--" << std::endl;
        for (int i = 0; i < ASSETS; ++i) {
            for (int j = 0; j < ASSETS; ++j) {
                std::cout << corrMatrix[i][j] << ", ";
            }
            std::cout << std::endl;
        }
        std::cout << "-----------------------" << std::endl;
#endif
        int kk = 0;
        DT buff[ASSETS][SampleNm];
        for (int m = 0; m < ASSETS; ++m) {
            for (int n = 0; n < timesteps; ++n) {
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount min = 1024 max = 1024
                buff[m][n] = 0;
            }
        }
#pragma HLS array_partition variable = buff dim = 1
        for (int p = 0; p < paths; ++p) {
            for (int n = 0; n < ASSETS; ++n) {
                for (int i = 0; i < timesteps; ++i) {
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount min = 1024 max = 1024
                    if (n < assets) {
                        DT dw = inStrm.read();
                        DT pre[ASSETS];
#pragma HLS array_partition variable = pre dim = 0
                        for (int k = 0; k < ASSETS; ++k) {
#pragma HLS unroll
                            if (n == 0)
                                pre[k] = 0;
                            else
                                pre[k] = buff[k][i];
                        }
                        DT newD[ASSETS];
#pragma HLS array_partition variable = newD dim = 0
                        for (int m = 0; m < ASSETS; ++m) {
#pragma HLS unroll
                            DT mulDw;
                            mulDw = FPTwoMul(corrMatrix[m][n], dw);
                            DT addTmp;
                            addTmp = FPTwoAdd(pre[m], mulDw);
                            newD[m] = addTmp;
                        }
                        for (int k = 0; k < ASSETS; ++k) {
#pragma HLS unroll
                            buff[k][i] = newD[k];
                        }
#ifndef __SYNTHESIS__
                        if (kk < 10) {
                            std::cout << "Number " << kk << " value=" << newD[n] << std::endl;
                        }
                        kk++;
#endif
                        outStrm.write(newD[n]);
                    }
                }
            }
        }
    }
};
} // namespace details
} // namespace fintech
} // namespace xf
#endif //#ifndef XF_FINTECH_CORRAND_H
