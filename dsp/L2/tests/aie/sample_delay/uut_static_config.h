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
#ifndef DATA_OUT_TYPE
#define DATA_OUT_TYPE DATA_TYPE
#endif

#ifndef NITER
#define NITER 16
#endif

#ifndef DELAY_INIT_VALUE
#define DELAY_INIT_VALUE 0
#endif

#ifndef ALL_TESTS
#define ALL_TESTS 0
#endif

#define INPUT_SAMPLES WINDOW_VSIZE* NITER
#define OUTPUT_SAMPLES WINDOW_VSIZE* NITER