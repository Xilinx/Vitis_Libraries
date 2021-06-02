#ifndef _DSPLIB_TEST_MON_HPP_
#define _DSPLIB_TEST_MON_HPP_

/*
Testbench monitor class.
This file contains definition of the class used to compare the output results
from reference model and UUT.
*/
#include <adf.h>
#include <string.h>
#include "utils.hpp"

#define MAX_FILENAME_SIZE 255

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
// Monitor class - takes 2 inputs and displays the comparison results.
template <typename TT_DATA, // type of data input and output
          size_t TP_SAMPLES // number of samples
          >
class test_mon {
   public:
    // Constructor
    test_mon();

    // Main operation function
    // This is the default variant of the templatized filter function. For all legal combinations, the specialised
    // variants will be called, so this
    // variant need only call out that the default is invalid
    void compare(TT_DATA* d_in, TT_DATA* r_in) { printf("Class type not supported"); }
};

// Now use partial template specialization to describe the main function for particular data types
// template class for int16 data
template <size_t TP_SAMPLES>
class test_mon<int16_t, TP_SAMPLES> {
   public:
    // arguments to the operator function are run-time variables
    unsigned int compare(const int16_t* d_in, const int16_t* r_in, const int debug_lvl

                         ) {
        unsigned int errors = 0;
        // Compare each input sample with reference output
        for (unsigned int i = 0; i < TP_SAMPLES; i++) {
            if (!isSame(d_in[i], r_in[i])) {
                // error
                errors++;
                printf("ERROR=%4d, SAMPLE=%d, GOT=(%d), REF=(%d)\n", errors, i, d_in[i], r_in[i]);
            } else {
                // no error
                if (debug_lvl >= DEBUG_LVL_NOTE) {
                    printf("PASS,       SAMPLE=%d, GOT=(%d), REF=(%d)\n", i, d_in[i], r_in[i]);
                }
            }
        }

        // Write output samples to an output file
        if (m_fileReady) {
            FILE* pFile;
            int real = 0;

            pFile = fopen(m_fileName, "a");
            for (int i = 0; i < TP_SAMPLES; i++) {
                fprintf(pFile, "%d, %d\n", d_in[i], r_in[i]);
            }
            fclose(pFile);
            pFile = NULL;
        }
        return errors;
    }
    // Clear file
    void prepFile(char* filename) {
        FILE* pFile;

        pFile = fopen(filename, "w");
        if (pFile != NULL) {
            strcpy(m_fileName, filename);
            m_fileReady = true;
            fclose(pFile);
            pFile = NULL;
        }
    }

   private:
    bool m_fileReady = false;
    char m_fileName[MAX_FILENAME_SIZE]; // max filename size
};

// template class for cint16 data
template <size_t TP_SAMPLES>
class test_mon<cint16_t, TP_SAMPLES> {
   public:
    unsigned int compare(const cint16_t* d_in, const cint16_t* r_in, const int debug_lvl) {
        unsigned int errors = 0;
        // Compare each input sample with reference output
        for (unsigned int i = 0; i < TP_SAMPLES; i++) {
            if (!isSame(d_in[i], r_in[i])) {
                // error
                errors++;
                printf("ERROR=%d,SAMPLE=%d, GOT=(%d,%d), REF=(%d,%d)\n", errors, i, d_in[i].real, d_in[i].imag,
                       r_in[i].real, r_in[i].imag);
            } else {
                // no error
                if (debug_lvl >= DEBUG_LVL_NOTE) {
                    printf("PASS,SAMPLE=%d, GOT=(%d,%d), REF=(%d,%d)\n", i, d_in[i].real, d_in[i].imag, r_in[i].real,
                           r_in[i].imag);
                }
            }
        }
        // Write output samples to an output file
        if (m_fileReady) {
            FILE* pFile;
            int real, imag;
            pFile = fopen(m_fileName, "a");
            for (int i = 0; i < (TP_SAMPLES); i++) {
                fprintf(pFile, "%d %d, %d %d \n", d_in[i].real, d_in[i].imag, r_in[i].real, r_in[i].imag);
            }
            fclose(pFile);
            pFile = NULL;
        }
        return errors;
    }
    // Clear file
    void prepFile(char* filename) {
        FILE* pFile;

        pFile = fopen(filename, "w");
        if (pFile != NULL) {
            strcpy(m_fileName, filename);
            m_fileReady = true;
            fclose(pFile);
            pFile = NULL;
        }
    }

