/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
* gcc -lm -o twiddle_32.o twiddle_32.c

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
    FILE* reffp;
    FILE* r4fp;
    short realshort, imagshort;
    int realint, imagint;
    double realdouble[PT_SIZE / 2], imagdouble[PT_SIZE / 2];
    int mag = 31; // power of 2 magnitude of 32 bit twiddle vals.
    double maxint = pow(2, mag);

    fp = fopen("../../../include/aie/fft_twiddle_lut_dit_all.h", "w");
    reffp = fopen("fft_ifft_dit_twiddle_lut_all.h", "w");
    r4fp = fopen("../../../include/aie/fft_r4_twiddles_all.h", "w");
    double reals[PT_SIZE], imags[PT_SIZE];
    double realf[PT_SIZE], imagf[PT_SIZE];
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
    fprintf(reffp, "#ifndef __FFT_TWIDDLE_LUT_DIT_ALL_H__\n#define __FFT_TWIDDLE_LUT_DIT_ALL_H__\n\n");
    fprintf(reffp, "#include \"fft_com_inc.h\"\n");
    fprintf(reffp, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using ../tests/inc/twiddle_32.c\n\n");
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

        // create cint16 table
        temp = round(reals[i] * 32768.0);
        realint = (int)temp;
        if (temp >= 32767.0) {
            realint = 32767;
        }
        temp = round(imags[i] * 32768.0);
        imagint = (int)temp;
        if (temp >= 32767.0) {
            imagint = 32767;
        }
        real16[i] = realint;
        imag16[i] = imagint;

        // create cfloat table
        realf[i] = (float)reals[i];
        imagf[i] = (float)imags[i];

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
    }
    fprintf(reffp, "const cfloat twiddle_master_cfloat[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(reffp, "{%f, %f}", realf[i], imagf[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(reffp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(reffp, "\n\n");
        }
    }
    fprintf(reffp, "};\n\n");
    fprintf(reffp, "const cint32 twiddle_master_cint32[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(reffp, "{%d, %d}", real32[i], imag32[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(reffp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(reffp, "\n\n");
        }
    }
    fprintf(reffp, "};\n\n");
    fprintf(reffp, "const cint32 twiddle_master_cint31[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(reffp, "{%d, %d}", real31[i], imag31[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(reffp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(reffp, "\n\n");
        }
    }
    fprintf(reffp, "};\n");
    fprintf(reffp, "const cint16 twiddle_master_cint16[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(reffp, "{%d, %d}", real16[i], imag16[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(reffp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(reffp, "\n");
        }
    }
    fprintf(reffp, "};\n");
    fprintf(reffp, "const cint16 twiddle_master_cint15[%d] = {\n", PT_SIZE / 2);
    for (i = 0; i < PT_SIZE / 2; i++) {
        fprintf(reffp, "{%d, %d}", real15[i], imag15[i]);
        if (i < PT_SIZE / 2 - 1) {
            fprintf(reffp, ", ");
        }
        if (i % 8 == 7) {
            fprintf(reffp, "\n");
        }
    }
    fprintf(reffp, "};\n\n");

    // cint32
    for (i = 11 /*2048*/; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint32 fft_lut_tw%d_cint32[%d] = {\n", ptsize, ptsize);
        for (k = 0; k < ptsize; k++) {
            fprintf(fp, "{%d,%d}", real32[k * step], imag32[k * step]);
            if (k < ptsize - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint32 half
    for (i = 11 /*2048*/; i > 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint32 fft_lut_tw%d_cint32_half[%d] = {\n", ptsize, ptsize / 2);
        for (k = 0; k < ptsize / 2; k++) {
            fprintf(fp, "{%d,%d}", real32[k * step], imag32[k * step]);
            if (k < ptsize / 2 - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint31
    for (i = 11 /*2048*/; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint32 fft_lut_tw%d_cint31[%d] = {\n", ptsize, ptsize);
        for (k = 0; k < ptsize; k++) {
            fprintf(fp, "{%d,%d}", real31[k * step], imag31[k * step]);
            if (k < ptsize - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint31 half
    for (i = 11 /*2048*/; i > 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint32 fft_lut_tw%d_cint31_half[%d] = {\n", ptsize, ptsize / 2);
        for (k = 0; k < ptsize / 2; k++) {
            fprintf(fp, "{%d,%d}", real31[k * step], imag31[k * step]);
            if (k < ptsize / 2 - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint16
    for (i = 11 /*2048*/; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint16 fft_lut_tw%d_cint16[%d] = {\n", ptsize, ptsize);
        for (k = 0; k < ptsize; k++) {
            fprintf(fp, "{%d,%d}", real16[k * step], imag16[k * step]);
            if (k < ptsize - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint16 half
    for (i = 11 /*2048*/; i > 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint16 fft_lut_tw%d_cint16_half[%d] = {\n", ptsize, ptsize / 2);
        for (k = 0; k < ptsize / 2; k++) {
            fprintf(fp, "{%d,%d}", real16[k * step], imag16[k * step]);
            if (k < ptsize / 2 - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint15
    for (i = 11 /*2048*/; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint16 fft_lut_tw%d_cint15[%d] = {\n", ptsize, ptsize);
        for (k = 0; k < ptsize; k++) {
            fprintf(fp, "{%d,%d}", real15[k * step], imag15[k * step]);
            if (k < ptsize - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cint15 half
    for (i = 11 /*2048*/; i > 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cint16 fft_lut_tw%d_cint15_half[%d] = {\n", ptsize, ptsize / 2);
        for (k = 0; k < ptsize / 2; k++) {
            fprintf(fp, "{%d,%d}", real15[k * step], imag15[k * step]);
            if (k < ptsize / 2 - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // cfloat
    for (i = 11 /*2048*/; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int entries = 0;
        fprintf(fp, "alignas(32) const cfloat fft_lut_tw%d_cfloat[%d] = {\n", ptsize, ptsize);
        for (k = 0; k < ptsize; k++) {
            fprintf(fp, "{%.9f,%.9f}", realf[k * step], imagf[k * step]);
            if (k < ptsize - 1) {
                fprintf(fp, ", ");
            }
            if (k % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }

    // R4 twiddle creation
    fprintf(r4fp, "#ifndef __FFT_R4_TWIDDLE_LUT_DIT_ALL_H__\n#define __FFT_R4_TWIDDLE_LUT_DIT_ALL_H__\n\n");
    fprintf(r4fp, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using ../tests/inc/twiddle_32.c\n\n");

    // cint32 entries
    fprintf(r4fp, "alignas(32) const cint32 fft_lut_cint32_r4_1_2[8] = {\n");
    fprintf(r4fp, "{2147483647, 0}, {-2147483648,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};\n\n");
    for (int t = 1; t < 11; t++) {
        int ptsize = 1 << t;
        int elements = ptsize < 8 ? 8 : ptsize;
        fprintf(r4fp, "alignas(32) const cint32 fft_lut_cint32_r4_%d_%d[%d] = {\n", ptsize, ptsize * 2, elements);
        for (i = 0; i < elements; i++) {
            int index = PT_SIZE * i * 3 / (4 * ptsize);
            if (i >= ptsize) {
                fprintf(r4fp, "{0, 0}");
            } else {
                fprintf(r4fp, "{%d, %d}", real32[index], imag32[index]);
            }
            if (i < elements - 1) {
                fprintf(r4fp, ", ");
            }
            if (i % 8 == 7) {
                fprintf(r4fp, "\n");
            }
        }
        fprintf(r4fp, "};\n\n");
    }

    // cint31 entries
    fprintf(r4fp, "alignas(32) const cint32 fft_lut_cint31_r4_1_2[8] = {\n");
    fprintf(r4fp, "{1073741824, 0}, {-1073741824,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};\n\n");
    for (int t = 1; t < 11; t++) {
        int ptsize = 1 << t;
        int elements = ptsize < 8 ? 8 : ptsize;
        fprintf(r4fp, "alignas(32) const cint32 fft_lut_cint31_r4_%d_%d[%d] = {\n", ptsize, ptsize * 2, elements);
        for (i = 0; i < elements; i++) {
            int index = PT_SIZE * i * 3 / (4 * ptsize);
            if (i >= ptsize) {
                fprintf(r4fp, "{0, 0}");
            } else {
                fprintf(r4fp, "{%d, %d}", real31[index], imag31[index]);
            }
            if (i < elements - 1) {
                fprintf(r4fp, ", ");
            }
            if (i % 8 == 7) {
                fprintf(r4fp, "\n");
            }
        }
        fprintf(r4fp, "};\n\n");
    }

    // cint16 entries
    fprintf(r4fp, "alignas(32) const cint16 fft_lut_cint16_r4_1_2[8] = {\n");
    fprintf(r4fp, "{32767, 0}, {-32768,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};\n\n");
    for (int t = 1; t < 11; t++) {
        int ptsize = 1 << t;
        int elements = ptsize < 8 ? 8 : ptsize;
        fprintf(r4fp, "alignas(32) const cint16 fft_lut_cint16_r4_%d_%d[%d] = {\n", ptsize, ptsize * 2, elements);
        for (i = 0; i < elements; i++) {
            int index = PT_SIZE * i * 3 / (4 * ptsize);
            if (i >= ptsize) {
                fprintf(r4fp, "{0, 0}");
            } else {
                fprintf(r4fp, "{%d, %d}", real16[index], imag16[index]);
            }
            if (i < elements - 1) {
                fprintf(r4fp, ", ");
            }
            if (i % 8 == 7) {
                fprintf(r4fp, "\n");
            }
        }
        fprintf(r4fp, "};\n\n");
    }

    // cint15 entries
    fprintf(r4fp, "alignas(32) const cint16 fft_lut_cint15_r4_1_2[8] = {\n");
    fprintf(r4fp, "{16384, 0}, {-16384,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};\n\n");
    for (int t = 1; t < 11; t++) {
        int ptsize = 1 << t;
        int elements = ptsize < 8 ? 8 : ptsize;
        fprintf(r4fp, "alignas(32) const cint16 fft_lut_cint15_r4_%d_%d[%d] = {\n", ptsize, ptsize * 2, elements);
        for (i = 0; i < elements; i++) {
            int index = PT_SIZE * i * 3 / (4 * ptsize);
            if (i >= ptsize) {
                fprintf(r4fp, "{0, 0}");
            } else {
                fprintf(r4fp, "{%d, %d}", real15[index], imag15[index]);
            }
            if (i < elements - 1) {
                fprintf(r4fp, ", ");
            }
            if (i % 8 == 7) {
                fprintf(r4fp, "\n");
            }
        }
        fprintf(r4fp, "};\n\n");
    }

    fprintf(r4fp, "#endif //__FFT_R4_TWIDDLE_LUT_DIT_ALL_H__\n");

    fprintf(reffp, "#endif //__FFT_TWIDDLE_LUT_DIT_ALL_H__\n");
    fclose(fp);
    fclose(reffp);
    fclose(r4fp);
}
