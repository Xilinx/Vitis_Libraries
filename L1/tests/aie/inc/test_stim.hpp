#ifndef _DSPLIB_TEST_STIM_HPP_
#define _DSPLIB_TEST_STIM_HPP_

/*
Testbench stimulus class.
This file contains definition of the class used to generate stimulus data
for  reference model and UUT.
*/

#include <adf.h>
#include <stdio.h> /* printf, scanf, puts, NULL */
#include <string>
//#include <stdlib.h>  //clashes with utils.h definition of srand
#include "utils.hpp"

#define FILENAME_MAX_LEN 255

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
// Template class, one class fits all cases. Generate data, based on template parameters.
template <typename TT_DATA,  // type of data output
          size_t TP_SAMPLES, // number of samples to generate
          size_t TP_OFFSET   // number of empty samples to generate
          >
class test_stim {
   public:
    // Constructor
    test_stim(std::string filename = std::string("data/input.txt")) {
        std::strcpy(m_fileName, filename.c_str());
        m_fileReady = true;
    }

    // Main operation function
    int gen(T_STIM_MODE mode, TT_DATA* d_out, bool symmetry = false) {
        int ret = RETURN_SUCCESS;
        size_t samples = TP_SAMPLES;
        if (symmetry) {
            samples = (TP_SAMPLES + 1) / 2;
        }
        // Based on mode, generate an array of TT_DATA elements
        switch (mode) {
            case STIM_GEN_SINE:
                sineData(samples, TP_OFFSET, d_out);
                break;
            case STIM_GEN_SYMINCONES:
                symIncOnesData(samples, TP_OFFSET, d_out);
                break;
            case STIM_GEN_INCONES:
                inconesData(samples, TP_OFFSET, d_out);
                break;
            case STIM_GEN_ALLONES:
                allonesData(samples, TP_OFFSET, d_out);
                break;
            case STIM_GEN_IMPULSE:
                impulseData(samples, TP_OFFSET, d_out);
                break;
            case STIM_GEN_PRBS:
                printf("Unsupported mode \n");
                ret = RETURN_ERROR;
                break;
            case STIM_GEN_FROM_FILE:
                if (m_fileReady) {
                    ret = read_array(samples, m_fileName, TP_OFFSET, d_out);
                } else {
                    printf("read file not ready\n");
                    ret = RETURN_ERROR;
                }
                break;
            case STIM_GEN_RANDOM:
                srand(m_seed);
                randData(samples, TP_OFFSET, d_out);
                break;
            default:
                printf("Unknown stimulus mode\n");
                ret = RETURN_ERROR;
                break;
        }
        // mirror
        /*
        if (symmetry) {
            for (int i=1;i<(TP_SAMPLES+1)/2;i++) {
                d_out[TP_SAMPLES-i] = d_out[i-1];
            }
        }
        */
        return ret;
    }

    // Clear file
    void prepFile(char* filename) {
        strcpy(m_fileName, filename);
        m_fileReady = true;
    }
    // Set seed
    inline void prepSeed(int seed) { m_seed = seed; }

   private:
    int m_seed;
    bool m_fileReady = false;
    char m_fileName[FILENAME_MAX_LEN]; // max filename size
};

template <typename T_D>
inline T_D addError(T_D inVal) {
    T_D retVal;
    // Increase input by arbitrary amount
    retVal = inVal * 6 / 11;
    return retVal;
}

template <>
inline cint16 addError<cint16>(cint16 inVal) {
    cint16 retVal;
    // Increase input by arbitrary amount
    retVal.real = inVal.real * 6 / 11;
    retVal.imag = inVal.imag * 13 / 27;
    return retVal;
}

template <>
inline cint32 addError<cint32>(cint32 inVal) {
    cint32 retVal;
    // Increase input by arbitrary amount
    retVal.real = inVal.real * 6 / 11;
    retVal.imag = inVal.imag * 13 / 27;
    return retVal;
}

template <>
inline cfloat addError<cfloat>(cfloat inVal) {
    cfloat retVal;
    // Increase input by arbitrary amount
    retVal.real = inVal.real * 6 / 11;
    retVal.imag = inVal.imag * 13 / 27;
    return retVal;
}
}
}
}
}
#endif // _DSPLIB_TEST_STIM_HPP_

/*  (c) Copyright 2019 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
