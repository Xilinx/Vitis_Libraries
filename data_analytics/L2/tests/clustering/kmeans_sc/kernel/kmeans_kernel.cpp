/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

/**
 *
 * @file aes256CbcEncryptKernel.cpp
 * @brief kernel code of Cipher Block Chaining (CBC) block cipher mode of operation.
 * This file is part of Vitis Security Library.
 *
 * @detail Containing scan, distribute, encrypt, merge, and write-out functions.
 *
 */

#include "xf_data_analytics/clustering/kmeansTrain.hpp"
#include <ap_int.h>
#include <hls_stream.h>
#include "config.hpp"
#include "kmeans_acc.hpp"

// @brief top of kernel
void kmeans_acc::hls_kernel(int bufferSize, int NC, ap_uint<512>* inputData, ap_uint<512>* centers) {
    xf::data_analytics::clustering::kMeansTrain<DType, DIM, KC, PCU, PDV>(inputData, centers);
}
