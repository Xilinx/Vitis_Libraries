/*
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
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

#ifndef XF_DATA_MOVER_MM2S_4D_HPP
#define XF_DATA_MOVER_MM2S_4D_HPP

/*
 * The mm2s_4d API is a configurable L2 kernel that feeds N streams of same width
 * from buffers. Unlike the mm2s_mp API, this kernel is argumented with runtime
 * control code from a buffer, to determing how the data is being read in 4D patterns.
 *
 * As HLS cannot accept template as kernel top, the API must be instantiated
 * though API metadata into a CPP file.
 * Refer to L2/tests/mm2s_4d_and_s2mm_4d for usecase example.
 */

#endif
