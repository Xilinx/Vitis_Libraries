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

#ifndef USE_PKT_SWITCHING
#define USE_PKT_SWITCHING 0
#endif

#if defined(USING_UUT)
#if (USE_PKT_SWITCHING != 0)
#define PKT_INPUT_FILE "./data/input_pkts.csv"
#define PKT_OUTPUT_FILE "./data/output_pkts.txt"
#endif
#else
// Ref model doesn't use packet switching.
#undef USE_PKT_SWITCHING
#define USE_PKT_SWITCHING 0
#endif

#ifdef USING_UUT
#if (USE_PKT_SWITCHING == 0)

#undef NPORT_I
#undef NPORT_O
#define NPORT_I CASC_LEN
#define NPORT_O UUT_SSR
#endif

#else
// Ref model doesn't use packet switching.
#undef USE_PKT_SWITCHING
#undef NPORT_I
#undef NPORT_O

#define USE_PKT_SWITCHING 0
#define NPORT_I 1
#define NPORT_O 1

#endif

#define NPLIO_I NPORT_I
#define NPLIO_O NPORT_O

#define PLIO_WIDTH 128
// Force PLIO bit width to 64-bit when packet switching
#if defined(USING_UUT)
#if (USE_PKT_SWITCHING != 0)
#undef PLIO_WIDTH
#define PLIO_WIDTH 64
#endif
#endif
