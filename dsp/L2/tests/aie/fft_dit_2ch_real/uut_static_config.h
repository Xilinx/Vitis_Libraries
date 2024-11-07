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
#define INPUT_SAMPLES WINDOW_VSIZE* NITER
#define OUTPUT_SAMPLES WINDOW_VSIZE* NITER

#ifndef INPUT_WINDOW_VSIZE
#if DATA_TYPE == cint16
#define INPUT_WINDOW_VSIZE ((8 * DYN_PT_SIZE) + POINT_SIZE)
#else
#define INPUT_WINDOW_VSIZE ((4 * DYN_PT_SIZE) + POINT_SIZE)
#endif
#endif