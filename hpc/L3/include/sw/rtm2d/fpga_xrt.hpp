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
#ifndef XF_RTM_FPGA_XRT_HPP
#define XF_RTM_FPGA_XRT_HPP

#include <vector>
#include <chrono>
#include "ert.h"
#include "xclhal2.h"
#include "xclbin.h"
#include "types.hpp"
#include "alignMem.hpp"
#include "xrtmbackward_hw.h"
#include "xrtmforward_hw.h"

using namespace std;

/**
 * @file fpga_xrt.hpp
 * @brief class FPGA using XRT APIs, ForwardKernel and BackwardKernel are defined here.
 */

/**
 * @brief class FPGA is used to manage FPGA info
 */
class FPGA {
   public:
    FPGA(const char* p_xclbin, int* p_err, unsigned int deviceIndex = 0) {
        if (deviceIndex >= xclProbe()) {
            *p_err = 1;
            return;
        }
        m_handle = xclOpen(deviceIndex, NULL, XCL_INFO);
        ifstream l_stream(p_xclbin);
        l_stream.seekg(0, l_stream.end);
        int l_size = l_stream.tellg();
        l_stream.seekg(0, l_stream.beg);

        char* l_header = new char[l_size];
        l_stream.read(l_header, l_size);

        const xclBin* l_blob = (const xclBin*)l_header;
        if (xclLoadXclBin(m_handle, l_blob)) {
            *p_err = 1;
            return;
        }
        // cout << "Finished downloading bitstream " << p_xclbin << "\n";
        const axlf* l_top = (const axlf*)l_header;

        uuid_copy(m_xclbinId, l_top->m_header.uuid);
        delete[] l_header;

        xclOpenContext(m_handle, m_xclbinId, 0, true);
        xclOpenContext(m_handle, m_xclbinId, 1, true);
    }

   public:
    xclDeviceHandle m_handle;
    uuid_t m_xclbinId;
};

/**
 * @brief class ForwardKernel is used to manage and run forward kernel on FPGA
 */
template <typename t_DataType, unsigned int t_Order, unsigned int t_PE>
class ForwardKernel {
   public:
    typedef WideData<t_DataType, t_PE> t_WideType;
    typedef WideData<t_WideType, 2> t_PairType;
    static const unsigned int _4k = 4096;

