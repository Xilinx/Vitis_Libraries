/*--
 * ---------------------------------------------------------------------------------------------------------------------*/
/*-- DISCLAIMER AND CRITICAL APPLICATIONS */
/*--
 * ---------------------------------------------------------------------------------------------------------------------*/
/*-- */
/*-- (c) Copyright 2019 Xilinx, Inc. All rights reserved. */
/*-- */
/*-- This file contains confidential and proprietary information of Xilinx, Inc.
 * and is protected under U.S. and          */
/*-- international copyright and other intellectual property laws. */
/*-- */
/*-- DISCLAIMER */
/*-- This disclaimer is not a license and does not grant any rights to the
 * materials distributed herewith. Except as      */
/*-- otherwise provided in a valid license issued to you by Xilinx, and to the
 * maximum extent permitted by applicable     */
/*-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS,
 * AND XILINX HEREBY DISCLAIMS ALL WARRANTIES  */
/*-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED
 * TO WARRANTIES OF MERCHANTABILITY, NON-     */
/*-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall
 * not be liable (whether in contract or tort,*/
/*-- including negligence, or under any other theory of liability) for any loss
 * or damage of any kind or nature           */
/*-- related to, arising under or in connection with these materials, including
 * for any direct, or any indirect,          */
/*-- special, incidental, or consequential loss or damage (including loss of
 * data, profits, goodwill, or any type of      */
/*-- loss or damage suffered as a retVal of any action brought by a third party)
 * even if such damage or loss was          */
/*-- reasonably foreseeable or Xilinx had been advised of the possibility of the
 * same.                                    */
/*-- */
/*-- CRITICAL APPLICATIONS */
/*-- Xilinx products are not designed or intended to be fail-safe, or for use in
 * any application requiring fail-safe      */
/*-- performance, such as life-support or safety devices or systems, Class III
 * medical devices, nuclear facilities,       */
/*-- applications related to the deployment of airbags, or any other
 * applications that could lead to death, personal      */
/*-- injury, or severe property or environmental damage (individually and
 * collectively, "Critical                         */
/*-- Applications"). Customer assumes the sole risk and liability of any use of
 * Xilinx products in Critical               */
/*-- Applications, subject only to applicable laws and regulations governing
 * limitations on product liability.            */
/*-- */
/*-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
 * AT ALL TIMES.                             */
/*--
 * ---------------------------------------------------------------------------------------------------------------------*/

#ifndef _XF_FINTECH_HCF_H_
#define _XF_FINTECH_HCF_H_

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "xf_fintech_device.hpp"
#include "xf_fintech_ocl_controller.hpp"
#include "xf_fintech_types.hpp"
// reference hcf engine in L2
#include "xf_fintech/hcf_engine.hpp"

namespace xf {

namespace fintech {

class hcf : public OCLController {
   public:
    struct hcf_input_data {
        float s0;    // stock price at t=0
        float v0;    // stock price variance at t=0
        float K;     // strike price
        float rho;   // correlation of the 2 Weiner processes
        float T;     // expiration time
        float r;     // risk free interest rate
        float kappa; // rate of reversion
        float vvol;  // volatility of volatility (sigma)
        float vbar;  // long term average variance (theta)
    };

    hcf();
    virtual ~hcf();

    // calculate one or more options
    int run(struct hcf_input_data* inputData, float* outputData, int numOptions);

    // change the model parameters
    void set_dw(int dw);
    void set_w_max(float w_max);
    int get_dw();
    float get_w_max();

   private:
    static const int MAX_OPTION_CALCULATIONS = 1024;

    // OCLController interface
    int createOCLObjects(Device* device);
    int releaseOCLObjects(void);

    cl::Context* m_pContext;
    cl::CommandQueue* m_pCommandQueue;
    cl::Program::Binaries m_binaries;
    cl::Program* m_pProgram;
    cl::Kernel* m_pHcfKernel;

    cl::Buffer* m_pHwInputBuffer;
    cl::Buffer* m_pHwOutputBuffer;

    std::vector<struct xf::fintech::hcfEngineInputDataType<float>,
                aligned_allocator<struct xf::fintech::hcfEngineInputDataType<float> > >
        m_hostInputBuffer;
    std::vector<float, aligned_allocator<float> > m_hostOutputBuffer;

    int m_w_max; // the upper limit for the integration
    float m_dw;  // the delta w for the integration

   private:
    std::string getXCLBINName(Device* device);
};

} // end namespace fintech
} // end namespace xf

#endif /* _XF_FINTECH_HCF_H_ */
