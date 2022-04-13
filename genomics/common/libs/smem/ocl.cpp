/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#include "ocl.hpp"
#include <stdio.h>
#include <iostream>
#include <vector>

bool init = false;
using namespace xf::genomics;
// Checks OpenCL error codes
void check(cl_int err_code) {
    if (err_code != CL_SUCCESS) {
        printf("ERROR: %d\n", err_code);
        exit(EXIT_FAILURE);
    }
}

// An event callback function that prints the operations performed by the OpenCL
// runtime.
void event_cb(cl_event event, cl_int cmd_status, void* data) {
    cl_command_type command;
    clGetEventInfo(event, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &command, nullptr);
    cl_int status;
    clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
    const char* command_str;
    const char* status_str;
    switch (command) {
        case CL_COMMAND_READ_BUFFER:
            command_str = "buffer read";
            break;
        case CL_COMMAND_WRITE_BUFFER:
            command_str = "buffer write";
            break;
        case CL_COMMAND_NDRANGE_KERNEL:
            command_str = "kernel";
            break;
    }
    switch (status) {
        case CL_QUEUED:
            status_str = "Queued";
            break;
        case CL_SUBMITTED:
            status_str = "Submitted";
            break;
        case CL_RUNNING:
            status_str = "Executing";
            break;
        case CL_COMPLETE:
            status_str = "Completed";
            break;
    }
    printf("%s %s %s\n", status_str, reinterpret_cast<char*>(data), command_str);
    fflush(stdout);
}

// Sets the callback for a particular event
void set_callback(cl_event event, const char* queue_name) {
    clSetEventCallback(event, CL_COMPLETE, event_cb, (void*)queue_name);
}

int load_file_to_memory(const char* filename, char** result) {
    size_t size = 0;
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        printf("ERROR : Kernel binary %s not exist!\n", filename);
        *result = NULL;
        return -1; // -1 means file opening fail
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = (char*)malloc(size + 1);
    if ((int)size != (int)fread(*result, sizeof(char), size, f)) {
        free(*result);
        return -2; // -2 means file reading fail
    }
    fclose(f);
    (*result)[size] = 0;
    return size;
}

// OpenCL setup initialization
int xilSmem::init_fpga_cpp(std::string& binaryFileName) {
    // Error handling in OpenCL is performed using the cl_int specifier. OpenCL
    // functions either return or accept pointers to cl_int types to indicate if
    // an error occurred.
    cl_int err;

    // Look for platform
    std::vector<cl::Platform> platforms;
    OCL_CHECK(err, err = cl::Platform::get(&platforms));
    auto num_platforms = platforms.size();
    if (num_platforms == 0) {
        std::cerr << "No Platforms were found this could be cased because of the OpenCL \
                      ICD not installed at /etc/OpenCL/vendors directory"
                  << std::endl;
        return -32;
    }

    std::string platformName;
    cl::Platform platform;
    bool foundFlag = false;
    for (auto p : platforms) {
        platform = p;
        OCL_CHECK(err, platformName = platform.getInfo<CL_PLATFORM_NAME>(&err));
        if (platformName == "Xilinx") {
            foundFlag = true;
#if (VERBOSE_LEVEL >= 2)
            std::cout << "Found Platform" << std::endl;
            std::cout << "Platform Name: " << platformName.c_str() << std::endl;
#endif
            break;
        }
    }
    if (foundFlag == false) {
        std::cerr << "Error: Failed to find Xilinx platform" << std::endl;
        return 1;
    }
    // Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    OCL_CHECK(err, err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
    m_device = devices[0];

    // OpenCL Setup Start
    // Creating Context and Command Queue for selected Device
    cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
    OCL_CHECK(err, m_context = new cl::Context(m_device, props, NULL, NULL, &err));
    if (err) {
        std::cerr << "Context creation Failed " << std::endl;
        return 1;
    }
// Import_binary() command will find the OpenCL binary file created using the
// v++ compiler load into OpenCL Binary and return as Binaries
// OpenCL and it can contain many functions which can be executed on the
// device.
#if (VERBOSE_LEVEL >= 2)
    std::cout << "INFO: Reading " << binaryFileName << std::endl;
#endif
#if 0
    if (access(binaryFileName.c_str(), R_OK) != 0) {
        std::cerr << "ERROR: " << binaryFileName.c_str() << " xclbin not available please build " << std::endl;
        return 1;
    }
#endif

#if (VERBOSE_LEVEL >= 2)
    // Loading XCL Bin into char buffer
    std::cout << "Loading: '" << binaryFileName.c_str() << "'\n";
#endif
    std::ifstream bin_file(binaryFileName.c_str(), std::ifstream::binary);
    if (bin_file.fail()) {
        std::cerr << "Unable to open binary file" << std::endl;
        return 1;
    }

    bin_file.seekg(0, bin_file.end);
    auto nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);

    std::vector<uint8_t> buf;
    buf.resize(nb);
    bin_file.read(reinterpret_cast<char*>(buf.data()), nb);

    cl::Program::Binaries bins{{buf.data(), buf.size()}};
    OCL_CHECK(err, m_program = new cl::Program(*m_context, {m_device}, bins, NULL, &err));

    // Create command quque
    OCL_CHECK(err, m_comqueue = new cl::CommandQueue(
                       *m_context, m_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE, &err));

    // OpenCL Host / Device Buffer Setup Start
    return 0;
}

