/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef __DEVICE_DEFS__
#define __DEVICE_DEFS__

// definitions for FFT R4 stage. AIE2 supports true Radix4, but AIE1 spoofs this with 2 stages of radix2.
#if __AIE_ARCH__ == 10
#define __FFT_SPOOF_R4__
#endif

#if __AIE_ARCH__ == 20 || __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
#define __FFT_TRUE_R4__
#endif

// reference to accumulator widths which do not exist on a device leads to a compile time error. These definitions allow
// such code to be claused out prior to that stage of compilation.
#if __AIE_ARCH__ == 10
#define __SUPPORTS_ACC48__
#define __SUPPORTS_ACC80__
#endif

#if __AIE_ARCH__ == 20 || __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22
#define __SUPPORTS_ACC32__
#define __SUPPORTS_ACC64__
#endif

#endif // __DEVICE_DEFS__
