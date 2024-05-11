/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#include "ifft_transpose.h"

using namespace std;
using namespace ifft_transpose;

// #include <fstream>

// Approach:
// 0) 5 matrix rows incoming (polyphase row order) at two samples per PL cycle
// 1) Read 2 samples each from 5 input streams coming from 2D matrix rows
// 2) Write 10 samples into 5 memory banks, with cyclic rotating bank ID to allow simultaneous R/W via rows or columns
// 3) Write 2 samples to 5 output streams per PL cycle

// ------------------------------------------------------------
// Unpack & store samples
// ------------------------------------------------------------

void ifft_unpack(TT_STREAM sig_i[NSTREAM], TT_SAMPLE (&trans_i)[2 * NSTREAM]) {
    // FILE *fptr=fopen("/home/uvimalku/debug_inputdata.txt", "a");
    int ip;
    int r1, r2, i1, i2;
READ:
    for (int ss = 0; ss < NSTREAM; ss++) {
        (trans_i[(ss << 1) + 1], trans_i[(ss << 1) + 0]) = sig_i[ss].read();
        // fprintf(fptr, "stream = %d data = [%d, %d] : [%d, %d]\n", ss, (trans_i[(ss<<1)+1] >> 32), (trans_i[(ss<<1)+1]
        // % (1 << 31)), (trans_i[(ss<<1)] >> 32), (trans_i[(ss<<1)] % (1 << 31)));
    }
    // fclose(fptr);
}

// ------------------------------------------------------------
// Transpose Samples:
// ------------------------------------------------------------

