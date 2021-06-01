/*-----------------------------
* program to calculate a lookup table of twiddle values
*

* To compile this program run
* gcc -lm -o twiddle.o twiddle.c

* the -lm is necessary to include the math library
*/

#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#define PI 3.14159265358979323846e0
#define PT_SIZE 4096
#define DIR -1

int main() {
    int i, k;
    int tableSizePower;
    int modulus;
    double theta, temp;
    FILE* fp;
    FILE* fp_uut;
    short realshort, imagshort;
    int realint, imagint;
    float realfloat[PT_SIZE / 2], imagfloat[PT_SIZE / 2];

    fp_uut = fopen("../../include/hw/fft_twiddle_lut_dit_cfloat.h", "w");
    fp = fopen("twiddle_master.h", "w");
    double reals[PT_SIZE / 2], imags[PT_SIZE / 2];

    for (i = 0; i < PT_SIZE / 2; i++) { // one octant, then extrapolate from there.
        theta = (double)i * 2.0 * PI / (double)PT_SIZE;
        reals[i] = cos(theta);
        imags[i] = sin(theta) * DIR;
        /*    //use octant symmetry to get second octant
        reals[PT_SIZE/4-i] = imags[i]*DIR;
        imags[PT_SIZE/4-i] = reals[i]*DIR;
        //use octant symmetry to get third octant
        reals[PT_SIZE/4+i] = -reals[PT_SIZE/4-i];
        imags[PT_SIZE/4+i] = imags[PT_SIZE/4-i];
        //use octant symmetry to get fourth octant
        reals[PT_SIZE/2-i] = -reals[i];
        imags[PT_SIZE/2-i] = imags[i];
        */
    }

    // cshort table
    fprintf(fp, "const cint16 twiddle_master[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        temp = round(reals[i] * 32768.0);
        realshort = (short)temp;
        if (temp >= 32767.0) {
            realshort = 32767;
        }
        temp = round(imags[i] * 32768.0);
        imagshort = (short)temp;
        if (temp >= 32768.0) {
            imagshort = 32767;
        }

        fprintf(fp, "{%d, %d}", realshort, imagshort);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "};\n");

    // cint table
    fprintf(fp, "const cint32 twiddle_master[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        temp = round(reals[i] * 2147483648.0);
        realint = (int)temp;
        if (temp >= 2147483647.0) {
            realint = 2147483647;
        }
        temp = round(imags[i] * 2147483648.0);
        imagint = (int)temp;
        if (temp >= 2147483648.0) {
            imagint = 2147483647;
        }

        fprintf(fp, "{%d, %d}", realint, imagint);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "};\n");

    // cfloat table
    fprintf(fp, "const cfloat twiddle_master[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        realfloat[i] = (float)reals[i];
        imagfloat[i] = (float)imags[i];
        fprintf(fp, "{%.9f, %.9f}", realfloat[i], imagfloat[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "};\n");
    fclose(fp);

    fprintf(fp_uut,
            "#ifndef __FFT_TWIDDLE_LUT_DIT_CFLOAT_H__\n#define __FFT_TWIDDLE_LUT_DIT_CFLOAT_H__\n\n#include "
            "\"fft_com_inc.h\"\n");
    fprintf(fp_uut, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using ../tests/inc/twiddle.c\n\n");

    for (tableSizePower = 11; tableSizePower >= 0; tableSizePower--) {
        k = 0;
        fprintf(fp_uut, "const cfloat chess_storage(%%chess_alignof(v4cint16)) fft_lut_tw%d_cfloat[%d] = {\n",
                (1 << tableSizePower), (1 << tableSizePower));
        for (i = 0; i < PT_SIZE / 2; i += (1 << (11 - tableSizePower))) {
            fprintf(fp_uut, "{%.9f, %.9f}", realfloat[i], imagfloat[i]);
            if (i < PT_SIZE / 2 - (1 << (11 - tableSizePower))) {
                fprintf(fp_uut, ", ");
            }
            if ((++k) == 8) {
                fprintf(fp_uut, "\n");
                k = 0;
            }
        }
        fprintf(fp_uut, "};\n");
    } // for tableSizePower
    fprintf(fp_uut, "#endif //__FFT_TWIDDLE_LUT_DIT_CFLOAT_H__\n");
    fclose(fp_uut);
}
/*  (c) Copyright 2014 - 2020 Xilinx, Inc. All rights reserved.

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
