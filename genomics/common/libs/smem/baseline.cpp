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
#include "baseline.hpp"

#define __occ_aux4(b) \
    (cnt_table[(b)&0xff] + cnt_table[(b) >> 8 & 0xff] + cnt_table[(b) >> 16 & 0xff] + cnt_table[(b) >> 24])

#define bwt_occ_intv(b, k) ((b)->bwt + ((k) >> 7 << 4))

uint64_t bwt_primary_baseline;
uint64_t L2_baseline[5];
double DRAM_trans_size[COMPUTE_UNIT];
int bank_id;

void bwt_occ4(uint32_t* bwt, bwtint_t k, bwtint_t cnt[4]) {
    bwtint_t x;
    uint32_t *p, tmp, *end;
    if (k == (bwtint_t)(-1)) {
        memset(cnt, 0, 4 * sizeof(bwtint_t));
        return;
    }
    k -= (k >= bwt_primary_baseline); // because $ is not in bwt
    p = bwt + ((k) >> 7 << 4);
    memcpy(cnt, p, 4 * sizeof(bwtint_t));
    DRAM_trans_size[bank_id] += 4 * sizeof(bwtint_t);
    p += sizeof(bwtint_t);                                // sizeof(bwtint_t) = 4*(sizeof(bwtint_t)/sizeof(uint32_t))
    end = p + ((k >> 4) - ((k & ~c_occ_intv_mask) >> 4)); // this is the end point of the following loop
    for (x = 0; p < end; ++p) {
        x += __occ_aux4(*p);
        DRAM_trans_size[bank_id] += sizeof(uint32_t);
    }
    tmp = *p & ~((1U << ((~k & 15) << 1)) - 1);
    x += __occ_aux4(tmp) - (~k & 15);
    cnt[0] += x & 0xff;
    cnt[1] += x >> 8 & 0xff;
    cnt[2] += x >> 16 & 0xff;
    cnt[3] += x >> 24;
}

// an analogy to bwt_occ4() but more efficient, requiring k <= l
void bwt_2occ4(uint32_t* bwt, bwtint_t k, bwtint_t l, bwtint_t cntk[4], bwtint_t cntl[4]) {
    bwtint_t _k, _l;
    _k = k - (k >= bwt_primary_baseline);
    _l = l - (l >= bwt_primary_baseline);
    if (_l >> c_occ_intv_shift != _k >> c_occ_intv_shift || k == (bwtint_t)(-1) || l == (bwtint_t)(-1)) {
        bwt_occ4(bwt, k, cntk);
        bwt_occ4(bwt, l, cntl);
    } else {
        bwtint_t x, y;
        uint32_t *p, tmp, *endk, *endl;
        k -= (k >= bwt_primary_baseline); // because $ is not in bwt
        l -= (l >= bwt_primary_baseline);
        p = bwt + ((k) >> 7 << 4);
        memcpy(cntk, p, 4 * sizeof(bwtint_t));
        DRAM_trans_size[bank_id] += 4 * sizeof(bwtint_t);
        p += sizeof(bwtint_t); // sizeof(bwtint_t) = 4*(sizeof(bwtint_t)/sizeof(uint32_t))
        // prepare cntk[]
        endk = p + ((k >> 4) - ((k & ~c_occ_intv_mask) >> 4));
        endl = p + ((l >> 4) - ((l & ~c_occ_intv_mask) >> 4));
        for (x = 0; p < endk; ++p) {
            x += __occ_aux4(*p);
            DRAM_trans_size[bank_id] += sizeof(uint32_t);
        }
        y = x;
        tmp = *p & ~((1U << ((~k & 15) << 1)) - 1);
        DRAM_trans_size[bank_id] += sizeof(uint32_t);
        x += __occ_aux4(tmp) - (~k & 15);
        // calculate cntl[] and finalize cntk[]
        for (; p < endl; ++p) {
            y += __occ_aux4(*p);
            DRAM_trans_size[bank_id] += sizeof(uint32_t);
        }
        tmp = *p & ~((1U << ((~l & 15) << 1)) - 1);
        DRAM_trans_size[bank_id] += sizeof(uint32_t);
        y += __occ_aux4(tmp) - (~l & 15);
        memcpy(cntl, cntk, 4 * sizeof(bwtint_t));
        cntk[0] += x & 0xff;
        cntk[1] += x >> 8 & 0xff;
        cntk[2] += x >> 16 & 0xff;
        cntk[3] += x >> 24;
        cntl[0] += y & 0xff;
        cntl[1] += y >> 8 & 0xff;
        cntl[2] += y >> 16 & 0xff;
        cntl[3] += y >> 24;
    }
}

