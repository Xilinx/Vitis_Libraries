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

#ifndef XF_BLAS_HOST_H
#define XF_BLAS_HOST_H

#include "assert.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "xcl2/xcl2.hpp"

#include "utility/utility.h"

using namespace std;

namespace xf {
namespace linear_algebra {
namespace blas {

typedef enum {
OpControl, OpGemv, OpGemm, OpTransp, OpSpmv, OpUspmv, OpResult, OpFail, OpFcn
} OpType;

class BLASArgs {
    public:
        virtual ~BLASArgs() {}
        virtual size_t sizeInBytes() = 0;
        virtual char* asByteArray() = 0;
};
  
class XFpga {
    private:
        cl::Kernel m_Kernel;
        vector<cl::Event>   m_waitInput;
        vector<cl::Event>   m_waitOutput;
    public:
        cl::Context m_Context;
        cl::CommandQueue m_CommandQueue;
        cl::Device m_Device;

        XFpga() = delete;
        XFpga(const string &p_xclbin, const string & p_kernelName, cl_int* p_err) {
            const char* l_kernelName = p_kernelName.c_str();
            vector<cl::Device> l_devices = xcl::get_xil_devices();
            cl::Device l_device = l_devices[0];
            string l_deviceName = l_device.getInfo<CL_DEVICE_NAME>();
            // Create the OpenCL context, cmmandQueue and program 
            cl::Context l_context(l_device);
            m_Context = l_context;
            m_Device = l_device;
            cl::CommandQueue l_cmdQueue(m_Context, l_device,  CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, p_err);
            m_CommandQueue = l_cmdQueue;
            vector<cl::Device> temp_devices;
            temp_devices.push_back(m_Device);
            cl::Program::Binaries l_bins = xcl::import_binary_file(p_xclbin);
            static cl::Program l_program(m_Context, temp_devices, l_bins, NULL, p_err);
            m_Kernel = move(cl::Kernel(l_program, l_kernelName));
        }
        
        ~XFpga() {}

        cl::Buffer createBuf(void *p_ptr, size_t p_szBytes) {
            return cl::Buffer(m_Context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,p_szBytes,p_ptr);
        }

        bool copyToFpga(const cl::Buffer & p_buf, bool p_syncSend) {
            cl::Event l_event;
            vector<cl::Memory> l_buff;
            l_buff.push_back(p_buf);
            // Send the input data to the accelerator
            m_CommandQueue.enqueueMigrateMemObjects(l_buff,0,NULL,&l_event);
            if (p_syncSend) {
                l_event.wait();
            } else {
                m_waitInput.push_back(l_event);
            }
            return true;
        }

        void copyFromFpga(const cl::Buffer & p_buf, bool p_syncExec = true) {
            cl::Event l_readEvents;
            m_CommandQueue.enqueueMigrateMemObjects({p_buf},CL_MIGRATE_MEM_OBJECT_HOST,&m_waitOutput,&l_readEvents);
            if ( p_syncExec ){
                l_readEvents.wait();
                m_waitOutput.clear();
            } else{
                m_waitOutput.push_back(l_readEvents);
            }
        }
        
        void execKernel(const cl::Buffer & p_instrBuf, bool p_syncExec = true ) {
            // Launch kernels
            m_Kernel.setArg(0,p_instrBuf);
            m_Kernel.setArg(1,p_instrBuf);

            cl::Event l_event;
            m_CommandQueue.enqueueTask(m_Kernel, &(m_waitInput),&l_event);

            if (p_syncExec) {
                l_event.wait();
            } else{
                m_waitOutput.push_back(l_event);
            }
            m_waitInput.clear();
        }

        void wait () {
            m_CommandQueue.finish();
            m_waitInput.clear();
            m_waitOutput.clear();
        }

};

template<typename T>
class XHost {
    protected:
        static const unsigned int PAGE_SIZE = 4096;
        static const unsigned int INSTR_BUF_SIZE = PAGE_SIZE;
        static const unsigned int KERN_DBG_BUF_SIZE = PAGE_SIZE;
        unordered_map<T, void*  > m_hostMat;
        unordered_map<T, unsigned long long > m_hostMatSz;
        unordered_map<T, cl::Buffer> m_devHandle;
        shared_ptr<XFpga> m_fpga;
        unsigned long long m_ddrDeviceBaseAddr;
        char* m_progBuf;
        char* m_instrBuf;
        cl::Buffer m_clInstrBuf;
        unsigned int m_instrOffset;
    public:
        XHost() = delete;
        XHost ( const string & p_xclbin, const string & p_kernelName, xfblasStatus_t* p_status) {
            cl_int l_err;
            m_fpga = shared_ptr<XFpga>(new XFpga(p_xclbin, p_kernelName, &l_err));
            if (l_err != CL_SUCCESS){
                *p_status = XFBLAS_STATUS_NOT_INITIALIZED;
                return;
            }
            void *l_alignedMem = nullptr;
            int l_memAllocStatus = posix_memalign(&l_alignedMem, PAGE_SIZE, INSTR_BUF_SIZE);
            if (l_memAllocStatus){    
                *p_status = XFBLAS_STATUS_ALLOC_FAILED;
            }
            m_instrBuf = (char*)l_alignedMem;
            m_progBuf = (char*)l_alignedMem;
            memset(m_instrBuf, 0, INSTR_BUF_SIZE);
            m_instrOffset = 0;
            this->m_clInstrBuf = this->m_fpga->createBuf(m_instrBuf, INSTR_BUF_SIZE+KERN_DBG_BUF_SIZE);
            l_err = xclGetMemObjDeviceAddress(this->m_clInstrBuf.get(), XHost<T>::m_fpga->m_Device.get(),
                                              sizeof(unsigned long long), &this->m_ddrDeviceBaseAddr);
            if (l_err != CL_SUCCESS){
                *p_status = XFBLAS_STATUS_NOT_INITIALIZED;
                return;
            }
        }
        
