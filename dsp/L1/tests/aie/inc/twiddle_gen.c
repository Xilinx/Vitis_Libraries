/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
* Master program to calculate the FFT twiddle lookup tables used by the AIE
* DSPLib FFT/IFFT and r2comb kernels.
*
* This consolidates what used to be three separate generators (twiddle.c,
* twiddle_32.c, twiddle_r2master.c, all in this same directory - now retired
* stubs that point here) into a single script, and additionally pads every
* generated table so that its allocated size is never smaller than the
* largest AIE vector-load width (512 bits / 64 bytes, on AIE-MLv2). Without
* this padding, aiesimulator's --enable-memory-check flags harmless
* out-of-bounds reads on the small end of the table set (e.g. fft_lut_tw1,
* tw2, tw4) because a fixed-width vector load walks past the end of the
* allocated array. See CR-1274079.
*
* To compile and run this program:
*   cd L1/tests/aie/inc
*   gcc -lm -o twiddle_gen.o twiddle_gen.c
*   ./twiddle_gen.o
*
* the -lm is necessary to include the math library. Must be run from this
* directory since all output paths are relative to it.
*/

#include <math.h>
#include <stdio.h>
#include <inttypes.h>

#define PI 3.14159265358979323846e0
#define DIR -1

// Worst-case AIE vector-load width across all supported variants (AIE-MLv2 = 512 bits).
// Every generated table is padded to at least this many bytes so a full-width
// load can never read past the end of the allocated array.
#define MIN_ACCESS_BYTES 64

static int minElemsFor(int elemBytes) {
    return (MIN_ACCESS_BYTES + elemBytes - 1) / elemBytes;
}

// Standard AMD/Xilinx Apache-2.0 license header, matching every other generated
// and hand-written file in this repo.
static void printLicenseHeader(FILE* fp) {
    fprintf(fp,
            "/*\n"
            " * Copyright (C) 2019-2022, Xilinx, Inc.\n"
            " * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.\n"
            " *\n"
            " * Licensed under the Apache License, Version 2.0 (the \"License\");\n"
            " * you may not use this file except in compliance with the License.\n"
            " * You may obtain a copy of the License at\n"
            " *\n"
            " *     http://www.apache.org/licenses/LICENSE-2.0\n"
            " *\n"
            " * Unless required by applicable law or agreed to in writing, software\n"
            " * distributed under the License is distributed on an \"AS IS\" BASIS,\n"
            " * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
            " * See the License for the specific language governing permissions and\n"
            " * limitations under the License.\n"
            " */\n");
}

/*=====================================================================================
 * Per-entry printers. Each prints `outCount` entries: the first `realCount` are
 * real table values (strided by `step` into the source arrays), the rest are
 * zero-padding to reach `outCount`.
 *===================================================================================*/
static void printInt32Entries(
    FILE* fp, const int32_t* real, const int32_t* imag, int realCount, int step, int outCount) {
    for (int k = 0; k < outCount; k++) {
        if (k < realCount) {
            fprintf(fp, "{%d,%d}", real[k * step], imag[k * step]);
        } else {
            fprintf(fp, "{0,0}");
        }
        if (k < outCount - 1) {
            fprintf(fp, ", ");
        }
        if (k % 8 == 7) {
            fprintf(fp, "\n");
        }
    }
}

static void printInt16Entries(FILE* fp, const short* real, const short* imag, int realCount, int step, int outCount) {
    for (int k = 0; k < outCount; k++) {
        if (k < realCount) {
            fprintf(fp, "{%d,%d}", real[k * step], imag[k * step]);
        } else {
            fprintf(fp, "{0,0}");
        }
        if (k < outCount - 1) {
            fprintf(fp, ", ");
        }
        if (k % 8 == 7) {
            fprintf(fp, "\n");
        }
    }
}

