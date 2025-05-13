/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
/*-----------------------------
* program to calculate a lookup table of twiddle values
*

* To compile this program run
* gcc -lm -o twiddle_r2master.o twiddle_r2master.c

* the -lm is necessary to include the math library
*/

#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#define PI 3.14159265358979323846e0
#define PT_SIZE 65536
#define DIR -1

int main() {
    int i, k;
    int tableSizePower;
    int modulus;
    double theta, temp;
    FILE* r2fp;
    short realshort, imagshort;
    int realint, imagint;
    int mag = 31; // power of 2 magnitude of 32 bit twiddle vals.
    double maxint = pow(2, mag);

    r2fp = fopen("../../../include/aie/fft_r2comb_twiddle_lut_all.hpp", "w");
    double reals[PT_SIZE], imags[PT_SIZE];
    float realf[PT_SIZE], imagf[PT_SIZE];
    int32_t real32[PT_SIZE], imag32[PT_SIZE];
    int32_t real31[PT_SIZE], imag31[PT_SIZE];
    short real16[PT_SIZE], imag16[PT_SIZE];
    short real15[PT_SIZE], imag15[PT_SIZE];

    // create a twiddle table of a full circle as an array of doubles
    for (i = 0; i < PT_SIZE; i++) {
        theta = (double)i * 2.0 * PI / (double)PT_SIZE;
        reals[i] = cos(theta);
        imags[i] = sin(theta) * DIR;
    }

    // value creation and ref model cint32 table
    fprintf(r2fp, "#ifndef __FFT_R2COMB_TWIDDLE_LUT_ALL_H__\n#define __FFT_R2COMB_TWIDDLE_LUT_ALL_H__\n\n");
    fprintf(r2fp, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using ../tests/inc/twiddle_r2master.c\n\n");
    fprintf(r2fp,
            "#ifndef INLINE_DECL\n#define INLINE_DECL inline __attribute__((always_inline))\n#endif\n#ifndef "
            "NOINLINE_DECL\n#define NOINLINE_DECL inline __attribute__((noinline))\n#endif\n\n//The values in this "
            "file were created using twiddle.c, then hand copied over.\n\nstatic constexpr int kR2MasterTableSize = "
            "32768;\n//The values in this file were created using twiddle.c\n");
    for (i = 0; i < PT_SIZE; i++) {
        // create cint32 table
        temp = round(reals[i] * 2147483648.0);
        realint = (int)temp;
        if (temp >= 2147483647.0) {
            realint = 2147483647;
        }
        temp = round(imags[i] * 2147483648.0);
        imagint = (int)temp;
        if (temp >= 2147483647.0) {
            imagint = 2147483647;
        }

        real32[i] = realint;
        imag32[i] = imagint;

        // create cint16  table
        temp = round(reals[i] * 32768.0);
        realint = (int)temp;
        if (temp >= 32768.0) {
            realint = 32767;
        }
        temp = round(imags[i] * 32768.0);
        imagint = (int)temp;
        if (temp >= 32768.0) {
            imagint = 32767;
        }

        real16[i] = realint;
        imag16[i] = imagint;
        // create 31 bit twiddle master table entries
        temp = round(reals[i] * 1073741824.0);
        real31[i] = (int)temp;
        temp = round(imags[i] * 1073741824.0);
        imag31[i] = (int)temp;
        // create 15 bit twiddle master table entries
        temp = round(reals[i] * 16384.0);
        real15[i] = (int)temp;
        temp = round(imags[i] * 16384.0);
        imag15[i] = (int)temp;

        // create float table
        realf[i] = (float)reals[i];
        imagf[i] = (float)imags[i];
    }
    fprintf(r2fp, "\n\n");
    fprintf(r2fp, "static constexpr cint16 r2_twiddle_master_cint16[kR2MasterTableSize] = {\n");
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(r2fp, "{%d, %d}", real16[i], imag16[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(r2fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(r2fp, "\n");
        }
    }
    fprintf(r2fp, "};\n\n");
    fprintf(r2fp, "static constexpr cint16 r2_twiddle_master_cint15[kR2MasterTableSize] = {\n");
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(r2fp, "{%d, %d}", real15[i], imag15[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(r2fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(r2fp, "\n");
        }
    }
    fprintf(r2fp, "};\n\n");
    fprintf(r2fp, "static constexpr cint32 r2_twiddle_master_cint32[kR2MasterTableSize] = {\n");
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(r2fp, "{%d, %d}", real32[i], imag32[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(r2fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(r2fp, "\n\n");
        }
    }
    fprintf(r2fp, "};\n\n");
    fprintf(r2fp, "static constexpr cint32 r2_twiddle_master_cint31[kR2MasterTableSize] = {\n");
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(r2fp, "{%d, %d}", real31[i], imag31[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(r2fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(r2fp, "\n\n");
        }
    }
    fprintf(r2fp, "};\n\n");
    fprintf(r2fp, "static constexpr cfloat r2_twiddle_master_cfloat[kR2MasterTableSize] = {\n");
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(r2fp, "{%f, %f}", realf[i], imagf[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(r2fp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(r2fp, "\n\n");
        }
    }
    fprintf(r2fp, "};\n\n");
    fprintf(r2fp,
            "template<typename TT_TWIDDLE, unsigned int TP_TWIDDLE_MODE>\nINLINE_DECL constexpr TT_TWIDDLE* "
            "fnGetR2TwiddleMasterBase(){};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cint16* fnGetR2TwiddleMasterBase<cint16,0>(){\n    return "
            "(cint16*)r2_twiddle_master_cint16;\n};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cint16* fnGetR2TwiddleMasterBase<cint16,1>(){\n    return "
            "(cint16*)r2_twiddle_master_cint15;\n};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cint32* fnGetR2TwiddleMasterBase<cint32,0>(){\n    return "
            "(cint32*)r2_twiddle_master_cint32;\n};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cint32* fnGetR2TwiddleMasterBase<cint32,1>(){\n    return "
            "(cint32*)r2_twiddle_master_cint31;\n};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cfloat* fnGetR2TwiddleMasterBase<cfloat,0>(){\n    return "
            "(cfloat*)r2_twiddle_master_cfloat;\n};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cfloat* fnGetR2TwiddleMasterBase<cfloat,1>(){\n    return "
            "(cfloat*)r2_twiddle_master_cfloat;\n};\n");
    fprintf(r2fp, "#endif //__FFT_R2COMB_TWIDDLE_LUT_ALL_H__\n");

    fclose(r2fp);
}
