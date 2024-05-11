
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
// This file holds constants defining default values for the configuration
// of the parameterized Single Rate Asymmetrical FIR filter graph class.
// This file further was required to capture an extern declaration
// of the specific class defined by these particular values of the generic
// class, as this was required before aiecompiler supported generic
// class definitions.
//------------------------------------------------------------------------------
// UUT DEFAULT CONFIGURATION

#ifndef DATA_TYPE
#define DATA_TYPE cint32
#endif

#ifndef TWIDDLE_TYPE
#define TWIDDLE_TYPE cint16
#endif

#ifndef POINT_SIZE
#define POINT_SIZE 65536
#endif

#ifndef FFT_NIFFT
#define FFT_NIFFT 1
#endif

#ifndef SHIFT
#define SHIFT 10
#endif

#ifndef STIM_TYPE
#define STIM_TYPE 5
#endif

#ifndef NITER
#define NITER 2
#endif

#ifndef DYN_PT_SIZE
#define DYN_PT_SIZE 0
#endif

#ifndef CASC_LEN
#define CASC_LEN 1
#endif

#ifndef WINDOW_VSIZE
#define WINDOW_VSIZE 65536
#endif

#ifndef API_IO
#define API_IO 0
#endif

#ifndef USE_WIDGETS
#define USE_WIDGETS 0
#endif

#ifndef ROUND_MODE
#define ROUND_MODE 4
#endif

#ifndef SAT_MODE
#define SAT_MODE 1
#endif

#ifndef DATA_SEED
#define DATA_SEED 1
#endif

#ifndef AIE_VARIANT
#define AIE_VARIANT 1
#endif

#ifndef DIFF_TOLERANCE
#define DIFF_TOLERANCE 0
#endif

#ifndef CC_TOLERANCE
#define CC_TOLERANCE 0
#endif

#ifndef SSR
#define SSR 4
#endif

#ifndef TWIDDLE_MODE
#define TWIDDLE_MODE 0
#endif

// END OF UUT CONFIGURATION
//------------------------------------------------------------------------------