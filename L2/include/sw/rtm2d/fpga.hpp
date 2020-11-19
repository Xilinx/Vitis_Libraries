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
#ifndef XF_RTM_FPGA_HPP
#define XF_RTM_FPGA_HPP

#include "alignMem.hpp"
#include "binFiles.hpp"
#include "types.hpp"
#include "xcl2.hpp"
#include <chrono>
#include <vector>

using namespace std;

/**
 * @file fpga.hpp
 * @brief class FPGA, ForwardKernel and BackwardKernel are defined here.
 */

/**
 * @brief class FPGA is used to manage FPGA info
 */
class FPGA {
   public:
    FPGA(string p_xclbinFile, unsigned int p_deviceId = 0) {
        m_deviceId = p_deviceId;
        m_devices = xcl::get_xil_devices();

        m_Context = cl::Context({m_devices[m_deviceId]});
        m_CommandQueue = cl::CommandQueue(m_Context, m_devices[m_deviceId], CL_QUEUE_PROFILING_ENABLE);
        m_bins = xcl::import_binary_file(p_xclbinFile);
        m_Program = cl::Program(m_Context, {m_devices[m_deviceId]}, m_bins);
    }

    cl::Kernel createKernel(const string& p_name) const { return cl::Kernel(m_Program, p_name.c_str()); }
    const cl::Context& getContext() const { return m_Context; }
    const cl::CommandQueue& getCommandQueue() const { return m_CommandQueue; }

    const cl::Device operator[](const unsigned int p_Idx) const { return m_devices[p_Idx]; }

