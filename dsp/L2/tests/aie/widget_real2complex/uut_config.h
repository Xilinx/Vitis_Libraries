
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
// This file holds constants defining default values for the configuration
// of the parameterized Single Rate Asymmetrical FIR filter graph class.
// This file further was required to capture an extern declaration
// of the specific class defined by these particular values of the generic
// class, as this was required before aiecompiler supported generic
// class definitions.
//------------------------------------------------------------------------------
// UUT DEFAULT CONFIGURATION

#ifndef DATA_TYPE
#define DATA_TYPE int16
#endif

#ifndef DATA_OUT_TYPE
#define DATA_OUT_TYPE cint16
#endif

#ifndef WINDOW_VSIZE
#define WINDOW_VSIZE 256
#endif

#ifndef AIE_VARIANT
#define AIE_VARIANT 1
#endif

#ifndef NITER
#define NITER 16
#endif

#ifndef DIFF_TOLERANCE
#define DIFF_TOLERANCE 0
#endif

#ifndef CC_TOLERANCE
#define CC_TOLERANCE 0
#endif

#ifndef DATA_SEED
#define DATA_SEED 1
#endif

#ifndef DATA_STIM_TYPE
#define DATA_STIM_TYPE 0
#endif

#ifndef P_INPUT_SAMPLES_A
#define P_INPUT_SAMPLES_A P_INPUT_WINDOW_VSIZE_A* NITER
#endif
#ifndef P_INPUT_SAMPLES_B
#define P_INPUT_SAMPLES_B P_INPUT_WINDOW_VSIZE_B* NITER
#endif
#define P_OUTPUT_SAMPLES P_INPUT_WINDOW_VSIZE_A / P_DIM_AB* P_INPUT_WINDOW_VSIZE_B / P_DIM_AB* NITER

// END OF UUT CONFIGURATION
//------------------------------------------------------------------------------