        bool addMat(const T & p_handle, void * p_matPtr, unsigned long long p_bufSize) {
            auto &l_hostPtr = m_hostMat;
            auto &l_hostSzPtr = m_hostMatSz;
            if (l_hostPtr.find(p_handle) == l_hostPtr.end()) {
                l_hostPtr[p_handle] = p_matPtr;
                l_hostSzPtr[p_handle] = p_bufSize;
                return true;
            } 
            return false;
        }
       
        xfblasStatus_t allocMat(const T & p_handle, void * p_matPtr, unsigned long long p_bufSize){
            if (!addMat(p_handle, p_matPtr, p_bufSize)){
                return XFBLAS_STATUS_ALLOC_FAILED;
            }
            auto &l_hostPtr = m_hostMat;
            auto &l_devPtr = m_devHandle;
            auto &l_hostSzPtr = m_hostMatSz;
            if (l_devPtr.find(p_handle) != l_devPtr.end()) {
                return XFBLAS_STATUS_ALLOC_FAILED; 
            } else {
                l_devPtr[p_handle] = m_fpga->createBuf(l_hostPtr[p_handle], l_hostSzPtr[p_handle]);
                return XFBLAS_STATUS_SUCCESS;
            }
        }
        
        xfblasStatus_t setMatToFPGA(const T & p_handle){
            auto &l_devPtr = m_devHandle;
            if (l_devPtr.find(p_handle) != l_devPtr.end()) {
                m_fpga->copyToFpga(l_devPtr[p_handle], false);
                return XFBLAS_STATUS_SUCCESS;
            } else {
                return XFBLAS_STATUS_ALLOC_FAILED;
            }
        }
        
        void addInstr  ( BLASArgs * p_args ) {
            char * l_instr = p_args->asByteArray();
            char * l_currPos = &m_progBuf[m_instrOffset];
            memcpy(l_currPos, l_instr, p_args->sizeInBytes());
            m_instrOffset += p_args->sizeInBytes();
        }
        
        xfblasStatus_t getMat(const T & p_handle, bool p_syncGet = true) {
            auto &l_hostPtr = m_hostMat;
            auto &l_devPtr = m_devHandle;
            if (l_hostPtr.find(p_handle) != l_hostPtr.end()) {
                m_fpga->copyFromFpga(l_devPtr[p_handle], p_syncGet);
            } else {
                return XFBLAS_STATUS_ALLOC_FAILED;
            }
            return XFBLAS_STATUS_SUCCESS;
        }
      
                
        void clearInstrBuf() {
            memset(this->m_progBuf, 0, PAGE_SIZE);
            this->m_instrOffset = 0;
        }
        
        xfblasStatus_t freeMat(const T & p_handle) {
            auto &l_hostPtr = m_hostMat;
            if (l_hostPtr.find(p_handle) == l_hostPtr.end()) {
                return XFBLAS_STATUS_ALLOC_FAILED;
            } else {
                this->m_devHandle.erase(p_handle);
                this->m_hostMat.erase(p_handle);
                this->m_hostMatSz.erase(p_handle);
                return XFBLAS_STATUS_SUCCESS;
            }
        }

};

template<typename T>
class BLASHost : public XHost<T> {
    public:
        BLASHost() = delete;
        virtual ~BLASHost() {}
        BLASHost(const BLASHost<T> &) = delete;

        static string getKernelName(unsigned PE) {
            return "gemxKernel_" + to_string(PE);
        }

        BLASHost(const string & p_xclbin, const string & p_kernelName, xfblasStatus_t* p_status) : XHost<T> ( p_xclbin, p_kernelName, p_status) {}
        
        xfblasStatus_t execute (bool p_syncExec = false) {
            this->m_fpga->copyToFpga(this->m_clInstrBuf, false);
            this->m_fpga->execKernel(this->m_clInstrBuf, p_syncExec);
            return XFBLAS_STATUS_SUCCESS;
        }
};


}
}
}
  
  
#endif