/*
 * Copyright 2021 Xilinx, Inc.
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
#include "XAcc_model.hpp"

ap_uint<16> num_nonzeros_counts_7x7[2][26][6][33];  // block factor=3 dim=4
ap_uint<16> num_nonzeros_counts_1x8[2][8][8][3][4]; //
ap_uint<16> num_nonzeros_counts_8x1[2][8][8][3][4]; //

ap_uint<16> residual_noise_counts[2][64][10][10];     // block factor=4 dim=2 !
ap_uint<16> residual_noise_counts_dc[12][10];         // !
ap_uint<64> residual_threshold_counts[2][256][8][32]; // complete dim=4 | uram !
// ap_uint<64> residual_threshold_counts[2][128][8][16];// complete dim=3 | uram !
ap_uint<22> addr_thre[5];
ap_uint<64> data_thre[5];

ap_uint<64> exponent_counts[2][10][49][3][11];   // complete dim=2 | uram
ap_uint<64> exponent_counts_x[2][10][15][3][11]; // complete dim=4 | uram
ap_uint<16> exponent_counts_dc[12][17][11];      //

ap_uint<16> sign_counts[2][4][12]; // !
ap_uint<8> addr_sign[4];
ap_uint<16> data_sign[4];

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void init_hlsmodel() {
#pragma HLS LOOP_MERGE
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 26; j++) {
            for (int k = 0; k < 6; k++) {
                for (int l = 0; l < 11; l++) {
#pragma HLS PIPELINE II = 1
                    num_nonzeros_counts_7x7[i][j][k][l] = 0x0101;
                    num_nonzeros_counts_7x7[i][j][k][l + 11] = 0x0101;
                    num_nonzeros_counts_7x7[i][j][k][l + 22] = 0x0101;
                }
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                for (int l = 0; l < 3; l++) {
                    for (int m = 0; m < 4; m++) {
#pragma HLS PIPELINE II = 1
                        num_nonzeros_counts_1x8[i][j][k][l][m] = 0x0101;
                        num_nonzeros_counts_8x1[i][j][k][l][m] = 0x0101;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 16; j++) {
            for (int k = 0; k < 10; k++) {
                for (int l = 0; l < 10; l++) {
#pragma HLS PIPELINE II = 1
                    residual_noise_counts[i][j][k][l] = 0x0101;
                    residual_noise_counts[i][j + 16][k][l] = 0x0101;
                    residual_noise_counts[i][j + 32][k][l] = 0x0101;
                    residual_noise_counts[i][j + 48][k][l] = 0x0101;
                }
            }
        }
    }

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 10; j++) {
#pragma HLS PIPELINE II = 1
            residual_noise_counts_dc[i][j] = 0x0101;
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 256; j++) {
            for (int k = 0; k < 8; k++) {
#pragma HLS PIPELINE II = 1
                for (int l = 0; l < 32; l++) {
#pragma HLS UNROLL
                    residual_threshold_counts[i][j][k][l] = 0x0101010101010101;
                }
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int k = 0; k < 49; k++) {
            for (int l = 0; l < 3; l++) {
                for (int m = 0; m < 11; m++) {
#pragma HLS PIPELINE II = 1
                    for (int j = 0; j < 10; j++) {
#pragma HLS UNROLL
                        exponent_counts[i][j][k][l][m] = 0x0101010101010101;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++) {
            for (int k = 0; k < 15; k++) {
                for (int m = 0; m < 11; m++) {
#pragma HLS PIPELINE II = 1
                    for (int l = 0; l < 3; l++) {
#pragma HLS UNROLL
                        exponent_counts_x[i][j][k][l][m] = 0x0101010101010101;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 17; j++) {
            for (int k = 0; k < 11; k++) {
#pragma HLS PIPELINE II = 1
                exponent_counts_dc[i][j][k] = 0x0101;
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 12; k++) {
#pragma HLS PIPELINE II = 1
                sign_counts[i][j][k] = 0x0101;
            }
        }
    }

    for (int i = 0; i < 4; i++) {
#pragma HLS PIPELINE II = 1
        addr_sign[i] = 0;
        data_sign[i] = 0x0101;
    }

    for (int i = 0; i < 5; i++) {
#pragma HLS PIPELINE II = 1
        addr_thre[i] = 0;
        data_thre[i] = 0x0101010101010101;
    }
}

// ------------------------------------------------------------
uint8_t calc_prob(ap_uint<16>& cnt) {
#pragma HLS INLINE
    if (cnt(7, 0) == 1 && cnt(15, 8) == 255) return 0;
    if (cnt(7, 0) == 255 && cnt(15, 8) == 1) return 255;
    return (cnt(7, 0) << 8) / (cnt(7, 0) + cnt(15, 8));
}

// ------------------------------------------------------------
void record_and_update(bool obs, ap_uint<16>& cnt) {
#pragma HLS INLINE
    if (obs) {
        if (cnt(15, 8) != 0xff)
            cnt(15, 8) = cnt(15, 8) + 1;
        else if (cnt(7, 0) == 1)
            cnt(15, 8) = 0xff;
        else {
            cnt(15, 8) = 129;
            cnt(7, 0) = (1 + cnt(7, 0)) >> 1;
        }
    } else {
        if (cnt(7, 0) != 0xff)
            cnt(7, 0) = cnt(7, 0) + 1;
        else if (cnt(15, 8) == 1)
            cnt(7, 0) = 0xff;
        else {
            cnt(7, 0) = 129;
            cnt(15, 8) = (1 + cnt(15, 8)) >> 1;
        }
    }
}

// template <class T>
// void StrmEnd2StrmLen_T(
//        int num_blk,
//        hls::stream<bool>& strm_e0,
//        hls::stream<T>& strm_len0
//        )
//{
//    int cnt_blk=0;
//    int cnt_len=0;
//    while(cnt_blk < num_blk){
//#pragma HLS pipeline II=1
//        if(strm_e0.read()==false)
//            cnt_len++;
//        else{
//            strm_len0.write(cnt_len);
//            cnt_len=0;
//            cnt_blk++;
//        }
//
//    }
//}

// ------------------------------------------------------------
void probability_look_up(ap_uint<1> ap_color,

                         hls::stream<ap_uint<4> >& strm_sel_tab,
                         hls::stream<bool>& strm_cur_bit,
                         hls::stream<bool>& strm_e_in,
                         hls::stream<ap_uint<16> >& strm_addr1,
                         hls::stream<ap_uint<16> >& strm_addr2,
                         hls::stream<ap_uint<16> >& strm_addr3,
                         hls::stream<ap_uint<16> >& strm_addr4,

                         hls::stream<bool>& strm_bit,
                         hls::stream<uint8_t>& strm_prob,
                         hls::stream<bool>& strm_e,
                         hls::stream<uint8_t>& strm_tab_dbg

                         ) {
// clang-format off
#pragma HLS ARRAY_PARTITION variable=num_nonzeros_counts_7x7 block factor=3 dim=4
#pragma HLS ARRAY_PARTITION variable=residual_noise_counts block factor=4 dim=2

#pragma HLS ARRAY_PARTITION variable=exponent_counts complete dim=2
#pragma HLS bind_storage variable=exponent_counts type=RAM_2P impl=URAM

#pragma HLS ARRAY_PARTITION variable=residual_noise_counts block factor=4 dim=2

#pragma HLS ARRAY_PARTITION variable=residual_threshold_counts complete dim=4
#pragma HLS bind_storage variable=residual_threshold_counts	type=RAM_2P impl=URAM

#pragma HLS ARRAY_PARTITION variable=exponent_counts_x complete dim=4
#pragma HLS bind_storage variable=exponent_counts_x 		type=RAM_2P impl=URAM

#pragma HLS ARRAY_PARTITION variable=addr_sign complete dim=1
#pragma HLS ARRAY_PARTITION variable=data_sign complete dim=1

#pragma HLS ARRAY_PARTITION variable=addr_thre complete dim=1
#pragma HLS ARRAY_PARTITION variable=data_thre complete dim=1
    // clang-format on

    bool e;
    ap_uint<16> cnt;
    ap_uint<16> addr1;
    ap_uint<16> addr2;
    ap_uint<16> addr3;
    ap_uint<16> addr4;

    e = strm_e_in.read();
    while (!e) {
#pragma HLS PIPELINE II = 1
// clang-format off
#pragma HLS DEPENDENCE variable=num_nonzeros_counts_7x7 	inter false
#pragma HLS DEPENDENCE variable=exponent_counts 			inter false
#pragma HLS DEPENDENCE variable=residual_noise_counts 		inter false
#pragma HLS DEPENDENCE variable=num_nonzeros_counts_8x1 	inter false
#pragma HLS DEPENDENCE variable=num_nonzeros_counts_1x8 	inter false
#pragma HLS DEPENDENCE variable=exponent_counts_x 			inter false
#pragma HLS DEPENDENCE variable=exponent_counts_dc 			inter false
#pragma HLS DEPENDENCE variable=residual_noise_counts_dc 	inter false
//#pragma HLS DEPENDENCE variable=data_thre 					inter false

#pragma HLS DEPENDENCE variable=residual_threshold_counts 	inter true RAW distance=5
#pragma HLS DEPENDENCE variable=sign_counts 				inter true RAW distance=4
        // clang-format on

        ap_uint<4> sel_tab = strm_sel_tab.read();
        e = strm_e_in.read();
        bool value = strm_cur_bit.read();
        strm_bit.write(value);
        addr1 = strm_addr1.read();
        addr2 = strm_addr2.read();
        addr3 = strm_addr3.read();
        addr4 = strm_addr4.read();

        if (sel_tab == NZ_CNT_7x7) {
            cnt = num_nonzeros_counts_7x7[ap_color][addr1][addr2][addr3];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(NZ_CNT_7x7);
            record_and_update(value, cnt);

            num_nonzeros_counts_7x7[ap_color][addr1][addr2][addr3] = cnt;
        } else if (sel_tab == EXP_CNT) {
            ap_uint<64> ram_data;
            ram_data = exponent_counts[ap_color][addr1][addr2][addr3(3, 2)][addr4];
            cnt = ram_data(((addr3(1, 0) + 1) << 4) - 1, addr3(1, 0) << 4);

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(EXP_CNT);
            record_and_update(value, cnt);

            ram_data(((addr3(1, 0) + 1) << 4) - 1, addr3(1, 0) << 4) = cnt;
            exponent_counts[ap_color][addr1][addr2][addr3(3, 2)][addr4] = ram_data;
        } else if (sel_tab == SIGN_CNT) {
            ap_uint<8> rd_addr = (ap_color, addr1(1, 0), addr2(3, 0));
            if (rd_addr == addr_sign[3])
                cnt = data_sign[3];
            else if (rd_addr == addr_sign[2])
                cnt = data_sign[2];
            else if (rd_addr == addr_sign[1])
                cnt = data_sign[1];
            else if (rd_addr == addr_sign[0])
                cnt = data_sign[0];
            else
                cnt = sign_counts[ap_color][addr1][addr2];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(SIGN_CNT);

            record_and_update(value, cnt);

            sign_counts[ap_color][addr1][addr2] = cnt;
            addr_sign[0] = addr_sign[1];
            addr_sign[1] = addr_sign[2];
            addr_sign[2] = addr_sign[3];
            addr_sign[3] = (ap_color, addr1(1, 0), addr2(3, 0));
            data_sign[0] = data_sign[1];
            data_sign[1] = data_sign[2];
            data_sign[2] = data_sign[3];
            data_sign[3] = cnt;

        } else if (sel_tab == NOIS_CNT) {
            cnt = residual_noise_counts[ap_color][addr1][addr2][addr3];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(NOIS_CNT);
            record_and_update(value, cnt);

            residual_noise_counts[ap_color][addr1][addr2][addr3] = cnt;
        } else if (sel_tab == NZ_CNT_8x1) {
            cnt = num_nonzeros_counts_8x1[ap_color][addr1][addr2][addr3][addr4];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(NZ_CNT_8x1);

            record_and_update(value, cnt);
            num_nonzeros_counts_8x1[ap_color][addr1][addr2][addr3][addr4] = cnt;
        } else if (sel_tab == NZ_CNT_1x8) {
            cnt = num_nonzeros_counts_1x8[ap_color][addr1][addr2][addr3][addr4];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(NZ_CNT_1x8);

            record_and_update(value, cnt);
            num_nonzeros_counts_1x8[ap_color][addr1][addr2][addr3][addr4] = cnt;
        } else if (sel_tab == EXP_CNT_X) {
            ap_uint<64> ram_data;
            ram_data = exponent_counts_x[ap_color][addr1][addr2][addr3(3, 2)][addr4];
            cnt = ram_data(((addr3(1, 0) + 1) << 4) - 1, addr3(1, 0) << 4);

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(EXP_CNT_X);

            record_and_update(value, cnt);
            ram_data(((addr3(1, 0) + 1) << 4) - 1, addr3(1, 0) << 4) = cnt;
            exponent_counts_x[ap_color][addr1][addr2][addr3(3, 2)][addr4] = ram_data;
        } else if (sel_tab == THRE_CNT) {
            ap_uint<64> ram_data;

            ap_uint<22> rd_addr = (ap_color, addr1(7, 0), addr2(7, 0), addr3(6, 2));
            if (rd_addr == addr_thre[4])
                ram_data = data_thre[4];
            else if (rd_addr == addr_thre[3])
                ram_data = data_thre[3];
            else if (rd_addr == addr_thre[2])
                ram_data = data_thre[2];
            else if (rd_addr == addr_thre[1])
                ram_data = data_thre[1];
            else if (rd_addr == addr_thre[0])
                ram_data = data_thre[0];
            else
                ram_data = residual_threshold_counts[ap_color][addr1][addr2][addr3(6, 2)];

            if (addr3(1, 0) == 0)
                cnt = ram_data(15, 0);
            else if (addr3(1, 0) == 1)
                cnt = ram_data(31, 16);
            else if (addr3(1, 0) == 2)
                cnt = ram_data(47, 32);
            else if (addr3(1, 0) == 3)
                cnt = ram_data(63, 48);

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(THRE_CNT);

            record_and_update(value, cnt);

            if (addr3(1, 0) == 0)
                ram_data(15, 0) = cnt;
            else if (addr3(1, 0) == 1)
                ram_data(31, 16) = cnt;
            else if (addr3(1, 0) == 2)
                ram_data(47, 32) = cnt;
            else if (addr3(1, 0) == 3)
                ram_data(63, 48) = cnt;

            residual_threshold_counts[ap_color][addr1][addr2][addr3(6, 2)] = ram_data;

            addr_thre[0] = addr_thre[1];
            addr_thre[1] = addr_thre[2];
            addr_thre[2] = addr_thre[3];
            addr_thre[3] = addr_thre[4];
            addr_thre[4] = (ap_color, addr1(7, 0), addr2(7, 0), addr3(6, 2));
            data_thre[0] = data_thre[1];
            data_thre[1] = data_thre[2];
            data_thre[2] = data_thre[3];
            data_thre[3] = data_thre[4];
            data_thre[4] = ram_data;

        } else if (sel_tab == EXP_CNT_DC) {
            cnt = exponent_counts_dc[addr1][addr2][addr3];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(EXP_CNT_DC);

            record_and_update(value, cnt);
            exponent_counts_dc[addr1][addr2][addr3] = cnt;
        } else if (sel_tab == NOIS_CNT_DC) {
            cnt = residual_noise_counts_dc[addr1][addr2];

            strm_prob.write(calc_prob(cnt));
            strm_e.write(false);
            strm_tab_dbg.write(NOIS_CNT_DC);

            record_and_update(value, cnt);
            residual_noise_counts_dc[addr1][addr2] = cnt;
        }
    }
    //            std::cout<<std::endl<<" min addr1: "<<min_addr1<<" max_addr1: "<<max_addr1<<" min_addr3:
    //            "<<min_addr3<<" max_addr3: "<<max_addr3<<std::endl<<std::endl;
    strm_e.write(true);
}

} // namespace details
} // namespace codec
} // namespace xf
