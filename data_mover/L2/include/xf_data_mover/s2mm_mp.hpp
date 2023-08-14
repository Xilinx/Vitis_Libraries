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

#ifndef XF_DATA_MOVER_S2MM_MP_HPP
#define XF_DATA_MOVER_S2MM_MP_HPP

/*
 * The s2mm_mp API is a configurable L2 kernel that saves N streams of same width
 * into buffers.
 *
 * As HLS cannot accept template as kernel top, the API must be instantiated
 * though API metadata into a CPP file.
 * Refer to L2/tests/mm2s_mp_and_s2mm_mp for usecase example.
 */

#endif
