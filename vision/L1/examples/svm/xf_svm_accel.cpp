/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_svm_config.h"

void svm_accel(xf::cv::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_1, XF_NPPC1>& Input1,
               xf::cv::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_1, XF_NPPC1>& Input2,
               unsigned short ind1,
               unsigned short ind2,
               unsigned short frac1,
               unsigned short frac2,
               unsigned short n,
               unsigned char& out_frac,
               ap_int<32>& resultFIX) {
    xf::cv::SVM<XF_16SC1, XF_16SC1, XF_32SC1, 1, IN_ARRAY_SIZE_1, 1, IN_ARRAY_SIZE_2, XF_NPPC1, NO_OF_KERNEL_ELEMENTS>(
        Input1, Input2, ind1, ind2, frac1, frac2, n, &out_frac, &resultFIX);
}