void ifft_do_transpose(TT_SAMPLE (&trans_i)[2 * NSTREAM], TT_SAMPLE (&trans_o)[2 * NSTREAM]) {
    // FILE *fptr      = fopen("/home/uvimalku/debug.txt", "a");
    // FILE *buff_data = fopen("/home/uvimalku/buf_data.txt", "a");
    static bool ping = 0;
    static TT_SAMPLE buff[2][NSTREAM][POINT_SIZE / NSTREAM];
#pragma HLS array_partition variable = buff dim = 1 type = complete
#pragma HLS array_partition variable = buff dim = 2 type = complete
#pragma HLS bind_storage variable = buff type = RAM_T2P impl = uram
#pragma HLS dependence variable = buff type = intra false

    static int wr_bank_addr = 0;
    static int phase = 0;
#pragma HLS reset variable = wr_bank_addr
#pragma HLS reset variable = phase

    static int rd_bank_addr = 0;
    static int rd_grp = 0; //
    static int rd_col = 0; //
    static int rd_row = 0; //
#pragma HLS reset variable = rd_bank_addr
#pragma HLS reset variable = rd_grp
#pragma HLS reset variable = rd_col
#pragma HLS reset variable = rd_row

    // Write data mux:
    TT_SAMPLE wr_data0[NSTREAM], wr_data1[NSTREAM];

    for (int i = 0; i < NSTREAM; i++) {
        wr_data0[((SAMPLES_PER_READ * NSTREAM) - (SAMPLES_PER_READ * phase) + i) % NSTREAM] = trans_i[i * 2];
        wr_data1[(((SAMPLES_PER_READ + 1) * NSTREAM) - 1 - (SAMPLES_PER_READ * phase) + i) % NSTREAM] =
            trans_i[i * 2 + 1];
        // printf("i = %d wr index0 = %d wr index1 = %d data = [%d, %d]\n", i, ((2 * NSTREAM) - (SAMPLES_PER_READ *
        // phase) + i) % NSTREAM, (((SAMPLES_PER_READ+1) * NSTREAM) - 1 - (SAMPLES_PER_READ * phase) + i) % NSTREAM,
        // trans_i[i * 2] >> 32, trans_i[i * 2] % (1 << 31));
    }

    // Read address mux:
    // for odd numbers
    // int offset = ( rd_grp == (NSTREAM-1) ) ? DEPTH : 0;
    int offset = 0;
    int rd_addr0[NSTREAM], rd_addr1[NSTREAM];
    for (int i = 0; i < NSTREAM; i++) {
        rd_addr0[i] = rd_bank_addr + ((2 * phase) + (NSTREAM - i)) % NSTREAM;
        rd_addr1[i] = rd_bank_addr + ((2 * phase) + 1 + (NSTREAM - i)) % NSTREAM + offset;
    }

    // Read and Write:
    TT_SAMPLE rd_data0[NSTREAM], rd_data1[NSTREAM];
    int wr_addr0 = wr_bank_addr;
    int wr_addr1 = wr_bank_addr + 1;
    for (int ss = 0; ss < NSTREAM; ss++) {
        // std::cout << "writing into : pingpong = " << ping << " bank " << ss << " wr_addr0 " << wr_addr0 << " data ["
        // << (wr_data0[ss] >> 32) << "," <<  wr_data0[ss] % (1 << 31)  << "]\twr_bank_addr" << "\t" << wr_bank_addr <<
        // " \n";
        buff[ping][ss][wr_addr0] = wr_data0[ss];
        // std::cout << "writing into : pingpong = " << ping << " bank " << ss << " wr_addr1 " << wr_addr1 << " data ["
        // << (wr_data1[ss] >> 32) << "," <<  wr_data1[ss] % (1 << 31)  << "]\n";
        // printf("read from buffer = [%d, %d] ss = %d \n", ss, buff[ping][ss][wr_addr0] >> 32, buff[ping][ss][wr_addr0]
        // % (1 << 31));
        buff[ping][ss][wr_addr1] = wr_data1[ss];
        // printf("read from buffer = [%d, %d]\n", buff[ping][ss][wr_addr1] >> 32, buff[ping][ss][wr_addr1] % (1 <<
        // 31));
        rd_data0[ss] = buff[!ping][ss][rd_addr0[ss]];
        rd_data1[ss] = buff[!ping][ss][rd_addr1[ss]];
        // std::cout << rd_data0[ss] << "\t";
    }

    // int printed = 0;
    // if(ping == 1 && printed == 0){
    //     for (int ss=0; ss < NSTREAM; ss++) {
    //         printf("stream id = %d NSTREAM = %d DEPTH = %d NROW = %d POINT_SIZE = %d\n", ss, NSTREAM, DEPTH, NROW,
    //         POINT_SIZE);
    //         fprintf(buff_data, "stream number %d\n", ss);
    //         for(int i = 0; i < (DEPTH * NROW)/NSTREAM; i++){
    //             printf("i = %d fprintfing [%d, %d]\t", i, buff[0][ss][i] >> 32, buff[0][ss][i] % (1 << 31));
    //             fprintf(buff_data, "[%d, %d]\t", buff[0][ss][i] >> 32, buff[0][ss][i] % (1 << 31));
    //         }
    //         fprintf(buff_data, "\n");
    //     }
    //     printed = 1;
    // }
    // Read Data Output Mux:
    for (int i = 0; i < NSTREAM; i++) {
        trans_o[i * 2] = rd_data0[((2 * phase) + (NSTREAM - i)) % NSTREAM];
        trans_o[i * 2 + 1] = rd_data1[(((2 * phase) + (NSTREAM - i)) + 1) % NSTREAM];
    }

    //  printf("[%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] \n",
    //                  (trans_o[0] >> 32), (trans_o[0] % (1 << 31 )),
    //                  (trans_o[1] >> 32), (trans_o[1] % (1 << 31 )),
    //                  (trans_o[2] >> 32), (trans_o[2] % (1 << 31 )),
    //                  (trans_o[3] >> 32), (trans_o[3] % (1 << 31 )),
    //                  (trans_o[4] >> 32), (trans_o[4] % (1 << 31 )),
    //                  (trans_o[5] >> 32), (trans_o[5] % (1 << 31 )),
    //                  (trans_o[6] >> 32), (trans_o[6] % (1 << 31 )),
    //                   (trans_o[7] >> 32), (trans_o[7] % (1 << 31 )));
    /*  fclose(buff_data);*/
    // Update ping:
    bool last_wr = (wr_bank_addr == DEPTH * NROW - 2);
    if (last_wr == 1) {
        ping = !ping;
        // std::cout << "END OF PING PONG FRAME" << std::endl;
    }

    // Update write address:
    wr_bank_addr = (last_wr == 1) ? 0 : wr_bank_addr + SAMPLES_PER_READ;
    phase = (phase == NSTREAM - 1) ? 0 : phase + 1;

    // Update read address:
    int last_row = (rd_row == (POINT_SIZE_D1 - 2));
    // std::cout << "last_row: " << last_row << "\n";
    if (last_wr == 1)
        rd_col = 0;
    else if (last_row == 1)
        rd_col = rd_col + 1;
    if (last_row == 1) {
        rd_bank_addr = rd_col * NSTREAM;
    }
    // condition for odd numbers
    // if ( rd_grp == (2*NSTREAM-2) || rd_grp == (NSTREAM-1) ) {
    else if (rd_grp == (NSTREAM - 2) || rd_grp == (2 * NSTREAM - 2)) {
        rd_bank_addr = rd_bank_addr + DEPTH;
    }
    rd_row = (last_row == 1) ? 0 : rd_row + SAMPLES_PER_READ;
    //  rd_grp = (rd_grp == (2*NSTREAM-2)) ? 0 : rd_grp + SAMPLES_PER_READ;
    rd_grp = (rd_grp == (2 * NSTREAM - 2)) ? 0 : rd_grp + SAMPLES_PER_READ;
    // fclose(fptr);
}

