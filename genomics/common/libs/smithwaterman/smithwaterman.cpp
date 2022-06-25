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
#include "smithwaterman.h"
#include "logger.h"
#include "sw.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sys/time.h"

// ROUNDS <= 10 valid
typedef struct _eventLog {
    double totalWrite;
    double totalRead;
    double totalExecute;
} eventLog;

unsigned int* generatePackedNReadRefPair(
    int N, int readSize, int refSize, unsigned int** maxVal, int computeOutput = 1);

static double timestamp() {
    double ms = 0.0;
#if defined(__linux__) || defined(linux)
    timeval time;
    gettimeofday(&time, NULL);
    ms = (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
#elif defined(WIN32)
    SYSTEMTIME time;
    GetSystemTime(&time);
    ms = (time.wSeconds * 1000) + time.wMilliseconds;
#endif
    return ms;
}

static double computeEventDurationInMS(const cl::Event& event) {
    cl_ulong ts_start = 0, ts_end = 0;
    cl_int err;
    double duration = 0;
    OCL_CHECK(err, err = event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_START, &ts_start));
    OCL_CHECK(err, err = event.getProfilingInfo<uint64_t>(CL_PROFILING_COMMAND_END, &ts_end));
    duration += (cl_double)(ts_end - ts_start) * (cl_double)(1e-06);

    return duration;
}

static int getToken(FILE* fp, char* tok) {
    int pos = 0;
    char ch;
    while ((ch = (char)(fgetc(fp)))) {
        if (ch == EOF) {
            return 0;
        }
        if (ch == ' ') {
            continue;
        }
        if (ch == ',' || ch == '\n') {
            tok[pos] = '\0';
            return 1;
        }
        tok[pos++] = ch;
    }
    return -1;
}

static int readReadRefFile(char* fname, unsigned int** pairs, unsigned int** maxv, int N) {
    FILE* fp = fopen(fname, "r");
    char* string = new char[1024];
    int rdSz = 0;
    int refSz = 0;
    int sampleNum = 0;
    int numInt = 0;
    int numSamples;
    while ((sampleNum < N) && getToken(fp, string)) {
        if (!strcmp(string, "rdsz")) {
            getToken(fp, string);
            rdSz = atoi(string);
        }
        if (!strcmp(string, "refsz")) {
            getToken(fp, string);
            refSz = atoi(string);
        }
        if (!strcmp(string, "samples")) {
            getToken(fp, string);
            numSamples = atoi(string);
            assert(N <= numSamples);
            printf("Reading %d samples out of %d in the file\n", N, numSamples);
            numInt = READREFUINTSZ(rdSz, refSz);
            *pairs = new unsigned int[N * numInt];
            *maxv = new unsigned int[3 * N];
        }
        if (string[0] == 'S') {
            for (int p = 0; p < numInt; ++p) {
                getToken(fp, string);
                unsigned int val = (unsigned int)atoll(string);
                (*pairs)[sampleNum * numInt + p] = val;
            }
            for (int p = 0; p < 3; ++p) {
                getToken(fp, string);
                unsigned int val = (unsigned int)atoll(string);
                (*maxv)[sampleNum * 3 + p] = val;
            }
            sampleNum++;
        }
    }
    delete[] string;
    fclose(fp);
    return sampleNum;
}

static int verify(int numSample, unsigned int* outputGolden, unsigned int* output) {
    int fail = 0;
    printf("Verifying computed MAXSORE returned to HOST\n");
    for (int i = 0; i < numSample; ++i) {
        int localFail = 0;
        if (outputGolden[3 * i + 2] != output[3 * i + 2]) {
            fail = 1;
            localFail = i + 1;
        }
        if (localFail) {
            printf("Fail %d:", localFail - 1);
            for (int j = 0; j < 3; ++j) {
                printf("g=%u, m=%u, ", outputGolden[3 * i + j], output[3 * i + j]);
            }
        }
    }
    if (fail) {
        printf("Fail\n");
    } else {
        printf("Pass\n");
    }
    return fail;
}