xilSmem::~xilSmem() {
    for (auto i = 0; i < COMPUTE_UNIT; i++) {
        delete m_kernel_smem[i];
        delete m_bwt_buffer[i];
        delete m_bwt_para_buffer[i];
        delete m_seq_buffer[i];
        delete m_seq_len_buffer[i];
        delete m_mem_buffer[i];
        delete m_mem_num_buffer[i];
    }

    delete m_context;
    delete m_program;
    delete m_comqueue;
}
void xilSmem::ocl_init(void) {
    int err;
    std::string kernel_comm = "mem_collect_intv_core";
    int kernel_max_batch_size = BATCH_SIZE / 4;
    if (!init) {
        init_fpga_cpp(m_binary);
        for (int i = 0; i < COMPUTE_UNIT; i++) {
            double total_dram_size = 0;
            auto cu_id = std::to_string(i + 1);
            std::string cur_kernel = kernel_comm + ":{" + kernel_comm + "_" + cu_id + "}";
            std::cout << "Kernel Name: " << cur_kernel << std::endl;

            OCL_CHECK(err, m_kernel_smem[i] = new cl::Kernel(*m_program, cur_kernel.c_str(), &err));
            printf("m_bwtSize %d \n", m_bwtSize);
            OCL_CHECK(err, m_bwt_buffer[i] = new cl::Buffer(*(this->m_context), CL_MEM_READ_ONLY,
                                                            m_bwtSize * sizeof(uint32_t), nullptr, &err));
            OCL_CHECK(err, m_bwt_para_buffer[i] = new cl::Buffer(*(this->m_context), CL_MEM_READ_ONLY,
                                                                 7 * sizeof(uint64_t), nullptr, &err));
            OCL_CHECK(err, m_seq_buffer[i] =
                               new cl::Buffer(*(this->m_context), CL_MEM_READ_ONLY,
                                              kernel_max_batch_size * c_seq_length * sizeof(uint8_t), nullptr, &err));
            OCL_CHECK(err, m_mem_buffer[i] = new cl::Buffer(
                               *(this->m_context), CL_MEM_WRITE_ONLY,
                               kernel_max_batch_size * c_max_intv_alloc * sizeof(bwtintv_t), nullptr, &err));
            OCL_CHECK(err,
                      m_seq_len_buffer[i] = new cl::Buffer(*(this->m_context), CL_MEM_READ_ONLY,
                                                           kernel_max_batch_size * sizeof(uint8_t), nullptr, &err));
            OCL_CHECK(err, m_mem_num_buffer[i] = new cl::Buffer(*(this->m_context), CL_MEM_WRITE_ONLY,
                                                                kernel_max_batch_size * sizeof(int), nullptr, &err));
        }
        init = true;
    }
}