// ------------------------------------------------------------
// Write output streams
// ------------------------------------------------------------

void ifft_write_streams(TT_SAMPLE (&trans_o)[2 * NSTREAM], TT_STREAM sig_o[NSTREAM]) {
    // FILE *fptr=fopen("/home/uvimalku/debug_write_streams.txt", "a");
    static bool running = 0;
    static ap_uint<13> startup = 0;
#pragma HLS reset variable = running
#pragma HLS reset variable = startup
// printf("[%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d] [%d, %d]\n",
//               (trans_o[0] >> 32), (trans_o[0] % (1 << 31 )),
//               (trans_o[1] >> 32), (trans_o[1] % (1 << 31 )),
//               (trans_o[2] >> 32), (trans_o[2] % (1 << 31 )),
//               (trans_o[3] >> 32), (trans_o[3] % (1 << 31 )),
//               (trans_o[4] >> 32), (trans_o[4] % (1 << 31 )),
//               (trans_o[5] >> 32), (trans_o[5] % (1 << 31 )),
//               (trans_o[6] >> 32), (trans_o[6] % (1 << 31 )),
//               (trans_o[7] >> 32), (trans_o[7] % (1 << 31 )));
WRITE:
    for (int ss = 0; ss < NSTREAM; ss++) {
        if (running == 1) {
            sig_o[ss].write((trans_o[(ss << 1) + 1], trans_o[(ss << 1) + 0]));
        }
    }
    if (startup == ap_uint<13>(DEPTH * NROW / 2 - 1)) {
        running = 1;
    } else {
        startup = ap_uint<13>(startup + 1);
    }
    // fclose(fptr);
}

// ------------------------------------------------------------
// Wrapper
// ------------------------------------------------------------

void ifft_transpose_wrapper(TT_STREAM sig_i[NSTREAM], TT_STREAM sig_o[NSTREAM]) {
#pragma HLS interface mode = ap_ctrl_none port = return
#pragma HLS pipeline II = 1
    TT_SAMPLE transpose_i[2 * NSTREAM], transpose_o[2 * NSTREAM];
#pragma HLS array_partition variable = transpose_i dim = 1
#pragma HLS array_partition variable = transpose_o dim = 1
    // for(int i = 0; i < POINT_SIZE/(NSTREAM); i++){
    // Unpack samples:
    ifft_unpack(sig_i, transpose_i);

    // Transpose samples:
    ifft_do_transpose(transpose_i, transpose_o);

    // Format output streams:
    ifft_write_streams(transpose_o, sig_o);
    // }
}