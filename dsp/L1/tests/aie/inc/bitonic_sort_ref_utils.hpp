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
#ifndef _DSPLIB_BITONIC_SORT_REF_UTILS_HPP_
#define _DSPLIB_BITONIC_SORT_REF_UTILS_HPP_

#include "device_defs.h"
#include "aie_api/utils.hpp" // for vector print function

template <typename TT>
inline void bitonicMergeRecursive(TT* arr, int low_index, int count, unsigned int direction) {
    if (count > 1) {
        int k = count / 2;
        for (int i = low_index; i < low_index + k; i++) {
            if (direction == 1) { // TODO: check if this resolves, or is determined at runtime?
                TT left = arr[i] < arr[i + k] ? arr[i] : arr[i + k];
                TT right = arr[i] < arr[i + k] ? arr[i + k] : arr[i];
                arr[i] = left;
                arr[i + k] = right;
            } else if (direction == 0) {
                TT right = arr[i] < arr[i + k] ? arr[i] : arr[i + k];
                TT left = arr[i] < arr[i + k] ? arr[i + k] : arr[i];
                arr[i] = left;
                arr[i + k] = right;
            }
        }
        bitonicMergeRecursive<TT>(arr, low_index, k, direction);
        bitonicMergeRecursive<TT>(arr, low_index + k, k, direction);
    }
};

template <typename TT>
inline void bitonicSortRecursive(TT* arr, int low_index, int count, unsigned int direction) {
    if (count > 1) {
        int k = count / 2;
        bitonicSortRecursive<TT>(arr, low_index, k, 1);
        bitonicSortRecursive<TT>(arr, low_index + k, k, 0);
        bitonicMergeRecursive<TT>(arr, low_index, count, direction);
    }
};

#endif