SmithWatermanApp::SmithWatermanApp(const string& vendor_name,
                                   const string& device_name,
                                   int selected_device,
                                   const string& strKernelFP,
                                   const string& strSampleFP,
                                   const string& binaryFile,
                                   const int numBlocks,
                                   const int blkSz,
                                   const bool doubleBuffered,
                                   const bool verifyMode,
                                   MatchArray* pm)

{
    // store path to input bitmap
    m_strSampleFP = strSampleFP;
    m_numSamples = numBlocks * blkSz * NUMPACKED;
    m_useDoubleBuffered = doubleBuffered;
    m_numBlocks = numBlocks;
    m_blockSz = blkSz;
    m_verifyMode = verifyMode;
    m_pMatchInfo = pm;

    cl_int err;
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
    OCL_CHECK(err, std::string dev_name = device.getInfo<CL_DEVICE_NAME>(&err));
    std::cout << "Found Device=" << dev_name.c_str() << std::endl;

    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    OCL_CHECK(err, m_program = cl::Program(context, devices, bins, NULL, &err));
    OCL_CHECK(err, m_clKernelSmithWaterman = cl::Kernel(m_program, "opencl_sw_maxscore", &err));
}

SmithWatermanApp::~SmithWatermanApp() {
    cl_int err;
    OCL_CHECK(err, err = q.flush());
    OCL_CHECK(err, err = q.finish());
}

bool SmithWatermanApp::unit_test_kernel_cpu() {
    LogInfo("Start unit tests for kernels on the CPU");

    LogInfo("End unit tests for kernels on the CPU");

    return true;
}

/*!
 * Unit test for the huffman cpu version
 */
bool SmithWatermanApp::unit_test_naive() {
    return true;
}

bool SmithWatermanApp::invoke_kernel(unsigned int* input,
                                     unsigned int* output,
                                     int* iterNum,
                                     int sz_input,
                                     int sz_output,
                                     int sz_sz,
                                     cl::Event events[evtCount],
                                     double eTotal[evtCount]) {
    if (m_useDoubleBuffered) {
        bool res = invoke_kernel_doublebuffered(input, output, iterNum, sz_input, sz_output, sz_sz, &events[0], eTotal);
        if (!res) {
            LogError("Failed Double Buffered SW. Test Failed");
            return false;
        }
    } else {
        bool res = invoke_kernel_blocking(input, output, iterNum, sz_input, sz_output, sz_sz, &events[0], eTotal);
        if (!res) {
            LogError("Failed Blocked SW. Test Failed");
            return false;
        }
    }

    return true;
}

bool SmithWatermanApp::invoke_kernel_blocking(unsigned int* input,
                                              unsigned int* output,
                                              int* iterNum,
                                              int sz_input,
                                              int sz_output,
                                              int sz_sz,
                                              cl::Event events[evtCount],
                                              double eTotal[evtCount]) {
    cl::Kernel kernel = m_clKernelSmithWaterman;
    cl_int err;
    cl::Buffer mem_input;
    OCL_CHECK(err, mem_input = cl::Buffer(context, CL_MEM_READ_WRITE, sz_input, NULL, &err));
    cl::Buffer mem_output;
    OCL_CHECK(err, mem_output = cl::Buffer(context, CL_MEM_READ_WRITE, sz_output, NULL, &err));

    cl::Buffer mem_sz_sz;
    OCL_CHECK(err, mem_sz_sz = cl::Buffer(context, CL_MEM_READ_WRITE, sz_sz, NULL, &err));

    err = 0;

    OCL_CHECK(err, err = kernel.setArg(0, mem_input));
    OCL_CHECK(err, err = kernel.setArg(1, mem_output));
    OCL_CHECK(err, err = kernel.setArg(2, mem_sz_sz));

    int numIter = m_numBlocks;

    cout << "Processing " << m_numSamples << " Samples \n";
    cout << "HW Block Size: " << m_blockSz * NUMPACKED << "\n";
    cout << "Total Number of blocks: " << m_numBlocks << "\n";
    for (int iter = 0; iter < numIter; ++iter) {
        // copy input dataset to OpenCL buffer
        // cout << "In iteration" << iter << "\n";

        OCL_CHECK(err, err = q.enqueueWriteBuffer(mem_input, CL_TRUE, 0, sz_input,
                                                  (input + iter * (sz_input / sizeof(unsigned int))), NULL,
                                                  &events[evtHostWrite]));
        OCL_CHECK(err, err = q.enqueueWriteBuffer(mem_sz_sz, CL_TRUE, 0, sz_sz, iterNum, NULL, NULL));

        // finish all memory writes
        OCL_CHECK(err, err = q.finish());

        // call once to guarantee that all buffers are migrated to device memory
        OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &events[evtKernelExec]));

        OCL_CHECK(err, err = q.finish());

        // read output size
        OCL_CHECK(err, err = q.enqueueReadBuffer(mem_output, CL_TRUE, 0, sz_output,
                                                 (output + iter * (sz_output / sizeof(unsigned int))), NULL,
                                                 &events[evtHostRead]));

        OCL_CHECK(err, err = q.finish());
        eTotal[evtHostWrite] += computeEventDurationInMS(events[evtHostWrite]);
        eTotal[evtKernelExec] += computeEventDurationInMS(events[evtKernelExec]);
        eTotal[evtHostRead] += computeEventDurationInMS(events[evtHostRead]);
    }
    return true;
}