static void printFloatEntries(FILE* fp, const float* real, const float* imag, int realCount, int step, int outCount) {
    for (int k = 0; k < outCount; k++) {
        if (k < realCount) {
            fprintf(fp, "{%.9f,%.9f}", real[k * step], imag[k * step]);
        } else {
            fprintf(fp, "{0.0,0.0}");
        }
        if (k < outCount - 1) {
            fprintf(fp, ", ");
        }
        if (k % 8 == 7) {
            fprintf(fp, "\n");
        }
    }
}

/*=====================================================================================
 * Per-stage table families (fft_lut_tw<N>_<suffix>[...] and, for the fixed-point
 * types, fft_lut_tw<N>_<suffix>_half[...]). `declType` is the C type used in the
 * declaration (e.g. cint32); `suffix` is the name suffix (e.g. cint31, which
 * shares the cint32 storage type but a different fixed-point scale).
 *===================================================================================*/
static void writeInt32Family(
    FILE* fp, const char* declType, const char* suffix, const int32_t* real, const int32_t* imag) {
    const int elemBytes = 8; // cint32 = 2 x int32
    const int minElems = minElemsFor(elemBytes);

    for (int i = 11; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int outCount = ptsize < minElems ? minElems : ptsize;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_tw%d_%s[%d] = {\n", declType, ptsize, suffix,
                outCount);
        printInt32Entries(fp, real, imag, ptsize, step, outCount);
        fprintf(fp, "};\n\n");
    }

    for (int i = 11; i > 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int half = ptsize / 2;
        int outCount = half < minElems ? minElems : half;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_tw%d_%s_half[%d] = {\n", declType, ptsize, suffix,
                outCount);
        printInt32Entries(fp, real, imag, half, step, outCount);
        fprintf(fp, "};\n\n");
    }
}

static void writeInt16Family(FILE* fp, const char* declType, const char* suffix, const short* real, const short* imag) {
    const int elemBytes = 4; // cint16 = 2 x int16
    const int minElems = minElemsFor(elemBytes);

    for (int i = 11; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int outCount = ptsize < minElems ? minElems : ptsize;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_tw%d_%s[%d] = {\n", declType, ptsize, suffix,
                outCount);
        printInt16Entries(fp, real, imag, ptsize, step, outCount);
        fprintf(fp, "};\n\n");
    }

    for (int i = 11; i > 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int half = ptsize / 2;
        int outCount = half < minElems ? minElems : half;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_tw%d_%s_half[%d] = {\n", declType, ptsize, suffix,
                outCount);
        printInt16Entries(fp, real, imag, half, step, outCount);
        fprintf(fp, "};\n\n");
    }
}

// No _half variant for the floating-point families (matches original twiddle_32.c).
static void writeFloatFamily(
    FILE* fp, const char* declType, const char* suffix, const float* real, const float* imag, int elemBytes) {
    const int minElems = minElemsFor(elemBytes);

    for (int i = 11; i >= 0; i--) {
        int ptsize = 1 << i;
        int step = 1 << (11 - i);
        int outCount = ptsize < minElems ? minElems : ptsize;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_tw%d_%s[%d] = {\n", declType, ptsize, suffix,
                outCount);
        printFloatEntries(fp, real, imag, ptsize, step, outCount);
        fprintf(fp, "};\n\n");
    }
}

/*=====================================================================================
 * Radix-4 table families (fft_lut_<suffix>_r4_<N>_<2N>[...]), plus the fixed
 * ptsize==1 special case (fft_lut_<suffix>_r4_1_2[...]) whose two live entries
 * are the exact full-scale +1/-1 endpoints rather than rounded trig values.
 *===================================================================================*/
