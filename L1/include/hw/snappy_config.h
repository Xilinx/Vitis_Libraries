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
#ifndef _XFCOMPRESSION_SNAPPY_CONFIG_HPP_
#define _XFCOMPRESSION_SNAPPY_CONFIG_HPP_

/**
 * @file snappy_config.h
 * @brief Header containing some configuration parameters.
 *
 * This file is part of XF Compression Library host code for snappy compression
 */

// Max literal length should be less than max literal stream size
// max literal length = 60 will encode literal length in 1 byte

//#define LARGE_LIT_RANGE 1

namespace xf {
namespace compression {

#ifdef LARGE_LIT_RANGE
//#define MAX_LIT_COUNT 		4090	// not used to avoid clashes with other algorithms
//#define MAX_LIT_STREAM_SIZE	4096
const int c_snappyMaxLiteralCount = 4090;
const int c_snappyMaxLiteralStream = 4096;
#else
//#define MAX_LIT_COUNT 		60
//#define MAX_LIT_STREAM_SIZE	64
const int c_snappyMaxLiteralCount = 60;
const int c_snappyMaxLiteralStream = 64;
#endif
} // end namespace compression
} // end namespace xf

#endif // _XFCOMPRESSION_SNAPPY_CONFIG_HPP_
