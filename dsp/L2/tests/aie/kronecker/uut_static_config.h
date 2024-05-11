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
#define INPUT_WINDOW_VSIZE_A MAT_A_ROWS* MAT_A_COLS* NUM_FRAMES
#define INPUT_WINDOW_VSIZE_B MAT_B_ROWS* MAT_B_COLS* NUM_FRAMES

#ifndef INPUT_SAMPLES_A
#define INPUT_SAMPLES_A INPUT_WINDOW_VSIZE_A* NITER
#endif
#ifndef INPUT_SAMPLES_B
#define INPUT_SAMPLES_B INPUT_WINDOW_VSIZE_B* NITER
#endif
#define OUTPUT_SAMPLES DIM_A_ROWS* DIM_A_COLS* DIM_B_ROWS* DIM_B_COLS* NUM_FRAMES