   private:
    unsigned int m_deviceId;
    vector<cl::Device> m_devices;
    cl::Context m_Context;
    cl::CommandQueue m_CommandQueue;
    cl::Program::Binaries m_bins;
    cl::Program m_Program;
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
        static const string t_KernelName = "rtmforward";
        m_fpga = fpga;
        m_Kernel = m_fpga->createKernel(t_KernelName);

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
        for (int s = 0; s < m_shots; s++) m_srcs[s] = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_time);

        m_coefx = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
        m_coefz = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
    }

    ~ForwardKernel() {
        free(m_coefx);
        free(m_coefz);
        free(m_taperx);
        free(m_taperz);
        free(m_v2dt2);
        for (int s = 0; s < m_shots; s++) free(m_srcs[s]);
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
        for (int s = 0; s < m_shots; s++)
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
     * @param p_p, seismic snapshot
     * @param p_upb the upper boundary data
     *
     */
    double run(unsigned int p_shot, unsigned int p_sx, AlignMem<t_PairType>& p_p, AlignMem<t_DataType>& p_upb) {
        AlignMem<t_PairType> l_p0(m_width * m_height / t_PE);
        AlignMem<t_DataType> l_upb(m_width * t_Order * m_time / 2);

        cl::Context m_Context = m_fpga->getContext();
        cl::CommandQueue m_CommandQueue = m_fpga->getCommandQueue();
        static cl::Buffer d_coefx =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * (t_Order + 1), m_coefx);
        static cl::Buffer d_coefz =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * (t_Order + 1), m_coefz);
        static cl::Buffer d_taperx =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * m_xb, m_taperx);
        static cl::Buffer d_taperz =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * m_zb, m_taperz);
        static cl::Buffer d_v2dt2 = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                               sizeof(t_DataType) * m_width * m_height, m_v2dt2);

        cl::Buffer d_srcs =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * m_time, m_srcs[p_shot]);

        cl::Buffer d_upb = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      sizeof(t_DataType) * m_width * t_Order * m_time / 2, l_upb.ptr());

        cl::Buffer d_p0 = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(t_PairType) * m_width * m_height / t_PE, l_p0.ptr());

        vector<cl::Memory> inBufVec, outBufVec;
        inBufVec.push_back(d_taperx);
        inBufVec.push_back(d_taperz);
        inBufVec.push_back(d_coefx);
        inBufVec.push_back(d_coefz);
        inBufVec.push_back(d_v2dt2);
        inBufVec.push_back(d_srcs);

        inBufVec.push_back(d_p0);
        inBufVec.push_back(d_upb);

        outBufVec.push_back(d_p0);
        outBufVec.push_back(d_upb);
        int fArg = 0;

        m_Kernel.setArg(fArg++, m_height);
        m_Kernel.setArg(fArg++, m_width);
        m_Kernel.setArg(fArg++, m_time);
        m_Kernel.setArg(fArg++, m_zb);
        m_Kernel.setArg(fArg++, p_sx);
        m_Kernel.setArg(fArg++, d_srcs);
        m_Kernel.setArg(fArg++, d_coefz);
        m_Kernel.setArg(fArg++, d_coefx);
        m_Kernel.setArg(fArg++, d_taperz);
        m_Kernel.setArg(fArg++, d_taperx);
        m_Kernel.setArg(fArg++, d_v2dt2);
        m_Kernel.setArg(fArg++, d_p0);
        m_Kernel.setArg(fArg++, d_p0);
        m_Kernel.setArg(fArg++, d_upb);
        m_CommandQueue.finish();

        m_CommandQueue.enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
        m_CommandQueue.finish();

        auto start = chrono::high_resolution_clock::now();
        m_CommandQueue.enqueueTask(m_Kernel);
        m_CommandQueue.finish();
        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;

        m_CommandQueue.enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_CommandQueue.finish();

        p_p = l_p0;

        p_upb = l_upb;
        return elapsed.count();
    }

   private:
    const FPGA* m_fpga;
    cl::Kernel m_Kernel;

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
        static const string t_KernelName = "rtmbackward";
        m_fpga = fpga;
        m_Kernel = m_fpga->createKernel(t_KernelName);

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
        for (int i = 0; i < m_shots; i++)
            m_receiver[i] = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * m_imgX * m_time);
        m_coefx = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
        m_coefz = (t_DataType*)aligned_alloc(_4k, sizeof(t_DataType) * (t_Order + 1));
    }

    ~BackwardKernel() {
        free(m_taperx);
        free(m_taperz);
        free(m_v2dt2);
        for (int i = 0; i < m_shots; i++) free(m_receiver[i]);
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
        for (int i = 0; i < m_shots; i++)
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
     * @param p_sx the shot coordinate
     * @param p_p, seismic snapshot
     * @param p_upb the upper boundary data
     *
     */
    double run(unsigned int p_shot,
               const AlignMem<t_PairType>& p_snaps,
               const AlignMem<t_DataType>& p_upb,
               AlignMem<t_PairType>& p_p,
               AlignMem<t_PairType>& p_r,
               AlignMem<t_DataType>& p_i) {
        cl::Context m_Context = m_fpga->getContext();
        cl::CommandQueue m_CommandQueue = m_fpga->getCommandQueue();

        static cl::Buffer d_coefx =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * (t_Order + 1), m_coefx);
        static cl::Buffer d_coefz =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * (t_Order + 1), m_coefz);
        static cl::Buffer d_taperx =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * m_xb, m_taperx);
        static cl::Buffer d_taperz =
            cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(t_DataType) * m_zb, m_taperz);
        static cl::Buffer d_v2dt2 = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                               sizeof(t_DataType) * m_width * m_height, m_v2dt2);

        AlignMem<t_PairType> l_p0(m_width * m_height / t_PE);
        AlignMem<t_PairType> l_r0(m_width * m_height / t_PE);
        AlignMem<t_DataType> l_i0 = p_i;
        if (l_i0.size() == 0) l_i0.alloc(m_imgX * m_imgZ);

        for (int i = 0; i < m_width * m_height / t_PE; i++) {
            l_p0[i][0] = p_snaps[i][1];
            l_p0[i][1] = p_snaps[i][0];
        }

        cl::Buffer d_receiver = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                           sizeof(t_DataType) * m_time * m_imgX, m_receiver[p_shot]);

        cl::Buffer d_upb = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                      sizeof(t_DataType) * m_width * t_Order * m_time / 2, p_upb.ptr());

        cl::Buffer d_p0 = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(t_PairType) * m_width * m_height / t_PE, l_p0.ptr());

        cl::Buffer d_r0 = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(t_PairType) * m_width * m_height / t_PE, l_r0.ptr());
        cl::Buffer d_i0 = cl::Buffer(m_Context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(RTM_dataType) * m_imgX * m_imgZ, l_i0.ptr());

        vector<cl::Memory> inBufVec, outBufVec;
        inBufVec.push_back(d_r0);
        inBufVec.push_back(d_i0);
        inBufVec.push_back(d_p0);
        inBufVec.push_back(d_v2dt2);
        inBufVec.push_back(d_taperx);
        inBufVec.push_back(d_taperz);
        inBufVec.push_back(d_coefx);
        inBufVec.push_back(d_coefz);
        inBufVec.push_back(d_upb);
        inBufVec.push_back(d_receiver);

        outBufVec.push_back(d_i0);
        outBufVec.push_back(d_p0);
        outBufVec.push_back(d_r0);

        int bArg = 0;
        m_Kernel.setArg(bArg++, m_height);
        m_Kernel.setArg(bArg++, m_width);
        m_Kernel.setArg(bArg++, m_time);
        m_Kernel.setArg(bArg++, m_zb);
        m_Kernel.setArg(bArg++, d_receiver);
        m_Kernel.setArg(bArg++, d_coefz);
        m_Kernel.setArg(bArg++, d_coefx);
        m_Kernel.setArg(bArg++, d_taperz);
        m_Kernel.setArg(bArg++, d_taperx);
        m_Kernel.setArg(bArg++, d_v2dt2);
        m_Kernel.setArg(bArg++, d_p0);
        m_Kernel.setArg(bArg++, d_p0);
        m_Kernel.setArg(bArg++, d_r0);
        m_Kernel.setArg(bArg++, d_r0);
        m_Kernel.setArg(bArg++, d_i0);
        m_Kernel.setArg(bArg++, d_i0);
        m_Kernel.setArg(bArg++, d_upb);
        m_CommandQueue.finish();

        m_CommandQueue.enqueueMigrateMemObjects(inBufVec, 0 /* 0 means from host*/);
        m_CommandQueue.finish();

        auto start = chrono::high_resolution_clock::now();
        m_CommandQueue.enqueueTask(m_Kernel);
        m_CommandQueue.finish();
        auto finish = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsed = finish - start;

        m_CommandQueue.enqueueMigrateMemObjects(outBufVec, CL_MIGRATE_MEM_OBJECT_HOST);
        m_CommandQueue.finish();

        p_p = l_p0;
        p_r = l_r0;
        p_i = l_i0;

        return elapsed.count();
    }

   private:
    const FPGA* m_fpga;
    cl::Kernel m_Kernel;

    unsigned int m_width, m_height, m_xb, m_zb, m_time, m_shots, m_imgX, m_imgZ;
    t_DataType *m_taperx, *m_taperz;
    t_DataType *m_v2dt2, **m_receiver;
    t_DataType *m_coefx, *m_coefz;
};

#endif
