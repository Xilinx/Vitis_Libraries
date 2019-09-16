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

#ifndef _FDHWE_KERNEL_H_
#define _FDHWE_KERNEL_H_

#include <ap_int.h>
#include "hls_stream.h"
#include "xf_fintech/fd_hullwhite_engine.hpp"

typedef double mytype;

const unsigned int _ETSizeMax = 5;
const unsigned int _xGridMax = 101;
const unsigned int _legPSizeMax = 5;
const unsigned int _legRSizeMax = 5;

extern "C" void FDHWE_k0(mytype stoppingTimes[_ETSizeMax + 1],
                         mytype payerAccrualTime[_legPSizeMax + 1],
                         mytype receiverAccrualTime[_legRSizeMax + 1],
                         mytype receiverAccrualPeriod[_legRSizeMax + 1],
                         mytype iborTime[_legRSizeMax + 1],
                         mytype iborPeriod[_legRSizeMax + 1],
                         unsigned int ETSize,
                         unsigned int xGrid,
                         unsigned int legPSize,
                         unsigned int legRSize,
                         mytype a_,
                         mytype sigma_,
                         mytype theta_,
                         mytype nominal_,
                         mytype fixedRate_,
                         mytype floatingRate_,
                         unsigned int tGrid,
                         mytype NPV[1]);
#endif
