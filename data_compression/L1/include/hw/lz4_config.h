/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef _XFCOMPRESSION_LZ4_CONFIG_H
#define _XFCOMPRESSION_LZ4_CONFIG_H

/**
 * @file lz4_config.h
 * @brief Header containing some configuration parameters.
 *
 * This file is part of XF Compression Library host code for lz4 compression.
 */

// avoid clashes with other algorithms
//#define MAX_LIT_COUNT	4096
namespace xf {
namespace compression {

const int c_lz4MaxLiteralCount = 4096; // MAX_LIT_COUNT;

} // end namespace compression
} // end namespace xf

#endif // _XFCOMPRESSION_LZ4_CONFIG_H
