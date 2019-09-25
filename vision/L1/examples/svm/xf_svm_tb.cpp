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

#include "common/xf_headers.hpp"
#include "xf_svm_config.h"

/*****************************************************************************
 * 	 main function: SVM core
 *****************************************************************************/
int main() {
    float in_1[IN_ARRAY_SIZE_1], in_2[IN_ARRAY_SIZE_2];

    float a = 0, bias = 0.567;

    std::vector<float> v1, v2;

    // Input data feed
    for (int i = 0; i < TOTAL_ARRAY_ELEMENTS; i++) {
        in_1[i] = a;
        in_2[i] = 0.2 - a;

        a += 0.0008;
    }

    unsigned char out_frac;
    //	int resultFIX;

    // top function call (fixed point computation)
    uint16_t ind1 = INDEX_ARR_1;
    uint16_t ind2 = INDEX_ARR_2;
    uint16_t arr_size1 = IN_ARRAY_SIZE_1;
    uint16_t arr_size2 = IN_ARRAY_SIZE_2;
    uint16_t frac1 = IN_FRAC_BITS_1;
    uint16_t frac2 = IN_FRAC_BITS_2;
    uint16_t n = NO_OF_KERNEL_ELEMENTS;

    static xf::cv::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_1, XF_NPPC1> Input1(1, arr_size1);
    static xf::cv::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_2, XF_NPPC1> Input2(1, arr_size2);

    // fixed point conversion of input data & filling into Mat
    ap_int<16> infix_1[IN_ARRAY_SIZE_1], infix_2[IN_ARRAY_SIZE_2];
    for (int i = 0; i < TOTAL_ARRAY_ELEMENTS; i++) {
        Input1.write(i, in_1[i] * pow(2, IN_FRAC_BITS_1));
        Input2.write(i, in_2[i] * pow(2, IN_FRAC_BITS_2));
    }

    ap_int<32> resultFIX;

    svm_accel(Input1, Input2, ind1, ind2, frac1, frac2, n, out_frac, resultFIX);
    int bias_fix = bias * pow(2, out_frac);
    resultFIX += bias_fix;

    float float_res_fix = resultFIX / pow(2, out_frac);

    // OpenCV reference function
    //	double ocv_fl = std::dot(v1,v2) + bias;

    // Error computation
    //	float diff = abs(float_res_fix)-abs(ocv_fl);
    std::cout << "HLS fixed point output: " << resultFIX << std::endl;
    std::cout << "HLS fix -> fl output: " << float_res_fix << std::endl;
    //	cout<<"opencv floating point output: "<<ocv_fl<<endl;
    //	cout<<"Absolute difference: "<<diff<<endl;

    return 0;
}
