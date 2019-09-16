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

#include "xf_fintech_error_codes.hpp"
#include "xf_fintech_trace.hpp"

#include "models/xf_fintech_pop_mcmc.hpp"
#include "xf_fintech_pop_mcmc_kernel_constants.hpp"

using namespace xf::fintech;

PopMCMC::PopMCMC() {
    m_pContext = nullptr;
    m_pCommandQueue = nullptr;
    m_pProgram = nullptr;
    m_pPopMCMCKernel = nullptr;

    m_hostInputBufferInv.clear();
    m_hostInputBufferSigma.clear();
    m_hostOutputBufferSamples.clear();

    mBufferInputInv = nullptr;
    mBufferInputSigma = nullptr;
    mBufferOutputSamples = nullptr;
}

PopMCMC::~PopMCMC() {
    if (deviceIsPrepared()) {
        releaseDevice();
    }
}

std::string PopMCMC::getXCLBINName(Device* device) {
    std::string xclbinName;
    std::string deviceTypeString = device->getDeviceTypeString();

    xclbinName = "mcmc_kernel.hw." + deviceTypeString + ".xclbin";

    return xclbinName;
}

int PopMCMC::createOCLObjects(Device* device) {
    int retval = XLNX_OK;
    cl_int cl_retval = CL_SUCCESS;
    std::string xclbinName;

    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;

    cl::Device clDevice;
    clDevice = device->getCLDevice();
    m_pContext = new cl::Context(clDevice, nullptr, nullptr, nullptr, &cl_retval);

    if (cl_retval == CL_SUCCESS) {
        m_pCommandQueue = new cl::CommandQueue(*m_pContext, clDevice, CL_QUEUE_PROFILING_ENABLE, &cl_retval);
    }

    if (cl_retval == CL_SUCCESS) {
        xclbinName = getXCLBINName(device);

        start = std::chrono::high_resolution_clock::now();
        m_binaries.clear();
        m_binaries = xcl::import_binary_file(xclbinName);
        end = std::chrono::high_resolution_clock::now();

        Trace::printInfo("[XLNX] Binary Import Time = %lld microseconds\n",
                         std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    }

    /////////////////////////
    // Create PROGRAM Object
    /////////////////////////
    if (cl_retval == CL_SUCCESS) {
        std::vector<cl::Device> devicesToProgram;
        devicesToProgram.push_back(clDevice);

        start = std::chrono::high_resolution_clock::now();
        m_pProgram = new cl::Program(*m_pContext, devicesToProgram, m_binaries, nullptr, &cl_retval);
        end = std::chrono::high_resolution_clock::now();

        Trace::printInfo("[XLNX] Device Programming Time = %lld microseconds\n",
                         std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    }

    /////////////////////////
    // Create KERNEL Objects
    /////////////////////////
    if (cl_retval == CL_SUCCESS) {
        m_pPopMCMCKernel = new cl::Kernel(*m_pProgram, "mcmc_top", &cl_retval);
    }

    //////////////////////////
    // Allocate HOST BUFFERS
    //////////////////////////
    m_hostInputBufferInv.resize(NUM_CHAINS);
    m_hostInputBufferSigma.resize(NUM_CHAINS);
    m_hostOutputBufferSamples.resize(NUM_SAMPLES_MAX);

    //////////////////////////
    // Allocate HOST BUFFERS
    //////////////////////////

    size_t sizeBufferInputInv = sizeof(double) * NUM_CHAINS;
    size_t sizeBufferInputSigma = sizeof(double) * NUM_CHAINS;
    size_t sizeBufferOutputSamples = sizeof(double) * NUM_SAMPLES_MAX;

    if (cl_retval == CL_SUCCESS) {
        mBufferInputInv = new cl::Buffer(*m_pContext, (CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY), sizeBufferInputInv,
                                         m_hostInputBufferInv.data(), &cl_retval);
    }

    if (cl_retval == CL_SUCCESS) {
        mBufferInputSigma = new cl::Buffer(*m_pContext, (CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY), sizeBufferInputSigma,
                                           m_hostInputBufferSigma.data(), &cl_retval);
    }

    if (cl_retval == CL_SUCCESS) {
        mBufferOutputSamples = new cl::Buffer(*m_pContext, (CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY),
                                              sizeBufferOutputSamples, m_hostOutputBufferSamples.data(), &cl_retval);
    }

    if (cl_retval != CL_SUCCESS) {
        setCLError(cl_retval);
        Trace::printError("[XLNX] OpenCL Error = %d\n", cl_retval);
        retval = XLNX_ERROR_OPENCL_CALL_ERROR;
    }

    return retval;
}

int PopMCMC::releaseOCLObjects(void) {
    unsigned int i;

    if (mBufferInputInv != nullptr) {
        delete (mBufferInputInv);
        mBufferInputInv = nullptr;
    }

    if (mBufferInputSigma != nullptr) {
        delete (mBufferInputSigma);
        mBufferInputSigma = nullptr;
    }

    if (mBufferOutputSamples != nullptr) {
        delete (mBufferOutputSamples);
        mBufferOutputSamples = nullptr;
    }

    if (m_pPopMCMCKernel != nullptr) {
        delete (m_pPopMCMCKernel);
        m_pPopMCMCKernel = nullptr;
    }

    if (m_pProgram != nullptr) {
        delete (m_pProgram);
        m_pProgram = nullptr;
    }

    for (i = 0; i < m_binaries.size(); i++) {
        std::pair<const void*, cl::size_type> binaryPair = m_binaries[i];
        delete[](char*)(binaryPair.first);
    }

    if (m_pCommandQueue != nullptr) {
        delete (m_pCommandQueue);
        m_pCommandQueue = nullptr;
    }

    if (m_pContext != nullptr) {
        delete (m_pContext);
        m_pContext = nullptr;
    }

    return 0;
}

int PopMCMC::run(int numSamples, int numBurnInSamples, double sigma, double* outputSamples) {
    int retval = XLNX_OK;

    if (deviceIsPrepared()) {
        // start time
        m_runStartTime = std::chrono::high_resolution_clock::now();

        // prepare the data
        for (unsigned int n = 0; n < NUM_CHAINS; n++) {
            m_hostInputBufferSigma[n] = sigma;
            double temp = pow(NUM_CHAINS / (NUM_CHAINS - n), 2);
            m_hostInputBufferInv[n] = 1 / temp;
        }

        // Set the arguments
        m_pPopMCMCKernel->setArg(0, *mBufferInputInv);
        m_pPopMCMCKernel->setArg(1, *mBufferInputSigma);
        m_pPopMCMCKernel->setArg(2, *mBufferOutputSamples);
        m_pPopMCMCKernel->setArg(3, numSamples);

        // Copy input data to device global memory
        m_pCommandQueue->enqueueMigrateMemObjects({*mBufferInputInv}, 0);
        m_pCommandQueue->enqueueMigrateMemObjects({*mBufferInputSigma}, 0);

        // Launch the Kernel
        m_pCommandQueue->enqueueTask(*m_pPopMCMCKernel);

        // Copy Result from Device Global Memory to Host Local Memory
        m_pCommandQueue->enqueueMigrateMemObjects({*mBufferOutputSamples}, CL_MIGRATE_MEM_OBJECT_HOST);
        m_pCommandQueue->finish();

        // --------------------------------
        // Give the caller back the results
        // --------------------------------
        for (int i = 0; i < (numSamples - numBurnInSamples); i++) {
            outputSamples[i] = m_hostOutputBufferSamples[i];
        }

        // end time
        m_runEndTime = std::chrono::high_resolution_clock::now();

    } else {
        retval = XLNX_ERROR_DEVICE_NOT_OWNED_BY_SPECIFIED_OCL_CONTROLLER;
    }

    return retval;
}

long long int PopMCMC::getLastRunTime(void) {
    long long int duration = 0;

    duration =
        (long long int)std::chrono::duration_cast<std::chrono::microseconds>(m_runEndTime - m_runStartTime).count();
    return duration;
}
