/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <common/xf_aie_const.hpp>

// tile dimensions are normally computed by tiler but we need to
// hardcode these values to set the graph window sizes
#define IOU_THRESH_FL 0.6
#define IOU_F_BITS 12
#define IOU_THRESH ((int)(IOU_THRESH_FL * pow(2, IOU_F_BITS)))
#define MAX_DET 100

using DATA_TYPE = int16_t;
static constexpr int TILE_ELEMENTS = 128;
static constexpr int TILE_WINDOW_SIZE = (TILE_ELEMENTS * sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA = TILE_ELEMENTS;
static constexpr int TILE_WINDOW_SIZE_OUT = ((TILE_ELEMENTS + 1) * 4 * sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_OUT = (TILE_ELEMENTS + 1) * 4;

#define DUMP_BOXES 0

#endif //__CONFIG_H_