    ForwardKernel(const FPGA* fpga,
                  unsigned int p_height,
                  unsigned int p_width,
                  unsigned int p_zb,
                  unsigned int p_xb,
                  unsigned int p_time,
                  unsigned int p_shots) {
        m_fpga = fpga;

        m_height = p_height;
        m_width = p_width;
        m_zb = p_zb;
        m_xb = p_xb;
        m_time = p_time;
        m_shots = p_shots;

        m_taperx = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_xb);
        m_taperz = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_zb);
        m_v2dt2 = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_width * m_height);
        m_srcs = new t_DataType*[m_shots];
        for (unsigned int s = 0; s < m_shots; s++)
            m_srcs[s] = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_time);

        m_coefx = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
        m_coefz = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
    }

    ~ForwardKernel() {
        free(m_coefx);
        free(m_coefz);
        free(m_taperx);
        free(m_taperz);
        free(m_v2dt2);
        for (unsigned int s = 0; s < m_shots; s++) free(m_srcs[s]);
        delete[] m_srcs;
    }

    /**
     * @brief loadData load parameters for RTM forward kernel from given path
     *
     * @param filePath the path where parameters are stored
     *
     */
    void loadData(const string filePath) {
        readBin(filePath + "v2dt2.bin", sizeof(t_DataType) * m_width * m_height, m_v2dt2);
        for (unsigned int s = 0; s < m_shots; s++)
            readBin(filePath + "src_s" + to_string(s) + ".bin", sizeof(t_DataType) * m_time, m_srcs[s]);
        readBin(filePath + "taperx.bin", sizeof(t_DataType) * m_xb, m_taperx);
        readBin(filePath + "taperz.bin", sizeof(t_DataType) * m_zb, m_taperz);
        readBin(filePath + "coefx.bin", sizeof(t_DataType) * (t_Order + 1), m_coefx);
        readBin(filePath + "coefz.bin", sizeof(t_DataType) * (t_Order + 1), m_coefz);
    }

    /**
     * @brief run launch the RTM forward kernel with given input parameters
     *
     * @param p_shot the shot id
     * @param p_sx the shot coordinate
     * @param p_p seismic snapshot
     * @param p_upb the upper boundary data
     *
     */
    double run(unsigned int p_shot, unsigned int p_sx, AlignMem<t_PairType>& p_p, AlignMem<t_DataType>& p_upb) {
        AlignMem<t_PairType> l_p0(m_width * m_height / t_PE);
        AlignMem<t_DataType> l_upb(m_width * t_Order * m_time / 2);

        m_handle = m_fpga->m_handle;

        unsigned d_coefx = xclAllocUserPtrBO(m_handle, m_coefx, sizeof(t_DataType) * (t_Order + 1), 32); // DDR[0]
        unsigned d_coefz = xclAllocUserPtrBO(m_handle, m_coefz, sizeof(t_DataType) * (t_Order + 1), 32);
        unsigned d_taperx = xclAllocUserPtrBO(m_handle, m_taperx, sizeof(t_DataType) * m_xb, 32);
        unsigned d_taperz = xclAllocUserPtrBO(m_handle, m_taperz, sizeof(t_DataType) * m_zb, 32);
        unsigned d_v2dt2 = xclAllocUserPtrBO(m_handle, m_v2dt2, sizeof(t_DataType) * m_width * m_height, 7); // HBM[7]

        unsigned d_srcs = xclAllocUserPtrBO(m_handle, m_srcs[p_shot], sizeof(t_DataType) * m_time, 32);
        unsigned d_upb =
            xclAllocUserPtrBO(m_handle, l_upb.ptr(), sizeof(t_DataType) * m_width * t_Order * m_time / 2, 32);
        unsigned d_p0 =
            xclAllocUserPtrBO(m_handle, l_p0.ptr(), sizeof(t_PairType) * m_width * m_height / t_PE, 8); // HBM[8]

        xclSyncBO(m_handle, d_taperx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_xb, 0);
        xclSyncBO(m_handle, d_taperz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_zb, 0);
        xclSyncBO(m_handle, d_coefx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_coefz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_v2dt2, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * m_height, 0);
        xclSyncBO(m_handle, d_srcs, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_time, 0);
        xclSyncBO(m_handle, d_p0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_upb, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * t_Order * m_time / 2, 0);

        xclBOProperties p;
        unsigned long long address_srcs = !xclGetBOProperties(m_handle, d_srcs, &p) ? p.paddr : -1;
        unsigned long long address_coefz = !xclGetBOProperties(m_handle, d_coefz, &p) ? p.paddr : -1;
        unsigned long long address_coefx = !xclGetBOProperties(m_handle, d_coefx, &p) ? p.paddr : -1;
        unsigned long long address_taperz = !xclGetBOProperties(m_handle, d_taperz, &p) ? p.paddr : -1;
        unsigned long long address_taperx = !xclGetBOProperties(m_handle, d_taperx, &p) ? p.paddr : -1;
        unsigned long long address_v2dt2 = !xclGetBOProperties(m_handle, d_v2dt2, &p) ? p.paddr : -1;
        unsigned long long address_p0 = !xclGetBOProperties(m_handle, d_p0, &p) ? p.paddr : -1;
        unsigned long long address_upb = !xclGetBOProperties(m_handle, d_upb, &p) ? p.paddr : -1;

        unsigned int execHandle = xclAllocBO(m_handle, 4096, xclBOKind(0), (1 << 31));

        void* execData = xclMapBO(m_handle, execHandle, true);
        auto ecmd = reinterpret_cast<ert_start_kernel_cmd*>(execData);
        auto rsz = XRTMFORWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 2; // regmap array size
        memset(ecmd, 0, (sizeof *ecmd) + rsz);
        ecmd->state = ERT_CMD_STATE_NEW;
        ecmd->opcode = ERT_START_CU;
        ecmd->count = 1 + rsz;
        ecmd->cu_mask = 0x1 << 1;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_AP_CTRL] = 0x0; // ap_start
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_Z_DATA / 4] = m_height;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_X_DATA / 4] = m_width;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_T_DATA / 4] = m_time;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRCZ_DATA / 4] = m_zb;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRCX_DATA / 4] = p_sx;

        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRC_DATA / 4] = address_srcs;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFZ_DATA / 4] = address_coefz;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFX_DATA / 4] = address_coefx;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4] = address_taperz;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERX_DATA / 4] = address_taperx;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_V2DT2_DATA / 4] = address_v2dt2;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P0_DATA / 4] = address_p0;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P1_DATA / 4] = address_p0;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_UPB_DATA / 4] = address_upb;

        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRC_DATA / 4 + 1] = address_srcs >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFZ_DATA / 4 + 1] = address_coefz >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFX_DATA / 4 + 1] = address_coefx >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4 + 1] = address_taperz >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERX_DATA / 4 + 1] = address_taperx >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_V2DT2_DATA / 4 + 1] = address_v2dt2 >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P0_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P1_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 1] = address_upb >> 32;

        xclExecBuf(m_handle, execHandle);

        auto start = chrono::high_resolution_clock::now();

        while (xclExecWait(m_handle, 1) == 0)
            ;

        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;

        xclSyncBO(m_handle, d_p0, XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);

        xclSyncBO(m_handle, d_upb, XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_DataType) * m_width * t_Order * m_time / 2, 0);

        p_p = l_p0;

        p_upb = l_upb;
        return elapsed.count();
    }

    /**
    * @brief run launch the RTM forward kernel with given input parameters (in order to support multi-kernel, run
    * function is split to before run and after_run)
    *
    */
    void before_run(unsigned int p_shot, unsigned int p_sx, AlignMem<t_PairType>& l_p0, AlignMem<t_DataType>& l_upb) {
        m_handle = m_fpga->m_handle;

        unsigned d_coefx = xclAllocUserPtrBO(m_handle, m_coefx, sizeof(t_DataType) * (t_Order + 1), 32); // DDR[0]
        unsigned d_coefz = xclAllocUserPtrBO(m_handle, m_coefz, sizeof(t_DataType) * (t_Order + 1), 32);
        unsigned d_taperx = xclAllocUserPtrBO(m_handle, m_taperx, sizeof(t_DataType) * m_xb, 32);
        unsigned d_taperz = xclAllocUserPtrBO(m_handle, m_taperz, sizeof(t_DataType) * m_zb, 32);
        unsigned d_v2dt2 = xclAllocUserPtrBO(m_handle, m_v2dt2, sizeof(t_DataType) * m_width * m_height, 7); // HBM[7]

        unsigned d_srcs = xclAllocUserPtrBO(m_handle, m_srcs[p_shot], sizeof(t_DataType) * m_time, 32);
        unsigned d_upb =
            xclAllocUserPtrBO(m_handle, l_upb.ptr(), sizeof(t_DataType) * m_width * t_Order * m_time / 2, 32);
        unsigned d_p0 =
            xclAllocUserPtrBO(m_handle, l_p0.ptr(), sizeof(t_PairType) * m_width * m_height / t_PE, 8); // HBM[8]

        m_boIndex[0] = d_upb;
        m_boIndex[1] = d_p0;
        m_boIndex[2] = d_coefx;
        m_boIndex[3] = d_coefz;
        m_boIndex[4] = d_taperx;
        m_boIndex[5] = d_taperz;
        m_boIndex[6] = d_v2dt2;
        m_boIndex[7] = d_srcs;

        xclSyncBO(m_handle, d_taperx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_xb, 0);
        xclSyncBO(m_handle, d_taperz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_zb, 0);
        xclSyncBO(m_handle, d_coefx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_coefz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_v2dt2, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * m_height, 0);
        xclSyncBO(m_handle, d_srcs, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_time, 0);
        xclSyncBO(m_handle, d_p0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_upb, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * t_Order * m_time / 2, 0);

        xclBOProperties p;
        unsigned long long address_srcs = !xclGetBOProperties(m_handle, d_srcs, &p) ? p.paddr : -1;
        unsigned long long address_coefz = !xclGetBOProperties(m_handle, d_coefz, &p) ? p.paddr : -1;
        unsigned long long address_coefx = !xclGetBOProperties(m_handle, d_coefx, &p) ? p.paddr : -1;
        unsigned long long address_taperz = !xclGetBOProperties(m_handle, d_taperz, &p) ? p.paddr : -1;
        unsigned long long address_taperx = !xclGetBOProperties(m_handle, d_taperx, &p) ? p.paddr : -1;
        unsigned long long address_v2dt2 = !xclGetBOProperties(m_handle, d_v2dt2, &p) ? p.paddr : -1;
        unsigned long long address_p0 = !xclGetBOProperties(m_handle, d_p0, &p) ? p.paddr : -1;
        unsigned long long address_upb = !xclGetBOProperties(m_handle, d_upb, &p) ? p.paddr : -1;

        unsigned int execHandle = xclAllocBO(m_handle, 4096, xclBOKind(0), (1 << 31));

        void* execData = xclMapBO(m_handle, execHandle, true);
        auto ecmd = reinterpret_cast<ert_start_kernel_cmd*>(execData);
        auto rsz = XRTMFORWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 2; // regmap array size
        memset(ecmd, 0, (sizeof *ecmd) + rsz);
        ecmd->state = ERT_CMD_STATE_NEW;
        ecmd->opcode = ERT_START_CU;
        ecmd->count = 1 + rsz;
        ecmd->cu_mask = 0x1 << 1;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_AP_CTRL] = 0x0; // ap_start
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_Z_DATA / 4] = m_height;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_X_DATA / 4] = m_width;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_T_DATA / 4] = m_time;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRCZ_DATA / 4] = m_zb;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRCX_DATA / 4] = p_sx;

        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRC_DATA / 4] = address_srcs;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFZ_DATA / 4] = address_coefz;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFX_DATA / 4] = address_coefx;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4] = address_taperz;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERX_DATA / 4] = address_taperx;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_V2DT2_DATA / 4] = address_v2dt2;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P0_DATA / 4] = address_p0;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P1_DATA / 4] = address_p0;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_UPB_DATA / 4] = address_upb;

        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_SRC_DATA / 4 + 1] = address_srcs >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFZ_DATA / 4 + 1] = address_coefz >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_COEFX_DATA / 4 + 1] = address_coefx >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4 + 1] = address_taperz >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_TAPERX_DATA / 4 + 1] = address_taperx >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_V2DT2_DATA / 4 + 1] = address_v2dt2 >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P0_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_P1_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMFORWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 1] = address_upb >> 32;

        xclExecBuf(m_handle, execHandle);
    }

    /**
     * @brief run launch the RTM forward kernel with given input parameters (in order to support multi-kernel, run
     * function is split to before run and after_run)
     */
    void after_run(AlignMem<t_PairType>& p_p,
                   AlignMem<t_DataType>& p_upb,
                   AlignMem<t_PairType>& l_p0,
                   AlignMem<t_DataType>& l_upb) {
        m_handle = m_fpga->m_handle;

        xclSyncBO(m_handle, m_boIndex[1], XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE,
                  0);

        xclSyncBO(m_handle, m_boIndex[0], XCL_BO_SYNC_BO_FROM_DEVICE,
                  sizeof(t_DataType) * m_width * t_Order * m_time / 2, 0);

        p_p = l_p0;

        p_upb = l_upb;

        for (int i = 0; i < 8; i++) {
            xclFreeBO(m_handle, m_boIndex[i]);
        }
    }

   private:
    const FPGA* m_fpga;
    xclDeviceHandle m_handle;
    unsigned int m_boIndex[8];

    unsigned int m_width, m_height, m_xb, m_zb, m_time, m_shots;
    t_DataType *m_taperx, *m_taperz;
    t_DataType *m_v2dt2, **m_srcs;
    t_DataType *m_coefx, *m_coefz;
};

