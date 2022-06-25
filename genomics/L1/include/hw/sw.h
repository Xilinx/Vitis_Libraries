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
#include <string.h>
#ifndef _SW_H
#define _SW_H

#define GAP -1
#define MATCH 2
#define MISS_MATCH -1
#define ABSMAXCOST MATCH
#define MINVAL -32000

#define UINTSZ sizeof(unsigned int)
#define UINTSZ_K (NUMPACKED * 2) / 8
#define BPSZ 2
#define UINTNUMBP ((UINTSZ * 8) / (BPSZ))
#define UINTNUMBP_K ((UINTSZ_K * 8) / (BPSZ))

#define MAXROW 128
#define MAXCOL 256
#define PACKEDSZ ((MAXROW + MAXCOL)) / (UINTNUMBP)
#define PACKEDSZ_K ((MAXROW + MAXCOL)) / (UINTNUMBP_K)
#define READREFUINTSZ(X, Y) ((((X) + (Y))) / (UINTNUMBP))
#define GMEM_DWIDTH 32

// A-0, C-1, G-2, T-3
const char bases[5] = "ACGT";
#endif