static void writeInt32R4Special(FILE* fp, const char* declType, const char* suffix, int32_t posOne, int32_t negOne) {
    const int elemBytes = 8;
    const int minElems = minElemsFor(elemBytes);
    fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_%s_r4_1_2[%d] = {\n", declType, suffix, minElems);
    fprintf(fp, "{%d, %d}, {%d,0}", posOne, 0, negOne);
    for (int i = 2; i < minElems; i++) {
        fprintf(fp, ", {0,0}");
    }
    fprintf(fp, "};\n\n");
}

static void writeInt32R4General(
    FILE* fp, const char* declType, const char* suffix, const int32_t* real, const int32_t* imag, int ptSizeFull) {
    const int elemBytes = 8;
    const int minElems = minElemsFor(elemBytes);
    for (int t = 1; t < 11; t++) {
        int ptsize = 1 << t;
        int elements = ptsize < minElems ? minElems : ptsize;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_%s_r4_%d_%d[%d] = {\n", declType, suffix, ptsize,
                ptsize * 2, elements);
        for (int i = 0; i < elements; i++) {
            int index = ptSizeFull * i * 3 / (4 * ptsize);
            if (i >= ptsize) {
                fprintf(fp, "{0, 0}");
            } else {
                fprintf(fp, "{%d, %d}", real[index], imag[index]);
            }
            if (i < elements - 1) {
                fprintf(fp, ", ");
            }
            if (i % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }
}

static void writeInt16R4Special(FILE* fp, const char* declType, const char* suffix, short posOne, short negOne) {
    const int elemBytes = 4;
    const int minElems = minElemsFor(elemBytes);
    fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_%s_r4_1_2[%d] = {\n", declType, suffix, minElems);
    fprintf(fp, "{%d, %d}, {%d,0}", posOne, 0, negOne);
    for (int i = 2; i < minElems; i++) {
        fprintf(fp, ", {0,0}");
    }
    fprintf(fp, "};\n\n");
}

static void writeInt16R4General(
    FILE* fp, const char* declType, const char* suffix, const short* real, const short* imag, int ptSizeFull) {
    const int elemBytes = 4;
    const int minElems = minElemsFor(elemBytes);
    for (int t = 1; t < 11; t++) {
        int ptsize = 1 << t;
        int elements = ptsize < minElems ? minElems : ptsize;
        fprintf(fp, "alignas(__ALIGN_BYTE_SIZE__)  const %s fft_lut_%s_r4_%d_%d[%d] = {\n", declType, suffix, ptsize,
                ptsize * 2, elements);
        for (int i = 0; i < elements; i++) {
            int index = ptSizeFull * i * 3 / (4 * ptsize);
            if (i >= ptsize) {
                fprintf(fp, "{0, 0}");
            } else {
                fprintf(fp, "{%d, %d}", real[index], imag[index]);
            }
            if (i < elements - 1) {
                fprintf(fp, ", ");
            }
            if (i % 8 == 7) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "};\n\n");
    }
}

/*=====================================================================================
 * Main per-stage twiddle tables (fft_twiddle_lut_dit_all.h), the matching
 * reference-model master tables (fft_ifft_dit_twiddle_lut_all.h), and the
 * radix-4 twiddle tables (fft_r4_twiddles_all.h). Formerly twiddle_32.c.
 *===================================================================================*/
static void generateMainAndR4() {
    const int PT_SIZE = 4096;
    int i;
    double theta, temp;
    FILE* fp;
    FILE* reffp;
    FILE* r4fp;
    double reals[PT_SIZE], imags[PT_SIZE];
    float realf[PT_SIZE], imagf[PT_SIZE];
    int32_t real32[PT_SIZE], imag32[PT_SIZE];
    int32_t real31[PT_SIZE], imag31[PT_SIZE];
    short real16[PT_SIZE], imag16[PT_SIZE];
    short real15[PT_SIZE], imag15[PT_SIZE];

    fp = fopen("../../../include/aie/fft_twiddle_lut_dit_all.h", "w");
    reffp = fopen("fft_ifft_dit_twiddle_lut_all.h", "w");
    r4fp = fopen("../../../include/aie/fft_r4_twiddles_all.h", "w");
    printLicenseHeader(fp);
    printLicenseHeader(reffp);
    printLicenseHeader(r4fp);

    // create a twiddle table of a full circle as an array of doubles
    for (i = 0; i < PT_SIZE; i++) {
        theta = (double)i * 2.0 * PI / (double)PT_SIZE;
        reals[i] = cos(theta);
        imags[i] = sin(theta) * DIR;
    }

    // value creation and ref model cint32 table
    fprintf(reffp, "#ifndef __FFT_TWIDDLE_LUT_DIT_ALL_H__\n#define __FFT_TWIDDLE_LUT_DIT_ALL_H__\n\n");
    fprintf(reffp, "#include \"device_defs.h\" //for _SUPPORTS_CBFLOAT_\n");
    fprintf(reffp, "#include \"fft_com_inc.h\"\n");
    fprintf(reffp, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using L1/tests/aie/inc/twiddle_gen.c\n\n");
    for (i = 0; i < PT_SIZE; i++) {
        int realint, imagint;
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
        real16[i] = (short)realint;
        imag16[i] = (short)imagint;

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
        real15[i] = (short)(int)temp;
        temp = round(imags[i] * 16384.0);
        imag15[i] = (short)(int)temp;
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
    fprintf(reffp, "#ifdef _SUPPORTS_CBFLOAT16_\n\n");
    fprintf(reffp, "const cbfloat16 twiddle_master_cbfloat16[%d] = {\n", PT_SIZE / 2);
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
    fprintf(reffp, "#endif //_SUPPORTS_CBFLOAT16_\n\n");
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

    // Per-stage twiddle tables, one array per FFT stage size, padded so that no
    // array is smaller than MIN_ACCESS_BYTES. No include guard/include line here:
    // this file is included directly, exactly once, by the kernel that already
    // has the relevant types and __ALIGN_BYTE_SIZE__ in scope (matches prior
    // behaviour of twiddle_32.c).
    writeInt32Family(fp, "cint32", "cint32", real32, imag32);
    writeInt32Family(fp, "cint32", "cint31", real31, imag31);
    writeInt16Family(fp, "cint16", "cint16", real16, imag16);
    writeInt16Family(fp, "cint16", "cint15", real15, imag15);
    writeFloatFamily(fp, "cfloat", "cfloat", realf, imagf, 8);
    fprintf(fp, "//--------cbfloat16 tables\n#ifdef _SUPPORTS_CBFLOAT16_\n");
    writeFloatFamily(fp, "cbfloat16", "cbfloat16", realf, imagf, 4);
    fprintf(fp, "#endif //_SUPPORTS_CBFLOAT16_\n");

    // R4 twiddle creation
    fprintf(r4fp, "#ifndef __FFT_R4_TWIDDLE_LUT_DIT_ALL_H__\n#define __FFT_R4_TWIDDLE_LUT_DIT_ALL_H__\n\n");
    fprintf(r4fp, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using L1/tests/aie/inc/twiddle_gen.c\n\n");

    writeInt32R4Special(r4fp, "cint32", "cint32", 2147483647, (int32_t)(-2147483648LL));
    writeInt32R4General(r4fp, "cint32", "cint32", real32, imag32, PT_SIZE);

    writeInt32R4Special(r4fp, "cint32", "cint31", 1073741824, -1073741824);
    writeInt32R4General(r4fp, "cint32", "cint31", real31, imag31, PT_SIZE);

    writeInt16R4Special(r4fp, "cint16", "cint16", 32767, -32768);
    writeInt16R4General(r4fp, "cint16", "cint16", real16, imag16, PT_SIZE);

    writeInt16R4Special(r4fp, "cint16", "cint15", 16384, -16384);
    writeInt16R4General(r4fp, "cint16", "cint15", real15, imag15, PT_SIZE);

    fprintf(r4fp, "#endif //__FFT_R4_TWIDDLE_LUT_DIT_ALL_H__\n");

    fprintf(reffp, "#endif //__FFT_TWIDDLE_LUT_DIT_ALL_H__\n");
    fclose(fp);
    fclose(reffp);
    fclose(r4fp);
}

/*=====================================================================================
 * r2comb (radix-2 combiner) master twiddle tables (fft_r2comb_twiddle_lut_all.hpp).
 * Formerly twiddle_r2master.c. These are single large (32768-entry) master
 * tables indexed into by the r2comb kernel, always far larger than
 * MIN_ACCESS_BYTES, so no padding is required here.
 *===================================================================================*/
static void generateR2Comb() {
    const int PT_SIZE = 65536;
    int i;
    double theta, temp;
    FILE* r2fp;
    double reals[PT_SIZE], imags[PT_SIZE];
    float realf[PT_SIZE], imagf[PT_SIZE];
    int32_t real32[PT_SIZE], imag32[PT_SIZE];
    int32_t real31[PT_SIZE], imag31[PT_SIZE];
    short real16[PT_SIZE], imag16[PT_SIZE];
    short real15[PT_SIZE], imag15[PT_SIZE];

    r2fp = fopen("../../../include/aie/fft_r2comb_twiddle_lut_all.hpp", "w");
    printLicenseHeader(r2fp);

    // create a twiddle table of a full circle as an array of doubles
    for (i = 0; i < PT_SIZE; i++) {
        theta = (double)i * 2.0 * PI / (double)PT_SIZE;
        reals[i] = cos(theta);
        imags[i] = sin(theta) * DIR;
    }

    fprintf(r2fp, "#ifndef __FFT_R2COMB_TWIDDLE_LUT_ALL_H__\n#define __FFT_R2COMB_TWIDDLE_LUT_ALL_H__\n\n");
    fprintf(r2fp, "// DO NOT HAND EDIT THIS FILE. IT WAS CREATED using L1/tests/aie/inc/twiddle_gen.c\n\n");
    fprintf(r2fp,
            "#include \"device_defs.h\"\n#ifndef INLINE_DECL\n#define INLINE_DECL inline "
            "__attribute__((always_inline))\n#endif\n#ifndef "
            "NOINLINE_DECL\n#define NOINLINE_DECL inline __attribute__((noinline))\n#endif\n\n//The values in this "
            "file were created using twiddle_gen.c.\n\nstatic constexpr int kR2MasterTableSize = "
            "32768;\n//The values in this file were created using twiddle_gen.c\n");
    for (i = 0; i < PT_SIZE; i++) {
        int realint, imagint;
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
        real16[i] = (short)realint;
        imag16[i] = (short)imagint;
        // create 31 bit twiddle master table entries
        temp = round(reals[i] * 1073741824.0);
        real31[i] = (int)temp;
        temp = round(imags[i] * 1073741824.0);
        imag31[i] = (int)temp;
        // create 15 bit twiddle master table entries
        temp = round(reals[i] * 16384.0);
        real15[i] = (short)(int)temp;
        temp = round(imags[i] * 16384.0);
        imag15[i] = (short)(int)temp;

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
    fprintf(r2fp, "/*\n#ifdef _SUPPORTS_CBFLOAT16_\n");
    fprintf(r2fp, "static constexpr cbfloat16 r2_twiddle_master_cbfloat16[kR2MasterTableSize] = {\n");
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
    fprintf(r2fp, "#endif // _SUPPORTS_CBFLOAT16_\n*/\n");
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
    fprintf(r2fp, "/*\n#ifdef _SUPPORTS_CBFLOAT16_\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cbfloat16* fnGetR2TwiddleMasterBase<cbfloat16,0>(){\n    return "
            "(cbfloat16*)r2_twiddle_master_cbfloat16;\n};\n");
    fprintf(r2fp,
            "template<>\nINLINE_DECL constexpr cbfloat16* fnGetR2TwiddleMasterBase<cbfloat16,1>(){\n    return "
            "(cbfloat16*)r2_twiddle_master_cbfloat16;\n};\n");
    fprintf(r2fp, "#endif // _SUPPORTS_CBFLOAT16_\n*/\n");
    fprintf(r2fp, "#endif //__FFT_R2COMB_TWIDDLE_LUT_ALL_H__\n");

    fclose(r2fp);
}

int main() {
    generateMainAndR4();
    generateR2Comb();
    return 0;
}
