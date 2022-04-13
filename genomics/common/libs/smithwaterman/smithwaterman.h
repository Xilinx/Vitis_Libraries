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
#include <assert.h>
#ifndef SWAPP_H_
#define SWAPP_H_

#include "xcl2.hpp"
#include "matcharray.h"

#define COMPUTE_UNITS 1

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef char i8;
typedef short i16;
typedef int i32;

class SmithWatermanApp {
   public:
    SmithWatermanApp(const string& vendor_name,
                     const string& device_name,
                     int selected_device,
                     const string& strKernelFP,
                     const string& strSampleFPi,
                     const string& binaryFile,
                     const int numBlocks,
                     const int blkSz,
                     const bool doubleBuffered,
                     const bool verifyMode,
                     MatchArray* pm);
    virtual ~SmithWatermanApp();

    enum EvBreakDown { evtHostWrite = 0, evtKernelExec = 1, evtHostRead = 2, evtCount = 3 };

    bool run(int idevice, int nruns);

    bool invoke_kernel(unsigned int* input,
                       unsigned int* output,
                       int* iterNum,
                       int sz_input,
                       int sz_output,
                       int sz_sz,
                       cl::Event events[evtCount],
                       double eTotal[evtCount]);
    bool invoke_kernel_blocking(unsigned int* input,
                                unsigned int* output,
                                int* iterNum,
                                int sz_input,
                                int sz_output,
                                int sz_sz,
                                cl::Event events[evtCount],
                                double eTotal[evtCount]);
    bool invoke_kernel_doublebuffered(unsigned int* input,
                                      unsigned int* output,
                                      int* iterNum,
                                      int sz_input,
                                      int sz_output,
                                      int sz_sz,
                                      cl::Event events[evtCount],
                                      double eTotal[evtCount]);

    static bool unit_test_kernel_cpu();
    static bool unit_test_naive();

   private:
    string m_strSampleFP;
    char* fileBuf;
    bool m_useDoubleBuffered;
    int m_numSamples;
    int m_numBlocks;
    int m_blockSz;
    bool m_verifyMode; // true == verify, false is not verify
    // bool m_writeMatchArray; // true == writeMatchArray
    cl::Program m_program;
    cl::Kernel m_clKernelSmithWaterman;
    cl::CommandQueue q;
    cl::Context context;

    MatchArray* m_pMatchInfo;
};

#endif /* SWAPP_H_ */