void xilSmem::ocl_invoke_seq(uint8_t* seq,
                             uint8_t* seq_len,
                             bwtintv_t* mem_output,
                             int* mem_num,
                             int batch_size,
                             double* cur_kernel_time,
                             uint64_t bwt_size) {
    int err;
    int kernel_batch_size = batch_size / COMPUTE_UNIT;
    int kernel_max_batch_size = BATCH_SIZE / COMPUTE_UNIT;

    int narg = 0;
    // Set kernel Args
    m_kernel_smem[0]->setArg(narg++, *(m_bwt_buffer[0]));
    m_kernel_smem[0]->setArg(narg++, *(m_mem_buffer[0]));
    m_kernel_smem[0]->setArg(narg++, *(m_bwt_para_buffer[0]));
    m_kernel_smem[0]->setArg(narg++, *(m_mem_num_buffer[0]));
    m_kernel_smem[0]->setArg(narg++, *(m_seq_buffer[0]));
    m_kernel_smem[0]->setArg(narg++, *(m_seq_len_buffer[0]));
    m_kernel_smem[0]->setArg(narg++, kernel_batch_size);

    OCL_CHECK(err,
              err = m_comqueue->enqueueWriteBuffer(*m_bwt_buffer[0], CL_FALSE, 0, sizeof(uint32_t) * m_bwtSize, m_bwt));
    OCL_CHECK(
        err, err = m_comqueue->enqueueWriteBuffer(*m_bwt_para_buffer[0], CL_FALSE, 0, sizeof(uint64_t) * 7, m_bwtpara));
    OCL_CHECK(err, err = m_comqueue->enqueueWriteBuffer(*m_seq_buffer[0], CL_FALSE, 0,
                                                        sizeof(uint8_t) * c_seq_length * kernel_batch_size, seq));
    OCL_CHECK(err, err = m_comqueue->enqueueWriteBuffer(*m_seq_len_buffer[0], CL_FALSE, 0,
                                                        sizeof(uint8_t) * kernel_batch_size, seq_len));

    m_comqueue->finish();

    cl::Event kernel_event;

    // Launch Kernels
    OCL_CHECK(err, err = m_comqueue->enqueueTask(*m_kernel_smem[0], nullptr, &kernel_event));
    m_comqueue->finish();

    cl_ulong start, end;
    kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
    kernel_event.getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
    cur_kernel_time[0] = (end - start);

    // Read data back
    OCL_CHECK(err, err = m_comqueue->enqueueReadBuffer(*m_mem_buffer[0], CL_FALSE, 0,
                                                       sizeof(bwtintv_t) * c_max_intv_alloc * kernel_batch_size,
                                                       &mem_output[0]));
    OCL_CHECK(err, err = m_comqueue->enqueueReadBuffer(*m_mem_num_buffer[0], CL_FALSE, 0,
                                                       sizeof(int) * kernel_batch_size, &mem_num[0]));

    m_comqueue->finish();
}

void xilSmem::ocl_invoke_overlap(uint8_t* seq,
                                 uint8_t* seq_len,
                                 bwtintv_t* mem_output,
                                 int* mem_num,
                                 int batch_size,
                                 double* cur_kernel_time,
                                 uint64_t bwt_size) {
    int err;
    int kernel_batch_size = batch_size / COMPUTE_UNIT;
    int kernel_max_batch_size = BATCH_SIZE / COMPUTE_UNIT;

    for (int i = 0; i < COMPUTE_UNIT; i++) {
        int narg = 0;
        // Set kernel Args
        m_kernel_smem[i]->setArg(narg++, *(m_bwt_buffer[i]));
        m_kernel_smem[i]->setArg(narg++, *(m_mem_buffer[i]));
        m_kernel_smem[i]->setArg(narg++, *(m_bwt_para_buffer[i]));
        m_kernel_smem[i]->setArg(narg++, *(m_mem_num_buffer[i]));
        m_kernel_smem[i]->setArg(narg++, *(m_seq_buffer[i]));
        m_kernel_smem[i]->setArg(narg++, *(m_seq_len_buffer[i]));
        m_kernel_smem[i]->setArg(narg++, kernel_batch_size);

        OCL_CHECK(err, err = m_comqueue->enqueueWriteBuffer(*m_bwt_buffer[i], CL_FALSE, 0, sizeof(uint32_t) * m_bwtSize,
                                                            m_bwt));
        OCL_CHECK(err, err = m_comqueue->enqueueWriteBuffer(*m_bwt_para_buffer[i], CL_FALSE, 0, sizeof(uint64_t) * 7,
                                                            m_bwtpara));
        OCL_CHECK(err, err = m_comqueue->enqueueWriteBuffer(*m_seq_buffer[i], CL_FALSE, 0,
                                                            sizeof(uint8_t) * c_seq_length * kernel_batch_size,
                                                            seq + i * c_seq_length * kernel_batch_size));
        OCL_CHECK(err, err = m_comqueue->enqueueWriteBuffer(*m_seq_len_buffer[i], CL_FALSE, 0,
                                                            sizeof(uint8_t) * kernel_batch_size,
                                                            seq_len + i * kernel_batch_size));
    }
    m_comqueue->finish();

    cl::Event kernel_event[COMPUTE_UNIT];

    // Launch Kernels
    for (int i = 0; i < COMPUTE_UNIT; i++) {
        OCL_CHECK(err, err = m_comqueue->enqueueTask(*m_kernel_smem[i], nullptr, &kernel_event[i]));
    }
    m_comqueue->finish();

    for (int i = 0; i < COMPUTE_UNIT; i++) {
        cl_ulong start, end;
        kernel_event[i].getProfilingInfo(CL_PROFILING_COMMAND_START, &start);
        kernel_event[i].getProfilingInfo(CL_PROFILING_COMMAND_END, &end);
        cur_kernel_time[i] = (end - start);
    }

    // Read data back
    for (int i = 0; i < COMPUTE_UNIT; i++) {
        OCL_CHECK(err, err = m_comqueue->enqueueReadBuffer(*m_mem_buffer[i], CL_TRUE, 0,
                                                           sizeof(bwtintv_t) * c_max_intv_alloc * kernel_batch_size,
                                                           &mem_output[i * c_max_intv_alloc * kernel_batch_size]));
        OCL_CHECK(err,
                  err = m_comqueue->enqueueReadBuffer(*m_mem_num_buffer[i], CL_TRUE, 0, sizeof(int) * kernel_batch_size,
                                                      &mem_num[i * kernel_batch_size]));
    }
    m_comqueue->finish();
}