/*********************
 * Bidirectional BWT *
 *********************/

void bwt_extend(uint32_t* bwt, const bwtintv_t* ik, bwtintv_t ok[4], int is_back) {
    bwtint_t tk[4], tl[4];
    int i;
    bwt_2occ4(bwt, ik->x[!is_back] - 1, ik->x[!is_back] - 1 + ik->x[2], tk, tl);
    for (i = 0; i != 4; ++i) {
        ok[i].x[!is_back] = L2_baseline[i] + 1 + tk[i];
        ok[i].x[2] = tl[i] - tk[i];
    }
    ok[3].x[is_back] = ik->x[is_back] + (ik->x[!is_back] <= bwt_primary_baseline &&
                                         ik->x[!is_back] + ik->x[2] - 1 >= bwt_primary_baseline);
    ok[2].x[is_back] = ok[3].x[is_back] + ok[3].x[2];
    ok[1].x[is_back] = ok[2].x[is_back] + ok[2].x[2];
    ok[0].x[is_back] = ok[1].x[is_back] + ok[1].x[2];
}

static void bwt_reverse_intvs(bwtintv_v* p) {
    if (p->n > 1) {
        int j;
        for (j = 0; j<p->n>> 1; ++j) {
            bwtintv_t tmp = p->a[p->n - 1 - j];
            p->a[p->n - 1 - j] = p->a[j];
            p->a[j] = tmp;
        }
    }
}

// NOTE: $max_intv is not currently used in BWA-MEM
int bwt_smem1a(uint32_t* bwt,
               int len,
               const uint8_t* q,
               int x,
               int min_intv,
               uint64_t max_intv,
               bwtintv_v* mem,
               bwtintv_v* tmpvec[2]) {
    int i, j, c, ret;
    bwtintv_t ik, ok[4];
    bwtintv_v a[2], *prev, *curr, *swap;

    mem->n = 0;
    if (q[x] > 3) return x + 1;
    if (min_intv < 1) min_intv = 1; // the interval size should be at least 1
    kv_init(a[0]);
    kv_init(a[1]);
    prev = tmpvec && tmpvec[0] ? tmpvec[0] : &a[0]; // use the temporary vector if provided
    curr = tmpvec && tmpvec[1] ? tmpvec[1] : &a[1];
    bwt_set_intv1(q[x], ik); // the initial interval of a single base
    ik.info = x + 1;

    for (i = x + 1, curr->n = 0; i < len; ++i) { // forward search
        if (ik.x[2] < max_intv) {                // an interval small enough
            kv_push(bwtintv_t, *curr, ik);
            break;
        } else if (q[i] < 4) { // an A/C/G/T base
            c = 3 - q[i];      // complement of q[i]
            bwt_extend(bwt, &ik, ok, 0);
            if (ok[c].x[2] != ik.x[2]) { // change of the interval size
                kv_push(bwtintv_t, *curr, ik);
                if (ok[c].x[2] < min_intv) break; // the interval size is too small to be extended further
            }
            ik = ok[c];
            ik.info = i + 1;
        } else { // an ambiguous base
            kv_push(bwtintv_t, *curr, ik);
            break; // always terminate extension at an ambiguous base; in this case, i<len always stands
        }
    }
    if (i == len) kv_push(bwtintv_t, *curr, ik); // push the last interval if we reach the end
    bwt_reverse_intvs(curr);                     // s.t. smaller intervals (i.e. longer matches) visited first
    ret = curr->a[0].info;                       // this will be the returned value
    swap = curr;
    curr = prev;
    prev = swap;

    for (i = x - 1; i >= -1; --i) {            // backward search for MEMs
        c = i < 0 ? -1 : q[i] < 4 ? q[i] : -1; // c==-1 if i<0 or q[i] is an ambiguous base
        for (j = 0, curr->n = 0; j < prev->n; ++j) {
            bwtintv_t* p = &prev->a[j];
            if (c >= 0 && ik.x[2] >= max_intv) bwt_extend(bwt, p, ok, 1);
            if (c < 0 || ik.x[2] < max_intv || ok[c].x[2] < min_intv) { // keep the hit if reaching the beginning or an
                                                                        // ambiguous base or the intv is small enough
                if (curr->n == 0) { // test curr->n>0 to make sure there are no longer matches
                    if (mem->n == 0 || i + 1 < mem->a[mem->n - 1].info >> 32) { // skip contained matches
                        ik = *p;
                        ik.info |= (uint64_t)(i + 1) << 32;
                        kv_push(bwtintv_t, *mem, ik);
                    }
                } // otherwise the match is contained in another longer match
            } else if (curr->n == 0 || ok[c].x[2] != curr->a[curr->n - 1].x[2]) {
                ok[c].info = p->info;
                kv_push(bwtintv_t, *curr, ok[c]);
            }
        }
        if (curr->n == 0) break;
        swap = curr;
        curr = prev;
        prev = swap;
    }
    bwt_reverse_intvs(mem); // s.t. sorted by the start coordinate

    if (tmpvec == 0 || tmpvec[0] == 0) free(a[0].a);
    if (tmpvec == 0 || tmpvec[1] == 0) free(a[1].a);
    return ret;
}

