/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef __TEST_PARAMS_H__
#define __TEST_PARAMS_H__

// const unsigned int ROW = 8;
// const unsigned int COL = 8;
const unsigned int ROW = 64;
const unsigned int COL = 32;
// const unsigned int ROW = 256;
// const unsigned int COL = 128;
// const unsigned int ROW = 512;
// const unsigned int COL = 256;
const int BlkNum = 4;
const int CoreNum = COL / BlkNum;
const int row_num = ROW;
const int col_num = COL;

std::string fin = "data/in_" + std::to_string(ROW) + "_" + std::to_string(COL) + ".txt";
std::string fgld = "data/gld_" + std::to_string(ROW) + "_" + std::to_string(COL) + ".txt";
std::string fout = "data/out_" + std::to_string(ROW) + "_" + std::to_string(COL) + ".txt";

#endif