   private:
    bool m_fileReady = false;
    char m_fileName[MAX_FILENAME_SIZE]; // max filename size
};

// Now use partial template specialization to describe the main function for particular data types
// template class for int32 data
template <size_t TP_SAMPLES>
class test_mon<int32_t, TP_SAMPLES> {
   public:
    // arguments to the operator function are run-time variables
    unsigned int compare(const int32_t* d_in, const int32_t* r_in, const int debug_lvl) {
        unsigned int errors = 0;
        // Compare each input sample with reference output
        for (unsigned int i = 0; i < TP_SAMPLES; i++) {
            if (!isSame(d_in[i], r_in[i])) {
                // error
                errors++;
                printf("ERROR=%d, SAMPLE=%d, GOT=(%d), REF=(%d)\n", errors, i, d_in[i], r_in[i]);
            } else {
                // no error
                if (debug_lvl >= DEBUG_LVL_NOTE) {
                    printf("PASS, SAMPLE=%d, GOT=(%d), REF=(%d)\n", i, d_in[i], r_in[i]);
                }
            }
        }

        // Write output samples to an output file
        if (m_fileReady) {
            FILE* pFile;
            int real = 0;

            pFile = fopen(m_fileName, "a");
            for (int i = 0; i < TP_SAMPLES; i++) {
                fprintf(pFile, "%d, %d\n", d_in[i], r_in[i]);
            }
            fclose(pFile);
            pFile = NULL;
        }
        return errors;
    }
    // Clear file
    void prepFile(char* filename) {
        FILE* pFile;

        pFile = fopen(filename, "w");
        if (pFile != NULL) {
            strcpy(m_fileName, filename);
            m_fileReady = true;
            fclose(pFile);
            pFile = NULL;
        }
    }

   private:
    bool m_fileReady = false;
    char m_fileName[MAX_FILENAME_SIZE]; // max filename size
};

// template class for cint32 data
template <size_t TP_SAMPLES>
class test_mon<cint32_t, TP_SAMPLES> {
   public:
    unsigned int compare(const cint32_t* d_in, const cint32_t* r_in, const int debug_lvl) {
        unsigned int errors = 0;
        // Compare each input sample with reference output
        for (unsigned int i = 0; i < TP_SAMPLES; i++) {
            if (!isSame(d_in[i], r_in[i])) {
                // error
                errors++;
                printf("ERROR=%d,SAMPLE=%d, GOT=(%d,%d), REF=(%d,%d)\n", errors, i, (int)d_in[i].real,
                       (int)d_in[i].imag, (int)r_in[i].real, (int)r_in[i].imag);
            } else {
                // no error
                if (debug_lvl >= DEBUG_LVL_NOTE) {
                    printf("PASS,SAMPLE=%d, GOT=(%d,%d), REF=(%d,%d)\n", i, d_in[i].real, d_in[i].imag, r_in[i].real,
                           r_in[i].imag);
                }
            }
        }
        // Write output samples to an output file
        if (m_fileReady) {
            FILE* pFile;
            int real, imag;
            pFile = fopen(m_fileName, "a");
            for (int i = 0; i < (TP_SAMPLES); i++) {
                fprintf(pFile, "%d %d, %d %d \n", d_in[i].real, d_in[i].imag, r_in[i].real, r_in[i].imag);
            }
            fclose(pFile);
            pFile = NULL;
        }
        return errors;
    }
    // Clear file
    void prepFile(char* filename) {
        FILE* pFile;

        pFile = fopen(filename, "w");
        if (pFile != NULL) {
            strcpy(m_fileName, filename);
            m_fileReady = true;
            fclose(pFile);
            pFile = NULL;
        }
    }

