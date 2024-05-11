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
#define INPUT_SAMPLES INPUT_WINDOW_VSIZE
#define INPUT_MARGIN(x, y) CEIL(x, (32 / sizeof(y)))
#define OUTPUT_SAMPLES (INPUT_WINDOW_VSIZE * INTERPOLATE_FACTOR) / DECIMATE_FACTOR

#ifndef COEFF_SEED
#define COEFF_SEED 0xC0FFEE
#endif

// Force reference model to ignore PARALLEL FACTOR for polyphase decomposition
#ifdef USING_UUT
#define P_PARA_DECI_POLY UUT_PARA_DECI_POLY
#define P_PARA_INTERP_POLY UUT_PARA_INTERP_POLY
#define P_SSR UUT_SSR
#else
#define P_PARA_DECI_POLY 1
#define P_PARA_INTERP_POLY 1
#define P_SSR 1
#endif

#ifndef USING_UUT
#undef PORT_API
#undef NUM_OUTPUTS
#undef DUAL_IP
#define PORT_API 0
#define NUM_OUTPUTS 1
#define DUAL_IP 0
#endif
#ifndef USE_COEFF_RELOAD
#define USE_COEFF_RELOAD 0
#endif
#ifndef CASC_LEN
#define CASC_LEN 1
#endif

#if (DUAL_IP == 1 && PORT_API == 1)
#define DUAL_INPUT_SAMPLES 1
#else
#define DUAL_INPUT_SAMPLES 0
#endif
