/*Copyright (c) 2012 The Broad Institute

*Permission is hereby granted, free of charge, to any person
*obtaining a copy of this software and associated documentation
*files (the "Software"), to deal in the Software without
*restriction, including without limitation the rights to use,
*copy, modify, merge, publish, distribute, sublicense, and/or sell
*copies of the Software, and to permit persons to whom the
*Software is furnished to do so, subject to the following
*conditions:

*The above copyright notice and this permission notice shall be
*included in all copies or substantial portions of the Software.

*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
*OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
*NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
*HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
*THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef COMMON_HEADERS_H
#define COMMON_HEADERS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

#include <sys/time.h>

//#include <immintrin.h>
//#include <emmintrin.h>
//#include <omp.h>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <fenv.h>

extern uint64_t exceptions_array[128];
extern FILE* g_debug_fptr;
#define STORE_FP_EXCEPTIONS(flagp, exceptions_array)                 \
    fegetexceptflag(&flagp, FE_ALL_EXCEPT | __FE_DENORM);            \
    exceptions_array[FE_INVALID] += ((flagp & FE_INVALID));          \
    exceptions_array[__FE_DENORM] += ((flagp & __FE_DENORM) >> 1);   \
    exceptions_array[FE_DIVBYZERO] += ((flagp & FE_DIVBYZERO) >> 2); \
    exceptions_array[FE_OVERFLOW] += ((flagp & FE_OVERFLOW) >> 3);   \
    exceptions_array[FE_UNDERFLOW] += ((flagp & FE_UNDERFLOW) >> 4); \
    feclearexcept(FE_ALL_EXCEPT | __FE_DENORM);

#define CONVERT_AND_PRINT(X) \
    g_converter.f = (X);     \
    fwrite(&(g_converter.i), 4, 1, g_debug_fptr);

#endif
