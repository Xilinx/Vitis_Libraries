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
 * @file kernel_renumber.hpp
 *
 * @brief This file contains top function of test case.
 */

#ifndef _XF_GRAPH_KERNEL_RENUMBER_HPP_
#define _XF_GRAPH_KERNEL_RENUMBER_HPP_

#include "xf_graph_L2.hpp"

extern "C" void kernel_renumber(int32_t* configs,
                                ap_int<DWIDTHS>* oldCids,
                                ap_int<DWIDTHS>* mapCid0,
                                ap_int<DWIDTHS>* mapCid1,
                                ap_int<DWIDTHS>* newCids);

#endif // _XF_GRAPH_KERNEL_RENUMBER_HPP_