bool SmithWatermanApp::invoke_kernel_doublebuffered(unsigned int* input,
                                                    unsigned int* output,
                                                    int* iterNum,
                                                    int sz_input,
                                                    int sz_output,
                                                    int sz_sz,
                                                    cl::Event events[evtCount],
                                                    double eTotal[evtCount]) {
    cl::Kernel kernel = m_clKernelSmithWaterman;

    cl_int err;

    cl::Buffer mem_input_ping;
    OCL_CHECK(err, mem_input_ping = cl::Buffer(context, CL_MEM_READ_WRITE, sz_input, NULL, &err));
    cl::Buffer mem_input_pong;
    OCL_CHECK(err, mem_input_pong = cl::Buffer(context, CL_MEM_READ_WRITE, sz_input, NULL, &err));

    cl::Buffer mem_output_ping;
    OCL_CHECK(err, mem_output_ping = cl::Buffer(context, CL_MEM_READ_WRITE, sz_output, NULL, &err));
    cl::Buffer mem_output_pong;
    OCL_CHECK(err, mem_output_pong = cl::Buffer(context, CL_MEM_READ_WRITE, sz_output, NULL, &err));

    cl::Buffer mem_sz_sz;
    OCL_CHECK(err, mem_sz_sz = cl::Buffer(context, CL_MEM_READ_WRITE, sz_sz, NULL, &err));

    OCL_CHECK(err, err = kernel.setArg(2, mem_sz_sz));

    cl::Event ping[3];
    cl::Event pong[3];

    int numIter = m_numBlocks;
    cout << "Processing " << m_numSamples << " Samples \n";
    if (numIter >= 1) {
        OCL_CHECK(err,
                  err = q.enqueueWriteBuffer(mem_input_ping, CL_FALSE, 0, sz_input, input, NULL, &ping[evtHostWrite]));
        OCL_CHECK(err, err = kernel.setArg(0, mem_input_ping));
        OCL_CHECK(err, err = kernel.setArg(1, mem_output_ping));

        if (numIter > 1) {
            OCL_CHECK(err, err = q.enqueueWriteBuffer(mem_input_pong, CL_FALSE, 0, sz_input,
                                                      (input + (sz_input / sizeof(unsigned int))), NULL,
                                                      &pong[evtHostWrite]));
        }
        std::vector<cl::Event> vec_evt1 = {ping[evtHostRead], ping[evtKernelExec]};
        OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &ping[evtKernelExec]));
        OCL_CHECK(err, err = q.enqueueReadBuffer(mem_output_ping, CL_FALSE, 0, sz_output, output, &vec_evt1, NULL));
    }

    if (numIter >= 2) {
        OCL_CHECK(err, err = kernel.setArg(0, mem_input_pong));
        OCL_CHECK(err, err = kernel.setArg(1, mem_output_pong));

        if (numIter > 2) {
            ping[evtHostWrite].wait();
            OCL_CHECK(err, err = q.enqueueWriteBuffer(mem_input_ping, CL_FALSE, 0, sz_input,
                                                      (input + (sz_input / sizeof(unsigned int))), NULL,
                                                      &ping[evtHostWrite]));
        }

        // call once to guarentee that all buffers are migrated to device memory
        OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &pong[evtKernelExec]));

        // read output size
        std::vector<cl::Event> vec_evt2 = {pong[evtHostRead], pong[evtKernelExec]};
        OCL_CHECK(err, err = q.enqueueReadBuffer(mem_output_pong, CL_FALSE, 0, sz_output,
                                                 (output + (sz_output / sizeof(unsigned int))), &vec_evt2, NULL));
    }

    for (int iter = 2; iter < numIter; ++iter) {
        // copy input dataset to OpenCL buffer
        // cout << "In iteration" << iter << "\n";
        if (iter & 1) { // pong
            OCL_CHECK(err, err = kernel.setArg(0, mem_input_pong));
            OCL_CHECK(err, err = kernel.setArg(1, mem_output_pong));

            if (iter < numIter - 1) {
                ping[evtHostWrite].wait();
                OCL_CHECK(err, err = q.enqueueWriteBuffer(mem_input_ping, CL_FALSE, 0, sz_input,
                                                          (input + (iter + 1) * (sz_input / sizeof(unsigned int))),
                                                          NULL, &ping[evtHostWrite]));
            }

            // finish all memory writes
            OCL_CHECK(err, err = q.finish());
            OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &pong[evtKernelExec]));

            // read output size
            OCL_CHECK(err, err = q.finish());
            OCL_CHECK(err, err = q.enqueueReadBuffer(mem_output_pong, CL_FALSE, 0, sz_output,
                                                     (output + iter * (sz_output / sizeof(unsigned int))), NULL,
                                                     &pong[evtHostRead]));
        } else { // ping
            OCL_CHECK(err, err = kernel.setArg(0, mem_input_ping));
            OCL_CHECK(err, err = kernel.setArg(1, mem_output_ping));

            if (iter < numIter - 1) {
                pong[evtHostWrite].wait();
                OCL_CHECK(err, err = q.enqueueWriteBuffer(mem_input_pong, CL_FALSE, 0, sz_input,
                                                          (input + (iter + 1) * (sz_input / sizeof(unsigned int))),
                                                          NULL, &pong[evtHostWrite]));
            }

            // call once to guarentee that all buffers are migrated to device memory
            OCL_CHECK(err, err = q.finish());
            OCL_CHECK(err, err = q.enqueueTask(kernel, NULL, &ping[evtKernelExec]));

            // read output size
            OCL_CHECK(err, err = q.finish());
            OCL_CHECK(err, err = q.enqueueReadBuffer(mem_output_ping, CL_FALSE, 0, sz_output,
                                                     (output + iter * (sz_output / sizeof(unsigned int))), NULL,
                                                     &ping[evtHostRead]));
        }
        eTotal[evtHostWrite] += computeEventDurationInMS(events[evtHostWrite]);
        eTotal[evtKernelExec] += computeEventDurationInMS(events[evtKernelExec]);
        eTotal[evtHostRead] += computeEventDurationInMS(events[evtHostRead]);
    }
    OCL_CHECK(err, err = q.finish());

    return true;
}

