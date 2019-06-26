/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef XF_BLAS_HOST_H
#define XF_BLAS_HOST_H

#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

#include "ert.h"
#include "xclhal2.h"
#include "xclbin.h"

#include "../utility/utility.h"
#include "utils.h"
#include "gemxkernel_0_hw.h"

# define IDX2R(i,j,ld) (((i)*( ld ))+(j))

using namespace std;

namespace xf {
namespace linear_algebra {
namespace blas {
  
class XFpga {
  public:
    unsigned m_cuIndex = 0;
    xclDeviceHandle m_handle;
    uuid_t m_xclbinId;
    int m_firstMem = -1;

    XFpga() = delete;
    XFpga(const char * p_xclbin, const char * p_logFile, int* p_err) {
      if(0 >= xclProbe()) { //0 is index
        *p_err = 1;
        return;
      }
      m_handle = xclOpen(0, p_logFile, XCL_INFO); // 0 is index
      if(xclLockDevice(m_handle)) {
        *p_err = 1;
        return;
      }
      ifstream l_stream(p_xclbin);
      l_stream.seekg(0, l_stream.end);
      int l_size = l_stream.tellg();
      l_stream.seekg(0, l_stream.beg);

      char *l_header = new char[l_size];
      l_stream.read(l_header, l_size);

      const xclBin *l_blob = (const xclBin *)l_header;
      if (xclLoadXclBin(m_handle, l_blob)) {
        *p_err = 1;
      }
      //cout << "Finished downloading bitstream " << p_xclbin << "\n";
      m_firstMem = 0;
      const axlf* l_top = (const axlf*)l_header;
      auto l_ip = xclbin::get_axlf_section(l_top, IP_LAYOUT);
      struct ip_layout* l_layout =  (ip_layout*) (l_header + l_ip->m_sectionOffset);

      if(m_cuIndex > (unsigned)l_layout->m_count) {
        *p_err = 1;
        return;
      }
      auto l_topo = xclbin::get_axlf_section(l_top, MEM_TOPOLOGY);
      struct mem_topology* l_topology = (mem_topology*)(l_header + l_topo->m_sectionOffset);

      for (int i=0; i<l_topology->m_count; ++i) {
        if (l_topology->m_mem_data[i].m_used) {
        m_firstMem = i;
        break;
        }
      }
      uuid_copy(m_xclbinId, l_top->m_header.uuid);
      delete [] l_header;     
      if (m_firstMem < 0) {
        *p_err = 1;
        return;
      }
      xclOpenContext(m_handle, m_xclbinId, m_cuIndex, true);
    }
    
    ~XFpga() {}

    unsigned int createBuf(void *p_ptr, size_t p_szBytes, unsigned int PE) {
      return xclAllocUserPtrBO(m_handle,p_ptr,p_szBytes,PE);
    }
    
    bool copyToFpga(unsigned int p_bufHandle, size_t p_szBytes) {
      if (xclSyncBO(m_handle,p_bufHandle,XCL_BO_SYNC_BO_TO_DEVICE,p_szBytes,0)){
        return false;
      }
      return true;
    }

    bool copyFromFpga(unsigned int p_bufHandle, size_t p_szBytes) {
      xclExecWait(m_handle,10);
      if (xclSyncBO(m_handle, p_bufHandle, XCL_BO_SYNC_BO_FROM_DEVICE, p_szBytes, 0)){
        return false;
      }
      return true;
    }

