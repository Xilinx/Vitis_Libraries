/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#include "xf_config_params.h"

// ROI in OpenCV coordinates: columns [TB_ROI_TLX, TB_ROI_BRX], rows [TB_ROI_TLY, TB_ROI_BRY].
// Non-square ROI (top half of 128x128) validates column/row mapping vs legacy row/col swap.
#ifndef TB_ROI_TLX
#define TB_ROI_TLX 0
#endif
#ifndef TB_ROI_TLY
#define TB_ROI_TLY 0
#endif
#ifndef TB_ROI_BRX
#define TB_ROI_BRX 127
#endif
#ifndef TB_ROI_BRY
#define TB_ROI_BRY 63
#endif
