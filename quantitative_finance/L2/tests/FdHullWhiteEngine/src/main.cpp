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

#ifndef HLS_TEST
#include "xcl2.hpp"
#endif
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include "ap_int.h"
#include "utils.hpp"
#include "FDHWE_kernel.hpp"

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

int main(int argc, const char* argv[]) {
    std::cout
        << "******************************************************************************************************"
        << std::endl;
    std::cout
        << "                     Finite-difference Hull-White Bermudan Swaption Pricing Engine                    "
        << std::endl;
    std::cout
        << "******************************************************************************************************"
        << std::endl;

    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    // number of call for kernel
    int loop_nm = 1;

    // allocate memory at host
    mytype* stoppingTimes_alloc = aligned_alloc<mytype>(_ETSizeMax + 1);
    mytype* payerAccrualTime_alloc = aligned_alloc<mytype>(_legPSizeMax + 1);
    mytype* receiverAccrualTime_alloc = aligned_alloc<mytype>(_legRSizeMax + 1);
    mytype* receiverAccrualPeriod_alloc = aligned_alloc<mytype>(_legRSizeMax + 1);
    mytype* iborTime_alloc = aligned_alloc<mytype>(_legRSizeMax + 1);
    mytype* iborPeriod_alloc = aligned_alloc<mytype>(_legRSizeMax + 1);
    mytype* output = aligned_alloc<mytype>(1);

    // setup k0 params
    int err = 0;
    mytype minErr = 1e-8;
    unsigned int _tGrid = 10;
    unsigned int _ETSize = 5;
    unsigned int _xGrid = 11;
    unsigned int _legPSize = 5;
    unsigned int _legRSize = 5;

    mytype Dates[_ETSize + 2] = {
        0,    // year 0
        365,  // year 1
        730,  // year 2
        1096, // year 3
        1461, // year 4
        1826, // year 5
        2191  // year 6
    };

    mytype Dates_ibor[_ETSize + 2] = {
        0,    // year 0
        365,  // year 1
        730,  // year 2
        1098, // year 3
        1462, // year 4
        1826, // year 5
        2191  // year 6
    };

loop_stoppingTimes:
    for (unsigned int i = 0; i <= _ETSize; i++) {
        if (0 == i) {
            stoppingTimes_alloc[i] = 0.99 * (1.0 / 365.0);
        } else {
            stoppingTimes_alloc[i] = (Dates[i] - Dates[0]) / 365.0;
        }
        cout << "stoppingTimes_alloc[" << i << "] = " << stoppingTimes_alloc[i] << endl;
        //	0.0027123287671233		1.0		2.0		3.002739726027397
        // 4.002739726027397		5.002739726027397
    }

// accrual times should be calculated according to the payment period,
// and it must ensure the loop which is used to calculate NPV in class FdHullWhiteEngine can hit the specific exercise
// time
loop_payerAccrualTime:
    for (unsigned int i = 0; i <= _legPSize; i++) {
        payerAccrualTime_alloc[i] = (Dates[i + 1] - Dates[0]) / 365.0;
        cout << "payerAccrualTime_alloc[" << i << "] = " << payerAccrualTime_alloc[i] << endl;
        //	1.0		2.0		3.002739726027397		4.002739726027397
        // 5.002739726027397		6.002739726027397
    }

// receiverAccrualTime is the same as payerAccrualTime
loop_receiverAccrualTime:
    for (unsigned int i = 0; i <= _legRSize; i++) {
        receiverAccrualTime_alloc[i] = (Dates[i + 1] - Dates[0]) / 365.0;
        cout << "receiverAccrualTime_alloc[" << i << "] = " << receiverAccrualTime_alloc[i] << endl;
        //	1.0		2.0		3.002739726027397		4.002739726027397
        // 5.002739726027397		6.002739726027397
    }

// receiver accrual period is calculated by divide them with 360.0
loop_receiverAccrualPeriod:
    for (unsigned int i = 0; i <= _legRSize; i++) {
        receiverAccrualPeriod_alloc[i] = (Dates[i + 1] - Dates[0]) / 360.0;
        cout << "receiverAccrualPeriod_alloc[" << i << "] = " << receiverAccrualPeriod_alloc[i] << endl;
        //	1.013888888888889		2.027777777777778		3.044444444444444
        // 4.058333333333333
        // 5.072222222222222		6.086111111111111
    }

// the dates in ibor calculation is different from its in fixed/floating rate calculation,
// so it needs to be re-calculated here
loop_iborTime:
    for (unsigned int i = 0; i <= _legRSize; i++) {
        iborTime_alloc[i] = (Dates_ibor[i + 1] - Dates_ibor[0]) / 365.0;
        cout << "iborTime_alloc[" << i << "] = " << iborTime_alloc[i] << endl;
        //	1.0		2.0		3.008219178082192		4.005479452054795
        // 5.002739726027397		6.002739726027397
    }

// since the receiver accrual period is calculated by divide them with 360.0 instead of 365.0,
// it needs to be re-calculated here besides the calculation of receiver accrual time
loop_iborPeriod:
    for (unsigned int i = 0; i <= _legRSize; i++) {
        iborPeriod_alloc[i] = (Dates_ibor[i + 1] - Dates_ibor[0]) / 360.0;
        cout << "iborPeriod_alloc[" << i << "] = " << iborPeriod_alloc[i] << endl;
        //	1.013888888888889		2.027777777777778		3.05		4.061111111111111
        // 5.072222222222222		6.086111111111111
    }

    mytype a_ = 0.055228873373796609;
    mytype sigma_ = 0.0061062754654949824;
    mytype theta_ = 0.5;
    mytype nominal_ = 1000.0;

    mytype floatingRate_ = 0.048758250000000003;

    mytype fixedRate_;
    mytype NPV_golden;
    // ATM
    fixedRate_ = 0.04996730017903863;
    if (101 == _xGrid) {
        if (100 == _tGrid) {
            NPV_golden = 13.2020427520457648; // t = 100, x = 101
        } else if (500 == _tGrid) {
            NPV_golden = 13.2017531875934573; // t = 500, x = 101
        } else if (1000 == _tGrid) {
            NPV_golden = 13.2017441801818745; // t = 1000, x = 101
        } else if (1500 == _tGrid) {
            NPV_golden = 13.2017425108939186; // t = 1500, x = 101
        } else if (2000 == _tGrid) {
            NPV_golden = 13.2017419256407305; // t = 2000, x = 101
        } else if (2500 == _tGrid) {
            NPV_golden = 13.2017416541563808; // t = 2500, x = 101
        } else if (3000 == _tGrid) {
            NPV_golden = 13.2017415069034296; // t = 3000, x = 101
        } else if (3500 == _tGrid) {
            NPV_golden = 13.2017414181875719; // t = 3500, x = 101
        } else if (4000 == _tGrid) {
            NPV_golden = 13.2017413604889153; // t = 4000, x = 101
        }
    } else if (51 == _xGrid) {
        if (100 == _tGrid) {
            NPV_golden = 13.1974405442079519; // t = 100, x = 51
        } else if (500 == _tGrid) {
            NPV_golden = 13.1971500768953351; // t = 500, x = 51
        } else if (1000 == _tGrid) {
            NPV_golden = 13.1971410040771051; // t = 1000, x = 51
        } else if (1500 == _tGrid) {
            NPV_golden = 13.1971393226786802; // t = 1500, x = 51
        } else if (2000 == _tGrid) {
            NPV_golden = 13.1971387331896945; // t = 2000, x = 51
        } else if (2500 == _tGrid) {
            NPV_golden = 13.1971384597466272; // t = 2500, x = 51
        } else if (3000 == _tGrid) {
            NPV_golden = 13.1971383114284624; // t = 3000, x = 51
        } else if (3500 == _tGrid) {
            NPV_golden = 13.1971382220698334; // t = 3500, x = 51
        } else if (4000 == _tGrid) {
            NPV_golden = 13.1971381639543281; // t = 4000, x = 51
        }
    } else if (11 == _xGrid) {
        if (10 == _tGrid) {
            NPV_golden = 13.13811089598575;
        }
    }
/*
// ITM
fixedRate_ = 0.03997384014323091;
NPV_golden = 42.318855599968728;		// t = 120, x = 101
//OTM
fixedRate_ = 0.059960760214846351;
NPV_golden = 2.6210458836641473;		// t = 120, x = 101
*/

#ifndef HLS_TEST
    // time counter
    struct timeval start_time, end_time;

    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    // cl::Program::Binaries xclBins = xcl::import_binary_file("../xclbin/MCAE_u250_hw.xclbin");
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    cl::Kernel kernel_FDHWE_k0(program, "FDHWE_k0");
    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_o[7];
    mext_o[0].obj = stoppingTimes_alloc;
    mext_o[0].param = 0;

    mext_o[1].obj = payerAccrualTime_alloc;
    mext_o[1].param = 0;

    mext_o[2].obj = receiverAccrualTime_alloc;
    mext_o[2].param = 0;

    mext_o[3].obj = receiverAccrualPeriod_alloc;
    mext_o[3].param = 0;

    mext_o[4].obj = iborTime_alloc;
    mext_o[4].param = 0;

    mext_o[5].obj = iborPeriod_alloc;
    mext_o[5].param = 0;

    mext_o[6].obj = output;
    mext_o[6].param = 0;
    for (int i = 0; i < 7; ++i) {
#ifndef USE_HBM
        mext_o[i].flags = XCL_MEM_DDR_BANK0;
#else
        mext_o[i].flags = XCL_BANK0;
#endif
    }

    // create device buffer and map dev buf to host buf
    cl::Buffer stoppingTimes_buf, payerAccrualTime_buf, receiverAccrualTime_buf, receiverAccrualPeriod_buf,
        iborTime_buf, iborPeriod_buf, output_buf;
    stoppingTimes_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   sizeof(mytype) * (_ETSize + 1), &mext_o[0]);
    payerAccrualTime_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      sizeof(mytype) * (_legPSize + 1), &mext_o[1]);
    receiverAccrualTime_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(mytype) * (_legRSize + 1), &mext_o[2]);
    receiverAccrualPeriod_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                           sizeof(mytype) * (_legRSize + 1), &mext_o[3]);
    iborTime_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                              sizeof(mytype) * (_legRSize + 1), &mext_o[4]);
    iborPeriod_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                sizeof(mytype) * (_legRSize + 1), &mext_o[5]);
    output_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(mytype),
                            &mext_o[6]);

    std::vector<cl::Memory> ob_out;
    ob_out.push_back(output_buf);

    q.finish();
    // launch kernel and calculate kernel execution time
    std::cout << "kernel start------" << std::endl;
    gettimeofday(&start_time, 0);
    for (int i = 0; i < loop_nm; ++i) {
        kernel_FDHWE_k0.setArg(0, stoppingTimes_buf);
        kernel_FDHWE_k0.setArg(1, payerAccrualTime_buf);
        kernel_FDHWE_k0.setArg(2, receiverAccrualTime_buf);
        kernel_FDHWE_k0.setArg(3, receiverAccrualPeriod_buf);
        kernel_FDHWE_k0.setArg(4, iborTime_buf);
        kernel_FDHWE_k0.setArg(5, iborPeriod_buf);
        kernel_FDHWE_k0.setArg(6, _ETSize);
        kernel_FDHWE_k0.setArg(7, _xGrid);
        kernel_FDHWE_k0.setArg(8, _legPSize);
        kernel_FDHWE_k0.setArg(9, _legRSize);
        kernel_FDHWE_k0.setArg(10, a_);
        kernel_FDHWE_k0.setArg(11, sigma_);
        kernel_FDHWE_k0.setArg(12, theta_);
        kernel_FDHWE_k0.setArg(13, nominal_);
        kernel_FDHWE_k0.setArg(14, fixedRate_);
        kernel_FDHWE_k0.setArg(15, floatingRate_);
        kernel_FDHWE_k0.setArg(16, _tGrid);
        kernel_FDHWE_k0.setArg(17, output_buf);

        q.enqueueTask(kernel_FDHWE_k0, nullptr, nullptr);
    }
    q.finish();

    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / loop_nm << "us" << std::endl;
    q.enqueueMigrateMemObjects(ob_out, 1, nullptr, nullptr);
    q.finish();

#else
    FDHWE_k0(stoppingTimes_alloc, payerAccrualTime_alloc, receiverAccrualTime_alloc, receiverAccrualPeriod_alloc,
             iborTime_alloc, iborPeriod_alloc, _ETSize, _xGrid, _legPSize, _legRSize, a_, sigma_, theta_, nominal_,
             fixedRate_, floatingRate_, _tGrid, output);
#endif

    mytype out = output[0];
    std::cout << "output = " << out << std::endl;

    if (std::fabs(out - NPV_golden) > minErr) {
        err++;
    }
    std::cout << "NPV = " << out << ", diff/NPV = " << (out - NPV_golden) / NPV_golden << std::endl;

    return err;
}
