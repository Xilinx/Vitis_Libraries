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
// This file holds constants defining default values for the configuration
// of the parameterized Single Rate Asymmetrical FIR filter graph class.
// This file further was required to capture an extern declaration
// of the specific class defined by these particular values of the generic
// class, as this was required before aiecompiler supported generic
// class definitions.
//------------------------------------------------------------------------------
// UUT DEFAULT CONFIGURATION
#ifndef DATA_TYPE
#define DATA_TYPE cint16
#endif
#ifndef INPUT_WINDOW_VSIZE
#define INPUT_WINDOW_VSIZE 256
#endif
#ifndef MIXER_MODE
#define MIXER_MODE 2
#endif
#ifndef P_API
#define P_API 0 // window
#endif
#ifndef UUT_SSR
#define UUT_SSR 1
#endif
#ifndef DDS_PHASE_INC
#define DDS_PHASE_INC 0xD6555555
#endif
#ifndef INITIAL_DDS_OFFSET
#define INITIAL_DDS_OFFSET 0
#endif

#ifndef INPUT_FILE
#define INPUT_FILE "data/input.txt"
#endif
#ifndef OUTPUT_FILE
#define OUTPUT_FILE "data/output.txt"
#endif

#ifndef NITER
#define NITER 16
#endif

#define INPUT_SAMPLES INPUT_WINDOW_VSIZE* NITER
#define INPUT_MARGIN(x, y) CEIL(x, (32 / sizeof(y)))
#define OUTPUT_SAMPLES INPUT_WINDOW_VSIZE* NITER

// END OF UUT CONFIGURATION
//------------------------------------------------------------------------------