int bwt_smem1a_new(uint32_t* bwt,
                   int len,
                   const uint8_t* q,
                   int x,
                   int min_intv,
                   uint64_t max_intv,
                   bwtintv_v* mem,
                   bwtintv_v* tmpvec[2]) {
    int i, j, c, ret;
    bwtintv_t ik, ok[4];
    bwtintv_v a[2], *curr;
    int start = x;
    int stop = x;
    bwtintv_v* back_intv;
    bwtintv_t temp;
    int k = 0;
    int m = 0;
    int isbreak = 0;
    // forward search
    if (q[x] > 3) return x + 1;
    if (min_intv < 1) min_intv = 1; // the interval size should be at least 1
    kv_init(a[0]);
    kv_init(a[1]);
    back_intv = &a[0];
    curr = &a[1];
    bwt_set_intv1(q[x], ik); // the initial interval of a single base
    ik.info = x + 1;

    for (i = x + 1, curr->n = 0; i < len; ++i) { // forward search
        if (ik.x[2] < max_intv) {                // an interval small enough
            kv_push(bwtintv_t, *curr, ik);
            break;
        } else if (q[i] < 4) { // an A/C/G/T base
            c = 3 - q[i];      // complement of q[i]
            bwt_extend(bwt, &ik, ok, 0);
            if (ok[c].x[2] != ik.x[2]) { // change of the interval size
                kv_push(bwtintv_t, *curr, ik);
                if (ok[c].x[2] < min_intv) break; // the interval size is too small to be extended further
            }
            ik = ok[c];
            ik.info = i + 1;
        } else { // an ambiguous base
            kv_push(bwtintv_t, *curr, ik);
            break; // always terminate extension at an ambiguous base; in this case, i<len always stands
        }
    }
    if (i == len) kv_push(bwtintv_t, *curr, ik); // push the last interval if we reach the end
    ret = curr->a[curr->n - 1].info;             // this will be the returned value
    // the new backward search
    i = 0;
    while (i < curr->n) {
        ik = curr->a[i];
        ik.info |= (uint64_t)(x) << 32;
        // backenlarge
        if (back_intv->n == 0 || stop - start >= 3) {
            back_intv->n = 0;
            kv_push(bwtintv_t, *back_intv, ik);
            for (k = x - 1; k >= -1; --k) {
                c = k < 0 ? -1 : q[k] < 4 ? q[k] : -1;
                if (c >= 0 && ik.x[2] >= max_intv) {
                    bwt_extend(bwt, &ik, ok, 1);
                    if (ok[c].x[2] < min_intv) break;
                    ik = ok[c];
                    ik.info = curr->a[i].info | (uint64_t)(k) << 32;
                    kv_push(bwtintv_t, *back_intv, ik);
                } else {
                    break;
                }
            }
            start = (int)curr->a[i].info;
            if (i == curr->n - 1)
                stop = len;
            else {
                stop = (int)curr->a[i + 1].info;
            }
            if (i == 0)
                temp = ik;
            else if (ik.info >> 32 > temp.info >> 32 && (int)temp.info - (temp.info >> 32) >= c_min_seed_len) {
                kv_push(bwtintv_t, *mem, temp);
                temp = ik;
            } else
                temp = ik;
        }
        // forwardenlarge
        else {
            stop = (int)curr->a[i].info;
            for (k = back_intv->n - 1; k >= 0; k--) {
                ik = back_intv->a[k];
                for (m = start + 1; m <= stop; m++) {
                    c = 3 - q[m - 1];
                    bwt_extend(bwt, &ik, ok, 0);
                    if (ok[c].x[2] < min_intv) break;
                    ik = ok[c];
                    if (m == stop) {
                        ik.info = curr->a[i].info | (uint64_t)(x - k) << 32;
                        isbreak = 1;
                    }
                }
                if (isbreak == 1) {
                    isbreak = 0;
                    if ((x - k) > temp.info >> 32 && (int)temp.info - (temp.info >> 32) >= c_min_seed_len) {
                        kv_push(bwtintv_t, *mem, temp);
                        temp = ik;
                    } else
                        temp = ik;
                    break;
                }
            }
        }
        i = i + 1;
        // compute the max len of next iteration
        int max_len;
        if (i < curr->n) max_len = (temp.info >> 32) + (int)curr->a[i].info;
        while (max_len < c_min_seed_len && i < curr->n) {
            i = i + 1;
            if (i < curr->n) stop = (int)curr->a[i].info;
            max_len = (temp.info >> 32) + stop;
        }
        if (i >= curr->n && (int)temp.info - (temp.info >> 32) >= c_min_seed_len) kv_push(bwtintv_t, *mem, temp);
    }
    free(back_intv->a);
    free(a[1].a);
    // if (tmpvec == 0 || tmpvec[0] == 0) free(a[0].a);
    // if (tmpvec == 0 || tmpvec[1] == 0) free(a[1].a);
    return ret;
}