    bool execKernel() {
      unsigned int execHandle = xclAllocBO(m_handle,4096+4096, xclBOKind(0), (1<<31));
      void* execData = xclMapBO(m_handle,execHandle,true);
      auto ecmd = reinterpret_cast<ert_start_kernel_cmd*>(execData);
      auto rsz = XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_P_DDRWR_M_VAL_DATA/4 + 1; // regmap array size
      memset(ecmd,0,(sizeof *ecmd) + rsz);
      ecmd->state = ERT_CMD_STATE_NEW;
      ecmd->opcode = ERT_START_CU;
      ecmd->count = 1 + rsz;
      ecmd->cu_mask = 0x1;
      ecmd->data[XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_AP_CTRL] = 0x0; // ap_start
      ecmd->data[XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_P_DDRRD_M_VAL_DATA/4] = 0;
      ecmd->data[XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_P_DDRWR_M_VAL_DATA/4] = 0;       
      if (xclExecBuf(m_handle, execHandle)) {  
        return false;
      }
      return true;
    }
};

class XHost {
  protected:
    static const unsigned int PAGE_SIZE = 4096;
    static const unsigned int INSTR_BUF_SIZE = PAGE_SIZE;
    static const unsigned int KERN_DBG_BUF_SIZE = PAGE_SIZE;
    unordered_map<void*, void*  > m_hostMat;
    unordered_map<void*, unsigned int> m_bufHandle;
    unordered_map<void*, unsigned long long > m_hostMatSz;
    shared_ptr<XFpga> m_fpga;
    vector<unsigned long long> m_ddrDeviceBaseAddr;
    char* m_progBuf;
    char* m_instrBuf;
    unsigned int m_instrOffset;
    vector<unsigned int> m_instrBufHandle;
  public:
    XHost() = delete;
    XHost ( const char * p_xclbin, const char * p_logFile, xfblasStatus_t* p_status, unsigned int nPE) {
      int l_err = 0;
      m_fpga = shared_ptr<XFpga>(new XFpga(p_xclbin, p_logFile, &l_err));
      if (l_err != 0){
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
      for(unsigned int i = 0; i <nPE;i++){
        unsigned int l_instrBufHandle = m_fpga->createBuf(m_instrBuf,INSTR_BUF_SIZE+KERN_DBG_BUF_SIZE, i);
        m_instrBufHandle.push_back(l_instrBufHandle);
        xclBOProperties p;
        uint64_t l_address = !xclGetBOProperties(m_fpga->m_handle, l_instrBufHandle, &p) ? p.paddr : -1;
        unsigned int l_ddrDeviceBaseAddr = (unsigned long long) l_address;
        m_ddrDeviceBaseAddr.push_back(l_ddrDeviceBaseAddr);
      }
    }
    
    bool addMatRestricted(void * p_hostHandle, void * p_matPtr, unsigned long long p_bufSize) {
      auto &l_hostPtr = m_hostMat;
      auto &l_hostSzPtr = m_hostMatSz;       
      if (((unsigned long)p_matPtr & (PAGE_SIZE-1)) != 0){
        void * l_matPtr;
        posix_memalign((void** )&l_matPtr, 4096, p_bufSize);
        memcpy(l_matPtr, p_matPtr,p_bufSize);
        if (l_hostPtr.find(p_hostHandle) == l_hostPtr.end()) {
          l_hostPtr[p_hostHandle] = l_matPtr;
          l_hostSzPtr[p_hostHandle] = p_bufSize;
          return true;
        } 
      } else {
        if (l_hostPtr.find(p_hostHandle) == l_hostPtr.end()) {
          l_hostPtr[p_hostHandle] = p_matPtr;
          l_hostSzPtr[p_hostHandle] = p_bufSize;
          return true;
        } 
      }
      return false;
    }
    
    xfblasStatus_t allocMatRestricted(void * p_hostHandle, void * p_matPtr, unsigned long long p_bufSize, unsigned int PE){
      if (!addMatRestricted(p_hostHandle, p_matPtr, p_bufSize)){
        return XFBLAS_STATUS_ALLOC_FAILED;
      }
      auto &l_hostPtr = m_hostMat;
      auto &l_devPtr = m_bufHandle;
      auto &l_hostSzPtr = m_hostMatSz;
      if (l_devPtr.find(p_hostHandle) != l_devPtr.end()) {
        return XFBLAS_STATUS_ALLOC_FAILED; 
      } else {
        l_devPtr[p_hostHandle] = m_fpga->createBuf(l_hostPtr[p_hostHandle], l_hostSzPtr[p_hostHandle], PE);
        return XFBLAS_STATUS_SUCCESS;
      }
    }
    
    template<typename t_dataType>
    xfblasStatus_t allocMat(t_dataType* p_devPtr, size_t p_bufSize, int PE){
      auto &l_devPtr = m_bufHandle;
      auto &l_hostSzPtr = m_hostMatSz;
      if (l_devPtr.find(*p_devPtr) != l_devPtr.end()) {
        return XFBLAS_STATUS_ALLOC_FAILED; 
      } else {
        unsigned int l_deviceHandle = xclAllocBO(m_fpga->m_handle, p_bufSize, XCL_BO_DEVICE_RAM, PE); 
        *p_devPtr = (t_dataType)xclMapBO(m_fpga->m_handle, l_deviceHandle, true);
        memset(*p_devPtr,0,p_bufSize);
        l_hostSzPtr[*p_devPtr] = p_bufSize;
        l_devPtr[*p_devPtr] = l_deviceHandle;
        return XFBLAS_STATUS_SUCCESS;
      }
    }
    
    template<typename t_dataType> 
    xfblasStatus_t setMatToFPGA(void* p_hostHandle, int p_rows, int p_lda, int p_paddedLda, t_dataType & p_hostPtr, t_dataType & p_devPtr){
      auto &l_devPtr = m_bufHandle;
      auto &l_hostSzPtr = m_hostMatSz;
      if (l_devPtr.find(p_hostHandle) != l_devPtr.end()) {
        for(int i = 0; i< p_rows; i ++){
        for(int j = 0; j < p_lda; j ++){
          p_devPtr[IDX2R (i,j,p_paddedLda)] = p_hostPtr[IDX2R (i,j,p_lda )]; 
        } 
        }       
        if(!m_fpga->copyToFpga(l_devPtr[p_hostHandle], l_hostSzPtr[p_hostHandle])){
        return XFBLAS_STATUS_ALLOC_FAILED;
        }        
      } else {
        return XFBLAS_STATUS_ALLOC_FAILED;
      }
      return XFBLAS_STATUS_SUCCESS;
    }
    
    xfblasStatus_t setMatToFPGARestricted(void* p_hostHandle){
      auto &l_devPtr = m_bufHandle;
      auto &l_hostSzPtr = m_hostMatSz;
      if (l_devPtr.find(p_hostHandle) != l_devPtr.end()) {
        if(!m_fpga->copyToFpga(l_devPtr[p_hostHandle], l_hostSzPtr[p_hostHandle])){
        return XFBLAS_STATUS_ALLOC_FAILED;
        }  
      } else {
        return XFBLAS_STATUS_ALLOC_FAILED;
      }
      return XFBLAS_STATUS_SUCCESS;
    }
    
    void addInstr(BLASArgs * p_args) {
      char* l_instr = p_args->asByteArray();
      char* l_currPos = &m_progBuf[m_instrOffset];
      memcpy(l_currPos, l_instr, p_args->sizeInBytes());
      m_instrOffset += p_args->sizeInBytes();
    }
    
    template<typename t_dataType>
    xfblasStatus_t getMat(void* p_hostHandle, int p_rows, int p_lda, int p_paddedLda, t_dataType & p_hostPtr, t_dataType & p_devPtr) {
      auto &l_hostSzPtr = m_hostMatSz;
      auto &l_devPtr = m_bufHandle;
      if (l_devPtr.find(p_hostHandle) != l_devPtr.end()) {
        if(!m_fpga->copyFromFpga(l_devPtr[p_hostHandle], l_hostSzPtr[p_hostHandle])){
        return XFBLAS_STATUS_ALLOC_FAILED;
        }
        for(int i = 0; i< p_rows; i ++){
        for(int j = 0; j < p_lda; j ++){
          p_hostPtr[IDX2R (i,j,p_lda )] = p_devPtr[ IDX2R (i,j,p_paddedLda)]; 
        } 
        }  
      } else {
        return XFBLAS_STATUS_ALLOC_FAILED;
      }
      return XFBLAS_STATUS_SUCCESS;
    }
    
    xfblasStatus_t getMatRestricted(void * p_hostHandle, void * p_matPtr) {
      auto &l_hostPtr = m_hostMat;
      auto &l_hostSzPtr = m_hostMatSz;
      auto &l_devPtr = m_bufHandle;
      if (l_hostPtr.find(p_hostHandle) != l_hostPtr.end()) {
        if(!m_fpga->copyFromFpga(l_devPtr[p_hostHandle], l_hostSzPtr[p_hostHandle])){
        return XFBLAS_STATUS_ALLOC_FAILED;
        }
        if (((unsigned long)p_matPtr & (PAGE_SIZE-1)) != 0){
        memcpy(p_matPtr,l_hostPtr[p_hostHandle],l_hostSzPtr[p_hostHandle]);
        }
      } else {
        return XFBLAS_STATUS_ALLOC_FAILED;
      }
      return XFBLAS_STATUS_SUCCESS;
    }
       
    void clearInstrBuf() {
      memset(this->m_progBuf, 0, PAGE_SIZE);
      this->m_instrOffset = 0;
    }
    
    xfblasStatus_t freeMat(void* p_hostHandle) {
      auto &l_devPtr = m_bufHandle;
      if (l_devPtr.find(p_hostHandle) == l_devPtr.end()) {
        return XFBLAS_STATUS_ALLOC_FAILED;
      } else {
        xclFreeBO(m_fpga->m_handle,l_devPtr[p_hostHandle]);
        this->m_bufHandle.erase(p_hostHandle);
        this->m_hostMatSz.erase(p_hostHandle);
        if (!m_hostMat.empty()){
        this->m_hostMat.erase(p_hostHandle);
        }
        return XFBLAS_STATUS_SUCCESS;
      }
    }
    
    xfblasStatus_t closeContext(){
      for(unsigned int i = 0; i<m_instrBufHandle.size();i++){
        xclFreeBO(m_fpga->m_handle, m_instrBufHandle[i]);
      }
      xclCloseContext(m_fpga->m_handle, m_fpga->m_xclbinId, m_fpga->m_cuIndex);
      return XFBLAS_STATUS_SUCCESS;
    }
};

class BLASHost : public XHost {
  private:
    bool execControl = true;
  public:
    BLASHost() = delete;
    virtual ~BLASHost() {}
    BLASHost(const BLASHost &) = delete;

    BLASHost(const char * p_xclbin, const char * p_logFile, xfblasStatus_t* p_status, unsigned int nPE) : XHost ( p_xclbin, p_logFile, p_status, nPE) {}
    
    xfblasStatus_t execute (unsigned int PE) {
      xfblasStatus_t l_status = XFBLAS_STATUS_SUCCESS;
      if(execControl){
        if(!this->m_fpga->copyToFpga(this->m_instrBufHandle[PE], this->INSTR_BUF_SIZE+this->KERN_DBG_BUF_SIZE)){
        l_status = XFBLAS_STATUS_ALLOC_FAILED;
        }
        if(!this->m_fpga->execKernel()){
        l_status = XFBLAS_STATUS_ALLOC_FAILED;
        }
      }
      return l_status;
    }
};


}
}
}
  
  
#endif