bool xilSmem::ocl_cmp(char* fname_golden, bwtintv_v& mem, int mem_num[BATCH_SIZE]) {
    bool pass = true;
    for (int k = 0; k < BATCH_SIZE; k++) {
        mem.n = mem_num[k];
        FILE* fgolden = fopen(fname_golden, "rb");
        int n = 0;
        bwtint_t golden_x0;
        bwtint_t golden_x1;
        bwtint_t golden_x2;
        bwtint_t golden_info;
        fread(&n, sizeof(int), 1, fgolden);
        if (n != mem.n) {
            fclose(fgolden);
            printf("In batch %d, there are %d intv in the result but %d in the golden.\n", k, mem.n, n);
            return false;
        } else {
            for (int i = 0; i < n; i++) {
                fread(&golden_x0, sizeof(bwtint_t), 1, fgolden);
                fread(&golden_x1, sizeof(bwtint_t), 1, fgolden);
                fread(&golden_x2, sizeof(bwtint_t), 1, fgolden);
                fread(&golden_info, sizeof(bwtint_t), 1, fgolden);
                if (golden_x0 != mem.a[k * c_max_intv_alloc + i].x[0]) {
                    printf("x0 is different for intv %d\n", i);
                    fclose(fgolden);
                    return false;
                }
                if (golden_x1 != mem.a[k * c_max_intv_alloc + i].x[1]) {
                    printf("x1 is different for intv %d\n", i);
                    fclose(fgolden);
                    return false;
                }
                if (golden_x2 != mem.a[k * c_max_intv_alloc + i].x[2]) {
                    printf("x2 is different for intv %d\n", i);
                    fclose(fgolden);
                    return false;
                }
                if (golden_info != mem.a[k * c_max_intv_alloc + i].info) {
                    printf("info is different for intv %d\n", i);
                    fclose(fgolden);
                    return false;
                }
            }
            fclose(fgolden);
        }
    }
    return true;
}

int xilSmem::smem_ocl(
    uint8_t* seq, uint8_t* seq_len, bwtintv_t* mem_output, int* mem_num, double kernel_time[COMPUTE_UNIT]) {
    if (this->is_seq()) {
        ocl_invoke_seq(seq, seq_len, mem_output, mem_num, m_batch_size, kernel_time, m_bwtpara[6]);
    } else {
        ocl_invoke_overlap(seq, seq_len, mem_output, mem_num, m_batch_size, kernel_time, m_bwtpara[6]);
    }

    return 0;
}