int bwt_seed_strategy1(uint32_t* bwt, int len, const uint8_t* q, int x, int min_len, int max_intv, bwtintv_t* mem) {
    int i, c;
    bwtintv_t ik, ok[4];

    memset(mem, 0, sizeof(bwtintv_t));
    if (q[x] > 3) return x + 1;
    bwt_set_intv1(q[x], ik);        // the initial interval of a single base
    for (i = x + 1; i < len; ++i) { // forward search
        if (q[i] < 4) {             // an A/C/G/T base
            c = 3 - q[i];           // complement of q[i]
            bwt_extend(bwt, &ik, ok, 0);
            if (ok[c].x[2] < max_intv && i - x >= min_len) {
                *mem = ok[c];
                mem->info = (uint64_t)x << 32 | (i + 1);
                return i + 1;
            }
            ik = ok[c];
        } else
            return i + 1;
    }
    return len;
}

int bwt_smem1(uint32_t* bwt, int len, const uint8_t* q, int x, int min_intv, bwtintv_v* mem, bwtintv_v* tmpvec[2]) {
    return bwt_smem1a(bwt, len, q, x, min_intv, 0, mem, tmpvec);
}

static void mem_collect_intv(uint32_t* bwt, int len, const uint8_t* seq, smem_aux_t* a) {
    int i, k, x = 0, old_n;
    int start_width = 1;
    int split_len = 28;
    int split_width = 10;
    int max_mem_intv = 20;

    a->mem.n = 0;
    // first pass: find all SMEMs
    while (x < len) {
        if (seq[x] < 4) {
            x = bwt_smem1(bwt, len, seq, x, start_width, &a->mem1, a->tmpv);
            for (i = 0; i < a->mem1.n; ++i) {
                bwtintv_t* p = &a->mem1.a[i];
                int slen = (uint32_t)p->info - (p->info >> 32); // seed length
                if (slen >= c_min_seed_len) kv_push(bwtintv_t, a->mem, *p);
            }
        } else
            ++x;
    }
    // second pass: find MEMs inside a long SMEM
    old_n = a->mem.n;
    for (k = 0; k < old_n; ++k) {
        bwtintv_t* p = &a->mem.a[k];
        int start = p->info >> 32, end = (int32_t)p->info;
        if (end - start < split_len || p->x[2] > split_width) continue;
        bwt_smem1(bwt, len, seq, (start + end) >> 1, p->x[2] + 1, &a->mem1, a->tmpv);
        for (i = 0; i < a->mem1.n; ++i)
            if ((uint32_t)a->mem1.a[i].info - (a->mem1.a[i].info >> 32) >= c_min_seed_len)
                kv_push(bwtintv_t, a->mem, a->mem1.a[i]);
    }
    // third pass: LAST-like
    if (max_mem_intv > 0) {
        x = 0;
        while (x < len) {
            if (seq[x] < 4) {
                if (1) {
                    bwtintv_t m;
                    x = bwt_seed_strategy1(bwt, len, seq, x, c_min_seed_len, max_mem_intv, &m);
                    if (m.x[2] > 0) kv_push(bwtintv_t, a->mem, m);
                } else { // for now, we never come to this block which is slower
                    x = bwt_smem1a(bwt, len, seq, x, start_width, max_mem_intv, &a->mem1, a->tmpv);
                    for (i = 0; i < a->mem1.n; ++i) kv_push(bwtintv_t, a->mem, a->mem1.a[i]);
                }
            } else
                ++x;
        }
    }
    // sort
    // ks_introsort(mem_intv, a->mem.n, a->mem.a);
}

