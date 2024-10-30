/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#ifndef _FOC_HPP_
#define _FOC_HPP_
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ap_fixed.h>

#include "utils.hpp"
#include "pid_control.hpp"
#include "clarke_2p.hpp"
#include "clarke_3p.hpp"
#include "park.hpp"
#include "field_weakening.hpp"
#include <math.h>

#ifndef __SYNTHESIS__
class VarRec {
   public:
    char name[128];
    float max_p;
    float max_n;
    float min_p;
    float min_n;
    void init() {
        max_p = 0;
        max_n = -0;
        min_p = +1.0;
        min_n = -1.0;
    }
    VarRec() {
        strcpy(name, "");
        init();
    }
    void init(char* nm, float v) {
        strcpy(name, nm);
        init();
        set(v);
    }

   public:
    bool set(float v) {
        if (v >= 0) {
            if (v > max_p) max_p = v;
            if (v < min_p) min_p = v;
            return true;
        }
        if (v < 0) {
            if (v < max_n) max_n = v;
            if (v > min_n) min_n = v;
            return true;
        }
        return false;
    }

    bool set(char* nm, float v) {
        if (strlen(name) == 0) {
            init(nm, v);
            return true;
        }
        if (0 != strcmp(nm, name)) return false;
        set(v);
        return true;
    }
    int bits(float v) {
        if (v == 0 || v == -0) return 0;
        bool neg = v < 0;
        if (neg) v = -v;
        bool frc = (v < 1.0);
        if (frc) v = 1.0 / v;
        if (!neg) v *= 1.0000001;
        float b = std::log2(v);
        if (!frc)
            return (int)(b + 1);
        else
            return -1 * (int)(b + 1);
    }
    char* f2s(float v, int len) {
        static int rnd = 0;
        const int mod = 8;
        static char strm[mod][32];
        rnd++;
        rnd %= mod;
        sprintf(strm[rnd], "%15.15f", v);
        strm[rnd][len] = 0;
        return strm[rnd];
    }
    char* f2s(float v, int len, int len_i) {
        static int rnd = 0;
        const int mod = 8;
        static char strm[mod][32];
        rnd++;
        rnd %= mod;
        char tmp[64];
        sprintf(tmp, "%15.15f", v);
        int l_strm = strlen(tmp);
        int pos = 0;
        while (tmp[pos] != '.') pos++;
        for (int i = 0; i < len_i - pos; i++) strm[rnd][i] = ' ';
        strcpy(strm[rnd] + (len_i - pos), tmp);
        strm[rnd][len] = 0;
        return strm[rnd];
    }
    char* f2s(float v) { return f2s(v, 15, 8); }
    void print(FILE* fp) {
        char nm[64];
        strcpy(nm, name);
        while (strlen(nm) < 40) strcat(nm, " ");
        fprintf(fp, "%s MAX[%s, %s],  Integer Bits(%3d, %3d);\t   MIN[%s, %s], Fractional Bits(%3d, %3d)\n", nm,
                f2s(max_n), f2s(max_p), bits(max_n), bits(max_p), f2s(min_n, 12, 3), f2s(min_p, 12, 3), bits(min_n),
                bits(min_p));
    }
};

class RangeTracer {
   public:
    int num_rec;
    VarRec recs[1000];

    RangeTracer() { num_rec = 0; }

    int find(char* nm) {
        for (int i = 0; i < num_rec; i++) {
            if (0 == strcmp(nm, recs[i].name)) return i;
        }
        return num_rec;
    }
    int add(char* nm, float v) {
        recs[num_rec].init(nm, v);
        return ++num_rec;
    }

   public:
    int set(char* nm, float v) {
        bool isSet = false;
        for (int i = 0; i < num_rec; i++) {
            isSet = recs[i].set(nm, v);
            if (isSet) break;
        }
        if (isSet == false) recs[num_rec++].set(nm, v);
        return num_rec;
    }
    void print(FILE* fp) {
        for (int i = 0; i < num_rec; i++) recs[i].print(fp);
    }
};

extern RangeTracer ranger;
#define RANGETRACER(n, v) ranger.set(n, v)
#else
#define RANGETRACER(n, v) ;
#endif

