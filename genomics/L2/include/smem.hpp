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

/**
 * @file smem.hpp
 * @brief Header for SMEM Kernel.
 *
 * This file is part of Vitis Genomics Library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ap_int.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>

typedef ap_uint<4> uint4_t;
typedef ap_uint<512> uint512_t;
#define MAX_INTV 0
#define MAX_INTV_ALLOC 200
#define BATCH_SIZE 20
#define SEQ_LENGTH 150

extern "C" {
/**
 * @brief SMEM kernel takes the input sequence and BWT tranformation and generates
 * super maximal exact match.
 *
 * @param bwt bwt sequence
 * @param mem output
 * @param bwt_para bwt parameters
 * @param mem_num number of output maximal matches
 * @param seq input sequence
 * @param seq_len sequence length
 * @param batch_size batch size
 *
 */
void mem_collect_intv_core(ap_uint<512>* bwt,
                           ap_uint<256>* mem,
                           ap_uint<64>* bwt_para,
                           ap_uint<32>* mem_num,
                           uint8_t* seq,
                           uint8_t* seq_len,
                           int batch_size);
}