void mem_collect_intv_new(uint32_t* bwt, int len, const uint8_t* seq, smem_aux_t* a) {
    int i, k, x = 0, old_n;
    int start_width = 1;
    int split_len = 28;
    int split_width = 10;
    int max_mem_intv = 20;
    a->mem.n = 0;
    // first pass: find all SMEMs
    while (x < len) {
        if (seq[x] < 4) {
            x = bwt_smem1a_new(bwt, len, seq, x, start_width, 0, &a->mem, a->tmpv);
        } else
            ++x;
    }
    // second pass: find MEMs inside a long SMEM
    old_n = a->mem.n;
    for (k = 0; k < old_n; ++k) {
        bwtintv_t* p = &a->mem.a[k];
        int start = p->info >> 32, end = (int32_t)p->info;
        if (end - start < split_len || p->x[2] > split_width) continue;
        bwt_smem1a_new(bwt, len, seq, (start + end) >> 1, p->x[2] + 1, 0, &a->mem, a->tmpv);
    }
    // third pass: LAST-like
    x = 0;
    while (x < len) {
        if (seq[x] < 4) {
            bwtintv_t m;
            x = bwt_seed_strategy1(bwt, len, seq, x, c_min_seed_len, max_mem_intv, &m);
            if (m.x[2] > 0) kv_push(bwtintv_t, a->mem, m);
        } else
            ++x;
    }
    // sort
    // ks_introsort(mem_intv, a->mem.n, a->mem.a);
}

int smem_baseline(const uint32_t* bwt,
                  const uint64_t* bwt_para,
                  uint8_t* seq,
                  uint8_t* seq_len,
                  int batch_size,
                  bwtintv_t* mem_output,
                  int* mem_num,
                  double mem_request_size[COMPUTE_UNIT]) {
    bwt_primary_baseline = bwt_para[0];
    L2_baseline[0] = bwt_para[1];
    L2_baseline[1] = bwt_para[2];
    L2_baseline[2] = bwt_para[3];
    L2_baseline[3] = bwt_para[4];
    L2_baseline[4] = bwt_para[5];
    for (int i = 0; i < COMPUTE_UNIT; i++) {
        DRAM_trans_size[i] = 0;
    }
    bank_id = 0;
    int bank_seg_size = batch_size / COMPUTE_UNIT;
    int count = 0;
    for (int i = 0; i < batch_size; i++) {
        smem_aux_t mem_aux;
        kv_init(mem_aux.mem);
        kv_init(mem_aux.mem1);
        mem_collect_intv_new((uint32_t*)bwt, seq_len[i], seq + i * c_seq_length, &mem_aux);
        mem_num[i] = mem_aux.mem.n;
        // printf("batch %d, number of intv is %d, seq_len is %d\n", i, mem_num[i], seq_len[i]);
        int mem_n_amend = mem_aux.mem.n;
        if (mem_n_amend > c_max_intv_alloc) {
            mem_n_amend = c_max_intv_alloc;
        }
        for (int j = 0; j < mem_n_amend; j++) {
            mem_output[i * c_max_intv_alloc + j] = mem_aux.mem.a[j];
        }
        if (count == bank_seg_size - 1) {
            count = 0;
            bank_id++;
        } else
            count++;
    }
    for (int i = 0; i < COMPUTE_UNIT; i++) {
        mem_request_size[i] = DRAM_trans_size[i];
    }
    return 0;
}