// clang-format off
/// brief Lookup table for the cosine function in the Q16.16 format.
/// CPR(COMM_MACRO_CPR) Number of encoder steps per one full revolution
/// Important: Update this table when the encoder resolution has changed, i.e. when #CPR has changed.
static short sin_table[COMM_MACRO_CPR] = {
    0,      206,    412,    618,    823,    1029,   1235,   1441,   1646,   1852,   2057,   2263,   2468,   2673,
    2879,   3084,   3289,   3493,   3698,   3902,   4107,   4311,   4515,   4719,   4922,   5126,   5329,   5532,
    5735,   5938,   6140,   6342,   6544,   6746,   6947,   7148,   7349,   7549,   7749,   7949,   8149,   8348,
    8547,   8746,   8944,   9142,   9339,   9536,   9733,   9930,   10126,  10321,  10516,  10711,  10905,  11099,
    11293,  11486,  11679,  11871,  12062,  12254,  12444,  12634,  12824,  13013,  13202,  13390,  13578,  13765,
    13952,  14138,  14323,  14508,  14692,  14876,  15059,  15242,  15424,  15605,  15786,  15966,  16145,  16324,
    16502,  16680,  16857,  17033,  17208,  17383,  17557,  17731,  17904,  18076,  18247,  18418,  18588,  18757,
    18925,  19093,  19260,  19426,  19592,  19756,  19920,  20083,  20245,  20407,  20568,  20727,  20886,  21045,
    21202,  21359,  21514,  21669,  21823,  21976,  22129,  22280,  22431,  22580,  22729,  22877,  23024,  23170,
    23315,  23459,  23602,  23745,  23886,  24027,  24166,  24305,  24442,  24579,  24715,  24849,  24983,  25116,
    25247,  25378,  25508,  25637,  25764,  25891,  26017,  26141,  26265,  26388,  26509,  26630,  26749,  26867,
    26985,  27101,  27216,  27330,  27443,  27555,  27666,  27776,  27885,  27992,  28099,  28204,  28308,  28411,
    28513,  28614,  28714,  28813,  28910,  29006,  29102,  29196,  29289,  29380,  29471,  29560,  29648,  29736,
    29821,  29906,  29990,  30072,  30153,  30233,  30312,  30390,  30466,  30541,  30615,  30688,  30759,  30830,
    30899,  30967,  31034,  31099,  31163,  31226,  31288,  31349,  31408,  31466,  31523,  31578,  31633,  31686,
    31738,  31788,  31837,  31886,  31932,  31978,  32022,  32065,  32107,  32147,  32187,  32225,  32261,  32297,
    32331,  32364,  32395,  32425,  32454,  32482,  32509,  32534,  32558,  32580,  32602,  32622,  32640,  32658,
    32674,  32689,  32702,  32715,  32726,  32735,  32744,  32751,  32757,  32761,  32764,  32766,  32767,  32766,
    32764,  32761,  32757,  32751,  32744,  32735,  32726,  32715,  32702,  32689,  32674,  32658,  32640,  32622,
    32602,  32580,  32558,  32534,  32509,  32482,  32454,  32425,  32395,  32364,  32331,  32297,  32261,  32225,
    32187,  32147,  32107,  32065,  32022,  31978,  31932,  31886,  31837,  31788,  31738,  31686,  31633,  31578,
    31523,  31466,  31408,  31349,  31288,  31226,  31163,  31099,  31034,  30967,  30899,  30830,  30759,  30688,
    30615,  30541,  30466,  30390,  30312,  30233,  30153,  30072,  29990,  29906,  29821,  29736,  29648,  29560,
    29471,  29380,  29289,  29196,  29102,  29006,  28910,  28813,  28714,  28614,  28513,  28411,  28308,  28204,
    28099,  27992,  27885,  27776,  27666,  27555,  27443,  27330,  27216,  27101,  26985,  26867,  26749,  26630,
    26509,  26388,  26265,  26141,  26017,  25891,  25764,  25637,  25508,  25378,  25247,  25116,  24983,  24849,
    24715,  24579,  24442,  24305,  24166,  24027,  23886,  23745,  23602,  23459,  23315,  23170,  23024,  22877,
    22729,  22580,  22431,  22280,  22129,  21976,  21823,  21669,  21514,  21359,  21202,  21045,  20886,  20727,
    20568,  20407,  20245,  20083,  19920,  19756,  19592,  19426,  19260,  19093,  18925,  18757,  18588,  18418,
    18247,  18076,  17904,  17731,  17557,  17383,  17208,  17033,  16857,  16680,  16502,  16324,  16145,  15966,
    15786,  15605,  15424,  15242,  15059,  14876,  14692,  14508,  14323,  14138,  13952,  13765,  13578,  13390,
    13202,  13013,  12824,  12634,  12444,  12254,  12062,  11871,  11679,  11486,  11293,  11099,  10905,  10711,
    10516,  10321,  10126,  9930,   9733,   9536,   9339,   9142,   8944,   8746,   8547,   8348,   8149,   7949,
    7749,   7549,   7349,   7148,   6947,   6746,   6544,   6342,   6140,   5938,   5735,   5532,   5329,   5126,
    4922,   4719,   4515,   4311,   4107,   3902,   3698,   3493,   3289,   3084,   2879,   2673,   2468,   2263,
    2057,   1852,   1646,   1441,   1235,   1029,   823,    618,    412,    206,    0,      -206,   -412,   -618,
    -823,   -1029,  -1235,  -1441,  -1646,  -1852,  -2057,  -2263,  -2468,  -2673,  -2879,  -3084,  -3289,  -3493,
    -3698,  -3902,  -4107,  -4311,  -4515,  -4719,  -4922,  -5126,  -5329,  -5532,  -5735,  -5938,  -6140,  -6342,
    -6544,  -6746,  -6947,  -7148,  -7349,  -7549,  -7749,  -7949,  -8149,  -8348,  -8547,  -8746,  -8944,  -9142,
    -9339,  -9536,  -9733,  -9930,  -10126, -10321, -10516, -10711, -10905, -11099, -11293, -11486, -11679, -11871,
    -12062, -12254, -12444, -12634, -12824, -13013, -13202, -13390, -13578, -13765, -13952, -14138, -14323, -14508,
    -14692, -14876, -15059, -15242, -15424, -15605, -15786, -15966, -16145, -16324, -16502, -16680, -16857, -17033,
    -17208, -17383, -17557, -17731, -17904, -18076, -18247, -18418, -18588, -18757, -18925, -19093, -19260, -19426,
    -19592, -19756, -19920, -20083, -20245, -20407, -20568, -20727, -20886, -21045, -21202, -21359, -21514, -21669,
    -21823, -21976, -22129, -22280, -22431, -22580, -22729, -22877, -23024, -23170, -23315, -23459, -23602, -23745,
    -23886, -24027, -24166, -24305, -24442, -24579, -24715, -24849, -24983, -25116, -25247, -25378, -25508, -25637,
    -25764, -25891, -26017, -26141, -26265, -26388, -26509, -26630, -26749, -26867, -26985, -27101, -27216, -27330,
    -27443, -27555, -27666, -27776, -27885, -27992, -28099, -28204, -28308, -28411, -28513, -28614, -28714, -28813,
    -28910, -29006, -29102, -29196, -29289, -29380, -29471, -29560, -29648, -29736, -29821, -29906, -29990, -30072,
    -30153, -30233, -30312, -30390, -30466, -30541, -30615, -30688, -30759, -30830, -30899, -30967, -31034, -31099,
    -31163, -31226, -31288, -31349, -31408, -31466, -31523, -31578, -31633, -31686, -31738, -31788, -31837, -31886,
    -31932, -31978, -32022, -32065, -32107, -32147, -32187, -32225, -32261, -32297, -32331, -32364, -32395, -32425,
    -32454, -32482, -32509, -32534, -32558, -32580, -32602, -32622, -32640, -32658, -32674, -32689, -32702, -32715,
    -32726, -32735, -32744, -32751, -32757, -32761, -32764, -32766, -32767, -32766, -32764, -32761, -32757, -32751,
    -32744, -32735, -32726, -32715, -32702, -32689, -32674, -32658, -32640, -32622, -32602, -32580, -32558, -32534,
    -32509, -32482, -32454, -32425, -32395, -32364, -32331, -32297, -32261, -32225, -32187, -32147, -32107, -32065,
    -32022, -31978, -31932, -31886, -31837, -31788, -31738, -31686, -31633, -31578, -31523, -31466, -31408, -31349,
    -31288, -31226, -31163, -31099, -31034, -30967, -30899, -30830, -30759, -30688, -30615, -30541, -30466, -30390,
    -30312, -30233, -30153, -30072, -29990, -29906, -29821, -29736, -29648, -29560, -29471, -29380, -29289, -29196,
    -29102, -29006, -28910, -28813, -28714, -28614, -28513, -28411, -28308, -28204, -28099, -27992, -27885, -27776,
    -27666, -27555, -27443, -27330, -27216, -27101, -26985, -26867, -26749, -26630, -26509, -26388, -26265, -26141,
    -26017, -25891, -25764, -25637, -25508, -25378, -25247, -25116, -24983, -24849, -24715, -24579, -24442, -24305,
    -24166, -24027, -23886, -23745, -23602, -23459, -23315, -23170, -23024, -22877, -22729, -22580, -22431, -22280,
    -22129, -21976, -21823, -21669, -21514, -21359, -21202, -21045, -20886, -20727, -20568, -20407, -20245, -20083,
    -19920, -19756, -19592, -19426, -19260, -19093, -18925, -18757, -18588, -18418, -18247, -18076, -17904, -17731,
    -17557, -17383, -17208, -17033, -16857, -16680, -16502, -16324, -16145, -15966, -15786, -15605, -15424, -15242,
    -15059, -14876, -14692, -14508, -14323, -14138, -13952, -13765, -13578, -13390, -13202, -13013, -12824, -12634,
    -12444, -12254, -12062, -11871, -11679, -11486, -11293, -11099, -10905, -10711, -10516, -10321, -10126, -9930,
    -9733,  -9536,  -9339,  -9142,  -8944,  -8746,  -8547,  -8348,  -8149,  -7949,  -7749,  -7549,  -7349,  -7148,
    -6947,  -6746,  -6544,  -6342,  -6140,  -5938,  -5735,  -5532,  -5329,  -5126,  -4922,  -4719,  -4515,  -4311,
    -4107,  -3902,  -3698,  -3493,  -3289,  -3084,  -2879,  -2673,  -2468,  -2263,  -2057,  -1852,  -1646,  -1441,
    -1235,  -1029,  -823,   -618,   -412,   -206};
