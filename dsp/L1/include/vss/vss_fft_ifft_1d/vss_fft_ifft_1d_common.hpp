/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_VSS_FFT_COMMON_HPP_
#define _DSPLIB_VSS_FFT_COMMON_HPP_

static constexpr unsigned int modeAIEffts = 1;
static constexpr unsigned int modePLffts = 2;
template<unsigned int TP_POINT_SIZE, unsigned int TP_VSS_MODE, unsigned int TP_SSR>
static constexpr unsigned int fnPtSizeD1(){
  if(TP_VSS_MODE == modeAIEffts){
      unsigned int sqrtVal = TP_POINT_SIZE == 65536 ? 256
                       : TP_POINT_SIZE == 32768 ? 256
                       : TP_POINT_SIZE == 16384 ? 128
                       : TP_POINT_SIZE == 8192 ? 128
                       : TP_POINT_SIZE == 4096 ? 64
                       : TP_POINT_SIZE == 2048 ? 64
                       : TP_POINT_SIZE == 1024 ? 32
                       : TP_POINT_SIZE == 512 ? 32
                       : TP_POINT_SIZE == 256 ? 16
                       : TP_POINT_SIZE == 128 ? 16
                       : TP_POINT_SIZE == 64 ? 8
                       : TP_POINT_SIZE == 32 ? 8
                       : TP_POINT_SIZE == 16 ? 4
                       : 0;
  return sqrtVal;
  } else {
    return TP_POINT_SIZE/TP_SSR;
  }
  return 0;
}

#endif