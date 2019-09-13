/* dc.h */

/*
  Copyright (C) 2001-2009 Makoto Matsumoto and Takuji Nishimura.
  Copyright (C) 2009 Mutsuo Saito
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DYNAMIC_CREATION
#define DYNAMIC_CREATION

#include <inttypes.h> /* C99 compiler */

typedef struct {
    uint32_t aaa;
    int mm,nn,rr,ww;
    uint32_t wmask,umask,lmask;
    int shift0, shift1, shiftB, shiftC;
    uint32_t maskB, maskC;
    int i;
    uint32_t *state;
}mt_struct;

/* old interface */
void init_dc(uint32_t seed);
mt_struct *get_mt_parameter(int w, int p);
mt_struct *get_mt_parameter_id(int w, int p, int id);
mt_struct **get_mt_parameters(int w, int p, int max_id, int *count);

/* new interface */
mt_struct *get_mt_parameter_st(int w, int p, uint32_t seed);
mt_struct *get_mt_parameter_id_st(int w, int p, int id, uint32_t seed);
mt_struct **get_mt_parameters_st(int w, int p, int start_id, int max_id,
				 uint32_t seed, int *count);
/* common */
void free_mt_struct(mt_struct *mts);
void free_mt_struct_array(mt_struct **mtss, int count);
void sgenrand_mt(uint32_t seed, mt_struct *mts);
uint32_t genrand_mt(mt_struct *mts);

#endif