/**
 * @brief class BackwardKernel is used to manage and run backward kernel on FPGA
 */
template <typename t_DataType, unsigned int t_Order, unsigned int t_PE>
class BackwardKernel {
   public:
    typedef WideData<t_DataType, t_PE> t_WideType;
    typedef WideData<t_WideType, 2> t_PairType;
    static const unsigned int _4k = 4096;

    BackwardKernel(const FPGA* fpga,
                   unsigned int p_height,
                   unsigned int p_width,
                   unsigned int p_zb,
                   unsigned int p_xb,
                   unsigned int p_time,
                   unsigned int p_shots) {
        m_fpga = fpga;
        m_height = p_height;
        m_width = p_width;
        m_zb = p_zb;
        m_xb = p_xb;
        m_time = p_time;
        m_shots = p_shots;

        m_imgX = m_width - 2 * m_xb;
        m_imgZ = m_height - 2 * m_zb;

        m_taperx = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_xb);
        m_taperz = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_zb);
        m_v2dt2 = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_width * m_height);
        m_receiver = new t_DataType*[m_shots];
        for (unsigned int i = 0; i < m_shots; i++)
            m_receiver[i] = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_imgX * m_time);

        m_coefx = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
        m_coefz = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
    }

    ~BackwardKernel() {
        free(m_taperx);
        free(m_taperz);
        free(m_v2dt2);
        for (unsigned int i = 0; i < m_shots; i++) free(m_receiver[i]);
        delete[] m_receiver;
        free(m_coefx);
        free(m_coefz);
    }

    /**
     * @brief loadData load parameters for RTM backward kernel from given path
     *
     * @param filePath the path where parameters are stored
     *
     */
    void loadData(const string filePath) {
        readBin(filePath + "v2dt2.bin", sizeof(t_DataType) * m_width * m_height, m_v2dt2);
        for (unsigned int i = 0; i < m_shots; i++)
            readBin(filePath + "sensor_s" + to_string(i) + ".bin", sizeof(t_DataType) * m_imgX * m_time, m_receiver[i]);
        readBin(filePath + "taperx.bin", sizeof(t_DataType) * m_xb, m_taperx);
        readBin(filePath + "taperz.bin", sizeof(t_DataType) * m_zb, m_taperz);
        readBin(filePath + "coefx.bin", sizeof(t_DataType) * (t_Order + 1), m_coefx);
        readBin(filePath + "coefz.bin", sizeof(t_DataType) * (t_Order + 1), m_coefz);
    }

    /**
     * @brief run launch the RTM backward kernel with given input parameters
     *
     * @param p_shot the shot id
     * @param p_upb the upper boundary data
     * @param p_snaps input seismic snapshot
     * @param p_p output seismic source wavefiled
     * @param p_r output seismic receiver wavefiled
     * @param p_i output seismic image
     *
     */
    double run(unsigned int p_shot,
               const AlignMem<t_PairType>& p_snaps,
               const AlignMem<t_DataType>& p_upb,
               AlignMem<t_PairType>& p_p,
               AlignMem<t_PairType>& p_r,
               AlignMem<t_DataType>& p_i) {
        m_handle = m_fpga->m_handle;

        unsigned d_coefx = xclAllocUserPtrBO(m_handle, m_coefx, sizeof(t_DataType) * (t_Order + 1), 33); // DDR[1]
        unsigned d_coefz = xclAllocUserPtrBO(m_handle, m_coefz, sizeof(t_DataType) * (t_Order + 1), 33);
        unsigned d_taperx = xclAllocUserPtrBO(m_handle, m_taperx, sizeof(t_DataType) * m_xb, 33);
        unsigned d_taperz = xclAllocUserPtrBO(m_handle, m_taperz, sizeof(t_DataType) * m_zb, 33);
        unsigned d_v2dt2 = xclAllocUserPtrBO(m_handle, m_v2dt2, sizeof(t_DataType) * m_width * m_height, 3);

        AlignMem<t_PairType> l_p0(m_width * m_height / t_PE);
        AlignMem<t_PairType> l_r0(m_width * m_height / t_PE);
        AlignMem<t_DataType> l_i0 = p_i;
        if (l_i0.size() == 0) l_i0.alloc(m_imgX * m_imgZ);

        for (unsigned int i = 0; i < m_width * m_height / t_PE; i++) {
            l_p0[i][0] = p_snaps[i][1];
            l_p0[i][1] = p_snaps[i][0];
        }

        unsigned d_receiver =
            xclAllocUserPtrBO(m_handle, m_receiver[p_shot], sizeof(t_DataType) * m_time * m_imgX, 33); // DDR[1]
        unsigned d_upb =
            xclAllocUserPtrBO(m_handle, p_upb.ptr(), sizeof(t_DataType) * m_width * t_Order * m_time / 2, 33);
        unsigned d_p0 =
            xclAllocUserPtrBO(m_handle, l_p0.ptr(), sizeof(t_PairType) * m_width * m_height / t_PE, 0); // HBM[0]
        unsigned d_r0 = xclAllocUserPtrBO(m_handle, l_r0.ptr(), sizeof(t_PairType) * m_width * m_height / t_PE, 1);
        unsigned d_i0 = xclAllocUserPtrBO(m_handle, l_i0.ptr(), sizeof(RTM_dataType) * m_imgX * m_imgZ, 2);

        xclSyncBO(m_handle, d_r0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_i0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(RTM_dataType) * m_imgX * m_imgZ, 0);
        xclSyncBO(m_handle, d_p0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_v2dt2, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * m_height, 0);
        xclSyncBO(m_handle, d_taperx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_xb, 0);
        xclSyncBO(m_handle, d_taperz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_zb, 0);
        xclSyncBO(m_handle, d_coefx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_coefz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_upb, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * t_Order * m_time / 2, 0);
        xclSyncBO(m_handle, d_receiver, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_time * m_imgX, 0);

        xclBOProperties p;
        unsigned long long address_receiver = !xclGetBOProperties(m_handle, d_receiver, &p) ? p.paddr : -1;
        unsigned long long address_coefz = !xclGetBOProperties(m_handle, d_coefz, &p) ? p.paddr : -1;
        unsigned long long address_coefx = !xclGetBOProperties(m_handle, d_coefx, &p) ? p.paddr : -1;
        unsigned long long address_taperz = !xclGetBOProperties(m_handle, d_taperz, &p) ? p.paddr : -1;
        unsigned long long address_taperx = !xclGetBOProperties(m_handle, d_taperx, &p) ? p.paddr : -1;
        unsigned long long address_v2dt2 = !xclGetBOProperties(m_handle, d_v2dt2, &p) ? p.paddr : -1;

        unsigned long long address_p0 = !xclGetBOProperties(m_handle, d_p0, &p) ? p.paddr : -1;
        unsigned long long address_r0 = !xclGetBOProperties(m_handle, d_r0, &p) ? p.paddr : -1;
        unsigned long long address_i0 = !xclGetBOProperties(m_handle, d_i0, &p) ? p.paddr : -1;

        unsigned long long address_upb = !xclGetBOProperties(m_handle, d_upb, &p) ? p.paddr : -1;

        unsigned int execHandle = xclAllocBO(m_handle, 4096, xclBOKind(0), (1 << 31));

        void* execData = xclMapBO(m_handle, execHandle, true);
        auto ecmd = reinterpret_cast<ert_start_kernel_cmd*>(execData);
        auto rsz = XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 2; // regmap array size
        memset(ecmd, 0, (sizeof *ecmd) + rsz);
        ecmd->state = ERT_CMD_STATE_NEW;
        ecmd->opcode = ERT_START_CU;
        ecmd->count = 1 + rsz;
        ecmd->cu_mask = 0x1 << 0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_AP_CTRL] = 0x0; // ap_start
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_Z_DATA / 4] = m_height;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_X_DATA / 4] = m_width;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_T_DATA / 4] = m_time;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_RECZ_DATA / 4] = m_zb;

        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_REC_DATA / 4] = address_receiver;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFZ_DATA / 4] = address_coefz;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFX_DATA / 4] = address_coefx;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4] = address_taperz;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERX_DATA / 4] = address_taperx;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_V2DT2_DATA / 4] = address_v2dt2;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P0_DATA / 4] = address_p0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P1_DATA / 4] = address_p0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R0_DATA / 4] = address_r0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R1_DATA / 4] = address_r0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I0_DATA / 4] = address_i0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I1_DATA / 4] = address_i0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA / 4] = address_upb;

        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_REC_DATA / 4 + 1] = address_receiver >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFZ_DATA / 4 + 1] = address_coefz >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFX_DATA / 4 + 1] = address_coefx >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4 + 1] = address_taperz >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERX_DATA / 4 + 1] = address_taperx >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_V2DT2_DATA / 4 + 1] = address_v2dt2 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P0_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P1_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R0_DATA / 4 + 1] = address_r0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R1_DATA / 4 + 1] = address_r0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I0_DATA / 4 + 1] = address_i0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I1_DATA / 4 + 1] = address_i0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 1] = address_upb >> 32;

        xclExecBuf(m_handle, execHandle);

        auto start = chrono::high_resolution_clock::now();

        while (xclExecWait(m_handle, 1) == 0)
            ;

        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;

        xclSyncBO(m_handle, d_i0, XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(RTM_dataType) * m_imgX * m_imgZ, 0);
        xclSyncBO(m_handle, d_p0, XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_r0, XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);

        p_p = l_p0;
        p_r = l_r0;
        p_i = l_i0;

        return elapsed.count();
    }

    /**
     * @brief run launch the RTM backward kernel with given input parameters (in order to support multi-kernel, run
     * function
     * is split to before run and after_run)
     */
    void before_run(unsigned int p_shot,
                    const AlignMem<t_PairType>& p_snaps,
                    const AlignMem<t_DataType>& p_upb,
                    AlignMem<t_PairType>& p_p,
                    AlignMem<t_PairType>& p_r,
                    AlignMem<t_DataType>& p_i,
                    AlignMem<t_PairType>& l_p0,
                    AlignMem<t_PairType>& l_r0,
                    AlignMem<t_DataType>& l_i0) {
        m_handle = m_fpga->m_handle;

        unsigned d_coefx = xclAllocUserPtrBO(m_handle, m_coefx, sizeof(t_DataType) * (t_Order + 1), 33); // DDR[1]
        unsigned d_coefz = xclAllocUserPtrBO(m_handle, m_coefz, sizeof(t_DataType) * (t_Order + 1), 33);
        unsigned d_taperx = xclAllocUserPtrBO(m_handle, m_taperx, sizeof(t_DataType) * m_xb, 33);
        unsigned d_taperz = xclAllocUserPtrBO(m_handle, m_taperz, sizeof(t_DataType) * m_zb, 33);
        unsigned d_v2dt2 = xclAllocUserPtrBO(m_handle, m_v2dt2, sizeof(t_DataType) * m_width * m_height, 3);

        for (unsigned int i = 0; i < m_width * m_height / t_PE; i++) {
            l_p0[i][0] = p_snaps[i][1];
            l_p0[i][1] = p_snaps[i][0];
        }

        unsigned d_receiver =
            xclAllocUserPtrBO(m_handle, m_receiver[p_shot], sizeof(t_DataType) * m_time * m_imgX, 33); // DDR[1]
        unsigned d_upb =
            xclAllocUserPtrBO(m_handle, p_upb.ptr(), sizeof(t_DataType) * m_width * t_Order * m_time / 2, 33);
        unsigned d_p0 =
            xclAllocUserPtrBO(m_handle, l_p0.ptr(), sizeof(t_PairType) * m_width * m_height / t_PE, 0); // HBM[0]
        unsigned d_r0 =
            xclAllocUserPtrBO(m_handle, l_r0.ptr(), sizeof(t_PairType) * m_width * m_height / t_PE, 1);     // HBM[2]
        unsigned d_i0 = xclAllocUserPtrBO(m_handle, l_i0.ptr(), sizeof(RTM_dataType) * m_imgX * m_imgZ, 2); // HBM[4]

        m_boIndex[0] = d_p0;
        m_boIndex[1] = d_r0;
        m_boIndex[2] = d_i0;

        m_boIndex[3] = d_coefx;
        m_boIndex[4] = d_coefz;
        m_boIndex[5] = d_taperx;
        m_boIndex[6] = d_taperz;
        m_boIndex[7] = d_v2dt2;
        m_boIndex[8] = d_receiver;
        m_boIndex[9] = d_upb;

        xclSyncBO(m_handle, d_r0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_i0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(RTM_dataType) * m_imgX * m_imgZ, 0);
        xclSyncBO(m_handle, d_p0, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE, 0);
        xclSyncBO(m_handle, d_v2dt2, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * m_height, 0);
        xclSyncBO(m_handle, d_taperx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_xb, 0);
        xclSyncBO(m_handle, d_taperz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_zb, 0);
        xclSyncBO(m_handle, d_coefx, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_coefz, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * (t_Order + 1), 0);
        xclSyncBO(m_handle, d_upb, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_width * t_Order * m_time / 2, 0);
        xclSyncBO(m_handle, d_receiver, XCL_BO_SYNC_BO_TO_DEVICE, sizeof(t_DataType) * m_time * m_imgX, 0);

        xclBOProperties p;
        unsigned long long address_receiver = !xclGetBOProperties(m_handle, d_receiver, &p) ? p.paddr : -1;
        unsigned long long address_coefz = !xclGetBOProperties(m_handle, d_coefz, &p) ? p.paddr : -1;
        unsigned long long address_coefx = !xclGetBOProperties(m_handle, d_coefx, &p) ? p.paddr : -1;
        unsigned long long address_taperz = !xclGetBOProperties(m_handle, d_taperz, &p) ? p.paddr : -1;
        unsigned long long address_taperx = !xclGetBOProperties(m_handle, d_taperx, &p) ? p.paddr : -1;
        unsigned long long address_v2dt2 = !xclGetBOProperties(m_handle, d_v2dt2, &p) ? p.paddr : -1;

        unsigned long long address_p0 = !xclGetBOProperties(m_handle, d_p0, &p) ? p.paddr : -1;
        unsigned long long address_r0 = !xclGetBOProperties(m_handle, d_r0, &p) ? p.paddr : -1;
        unsigned long long address_i0 = !xclGetBOProperties(m_handle, d_i0, &p) ? p.paddr : -1;

        unsigned long long address_upb = !xclGetBOProperties(m_handle, d_upb, &p) ? p.paddr : -1;

        unsigned int execHandle = xclAllocBO(m_handle, 4096, xclBOKind(0), (1 << 31));

        void* execData = xclMapBO(m_handle, execHandle, true);
        auto ecmd = reinterpret_cast<ert_start_kernel_cmd*>(execData);
        auto rsz = XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 2; // regmap array size
        memset(ecmd, 0, (sizeof *ecmd) + rsz);
        ecmd->state = ERT_CMD_STATE_NEW;
        ecmd->opcode = ERT_START_CU;
        ecmd->count = 1 + rsz;
        ecmd->cu_mask = 0x1 << 0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_AP_CTRL] = 0x0; // ap_start
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_Z_DATA / 4] = m_height;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_X_DATA / 4] = m_width;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_T_DATA / 4] = m_time;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_RECZ_DATA / 4] = m_zb;

        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_REC_DATA / 4] = address_receiver;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFZ_DATA / 4] = address_coefz;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFX_DATA / 4] = address_coefx;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4] = address_taperz;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERX_DATA / 4] = address_taperx;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_V2DT2_DATA / 4] = address_v2dt2;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P0_DATA / 4] = address_p0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P1_DATA / 4] = address_p0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R0_DATA / 4] = address_r0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R1_DATA / 4] = address_r0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I0_DATA / 4] = address_i0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I1_DATA / 4] = address_i0;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA / 4] = address_upb;

        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_REC_DATA / 4 + 1] = address_receiver >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFZ_DATA / 4 + 1] = address_coefz >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_COEFX_DATA / 4 + 1] = address_coefx >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERZ_DATA / 4 + 1] = address_taperz >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_TAPERX_DATA / 4 + 1] = address_taperx >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_V2DT2_DATA / 4 + 1] = address_v2dt2 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P0_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_P1_DATA / 4 + 1] = address_p0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R0_DATA / 4 + 1] = address_r0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_R1_DATA / 4 + 1] = address_r0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I0_DATA / 4 + 1] = address_i0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_I1_DATA / 4 + 1] = address_i0 >> 32;
        ecmd->data[XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA / 4 + 1] = address_upb >> 32;

        xclExecBuf(m_handle, execHandle);
    }

    /**
    * @brief run launch the RTM backward kernel with given input parameters (in order to support multi-kernel, run
    * function
    * is split to before run and after_run)
    */
    void after_run(AlignMem<t_PairType>& p_p,
                   AlignMem<t_PairType>& p_r,
                   AlignMem<t_DataType>& p_i,
                   AlignMem<t_PairType>& l_p0,
                   AlignMem<t_PairType>& l_r0,
                   AlignMem<t_DataType>& l_i0) {
        m_handle = m_fpga->m_handle;

        xclSyncBO(m_handle, m_boIndex[2], XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(RTM_dataType) * m_imgX * m_imgZ, 0);
        xclSyncBO(m_handle, m_boIndex[0], XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE,
                  0);
        xclSyncBO(m_handle, m_boIndex[1], XCL_BO_SYNC_BO_FROM_DEVICE, sizeof(t_PairType) * m_width * m_height / t_PE,
                  0);

        p_p = l_p0;
        p_r = l_r0;
        p_i = l_i0;

        for (int i = 0; i < 10; i++) {
            xclFreeBO(m_handle, m_boIndex[i]);
        }
    }

   private:
    const FPGA* m_fpga;
    xclDeviceHandle m_handle;
    unsigned int m_boIndex[10];

    unsigned int m_width, m_height, m_xb, m_zb, m_time, m_shots, m_imgX, m_imgZ;

    t_DataType *m_taperx, *m_taperz;
    t_DataType *m_v2dt2, **m_receiver;
    t_DataType *m_coefx, *m_coefz;
};

#endif
