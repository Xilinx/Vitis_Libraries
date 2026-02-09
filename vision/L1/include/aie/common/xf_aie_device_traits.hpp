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

#ifndef _XF_AIE_DEVICE_TRAITS_HPP_
#define _XF_AIE_DEVICE_TRAITS_HPP_

#include <type_traits>
//#include <aie_ml/lib/me_defines.h>
//#include <aie2ps/lib/me_version.h>

#if (__AIE_ARCH__ == 22) || (__AIE_ARCH__ == 21)
#define __SUPPORT_ADDMAC64__
#define __SUPPORT_MUL64__
#define __MASK_WITH64__
#define __SUPPORT_ACC64__
#define __SUPPORT_16BIT_VEC__
#endif

#if (__AIE_ARCH__ == 20)
#define __SUPPORT_ADDMAC32__
#endif

//----------------------------------
// Need to enable below as per need
//----------------------------------
/*
#if (__AIE_ARCH__ == 10)
#define __SUPPORTS_ACC48__
#define __SUPPORTS_ACC80__
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22)
#define __SUPPORTS_ACC32__
#define __SUPPORTS_ACC64__
#endif

#if (__AIE_ARCH__ == 10)
#define __SUPPORTS_V8INT16__ 1
#define __SUPPORTS_V16INT16__ 1
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22)
#define __SUPPORTS_V8INT16__ 0
#define __SUPPORTS_V16INT16__ 0
#endif

#if (__AIE_ARCH__ == 20) || (__AIE_ARCH__ == 21) || (__AIEARCH__ == 20)
#define __VECTORIZATION__ 32
#elif (__AIE_ARCH__ == 22)
#define __VECTORIZATION__ 64
#endif
*/

#endif