static short cos_table[COMM_MACRO_CPR] = {
    32767,  32766,  32764,  32761,  32757,  32751,  32744,  32735,  32726,  32715,  32702,  32689,  32674,  32658,
    32640,  32622,  32602,  32580,  32558,  32534,  32509,  32482,  32454,  32425,  32395,  32364,  32331,  32297,
    32261,  32225,  32187,  32147,  32107,  32065,  32022,  31978,  31932,  31886,  31837,  31788,  31738,  31686,
    31633,  31578,  31523,  31466,  31408,  31349,  31288,  31226,  31163,  31099,  31034,  30967,  30899,  30830,
    30759,  30688,  30615,  30541,  30466,  30390,  30312,  30233,  30153,  30072,  29990,  29906,  29821,  29736,
    29648,  29560,  29471,  29380,  29289,  29196,  29102,  29006,  28910,  28813,  28714,  28614,  28513,  28411,
    28308,  28204,  28099,  27992,  27885,  27776,  27666,  27555,  27443,  27330,  27216,  27101,  26985,  26867,
    26749,  26630,  26509,  26388,  26265,  26141,  26017,  25891,  25764,  25637,  25508,  25378,  25247,  25116,
    24983,  24849,  24715,  24579,  24442,  24305,  24166,  24027,  23886,  23745,  23602,  23459,  23315,  23170,
    23024,  22877,  22729,  22580,  22431,  22280,  22129,  21976,  21823,  21669,  21514,  21359,  21202,  21045,
    20886,  20727,  20568,  20407,  20245,  20083,  19920,  19756,  19592,  19426,  19260,  19093,  18925,  18757,
    18588,  18418,  18247,  18076,  17904,  17731,  17557,  17383,  17208,  17033,  16857,  16680,  16502,  16324,
    16145,  15966,  15786,  15605,  15424,  15242,  15059,  14876,  14692,  14508,  14323,  14138,  13952,  13765,
    13578,  13390,  13202,  13013,  12824,  12634,  12444,  12254,  12062,  11871,  11679,  11486,  11293,  11099,
    10905,  10711,  10516,  10321,  10126,  9930,   9733,   9536,   9339,   9142,   8944,   8746,   8547,   8348,
    8149,   7949,   7749,   7549,   7349,   7148,   6947,   6746,   6544,   6342,   6140,   5938,   5735,   5532,
    5329,   5126,   4922,   4719,   4515,   4311,   4107,   3902,   3698,   3493,   3289,   3084,   2879,   2673,
    2468,   2263,   2057,   1852,   1646,   1441,   1235,   1029,   823,    618,    412,    206,    0,      -206,
    -412,   -618,   -823,   -1029,  -1235,  -1441,  -1646,  -1852,  -2057,  -2263,  -2468,  -2673,  -2879,  -3084,
    -3289,  -3493,  -3698,  -3902,  -4107,  -4311,  -4515,  -4719,  -4922,  -5126,  -5329,  -5532,  -5735,  -5938,
    -6140,  -6342,  -6544,  -6746,  -6947,  -7148,  -7349,  -7549,  -7749,  -7949,  -8149,  -8348,  -8547,  -8746,
    -8944,  -9142,  -9339,  -9536,  -9733,  -9930,  -10126, -10321, -10516, -10711, -10905, -11099, -11293, -11486,
    -11679, -11871, -12062, -12254, -12444, -12634, -12824, -13013, -13202, -13390, -13578, -13765, -13952, -14138,
    -14323, -14508, -14692, -14876, -15059, -15242, -15424, -15605, -15786, -15966, -16145, -16324, -16502, -16680,
    -16857, -17033, -17208, -17383, -17557, -17731, -17904, -18076, -18247, -18418, -18588, -18757, -18925, -19093,
    -19260, -19426, -19592, -19756, -19920, -20083, -20245, -20407, -20568, -20727, -20886, -21045, -21202, -21359,
    -21514, -21669, -21823, -21976, -22129, -22280, -22431, -22580, -22729, -22877, -23024, -23170, -23315, -23459,
    -23602, -23745, -23886, -24027, -24166, -24305, -24442, -24579, -24715, -24849, -24983, -25116, -25247, -25378,
    -25508, -25637, -25764, -25891, -26017, -26141, -26265, -26388, -26509, -26630, -26749, -26867, -26985, -27101,
    -27216, -27330, -27443, -27555, -27666, -27776, -27885, -27992, -28099, -28204, -28308, -28411, -28513, -28614,
    -28714, -28813, -28910, -29006, -29102, -29196, -29289, -29380, -29471, -29560, -29648, -29736, -29821, -29906,
    -29990, -30072, -30153, -30233, -30312, -30390, -30466, -30541, -30615, -30688, -30759, -30830, -30899, -30967,
    -31034, -31099, -31163, -31226, -31288, -31349, -31408, -31466, -31523, -31578, -31633, -31686, -31738, -31788,
    -31837, -31886, -31932, -31978, -32022, -32065, -32107, -32147, -32187, -32225, -32261, -32297, -32331, -32364,
    -32395, -32425, -32454, -32482, -32509, -32534, -32558, -32580, -32602, -32622, -32640, -32658, -32674, -32689,
    -32702, -32715, -32726, -32735, -32744, -32751, -32757, -32761, -32764, -32766, -32767, -32766, -32764, -32761,
    -32757, -32751, -32744, -32735, -32726, -32715, -32702, -32689, -32674, -32658, -32640, -32622, -32602, -32580,
    -32558, -32534, -32509, -32482, -32454, -32425, -32395, -32364, -32331, -32297, -32261, -32225, -32187, -32147,
    -32107, -32065, -32022, -31978, -31932, -31886, -31837, -31788, -31738, -31686, -31633, -31578, -31523, -31466,
    -31408, -31349, -31288, -31226, -31163, -31099, -31034, -30967, -30899, -30830, -30759, -30688, -30615, -30541,
    -30466, -30390, -30312, -30233, -30153, -30072, -29990, -29906, -29821, -29736, -29648, -29560, -29471, -29380,
    -29289, -29196, -29102, -29006, -28910, -28813, -28714, -28614, -28513, -28411, -28308, -28204, -28099, -27992,
    -27885, -27776, -27666, -27555, -27443, -27330, -27216, -27101, -26985, -26867, -26749, -26630, -26509, -26388,
    -26265, -26141, -26017, -25891, -25764, -25637, -25508, -25378, -25247, -25116, -24983, -24849, -24715, -24579,
    -24442, -24305, -24166, -24027, -23886, -23745, -23602, -23459, -23315, -23170, -23024, -22877, -22729, -22580,
    -22431, -22280, -22129, -21976, -21823, -21669, -21514, -21359, -21202, -21045, -20886, -20727, -20568, -20407,
    -20245, -20083, -19920, -19756, -19592, -19426, -19260, -19093, -18925, -18757, -18588, -18418, -18247, -18076,
    -17904, -17731, -17557, -17383, -17208, -17033, -16857, -16680, -16502, -16324, -16145, -15966, -15786, -15605,
    -15424, -15242, -15059, -14876, -14692, -14508, -14323, -14138, -13952, -13765, -13578, -13390, -13202, -13013,
    -12824, -12634, -12444, -12254, -12062, -11871, -11679, -11486, -11293, -11099, -10905, -10711, -10516, -10321,
    -10126, -9930,  -9733,  -9536,  -9339,  -9142,  -8944,  -8746,  -8547,  -8348,  -8149,  -7949,  -7749,  -7549,
    -7349,  -7148,  -6947,  -6746,  -6544,  -6342,  -6140,  -5938,  -5735,  -5532,  -5329,  -5126,  -4922,  -4719,
    -4515,  -4311,  -4107,  -3902,  -3698,  -3493,  -3289,  -3084,  -2879,  -2673,  -2468,  -2263,  -2057,  -1852,
    -1646,  -1441,  -1235,  -1029,  -823,   -618,   -412,   -206,   0,      206,    412,    618,    823,    1029,
    1235,   1441,   1646,   1852,   2057,   2263,   2468,   2673,   2879,   3084,   3289,   3493,   3698,   3902,
    4107,   4311,   4515,   4719,   4922,   5126,   5329,   5532,   5735,   5938,   6140,   6342,   6544,   6746,
    6947,   7148,   7349,   7549,   7749,   7949,   8149,   8348,   8547,   8746,   8944,   9142,   9339,   9536,
    9733,   9930,   10126,  10321,  10516,  10711,  10905,  11099,  11293,  11486,  11679,  11871,  12062,  12254,
    12444,  12634,  12824,  13013,  13202,  13390,  13578,  13765,  13952,  14138,  14323,  14508,  14692,  14876,
    15059,  15242,  15424,  15605,  15786,  15966,  16145,  16324,  16502,  16680,  16857,  17033,  17208,  17383,
    17557,  17731,  17904,  18076,  18247,  18418,  18588,  18757,  18925,  19093,  19260,  19426,  19592,  19756,
    19920,  20083,  20245,  20407,  20568,  20727,  20886,  21045,  21202,  21359,  21514,  21669,  21823,  21976,
    22129,  22280,  22431,  22580,  22729,  22877,  23024,  23170,  23315,  23459,  23602,  23745,  23886,  24027,
    24166,  24305,  24442,  24579,  24715,  24849,  24983,  25116,  25247,  25378,  25508,  25637,  25764,  25891,
    26017,  26141,  26265,  26388,  26509,  26630,  26749,  26867,  26985,  27101,  27216,  27330,  27443,  27555,
    27666,  27776,  27885,  27992,  28099,  28204,  28308,  28411,  28513,  28614,  28714,  28813,  28910,  29006,
    29102,  29196,  29289,  29380,  29471,  29560,  29648,  29736,  29821,  29906,  29990,  30072,  30153,  30233,
    30312,  30390,  30466,  30541,  30615,  30688,  30759,  30830,  30899,  30967,  31034,  31099,  31163,  31226,
    31288,  31349,  31408,  31466,  31523,  31578,  31633,  31686,  31738,  31788,  31837,  31886,  31932,  31978,
    32022,  32065,  32107,  32147,  32187,  32225,  32261,  32297,  32331,  32364,  32395,  32425,  32454,  32482,
    32509,  32534,  32558,  32580,  32602,  32622,  32640,  32658,  32674,  32689,  32702,  32715,  32726,  32735,
    32744,  32751,  32757,  32761,  32764,  32766};
