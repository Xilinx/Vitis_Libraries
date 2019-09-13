/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software 
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "xf_headers.h"
#include "xf_svm_config.h"


/*****************************************************************************
 * 	 main function: SVM core
 *****************************************************************************/
int main()
{
	float in_1[IN_ARRAY_SIZE_1], in_2[IN_ARRAY_SIZE_2];

	float a=0, bias=0.567;

	std::vector<float> v1, v2;

	// Input data feed
	for(int i=0; i<TOTAL_ARRAY_ELEMENTS; i++)
	{
		in_1[i] = a;
		in_2[i] = 0.2-a;

		a+=0.0008;
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

	static xf::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_1, XF_NPPC1> Input1(1,arr_size1);
	static xf::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_2, XF_NPPC1> Input2(1,arr_size2);

	// fixed point conversion of input data & filling into Mat
	ap_int<16> infix_1[IN_ARRAY_SIZE_1], infix_2[IN_ARRAY_SIZE_2];
	for(int i=0; i<TOTAL_ARRAY_ELEMENTS; i++)
	{
		Input1.write(i, in_1[i] * pow(2,IN_FRAC_BITS_1));
		Input2.write(i, in_2[i] * pow(2,IN_FRAC_BITS_2));
	}

	ap_int<32> resultFIX;

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	svm_accel(Input1, Input2, ind1,ind2, frac1, frac2, n, out_frac, resultFIX);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();

#endif
	int bias_fix = bias * pow(2,out_frac);
	resultFIX += bias_fix;

	float float_res_fix = resultFIX/pow(2,out_frac);

	// OpenCV reference function
//	double ocv_fl = std::dot(v1,v2) + bias;

	// Error computation
//	float diff = abs(float_res_fix)-abs(ocv_fl);
	std::cout<<"HLS fixed point output: "<<resultFIX<<std::endl;
	std::cout<<"HLS fix -> fl output: "<<float_res_fix<<std::endl;
//	cout<<"opencv floating point output: "<<ocv_fl<<endl;
//	cout<<"Absolute difference: "<<diff<<endl;

	return 0;
}
