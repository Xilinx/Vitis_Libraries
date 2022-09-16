/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#ifndef _DUT_TYPE_HPP_
#define _DUT_TYPE_HPP_

#include <string>

const int M = 120; // 117-120
const int t = 5;
const int alph = 5;
const int n = 252;
const int sbox = 0;
const int field = 1;
const bool security_margin = true;
const int N = (n * t);
const int RF = 8;
const int RP = 59;
const std::string PRIME = "0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001";

const unsigned int inputLength = 3; // 3, 4, 5, 6, 8, 10
#endif