bool SmithWatermanApp::run(int idevice, int nruns) {
    if (nruns <= 0) return false;

    assert(unit_test_kernel_cpu());

    int err;
    unsigned int* output;
    unsigned int* outputGolden;
    unsigned int* input = 0;
    int* iterNum;
    int hwBlockSize = NUMPACKED * m_blockSz;
    int totalSamples = m_numSamples;
    cout << "------FPGA Accelerator Summary --------\n";
    cout << "Number of SmithWaterman instances on FPGA:" << NUMPACKED << "\n";
    cout << "Total processing elements:" << MAXPE * NUMPACKED << "\n";
    cout << "Length of reference string:" << MAXCOL << "\n";
    cout << "Length of read(query) string:" << MAXROW << "\n";
    cout << "Read-Ref pair block size(HOST to FPGA):" << m_blockSz << "\n";
    cout << "Verify Mode is:" << m_verifyMode << "\n";
    cout << "---------------------------------------\n";

    // seed random
    srand(time(NULL));

    if (m_verifyMode) {
        cout << "Reading read-ref samples\n";
        err = readReadRefFile((char*)m_strSampleFP.c_str(), &input, &outputGolden, totalSamples);
        if (err != totalSamples) {
            LogError("Unable to read sample file: [%s]", m_strSampleFP.c_str());
            return false;
        }
    } else {
        cout << "Generating read-ref samples\n";
        input = generatePackedNReadRefPair(totalSamples, MAXROW, MAXCOL, &outputGolden,
                                           0); // do not generate compute output
    }

    // input buffer size
    int inSz = sizeof(unsigned int) * (hwBlockSize * PACKEDSZ);
    int outSz = sizeof(unsigned int) * (hwBlockSize * 3);
    int szSz = sizeof(unsigned int);

    output = new unsigned int[totalSamples * 3];
    iterNum = new int;
    *iterNum = m_blockSz;

    // timings
    cl::Event events[evtCount];
    double eTotal[evtCount];
    double durations[evtCount];
    for (int i = 0; i < evtCount; i++) {
        durations[i] = 0.0;
        eTotal[i] = 0.0;
    }

    // start time stamps
    double startMS = timestamp();

    // execute
    for (int i = 0; i < nruns; i++) {
        bool res = invoke_kernel(input, output, iterNum, inSz, outSz, szSz, &events[0], eTotal);
        if (!res) {
            LogError("Failed to encode the input. Test Failed");
            return false;
        }
    }

    // collect times
    for (int i = 0; i < evtCount; i++) {
        durations[i] += computeEventDurationInMS(events[i]);
    }

    double totaltime = timestamp() - startMS;
    // set stats to valid data
    LogInfo("nruns = %u", nruns);
    LogInfo("total [ms] = %.3f", totaltime);
    LogInfo("Host write [ms] = %.3f", eTotal[evtHostWrite]);
    LogInfo("Krnl exec [ms] = %.3f", eTotal[evtKernelExec]);
    LogInfo("Host read [ms] = %.3f", eTotal[evtHostRead]);

    float gcups = (float)(totalSamples / (eTotal[evtKernelExec]));
    gcups = gcups / (1024 * 1024 * 1.024);
    gcups = gcups * MAXROW * MAXCOL;
    cout << "GCups(based on kernel execution time):" << gcups << "\n";
    gcups = (float)(totalSamples / (totaltime));
    gcups = gcups / (1024 * 1024 * 1.024);
    gcups = gcups * MAXROW * MAXCOL;
    cout << "GCups(based on total execution time):" << gcups << "\n";

    // compute transfer rate for host write
    if (eTotal[evtHostWrite] > 0) {
        u32 sz_bytes = inSz * m_numBlocks;

        // bits per second
        double tmp = (sz_bytes * 8.0) / (eTotal[evtHostWrite] / 1000.0);

        // mega-bits per second
        tmp = tmp / (1024.0 * 1024.0);

        LogInfo("Host2Device rate [mbps] = %f", tmp);
    }

    // compute transfer rate for host read
    if (eTotal[evtHostRead] > 0) {
        u32 sz_bytes = outSz * m_numBlocks;

        // bits per second
        double tmp = (sz_bytes * 8.0) / (eTotal[evtHostRead] / 1000.0);

        // mega-bits per second
        tmp = tmp / (1024.0 * 1024.0);
        LogInfo("Device2Host rate [mbps] = %f", tmp);
    }

    if (m_verifyMode) {
        verify(totalSamples, outputGolden, output);
    }

    delete[] input;
    delete[] output;
    delete iterNum;
    return true;
}
