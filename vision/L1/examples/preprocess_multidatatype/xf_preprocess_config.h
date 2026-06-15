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

#ifndef _XF_PREPROCESS_CONFIG_H_
#define _XF_PREPROCESS_CONFIG_H_

#include "xf_config_params.h"

#ifndef TB_DATATYPE
#define TB_DATATYPE data_types::FP32
#endif

#ifndef TB_RESIZE_WIDTH
#define TB_RESIZE_WIDTH 0
#endif

#ifndef TB_RESIZE_HEIGHT
#define TB_RESIZE_HEIGHT 0
#endif

#ifndef TB_ALPHA0
#define TB_ALPHA0 0.0f
#endif
#ifndef TB_ALPHA1
#define TB_ALPHA1 0.0f
#endif
#ifndef TB_ALPHA2
#define TB_ALPHA2 0.0f
#endif
#ifndef TB_ALPHA3
#define TB_ALPHA3 0.0f
#endif

#ifndef TB_BETA0
#define TB_BETA0 1.0f
#endif
#ifndef TB_BETA1
#define TB_BETA1 1.0f
#endif
#ifndef TB_BETA2
#define TB_BETA2 1.0f
#endif
#ifndef TB_BETA3
#define TB_BETA3 1.0f
#endif

#endif
