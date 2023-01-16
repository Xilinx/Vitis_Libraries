/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_THRESHOLD_CONFIG_H_
#define _XF_THRESHOLD_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_threshold.hpp"
#include "xf_config_params.h"

#include "imgproc/xf_median_blur.hpp"

#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_otsuthreshold.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_cca_custom_imp.hpp"
#include "imgproc/xf_custom_bgr2y8.hpp"
/*#include "imgproc/xf_cvt_color.hpp"*/

typedef ap_uint<8> ap_uint8_t;
typedef ap_uint<64> ap_uint64_t;

/*  set the height and weight */
#define HEIGHT 1080
#define WIDTH 1920
#define STRIDE 2048

#define RGB2GRAY 0
#define GRAY 1

#define NPIX XF_NPPC1
#define NPC1 XF_NPPC1

#define TYPE XF_8UC1

#if L1NORM
#define NORM_TYPE XF_L1NORM
#elif L2NORM
#define NORM_TYPE XF_L2NORM
#endif

/* For median blur */
// Set the optimization type:
#if SPC == 1
#define NPC1 XF_NPPC1
#define PTR_WIDTH 32
#else

#define PTR_WIDTH 128
#endif

// Set the pixel depth:
#if GRAY
#define TYPE XF_8UC1
#else
#define TYPE XF_8UC3
#endif

#endif // end of _XF_THRESHOLD_CONFIG_H_