// clang-format on

namespace xf {
namespace motorcontrol {

// clang-format off
/// all FOC Mode
enum FOC_Mode {
    // usr modes
    MOD_STOPPED = 0,
    MOD_SPEED_WITH_TORQUE,
    MOD_TORQUE_WITHOUT_SPEED,
    MOD_FLUX,
    // expert modes
    MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED,
    MOD_MANUAL_TORQUE_FLUX,
    MOD_MANUAL_TORQUE,
    MOD_MANUAL_FLUX,
    MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE,
    MOD_TOTAL_NUM
};

} // namespace xf
} // namespace motorcontrol

// clang-format on

namespace xf {
namespace motorcontrol {
namespace details {

//--------------------------------------------------------------------------
// The motor should rotate regardless of the encoder output
//--------------------------------------------------------------------------
template <class T_IO, class T_SINCOS, class T_Mode>
void Control_foc_ap_fixed(T_IO& Vd,
                          T_IO& Vq,
                          T_SINCOS& cos_out,
                          T_SINCOS& sin_out,
                          T_Mode FOC_mode,
                          T_SINCOS cos_gen_angle,
                          T_SINCOS sin_gen_angle,
                          T_SINCOS cos_in,
                          T_SINCOS sin_in,
                          T_IO Flux_out,
                          T_IO Torque_out,
                          T_IO args_vd,
                          T_IO args_vq) {
#pragma HLS INLINE off

    // Control Vd and Vq depending on work mode
    switch (FOC_mode) {
        case MOD_STOPPED: // Motor stop
            Vd = 0;       // Set zero Vd
            Vq = 0;       // Set zero Vq
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_SPEED_WITH_TORQUE: // Work mode speed loop
            Vd = Flux_out;          // Sorce Vd from Flux PI
            Vq = Torque_out;        // Sorce Vq from Torque PI
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_TORQUE_WITHOUT_SPEED: // Disable Speed PI
            Vd = Flux_out;             // Sorce Vd from Flux PI
            Vq = Torque_out;           // Sorce Vq from Torque PI
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_FLUX:
            Vd = Flux_out;
            Vq = Torque_out;
            cos_out = cos_in;
            sin_out = sin_in;
            break;

        case MOD_MANUAL_TORQUE_FLUX: // Manual Vd/Vq with real angle
            Vd = args_vd;            // Sorce Vd from register
            Vq = args_vq;            // Sorce Vq from register
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        case MOD_MANUAL_TORQUE: // Manual torque
            Vd = Flux_out;      // Sorce Vd from Flux PI
            Vq = args_vq;       // Sorce Vq from register
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        // Manual mode with angle generator
        case MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED:
            Vd = args_vd;            // Sorce Vd from register
            Vq = args_vq;            // Sorce Vq from register
            cos_out = cos_gen_angle; // Generated angle cos
            sin_out = sin_gen_angle; // Generated angle sin
            break;
        case MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE:
            Vd = args_vd;     // Sorce Vd from register
            Vq = args_vq;     // Sorce Vq from register
            cos_out = cos_in; //  angle cos from aix register
            sin_out = sin_in; //  angle sin from aix register
            break;
        case MOD_MANUAL_FLUX:
            Vd = args_vd;
            Vq = Torque_out;
            cos_out = cos_in;
            sin_out = sin_in;
            break;
        default:    // Motor OFF
            Vd = 0; // Set zero Vd
            Vq = 0; // Set zero Vq
            cos_out = cos_in;
            sin_out = sin_in;
            break;
    }
}

//--------------------------------------------------------------------------

template <int VALUE_CPR, class T_IO, int MAX_IO, int W, int I>
void foc_core_ap_fixed(
    // Input
    T_IO Ia,     // Phase A current
    T_IO Ib,     // Phase B current
    T_IO Ic,     // Phase B current
    short RPM,   // RPM
    short Angle, // Encoder count
    // Output for GPIO
    T_IO& Va_cmd,
    T_IO& Vb_cmd,
    T_IO& Vc_cmd,
    // Angle translation parameters
    int tab_map_factor,
    short cpr_div_ppr,
    T_IO RPM_to_speed,
    // Inout put for parameters
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    // the following are all representated in q15q16 format
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& trigger_args,
    // int format
    volatile int& control2_args,
    // the following are all representated in q15q16 format
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    // the following are all representated in q15q16 format
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Ialpha_stts,
    volatile int& Ibeta_stts,
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args) {
#pragma HLS INLINE off
#pragma HLS BIND_STORAGE variable = sin_table type = RAM_2P impl = BRAM
#pragma HLS BIND_STORAGE variable = cos_table type = RAM_2P impl = BRAM
    //#pragma HLS pipeline enable_flush

    const int W_pid = 16;
    const int W_sin = 16;

    typedef short t_angle;
    typedef ap_uint<4> t_mode;
    typedef ap_fixed<32, 16, AP_RND, AP_WRAP> t_mid2;
    typedef ap_fixed<W_sin, 1, AP_TRN, AP_WRAP> t_sincos;
    typedef ap_fixed<W_pid, 8, AP_TRN, AP_WRAP> t_pid; // just a coefficients
    typedef ap_fixed<W + 8, I + 8, AP_TRN, AP_WRAP> t_mid;
    // typedef ap_fixed<48, 32> t_mid2;

    //--------------------------------------------------------------------------
    const int W_RPM = 16;
    const int I_RPM = 16;
    const int W_RPME = W_RPM + 1;
    const int I_RPME = I_RPM + 1;
    const int W_RPMI = W_RPM + 10;
    const int I_RPMI = I_RPM + 10;
    typedef ap_fixed<W_RPM, I_RPM, AP_TRN, AP_SAT> t_RPM;
    typedef ap_fixed<W_RPME, I_RPME, AP_TRN, AP_SAT> t_RPME;
    typedef ap_fixed<W_RPMI, I_RPMI, AP_TRN, AP_SAT> t_RPMI;

    // for intermediate variables type
    const int I_used = (I < 8) ? 8 : I;
    const int W_IQ = I_used + 16;
    const int I_IQ = I_used + 8;
    const int W_IQE = W_IQ + 1;
    const int I_IQE = I_IQ + 1;
    const int W_IQI = W_IQ + 10;
    const int I_IQI = I_IQ + 10;
    typedef ap_fixed<W_IQ, I_IQ, AP_TRN, AP_WRAP> t_IQ;
    typedef ap_fixed<W_IQE, I_IQE, AP_TRN, AP_SAT> t_IQE;
    typedef ap_fixed<W_IQI, I_IQI, AP_TRN, AP_SAT> t_IQI;

    const int W_ID = I_used + 16;
    const int I_ID = I_used + 8;
    const int W_IDE = W_ID + 1;
    const int I_IDE = I_ID + 1;
    const int W_IDI = W_ID + 10;
    const int I_IDI = I_ID + 10;
    typedef ap_fixed<W_ID, I_ID, AP_TRN, AP_WRAP> t_ID;
    typedef ap_fixed<W_IDE, I_IDE, AP_TRN, AP_SAT> t_IDE;
    typedef ap_fixed<W_IDI, I_IDI, AP_TRN, AP_SAT> t_IDI;

    // clang-format off
    t_glb_q15q16 apx_flux_sp_args;
    apx_flux_sp_args(31, 0) = flux_sp_args;
    t_glb_q15q16 apx_flux_kp_args;
    apx_flux_kp_args(31, 0) = flux_kp_args;
    t_glb_q15q16 apx_flux_ki_args;
    apx_flux_ki_args(31, 0) = flux_ki_args;
    t_glb_q15q16 apx_flux_kd_args;
    apx_flux_kd_args(31, 0) = flux_kd_args;
    t_glb_q15q16 apx_torque_sp_args;
    apx_torque_sp_args(31, 0) = torque_sp_args;
    t_glb_q15q16 apx_torque_kp_args;
    apx_torque_kp_args(31, 0) = torque_kp_args;
    t_glb_q15q16 apx_torque_ki_args;
    apx_torque_ki_args(31, 0) = torque_ki_args;
    t_glb_q15q16 apx_torque_kd_args;
    apx_torque_kd_args(31, 0) = torque_kd_args;
    t_glb_q15q16 apx_speed_sp_args;
    apx_speed_sp_args(31, 0) = speed_sp_args;
    t_glb_q15q16 apx_speed_kp_args;
    apx_speed_kp_args(31, 0) = speed_kp_args;
    t_glb_q15q16 apx_speed_ki_args;
    apx_speed_ki_args(31, 0) = speed_ki_args;
    t_glb_q15q16 apx_speed_kd_args;
    apx_speed_kd_args(31, 0) = speed_kd_args;
    t_glb_q15q16 apx_angle_sh_args;
    apx_angle_sh_args(31, 0) = angle_sh_args;
    t_glb_q15q16 apx_vd_args;
    apx_vd_args(31, 0) = vd_args;
    t_glb_q15q16 apx_vq_args;
    apx_vq_args(31, 0) = vq_args;
    t_glb_q15q16 apx_fw_kp_args;
    apx_fw_kp_args(31, 0) = fw_kp_args;
    t_glb_q15q16 apx_fw_ki_args;
    apx_fw_ki_args(31, 0) = fw_ki_args;
    t_glb_q15q16 apx_fixed_angle_args;
    apx_fixed_angle_args(31, 0) = fixed_angle_args;
    short fixed_angle_args_short = apx_fixed_angle_args;
    // clang-format on

    // static T_Vabc SVM_inv_index = MAX_LIM >> 1;
    // short V_fw = 1;
    static t_RPMI Speed_I_err_prev = 0;
    static t_RPME Speed_Error_prev = 0;

    static t_ID Vd_weakened = 0; // case FW

    static t_IDI Flux_I_err_prev = 0; // Variable for previous integral value
    static t_IDE Flux_Error_prev = 0; // Variable for previous derivative value

    static t_IQI Torque_I_err_prev = 0; // Variable for previous integral value
    static t_IQE Torque_Error_prev = 0; // Variable for previous derivative value

    static t_IDI FWiE_prev = 0;
    static t_IDE FW_err_prev = 0;

    static int gen_delay = 0;     // Generator period counter
    static t_angle gen_angle = 0; // Generator angle counter
    //--------------------------------------------------------------------------
    // load args
    //--------------------------------------------------------------------------
    static t_mode Mode_Prev = MOD_STOPPED; // Previous control Register.
    t_mode FOC_mode = control_mode_args;
    /*
    if(FOC_mode == MOD_STOPPED){
        Speed_I_err_prev = 0;
        Speed_Error_prev = 0;

        Vd_weakened = 0;// case FW

        Flux_I_err_prev = 0; // Variable for previous integral value
        Flux_Error_prev = 0; // Variable for previous derivative value

        Torque_I_err_prev = 0; // Variable for previous integral value
        Torque_Error_prev = 0; // Variable for previous derivative value

        FWiE_prev = 0;
        FW_err_prev = 0;

        gen_delay = 0;   // Generator period counter
        gen_angle = 0;   // Generator angle counter

        Mode_Prev = MOD_STOPPED;
        return;
    }*/
    bool Mode_Change = (FOC_mode != Mode_Prev) || (FOC_mode == MOD_STOPPED);
    Mode_Prev = FOC_mode;

    int FixPeriod = control_fixperiod_args;
    //--------------------------------------------------------------------------
    // Angle
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // Generated angle period = FixPeriod * II / freq
    // FixPeriod = angle period * freq / latency
    // static int gen_delay = 0;   // Generator period counter
    // static t_angle gen_angle = 0;   // Generator angle counter
    // Simple angle generator for manual mode
    // The motor should rotate regardless of the encoder output
    if (gen_delay >= FixPeriod) { // Period loop
        gen_delay = 0;
        if (gen_angle >= (VALUE_CPR - 1)) { // Angle loop
            gen_angle = 0;
        } else {
            ++gen_angle;
        }
    } else {
        ++gen_delay;
    }
    // t_sincos cos_gen_angle; cos_gen_angle(15, 0) = cos_table[gen_angle];
    // t_sincos sin_gen_angle; sin_gen_angle(15, 0) = sin_table[gen_angle];

    t_angle Theta = Angle - angle_sh_args; // Apply angle correction
    // Theta = (FOC_mode == MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED) ? gen_angle : Theta;
    Theta = (FOC_mode == MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED)
                ? gen_angle
                : (FOC_mode == MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE) ? fixed_angle_args_short : Theta;
    Theta = (Theta < 0) ? (short)(Theta + VALUE_CPR) : Theta;          // Correct negative angle
    Theta = (Theta >= VALUE_CPR) ? (short)(Theta - VALUE_CPR) : Theta; // Correct angle overload to (0, CPR)
    t_angle Q = (Theta / cpr_div_ppr); // Correct angle overload round to (0. cpr_div_ppr)
    Theta = Theta - Q * cpr_div_ppr;
    Theta = (tab_map_factor * Theta) >> 16; // norm_theta_e = theta_e * len / cpr_div_ppr
    RANGETRACER("FOC.Theta", (float)Theta);

    // clang-format off
    //--------------------------------------------------------------------------
    T_IO Ialpha, Ibeta, Ihomopolar; // Transfom result
    Clarke_Direct_3p_ap_fixed(
        Ialpha, 
        Ibeta, 
        Ihomopolar, 
        Ia,
        Ib, 
        Ic);
    RANGETRACER("FOC.CLARK.Ia", Ia);
    RANGETRACER("FOC.CLARK.Ib", Ib);
    RANGETRACER("FOC.CLARK.Ic", Ic);
    RANGETRACER("FOC.CLARK.Ialpha", Ialpha);
    RANGETRACER("FOC.CLARK.Ibeta", Ibeta);
    RANGETRACER("FOC.CLARK.Ihomopolar", Ihomopolar);

    //--------------------------------------------------------------------------
    T_IO Id, Iq;
    t_sincos cos_theta; cos_theta(15, 0) = cos_table[Theta];
    t_sincos sin_theta; sin_theta(15, 0) = sin_table[Theta];
    #pragma HLS BIND_STORAGE variable=sin_table type=RAM_2P impl=BRAM
    #pragma HLS BIND_STORAGE variable=cos_table type=RAM_2P impl=BRAM
    Park_Direct_ap_fixed<T_IO, t_sincos>(
        Id, 
        Iq, 
        Ialpha, 
        Ibeta, 
        cos_theta,
        sin_theta);
    RANGETRACER("FOC.PARK.Id", Id);
    RANGETRACER("FOC.PARK.Iq", Iq);

    t_RPM         Speed_pid_din  = RPM;
    t_RPM         Speed_pid_dout;
    PID_Control_ap_fixed<t_RPM, t_RPMI, t_RPME, t_pid>(
        Speed_pid_dout, 
        Speed_I_err_prev, 
        Speed_Error_prev, 
        Speed_pid_din, 
        apx_speed_sp_args,
        apx_speed_kp_args,
        apx_speed_ki_args,
        apx_speed_kd_args,
        Mode_Change);
    RANGETRACER("FOC.PID.Speed_I_err_prev", Speed_I_err_prev);
    RANGETRACER("FOC.PID.Speed_pid_dout", Speed_pid_dout);
    RANGETRACER("FOC.PID.Speed_I_err_prev", Speed_I_err_prev);
    RANGETRACER("FOC.PID.Speed_Error_prev", Speed_Error_prev);
    RANGETRACER("FOC.PID.Speed_pid_din", Speed_pid_din);
    RANGETRACER("FOC.PID.apx_speed_sp_args", apx_speed_sp_args);


    //--------------------------------------------------------------------------
    // for flux
    t_ID SVM_inv_index = MAX_IO/2;  // case FW

    t_ID Flux_pid_dout;                // Partial results
    t_ID Flux_sp = (FOC_mode == MOD_FLUX) 
                    ? (t_ID)(apx_flux_sp_args - Vd_weakened)
                    : (t_ID)apx_flux_sp_args;
    PID_Control_ap_fixed<t_ID, t_IDI, t_IDE, t_pid>(
        Flux_pid_dout, 
        Flux_I_err_prev, 
        Flux_Error_prev, 
        Id, 
        Flux_sp, 
        apx_flux_kp_args, 
        apx_flux_ki_args, 
        apx_flux_kd_args, 
        Mode_Change);

    T_IO Flux_pid_dout_io = (T_IO)Clip_AP<t_IQ>(Flux_pid_dout, (t_IQ)(0-MAX_IO), (t_IQ)MAX_IO);
    RANGETRACER("FOC.Flux_pid.Flux_pid_dout", Flux_pid_dout);
    RANGETRACER("FOC.Flux_pid.Flux_I_err_prev", Flux_I_err_prev);
    RANGETRACER("FOC.Flux_pid.Flux_Error_prev", Flux_Error_prev);
    RANGETRACER("FOC.Flux_pid.Id", Id);
    RANGETRACER("FOC.Flux_pid.Flux_sp", Flux_sp);
    RANGETRACER("FOC.Flux_pid.apx_flux_sp_args", apx_flux_sp_args);
    RANGETRACER("FOC.Flux_pid.Vd_weakened", Vd_weakened);
    RANGETRACER("FOC.Flux_pid.Flux_pid_dout_io", Flux_pid_dout_io);

    //Torque PI Controller--------------------------------------------------------------------------
    t_IQ Torque_pid_dout;                // Partial results
    t_IQ Torque_Sp = (FOC_mode == MOD_TORQUE_WITHOUT_SPEED)
                    ? (t_IQ)(apx_torque_sp_args)
                    : (t_IQ)Speed_pid_dout; // Only in Torque mode Speed_pid not be the setpoint
    PID_Control_ap_fixed<t_IQ, t_IQI, t_IQE, t_pid>(
        Torque_pid_dout, 
        Torque_I_err_prev, 
        Torque_Error_prev, 
        Iq,
        Torque_Sp, 
        apx_torque_kp_args, 
        apx_torque_ki_args,
        apx_torque_kd_args, 
        Mode_Change);
    T_IO Torque_pid_dout_io = (T_IO)Clip_AP<t_IQ>(Torque_pid_dout, (t_IQ)(0-MAX_IO), (t_IQ)MAX_IO);

    RANGETRACER("FOC.Torque_pid.Torque_pid_dout", Torque_pid_dout);
    RANGETRACER("FOC.Torque_pid.Torque_I_err_prev", Torque_I_err_prev);
    RANGETRACER("FOC.Torque_pid.Torque_Error_prev", Torque_Error_prev);
    RANGETRACER("FOC.Torque_pid.Iq", Iq);
    RANGETRACER("FOC.Torque_pid.Torque_Sp", Torque_Sp);
    RANGETRACER("FOC.Torque_pid.apx_torque_sp_args", apx_torque_sp_args);
    RANGETRACER("FOC.Torque_pid.Speed_pid_dout", Speed_pid_dout);
    RANGETRACER("FOC.Torque_pid.Torque_pid_dout_io", Torque_pid_dout_io);
    //--------------------------------------------------------------------------
    T_IO Flux_decoupled = 0;
    T_IO Torque_decoupled = 0;
    Decoupling_T_ap_fixed<t_IQI, t_IQI, t_IQI, T_IO, MAX_IO>(
        Flux_decoupled, 
        Torque_decoupled, 
        (t_IQI)Id, 
        (t_IQI)Iq, 
        (t_IQI)Flux_pid_dout_io, 
        (t_IQI)Torque_pid_dout_io,
        (t_IQI)RPM,
        (t_IQI)RPM_to_speed);

    RANGETRACER("FOC.Decoupling.Flux_decoupled", Flux_decoupled);
    RANGETRACER("FOC.Decoupling.Torque_decoupled", Torque_decoupled);
    RANGETRACER("FOC.Decoupling.Flux_pid_dout_io", Flux_pid_dout_io);
    RANGETRACER("FOC.Decoupling.Torque_pid_dout_io", Torque_pid_dout_io);

    //--------------------------------------------------------------------------
    T_IO Modulation, Modulation_sp;
    Field_Weakening_T<T_IO, T_IO, t_mid2>(
        Modulation, 
        Modulation_sp, 
        Flux_decoupled, 
        Torque_decoupled, 
        1,//V,
        SVM_inv_index);//SVM_inv_index);
    RANGETRACER("FOC.Field_W.Modulation", Modulation);
    RANGETRACER("FOC.Field_W.Modulation_sp", Modulation_sp);
    RANGETRACER("FOC.Field_W.Flux_decoupled", Flux_decoupled);
    RANGETRACER("FOC.Field_W.Torque_decoupled", Torque_decoupled);
    const T_IO MAX_CURRENT = COMM_MOTOR_PARA_IMAX; // stall current limit which is ~2.7A per the motor datasheet 

    //Modulation = Clip_AP<T_IO>(Modulation, (T_IO)(0-MAX_IO), (T_IO)MAX_IO);  

    PID_Control_ap_fixed<t_ID, t_IDI, t_IDE, t_pid>(
        Vd_weakened, 
        FWiE_prev, 
        FW_err_prev, 
        (t_ID)Modulation,
        (t_ID)Modulation_sp, 
        (t_pid)apx_fw_kp_args,
        (t_pid)apx_fw_ki_args,
        (t_pid)0, 
        Mode_Change);   

    RANGETRACER("FOC.PID.Field_W.Vd_weakened", Vd_weakened);

    Vd_weakened = Clip_AP<T_IO>(Vd_weakened, (T_IO)(0-MAX_CURRENT), (T_IO)MAX_CURRENT);                                                                

   
    RANGETRACER("FOC.PID.Field_W.FWiE_prev", FWiE_prev);
    RANGETRACER("FOC.PID.Field_W.Modulation", Modulation);
    RANGETRACER("FOC.PID.Field_W.Modulation_sp", Modulation_sp);
    RANGETRACER("FOC.PID.Field_W.apx_fw_kp_args", apx_fw_kp_args);
    RANGETRACER("FOC.PID.Field_W.apx_fw_ki_args", apx_fw_ki_args);
    RANGETRACER("FOC.PID.Field_W.Clip_Vd_weakened", Vd_weakened);

    //--------------------------------------------------------------------------
    // volatile int Vd, Vq;
    T_IO Vd_ctrl, Vq_ctrl;
    t_sincos cos_theta_ctrl, sin_theta_ctrl;
    Control_foc_ap_fixed<T_IO, t_sincos, t_mode>(
        //output
        Vd_ctrl, 
        Vq_ctrl, 
        cos_theta_ctrl, 
        sin_theta_ctrl, 
        //input
        FOC_mode,
        cos_theta,//cos_gen_angle, 
        sin_theta,//sin_gen_angle, 
        cos_theta, 
        sin_theta, 
        Flux_pid_dout_io, 
        Torque_pid_dout_io,
        apx_vd_args,
        apx_vq_args);
    Vd_ctrl = Clip_AP<T_IO>(Vd_ctrl, (T_IO)(0-MAX_IO), (T_IO)MAX_IO);
    Vq_ctrl = Clip_AP<T_IO>(Vq_ctrl, (T_IO)(0-MAX_IO), (T_IO)MAX_IO);
    T_IO Valpha, Vbeta; // Transfom result
    Park_Inverse_ap_fixed<T_IO, t_sincos>(
        Valpha, 
        Vbeta, 
        Vd_ctrl, 
        Vq_ctrl,
        cos_theta_ctrl, 
        sin_theta_ctrl
        );
    RANGETRACER("FOC.InversPark.Valpha", Valpha);
    RANGETRACER("FOC.InversPark.Vbeta", Vbeta);
    RANGETRACER("FOC.InversPark.Vd_ctrl", Vd_ctrl);
    RANGETRACER("FOC.InversPark.Vq_ctrl", Vq_ctrl);


    T_IO Va_iclk, Vb_iclk, Vc_iclk;
    Clarke_Inverse_2p_ap_fixed<T_IO>(
        Va_iclk, 
        Vb_iclk, 
        Vc_iclk, 
        Valpha, 
        Vbeta);
    RANGETRACER("FOC.InversClarke.Va_iclk", Va_iclk);
    RANGETRACER("FOC.InversClarke.Vb_iclk", Vb_iclk);
    RANGETRACER("FOC.InversClarke.Vc_iclk", Vc_iclk);

    // clang-format on

    Va_cmd = Clip_AP<T_IO>(Va_iclk, (T_IO)(0 - MAX_IO), (T_IO)MAX_IO);
    Vb_cmd = Clip_AP<T_IO>(Vb_iclk, (T_IO)(0 - MAX_IO), (T_IO)MAX_IO);
    Vc_cmd = Clip_AP<T_IO>(Vc_iclk, (T_IO)(0 - MAX_IO), (T_IO)MAX_IO);

    RANGETRACER("FOC.InversClarke.Va_cmd", Va_cmd);
    RANGETRACER("FOC.InversClarke.Vb_cmd", Vb_cmd);
    RANGETRACER("FOC.InversClarke.Vc_cmd", Vc_cmd);

    t_glb_q15q16 apx_id_stts = Id;
    t_glb_q15q16 apx_iq_stts = Iq;
    t_glb_q15q16 apx_flux_acc_stts = Flux_I_err_prev;
    t_glb_q15q16 apx_flux_err_stts = Flux_Error_prev;
    t_glb_q15q16 apx_flux_out_stts = Flux_pid_dout;
    t_glb_q15q16 apx_torque_acc_stts = Torque_I_err_prev;
    t_glb_q15q16 apx_torque_err_stts = Torque_Error_prev;
    t_glb_q15q16 apx_torque_out_stts = Torque_pid_dout;
    t_glb_q15q16 apx_speed_acc_stts = Speed_I_err_prev;
    t_glb_q15q16 apx_speed_err_stts = Speed_Error_prev;
    t_glb_q15q16 apx_speed_out_stts = Speed_pid_dout;

    t_glb_q15q16 apx_speed_Ialpha_stts = Ialpha;
    t_glb_q15q16 apx_speed_Ibeta_stts = Ibeta;
    t_glb_q15q16 apx_speed_Ihomopolar_stts = Ihomopolar;

    t_glb_q15q16 apx_speed_stts = RPM;
    t_glb_q15q16 apx_angle_stts = Angle;

    speed_stts = apx_speed_stts.range(31, 0);
    angle_stts = apx_angle_stts.range(31, 0);
    id_stts = apx_id_stts.range(31, 0);
    iq_stts = apx_iq_stts.range(31, 0);
    flux_acc_stts = apx_flux_acc_stts.range(31, 0);     // Flux_I_err_prev;
    flux_err_stts = apx_flux_err_stts.range(31, 0);     // Flux_Error_prev;
    flux_out_stts = apx_flux_out_stts.range(31, 0);     // Flux_pid_dout;
    torque_acc_stts = apx_torque_acc_stts.range(31, 0); // Torque_I_err_prev;
    torque_err_stts = apx_torque_err_stts.range(31, 0); // Torque_Error_prev;
    torque_out_stts = apx_torque_out_stts.range(31, 0); // Torque_pid_dout;
    speed_acc_stts = apx_speed_acc_stts.range(31, 0);   // Speed_I_err_prev;
    speed_err_stts = apx_speed_err_stts.range(31, 0);   // Speed_Error_prev;
    speed_out_stts = apx_speed_out_stts.range(31, 0);   // Speed_pid_dout;
    Ialpha_stts = apx_speed_Ialpha_stts.range(31, 0);
    Ibeta_stts = apx_speed_Ibeta_stts.range(31, 0);
    Ihomopolar_stts = apx_speed_Ihomopolar_stts.range(31, 0);
}

} // namespace details

//--------------------------------------------------------------------------
// FOC templated top functions'hls_foc_strm_ap_fixed' has following features
// 1) Based on ap_fixed data type to retain the meaning of physical quantities;
// 2) stream-style input and output consumed and fed withing a almost infinate loop.
//--------------------------------------------------------------------------
// clang-format off
/**
 * @brief The function hls_foc_strm_ap_fixed is sensor based field-orientated control (FOC)
 * @tparam VALUE_CPR        Number of encoder steps per one full revolution. ex. 1000
 * @tparam T_IO             Data type for input currents and output commands. ex. ap_fixed<24, 8>
 * @tparam MAX_IO           Maximum absolute value for input currents and output commands. ex. 24(V) 
 * @tparam W                Width of T_IO. ex. 24 for ap_fixed<24, 8>
 * @tparam I                Integer part width of T_IO(inluding sign bit). ex. 8 for ap_fixed<24, 8>
 * @tparam T_RPM_THETA_FOC  Data type for packaged RPM and Theta scale value mode by VALUE_CPR, 32-bit aligned  
 * @param  Ia			    Input Phase A current
 * @param  Ib			    Input Phase B current
 * @param  Ic			    Input Phase C current
 * @param  FOC_RPM_THETA_m  Input THETA_m in [31:16] and RPM in [15:0] 
 * @param  Va_cmd 			Output Va
 * @param  Vb_cmd 			Output Vb
 * @param  Vc_cmd 			Output Vc
 * @param  ppr_args         input number of pole pairs per phase of the motor; full sinus periods per revolution.
 * @param  control_mode_args            Input control mode of foc, enum FOC_Mode. Read every latency cycles of LOOP_FOC
 * @param  control_fixperiod_args       input control_fixperiod. Read every latency cycles of LOOP_FOC
 * @param  flux_sp_args     Input Args setting point for PID control of Flux
 * @param  flux_kp_args     Input Args Proportional coefficient for PID control of Flux
 * @param  flux_ki_args     Input Args Integral coefficient for PID control of Flux
 * @param  flux_kd_args     Input Args Differential coefficient for PID control of Flux
 * @param  torque_sp_args    Input Args setting point for PID control of Torque
 * @param  torque_kp_args    Input Args Proportional coefficient for PID control of Torque
 * @param  torque_ki_args    Input Args Integral coefficient for PID control of Torque
 * @param  torque_kd_args    Input Args Differential coefficient for PID control of Torque
 * @param  speed_sp_args    Input Args setting point for PID control of RPM
 * @param  speed_kp_args    Input Args Proportional coefficient for PID control of RPM
 * @param  speed_ki_args    Input Args Integral coefficient for PID control of RPM
 * @param  speed_kd_args    Input Args Differential coefficient for PID control of RPM
 * @param  angle_sh_args    Input Args for angle shift
 * @param  vd_args          Input Args for setting fixed vd
 * @param  vq_args          Input Args for setting fixed vq
 * @param  fw_kp_args       Input Args setting point for PID control of field weakening
 * @param  fw_ki_args       Input Args Integral coefficient for PID control of field weakening
 * @param  id_stts          Output status to monitor stator d-axis current
 * @param  flux_acc_stts    Output status to monitor flux accumulate value
 * @param  flux_err_stts    Output status to monitor flux latest error value
 * @param  flux_out_stts    Output status to monitor flux PID's output
 * @param  iq_stts          Output status to monitor stator q-axis current
 * @param  torque_acc_stts  Output status to monitor torque accumulate value
 * @param  torque_err_stts  Output status to monitor torque latest error value
 * @param  torque_out_stts  Output status to monitor torque PID's output
 * @param  speed_stts       Output status to monitor speed(RPM) of motor
 * @param  speed_acc_stts   Output status to monitor speed(RPM) accumulate value
 * @param  speed_err_stts   Output status to monitor speed(RPM) latest error value
 * @param  speed_out_stts   Output status to monitor speed(RPM)  PID's output
 * @param  angle_stts       Output status to monitor Theta_m of motor (scale value to [0, VALUE_CPR]) 
 * @param  Va_cmd_stts      Output status to monitor Output Va
 * @param  Vb_cmd_stts      Output status to monitor Output Vb
 * @param  Vc_cmd_stts      Output status to monitor Output Vc
 * @param  Ialpha_stts      Output status to monitor Ialpha (output of Clarke_Direct) 
 * @param  Ibeta_stts       Output status to monitor Ibeta (output of Clarke_Direct) 
 * @param  Ihomopolar_stts  Output status to monitor Ihomopolar (output of Clarke_Direct) 
 * @param  fixed_angle_args Input Args for fixed angle value in CPR range by Q15Q16 format 
 * @param  trip_cnt         Input Args to set the trip count of foc loop
 */
// clang-format on
template <int VALUE_CPR, typename T_IO, int MAX_IO, int W, int I, typename T_RPM_THETA_FOC>
void hls_foc_strm_ap_fixed(
    // Input
    hls::stream<T_IO>& Ia,
    hls::stream<T_IO>& Ib,
    hls::stream<T_IO>& Ic,
    hls::stream<T_RPM_THETA_FOC>& FOC_RPM_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<T_IO>& Va_cmd,
    hls::stream<T_IO>& Vb_cmd,
    hls::stream<T_IO>& Vc_cmd,
    // In-out for parameters
    volatile int& ppr_args,
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    //
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Va_cmd_stts,
    volatile int& Vb_cmd_stts,
    volatile int& Vc_cmd_stts,
    volatile int& Ialpha_stts,
    volatile int& Ibeta_stts,
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args,
    volatile long& trip_cnt) {
    short cpr_div_ppr = VALUE_CPR / ppr_args;
    unsigned int tab_map_factor = ((COMM_MACRO_TLB_LENTH * (unsigned int)ppr_args) << 16) / VALUE_CPR;
    const T_IO RPM_factor = 60.0 / 2 / 3.1415926535;
    T_IO RPM_to_speed = (t_glb_q15q16)ppr_args / RPM_factor;
LOOP_FOC_STRM:
    for (long i = 0; i < trip_cnt; i++) {
#pragma HLS pipeline II = 5
        static int FOC_RPM_THETA_m_in;
        static T_IO Ia_in;
        static T_IO Ib_in;
        static T_IO Ic_in;
        if (!Ia.empty()) {
            Ia_in = Ia.read();
            Ib_in = Ib.read();
            Ic_in = Ic.read();
        }

        if (!FOC_RPM_THETA_m.empty()) FOC_RPM_THETA_m_in = FOC_RPM_THETA_m.read();

        short RPM_in = (FOC_RPM_THETA_m_in & 0x0000FFFF);
        short Angle_in = (FOC_RPM_THETA_m_in & 0xFFFF0000) >> 16;

        T_IO Va_out, Vb_out, Vc_out;
        // reserved word
        // int ppr_args = 0;
        // int cpr_args = 0;
        int trigger_args = 0;
        int control2_args = 0;

        details::foc_core_ap_fixed<VALUE_CPR, T_IO, MAX_IO, W, I>(
            Ia_in, Ib_in, Ic_in, RPM_in, Angle_in, Va_out, Vb_out, Vc_out, tab_map_factor, cpr_div_ppr, RPM_to_speed,
            control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args, flux_kd_args,
            torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args, speed_ki_args,
            speed_kd_args, angle_sh_args, vd_args, vq_args, trigger_args, control2_args, fw_kp_args, fw_ki_args,
            //
            id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
            torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Ialpha_stts,
            Ibeta_stts, Ihomopolar_stts, fixed_angle_args);

        T_IO va = Va_out;
        T_IO vb = Vb_out;
        T_IO vc = Vc_out;

        t_glb_q15q16 apx_va = va;
        t_glb_q15q16 apx_vb = vb;
        t_glb_q15q16 apx_vc = vc;

        Va_cmd_stts = apx_va(31, 0);
        Vb_cmd_stts = apx_vb(31, 0);
        Vc_cmd_stts = apx_vc(31, 0);

        if (!Va_cmd.full()) Va_cmd.write(va);
        if (!Vb_cmd.full()) Vb_cmd.write(vb);
        if (!Vc_cmd.full()) Vc_cmd.write(vc);
    } // for(long i = 0; i < trip_cnt; i++)
}

template <int VALUE_CPR, int MAX_IO, int W, int I, typename T_RPM_THETA_FOC>
void hls_foc_strm_int(
    // Input
    hls::stream<int>& Ia,
    hls::stream<int>& Ib,
    hls::stream<int>& Ic,
    hls::stream<T_RPM_THETA_FOC>& FOC_RPM_THETA_m, // RPM & Theta_m
    // Output
    hls::stream<int>& Va_cmd,
    hls::stream<int>& Vb_cmd,
    hls::stream<int>& Vc_cmd,
    // In-out for parameters
    volatile int& ppr_args,
    volatile int& control_mode_args,
    volatile int& control_fixperiod_args,
    volatile int& flux_sp_args,
    volatile int& flux_kp_args,
    volatile int& flux_ki_args,
    volatile int& flux_kd_args,
    volatile int& torque_sp_args,
    volatile int& torque_kp_args,
    volatile int& torque_ki_args,
    volatile int& torque_kd_args,
    volatile int& speed_sp_args,
    volatile int& speed_kp_args,
    volatile int& speed_ki_args,
    volatile int& speed_kd_args,
    volatile int& angle_sh_args,
    volatile int& vd_args,
    volatile int& vq_args,
    volatile int& fw_kp_args,
    volatile int& fw_ki_args,
    //
    volatile int& id_stts,
    volatile int& flux_acc_stts,
    volatile int& flux_err_stts,
    volatile int& flux_out_stts,
    volatile int& iq_stts,
    volatile int& torque_acc_stts,
    volatile int& torque_err_stts,
    volatile int& torque_out_stts,
    volatile int& speed_stts,
    volatile int& speed_acc_stts,
    volatile int& speed_err_stts,
    volatile int& speed_out_stts,
    volatile int& angle_stts,
    volatile int& Va_cmd_stts,
    volatile int& Vb_cmd_stts,
    volatile int& Vc_cmd_stts,
    volatile int& Ialpha_stts,
    volatile int& Ibeta_stts,
    volatile int& Ihomopolar_stts,
    volatile int& fixed_angle_args,
    volatile long& trip_cnt) {
    short cpr_div_ppr = VALUE_CPR / ppr_args;
    unsigned int tab_map_factor = ((COMM_MACRO_TLB_LENTH * (unsigned int)ppr_args) << 16) / VALUE_CPR;
    typedef ap_fixed<W, I> T_M;
    const T_M RPM_factor = 60.0 / 2 / 3.1415926535;
    T_M RPM_to_speed = (t_glb_q15q16)ppr_args / RPM_factor;
LOOP_FOC_STRM:
    for (long i = 0; i < trip_cnt; i++) {
#pragma HLS pipeline II = 5
        static int FOC_RPM_THETA_m_in;
        static int Ia_in0;
        static int Ib_in0;
        static int Ic_in0;
        if (!Ia.empty()) {
            Ia_in0 = Ia.read();
            Ib_in0 = Ib.read();
            Ic_in0 = Ic.read();
        }
        t_glb_q15q16 Ia_in;
        t_glb_q15q16 Ib_in;
        t_glb_q15q16 Ic_in;

        Ia_in(31, 0) = Ia_in0;
        Ib_in(31, 0) = Ib_in0;
        Ic_in(31, 0) = Ic_in0;

        if (!FOC_RPM_THETA_m.empty()) FOC_RPM_THETA_m_in = FOC_RPM_THETA_m.read();

        short RPM_in = (FOC_RPM_THETA_m_in & 0x0000FFFF);
        short Angle_in = (FOC_RPM_THETA_m_in & 0xFFFF0000) >> 16;
        T_M Va_out, Vb_out, Vc_out;

        // reserved word
        // int ppr_args = 0;
        // int cpr_args = 0;
        int trigger_args = 0;
        int control2_args = 0;

        details::foc_core_ap_fixed<VALUE_CPR, T_M, MAX_IO, W, I>(
            (T_M)Ia_in, (T_M)Ib_in, (T_M)Ic_in, RPM_in, Angle_in, Va_out, Vb_out, Vc_out, tab_map_factor, cpr_div_ppr,
            (T_M)RPM_to_speed, control_mode_args, control_fixperiod_args, flux_sp_args, flux_kp_args, flux_ki_args,
            flux_kd_args, torque_sp_args, torque_kp_args, torque_ki_args, torque_kd_args, speed_sp_args, speed_kp_args,
            speed_ki_args, speed_kd_args, angle_sh_args, vd_args, vq_args, trigger_args, control2_args, fw_kp_args,
            fw_ki_args,
            //
            id_stts, flux_acc_stts, flux_err_stts, flux_out_stts, iq_stts, torque_acc_stts, torque_err_stts,
            torque_out_stts, speed_stts, speed_acc_stts, speed_err_stts, speed_out_stts, angle_stts, Ialpha_stts,
            Ibeta_stts, Ihomopolar_stts, fixed_angle_args);

        t_glb_q15q16 apx_va = Va_out;
        t_glb_q15q16 apx_vb = Vb_out;
        t_glb_q15q16 apx_vc = Vc_out;

        Va_cmd_stts = apx_va(31, 0);
        Vb_cmd_stts = apx_vb(31, 0);
        Vc_cmd_stts = apx_vc(31, 0);

        if (!Va_cmd.full()) Va_cmd.write(apx_va(31, 0));
        if (!Vb_cmd.full()) Vb_cmd.write(apx_vb(31, 0));
        if (!Vc_cmd.full()) Vc_cmd.write(apx_vc(31, 0));
    }
}
// clang-format on

} // namespace motorcontrol
} // namespace xf
#endif