   private:
    bool m_fileReady = false;
    char m_fileName[MAX_FILENAME_SIZE]; // max filename size
};

// template class for float data
template <size_t TP_SAMPLES>
class test_mon<float, TP_SAMPLES> {
   public:
    // arguments to the operator function are run-time variables
    unsigned int compare(const float* d_in, const float* r_in, const int debug_lvl) {
        unsigned int errors = 0;
        // Compare each input sample with reference output
        for (unsigned int i = 0; i < TP_SAMPLES; i++) {
            if (!isSame(d_in[i], r_in[i])) {
                // error
                errors++;
                printf("ERROR=%d, SAMPLE=%d, GOT=(%f), REF=(%f)\n", errors, i, d_in[i], r_in[i]);
            } else {
                // no error
                if (debug_lvl >= DEBUG_LVL_NOTE) {
                    printf("PASS, SAMPLE=%d, GOT=(%f), REF=(%f)\n", i, d_in[i], r_in[i]);
                }
            }
        }

        // Write output samples to an output file
        if (m_fileReady) {
            FILE* pFile;
            int real = 0;

            pFile = fopen(m_fileName, "a");
            for (int i = 0; i < TP_SAMPLES; i++) {
                fprintf(pFile, "%f, %f\n", d_in[i], r_in[i]);
            }
            fclose(pFile);
            pFile = NULL;
        }
        return errors;
    }
    // Clear file
    void prepFile(char* filename) {
        FILE* pFile;

        pFile = fopen(filename, "w");
        if (pFile != NULL) {
            strcpy(m_fileName, filename);
            m_fileReady = true;
            fclose(pFile);
            pFile = NULL;
        }
    }

   private:
    bool m_fileReady = false;
    char m_fileName[MAX_FILENAME_SIZE]; // max filename size
};

// template class for cfloat data
template <size_t TP_SAMPLES>
class test_mon<cfloat, TP_SAMPLES> {
   public:
    unsigned int compare(const cfloat* d_in, const cfloat* r_in, const int debug_lvl) {
        unsigned int errors = 0;
        // Compare each input sample with reference output
        for (unsigned int i = 0; i < TP_SAMPLES; i++) {
            if (!isSame(d_in[i], r_in[i])) {
                // error
                errors++;
                printf("ERROR=%d,SAMPLE=%d, GOT=(%f,%f), REF=(%f,%f)\n", errors, i, (float)d_in[i].real,
                       (float)d_in[i].imag, (float)r_in[i].real, (float)r_in[i].imag);
            } else {
                // no error
                if (debug_lvl >= DEBUG_LVL_NOTE) {
                    printf("PASS,SAMPLE=%d, GOT=(%f,%f), REF=(%f,%f)\n", i, d_in[i].real, d_in[i].imag, r_in[i].real,
                           r_in[i].imag);
                }
            }
        }
        // Write output samples to an output file
        if (m_fileReady) {
            FILE* pFile;
            int real, imag;
            pFile = fopen(m_fileName, "a");
            for (int i = 0; i < (TP_SAMPLES); i++) {
                fprintf(pFile, "%f %f, %f %f \n", d_in[i].real, d_in[i].imag, r_in[i].real, r_in[i].imag);
            }
            fclose(pFile);
            pFile = NULL;
        }
        return errors;
    }
    // Clear file
    void prepFile(char* filename) {
        FILE* pFile;

        pFile = fopen(filename, "w");
        if (pFile != NULL) {
            strcpy(m_fileName, filename);
            m_fileReady = true;
            fclose(pFile);
            pFile = NULL;
        }
    }

   private:
    bool m_fileReady = false;
    char m_fileName[MAX_FILENAME_SIZE]; // max filename size
};
}
}
}
}
#endif // _DSPLIB_TEST_MON_HPP_

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
