// Copyright 2011 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Paginated token buffer
//
//  A 'token' is a bit value associated with a probability, either fixed
// or a later-to-be-determined after statistics have been collected.
// For dynamic probability, we just record the slot id (idx) for the probability
// value in the final probability array (uint8_t* probas in VP8EmitTokens).
//
// Author: Skal (pascal.massimino@gmail.com)

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "./cost.h"
#include "./vp8enci.h"
#include "../utils/utils.h"
#include "../utils/profiling.h"

#if !defined(DISABLE_TOKEN_BUFFER)

// we use pages to reduce the number of memcpy()
#define MIN_PAGE_SIZE 8192 // minimum number of token per page
#define FIXED_PROBA_BIT (1u << 14)

typedef uint16_t token_t; // bit #15: bit value
                          // bit #14: flags for constant proba or idx
                          // bits #0..13: slot or constant proba
struct VP8Tokens {
    VP8Tokens* next_; // pointer to next page
};
// Token data is located in memory just after the next_ field.
// This macro is used to return their address and hide the trick.
#define TOKEN_DATA(p) ((const token_t*)&(p)[1])

//------------------------------------------------------------------------------
// Token buffer
#define DEBUG_TOKENS 0
void debug_tokens(VP8TBuffer* const b, VP8EncIterator* const it_var) {
#if DEBUG_TOKENS
    const VP8Tokens* p = b->pages_;

    printf("\n[%d][%d]\n", it_var->y_, it_var->x_);
    int page_index = 0;
    while (p != NULL) {
        const VP8Tokens* const next = p->next_;
        // printf("%s p:%d next:%d\n", __FUNCTION__, p, next);
        int n = b->page_size_;
        const token_t* const tokens = TOKEN_DATA(p);
        int i = 0;
        printf("page:%d \n", page_index++);
        for (i = 0; i < n; i++) {
            printf("%4d ", tokens[i]);
            if (i % 30 == 0) {
                printf("\n");
            }
        }
        p = next;
        printf("\n");
    }
    printf("\n");
#endif
}
static int TBufferNewPageFromKernel(VP8TBuffer* const b, VP8TBufferKernel* const tokens_kernel, int tokens_index) {
    VP8Tokens* page = NULL;
    const size_t size = PAGE_SIZE;

    static int i = 0;
    // printf("i:%d size:%d sizeof(*page):%d b->page_size_:%d sizeof(token_t):%d\n",
    //     i, size, sizeof(*page), b->page_size_, sizeof(token_t));
    i++;
    page = (VP8Tokens*)WebPSafeMalloc(1ULL, size);

    if (page == NULL) {
        printf("Failed to allocate pages[%d]\n", tokens_index);
        return 0;
    }

    // static int j = 0;
    // if (b->pages_ != NULL && b->tokens_ != NULL) {
    // printf("jj:%d page:%d b:%d b->pages:%d b->pages_->next_:%d b->last_page_:%d *(b->last_page_):%d  b->left_:%d
    // b->tokens_:%d sizeof(page->next_):%d sizeof(unsigned long long):%d\n",
    //     j, page, b, b->pages_, b->pages_->next_, b->last_page_ , *(b->last_page_), b->left_, b->tokens_,
    //     sizeof(page->next_), sizeof(unsigned long long));
    // }

    page->next_ = NULL;

    *b->last_page_ = page;
    b->last_page_ = &page->next_;
    // b->left_ = b->page_size_;
    b->tokens_ = (token_t*)TOKEN_DATA(page);

    // printf("j:%d page:%d b:%d b->pages:%d b->pages_->next_:%d b->last_page_:%d *(b->last_page_):%d  b->left_:%d
    // b->tokens_:%d sizeof(page->next_):%d sizeof(unsigned long long):%d\n\n",
    //     j, page, b, b->pages_, b->pages_->next_, b->last_page_ , *(b->last_page_), b->left_, b->tokens_,
    //     sizeof(page->next_), sizeof(unsigned long long));
    // j++;

    memcpy(b->tokens_, tokens_kernel->tokens_ + tokens_index * TOKENS_COUNT_PER_PAGE,
           TOKENS_COUNT_PER_PAGE * TOKENS_SIZE);
    b->left_ = tokens_kernel->left_;
    b->error_ = tokens_kernel->error_;
    b->page_size_ = tokens_kernel->page_size_;
    return 1;
}

void ReadTokenFromKernel(VP8TBuffer* const b, VP8TBufferKernel* const tokens_kernel) {
    int tokens_index = 0;
    for (tokens_index = 0; tokens_index < tokens_kernel->page_count_; tokens_index++) {
        if (0 == TBufferNewPageFromKernel(b, tokens_kernel, tokens_index)) {
            printf("ReadTokenFromKernel: Failed to allocate pages[%d]\n", tokens_index);
        }
    }
}

#if TOKEN_RECONSTRUCT
void VP8TBufferInit(VP8TBuffer* const b, int page_size) {
    b->tokens_ = NULL;
    b->pages_ = NULL;
    b->last_page_ = &b->pages_;
    b->left_ = 0;
    b->page_size_ = (page_size < MIN_PAGE_SIZE) ? MIN_PAGE_SIZE : page_size;
    b->error_ = 0;
}

void VP8TBufferKernelInit(VP8TBufferKernel* const b, int page_size) {
    // b->tokens_ = NULL;
    // b->pages_ = NULL;
    // b->last_page_ = &b->pages_;
    memset(b->tokens_, 0, PAGE_COUNT * TOKENS_COUNT_PER_PAGE * sizeof(token_t));
    b->cur_page_ = 0;
    b->page_count_ = 0;
    b->left_ = 0;
    b->page_size_ = (page_size < MIN_PAGE_SIZE) ? MIN_PAGE_SIZE : page_size;
    b->error_ = 0;
}

void VP8TBufferClear(VP8TBuffer* const b) {
    if (b != NULL) {
        VP8Tokens* p = b->pages_;
        while (p != NULL) {
            VP8Tokens* const next = p->next_;
            WebPSafeFree(p);
            p = next;
        }
        VP8TBufferInit(b, b->page_size_);
    }
}

static int TBufferNewPageFromKernel(VP8TBuffer* const b, VP8TBufferKernel* const tokens_kernel, int tokens_index) {
    VP8Tokens* page = NULL;
    const size_t size = PAGE_SIZE;

    static int i = 0;
    // printf("i:%d size:%d sizeof(*page):%d b->page_size_:%d sizeof(token_t):%d\n",
    //     i, size, sizeof(*page), b->page_size_, sizeof(token_t));
    i++;
    page = (VP8Tokens*)WebPSafeMalloc(1ULL, size);

    if (page == NULL) {
        printf("Failed to allocate pages[%d]\n", tokens_index);
        return 0;
    }

    // static int j = 0;
    // if (b->pages_ != NULL && b->tokens_ != NULL) {
    // printf("jj:%d page:%d b:%d b->pages:%d b->pages_->next_:%d b->last_page_:%d *(b->last_page_):%d  b->left_:%d
    // b->tokens_:%d sizeof(page->next_):%d sizeof(unsigned long long):%d\n",
    //     j, page, b, b->pages_, b->pages_->next_, b->last_page_ , *(b->last_page_), b->left_, b->tokens_,
    //     sizeof(page->next_), sizeof(unsigned long long));
    // }

    page->next_ = NULL;

    *b->last_page_ = page;
    b->last_page_ = &page->next_;
    // b->left_ = b->page_size_;
    b->tokens_ = (token_t*)TOKEN_DATA(page);

    // printf("j:%d page:%d b:%d b->pages:%d b->pages_->next_:%d b->last_page_:%d *(b->last_page_):%d  b->left_:%d
    // b->tokens_:%d sizeof(page->next_):%d sizeof(unsigned long long):%d\n\n",
    //     j, page, b, b->pages_, b->pages_->next_, b->last_page_ , *(b->last_page_), b->left_, b->tokens_,
    //     sizeof(page->next_), sizeof(unsigned long long));
    // j++;

    memcpy(b->tokens_, tokens_kernel->tokens_ + tokens_index * TOKENS_COUNT_PER_PAGE,
           TOKENS_COUNT_PER_PAGE * TOKENS_SIZE);
    b->left_ = tokens_kernel->left_;
    b->error_ = tokens_kernel->error_;
    b->page_size_ = tokens_kernel->page_size_;
    return 1;
}

void ReadTokenFromKernel(VP8TBuffer* const b, VP8TBufferKernel* const tokens_kernel) {
    int tokens_index = 0;
    for (tokens_index = 0; tokens_index < tokens_kernel->page_count_; tokens_index++) {
        if (0 == TBufferNewPageFromKernel(b, tokens_kernel, tokens_index)) {
            printf("ReadTokenFromKernel: Failed to allocate pages[%d]\n", tokens_index);
        }
    }
}

static int TBufferNewPage(VP8TBufferKernel* const b) {
    // VP8Tokens* page = NULL;
    if (!b->error_) {
        if (0 != b->page_count_) {
            b->cur_page_++;
        }

        b->page_count_++;
        printf("%s %s %d b->cur_page_:%d b->page_count_:%d\n", __FILE__, __FUNCTION__, __LINE__, b->cur_page_,
               b->page_count_);
    }
    if (b->page_count_ >= PAGE_COUNT) {
        b->error_ = 1;
        return 0;
    }

    b->left_ = b->page_size_;

    return 1;
}

//------------------------------------------------------------------------------

#define TOKEN_ID(t, b, ctx) (NUM_PROBAS * ((ctx) + NUM_CTX * ((b) + NUM_BANDS * (t))))

static WEBP_INLINE uint32_t AddToken(VP8TBufferKernel* const b, uint32_t bit, uint32_t proba_idx) {
    assert(proba_idx < FIXED_PROBA_BIT);
    assert(bit <= 1);
    if (b->left_ > 0 || TBufferNewPage(b)) {
        const int slot = --b->left_;
        b->tokens_[b->cur_page_ * TOKENS_COUNT_PER_PAGE + slot] = (bit << 15) | proba_idx;
    }
    return bit;
}

static WEBP_INLINE void AddConstantToken(VP8TBufferKernel* const b, uint32_t bit, uint32_t proba) {
    assert(proba < 256);
    assert(bit <= 1);
    if (b->left_ > 0 || TBufferNewPage(b)) {
        const int slot = --b->left_;
        b->tokens_[b->cur_page_ * TOKENS_COUNT_PER_PAGE + slot] = (bit << 15) | FIXED_PROBA_BIT | proba;
    }
}

int VP8RecordCoeffTokens(const int ctx,
                         const int coeff_type,
                         int first,
                         int last,
                         const int16_t* const coeffs,
                         VP8TBufferKernel* const tokens) {
    int n = first;
    uint32_t base_id = TOKEN_ID(coeff_type, n, ctx);
    if (!AddToken(tokens, last >= 0, base_id + 0)) {
        return 0;
    }

    while (n < 16) {
        const int c = coeffs[n++];
        const int sign = c < 0;
        const uint32_t v = sign ? -c : c;
        if (!AddToken(tokens, v != 0, base_id + 1)) {
            base_id = TOKEN_ID(coeff_type, VP8EncBands[n], 0); // ctx=0
            continue;
        }
        if (!AddToken(tokens, v > 1, base_id + 2)) {
            base_id = TOKEN_ID(coeff_type, VP8EncBands[n], 1); // ctx=1
        } else {
            if (!AddToken(tokens, v > 4, base_id + 3)) {
                if (AddToken(tokens, v != 2, base_id + 4)) AddToken(tokens, v == 4, base_id + 5);
            } else if (!AddToken(tokens, v > 10, base_id + 6)) {
                if (!AddToken(tokens, v > 6, base_id + 7)) {
                    AddConstantToken(tokens, v == 6, 159);
                } else {
                    AddConstantToken(tokens, v >= 9, 165);
                    AddConstantToken(tokens, !(v & 1), 145);
                }
            } else {
                int mask;
                const uint8_t* tab;
                uint32_t residue = v - 3;
                if (residue < (8 << 1)) { // VP8Cat3  (3b)
                    AddToken(tokens, 0, base_id + 8);
                    AddToken(tokens, 0, base_id + 9);
                    residue -= (8 << 0);
                    mask = 1 << 2;
                    tab = VP8Cat3;
                } else if (residue < (8 << 2)) { // VP8Cat4  (4b)
                    AddToken(tokens, 0, base_id + 8);
                    AddToken(tokens, 1, base_id + 9);
                    residue -= (8 << 1);
                    mask = 1 << 3;
                    tab = VP8Cat4;
                } else if (residue < (8 << 3)) { // VP8Cat5  (5b)
                    AddToken(tokens, 1, base_id + 8);
                    AddToken(tokens, 0, base_id + 10);
                    residue -= (8 << 2);
                    mask = 1 << 4;
                    tab = VP8Cat5;
                } else { // VP8Cat6 (11b)
                    AddToken(tokens, 1, base_id + 8);
                    AddToken(tokens, 1, base_id + 10);
                    residue -= (8 << 3);
                    mask = 1 << 10;
                    tab = VP8Cat6;
                }
                while (mask) {
                    AddConstantToken(tokens, !!(residue & mask), *tab++);
                    mask >>= 1;
                }
            }
            base_id = TOKEN_ID(coeff_type, VP8EncBands[n], 2); // ctx=2
        }
        AddConstantToken(tokens, sign, 128);
        if (n == 16 || !AddToken(tokens, n <= last, base_id + 0)) {
            return 1; // EOB
        }
    }
    return 1;
}

#else

//------------------------------------------------------------------------------

void VP8TBufferInit(VP8TBuffer* const b, int page_size) {
    b->tokens_ = NULL;
    b->pages_ = NULL;
    b->last_page_ = &b->pages_;
    b->left_ = 0;
    b->page_size_ = (page_size < MIN_PAGE_SIZE) ? MIN_PAGE_SIZE : page_size;
    b->error_ = 0;
}

void VP8TBufferClear(VP8TBuffer* const b) {
    if (b != NULL) {
        VP8Tokens* p = b->pages_;
        while (p != NULL) {
            VP8Tokens* const next = p->next_;
            WebPSafeFree(p);
            p = next;
        }
        VP8TBufferInit(b, b->page_size_);
    }
}

#include <stdio.h>
static int TBufferNewPage(VP8TBuffer* const b) {
    VP8Tokens* page = NULL;
    if (!b->error_) {
        const size_t size = sizeof(*page) + b->page_size_ * sizeof(token_t);

        static int i = 0;
        /* printf("i:%d size:%d sizeof(*page):%d b->page_size_:%d sizeof(token_t):%d\n", */
        /*     i, size, sizeof(*page), b->page_size_, sizeof(token_t)); */
        i++;
        page = (VP8Tokens*)WebPSafeMalloc(1ULL, size);
    }
    if (page == NULL) {
        b->error_ = 1;
        return 0;
    }

    // static int j = 0;
    // if (b->pages_ != NULL && b->tokens_ != NULL) {
    // printf("jj:%d page:%d b:%d b->pages:%d b->pages_->next_:%d b->last_page_:%d *(b->last_page_):%d  b->left_:%d
    // b->tokens_:%d sizeof(page->next_):%d sizeof(unsigned long long):%d\n",
    //     j, page, b, b->pages_, b->pages_->next_, b->last_page_ , *(b->last_page_), b->left_, b->tokens_,
    //     sizeof(page->next_), sizeof(unsigned long long));
    // }

    page->next_ = NULL;

    *b->last_page_ = page;
    b->last_page_ = &page->next_;
    b->left_ = b->page_size_;
    b->tokens_ = (token_t*)TOKEN_DATA(page);

    // printf("j:%d page:%d b:%d b->pages:%d b->pages_->next_:%d b->last_page_:%d *(b->last_page_):%d  b->left_:%d
    // b->tokens_:%d sizeof(page->next_):%d sizeof(unsigned long long):%d\n\n",
    //     j, page, b, b->pages_, b->pages_->next_, b->last_page_ , *(b->last_page_), b->left_, b->tokens_,
    //     sizeof(page->next_), sizeof(unsigned long long));
    // //(*(b->last_page_))->next_:%d
    // j++;
    return 1;
}

//------------------------------------------------------------------------------

#define TOKEN_ID(t, b, ctx) (NUM_PROBAS * ((ctx) + NUM_CTX * ((b) + NUM_BANDS * (t))))

static WEBP_INLINE uint32_t AddToken(VP8TBuffer* const b, uint32_t bit, uint32_t proba_idx) {
    assert(proba_idx < FIXED_PROBA_BIT);
    assert(bit <= 1);
    if (b->left_ > 0 || TBufferNewPage(b)) {
        const int slot = --b->left_;

        // printf("0 %s %d b->tokens_[slot] :%d %d %d %d\n",
        //        __FUNCTION__, __LINE__, b->tokens_[slot], bit<<15, proba_idx, (bit << 15) | proba_idx);
        b->tokens_[slot] = (bit << 15) | proba_idx;
        // printf("1 %s %d b->tokens_[slot] :%d %d %d %d\n",
        //        __FUNCTION__, __LINE__, b->tokens_[slot], bit<<15, proba_idx, (bit << 15) | proba_idx);
    }
    return bit;
}

static WEBP_INLINE void AddConstantToken(VP8TBuffer* const b, uint32_t bit, uint32_t proba) {
    assert(proba < 256);
    assert(bit <= 1);
    if (b->left_ > 0 || TBufferNewPage(b)) {
        const int slot = --b->left_;
        b->tokens_[slot] = (bit << 15) | FIXED_PROBA_BIT | proba;
    }
}

int VP8RecordCoeffTokens(
    const int ctx, const int coeff_type, int first, int last, const int16_t* const coeffs, VP8TBuffer* const tokens) {
    int n = first;
    uint32_t base_id = TOKEN_ID(coeff_type, n, ctx);
    if (!AddToken(tokens, last >= 0, base_id + 0)) {
        return 0;
    }

    while (n < 16) {
        const int c = coeffs[n++];
        const int sign = c < 0;
        const uint32_t v = sign ? -c : c;
        if (!AddToken(tokens, v != 0, base_id + 1)) {
            base_id = TOKEN_ID(coeff_type, VP8EncBands[n], 0); // ctx=0
            continue;
        }
        if (!AddToken(tokens, v > 1, base_id + 2)) {
            base_id = TOKEN_ID(coeff_type, VP8EncBands[n], 1); // ctx=1
        } else {
            if (!AddToken(tokens, v > 4, base_id + 3)) {
                if (AddToken(tokens, v != 2, base_id + 4)) AddToken(tokens, v == 4, base_id + 5);
            } else if (!AddToken(tokens, v > 10, base_id + 6)) {
                if (!AddToken(tokens, v > 6, base_id + 7)) {
                    AddConstantToken(tokens, v == 6, 159);
                } else {
                    AddConstantToken(tokens, v >= 9, 165);
                    AddConstantToken(tokens, !(v & 1), 145);
                }
            } else {
                int mask;
                const uint8_t* tab;
                uint32_t residue = v - 3;
                if (residue < (8 << 1)) { // VP8Cat3  (3b)
                    AddToken(tokens, 0, base_id + 8);
                    AddToken(tokens, 0, base_id + 9);
                    residue -= (8 << 0);
                    mask = 1 << 2;
                    tab = VP8Cat3;
                } else if (residue < (8 << 2)) { // VP8Cat4  (4b)
                    AddToken(tokens, 0, base_id + 8);
                    AddToken(tokens, 1, base_id + 9);
                    residue -= (8 << 1);
                    mask = 1 << 3;
                    tab = VP8Cat4;
                } else if (residue < (8 << 3)) { // VP8Cat5  (5b)
                    AddToken(tokens, 1, base_id + 8);
                    AddToken(tokens, 0, base_id + 10);
                    residue -= (8 << 2);
                    mask = 1 << 4;
                    tab = VP8Cat5;
                } else { // VP8Cat6 (11b)
                    AddToken(tokens, 1, base_id + 8);
                    AddToken(tokens, 1, base_id + 10);
                    residue -= (8 << 3);
                    mask = 1 << 10;
                    tab = VP8Cat6;
                }
                while (mask) {
                    AddConstantToken(tokens, !!(residue & mask), *tab++);
                    mask >>= 1;
                }
            }
            base_id = TOKEN_ID(coeff_type, VP8EncBands[n], 2); // ctx=2
        }
        AddConstantToken(tokens, sign, 128);
        if (n == 16 || !AddToken(tokens, n <= last, base_id + 0)) {
            return 1; // EOB
        }
    }
    return 1;
}

#undef TOKEN_ID
#endif
//------------------------------------------------------------------------------
// This function works, but isn't currently used. Saved for later.

#if 0

static void Record(int bit, proba_t* const stats) {
  proba_t p = *stats;
  if (p >= 0xffff0000u) {               // an overflow is inbound.
    p = ((p + 1u) >> 1) & 0x7fff7fffu;  // -> divide the stats by 2.
  }
  // record bit count (lower 16 bits) and increment total count (upper 16 bits).
  p += 0x00010000u + bit;
  *stats = p;
}

void VP8TokenToStats(const VP8TBuffer* const b, proba_t* const stats) {
  const VP8Tokens* p = b->pages_;
  while (p != NULL) {
    const int N = (p->next_ == NULL) ? b->left_ : 0;
    int n = MAX_NUM_TOKEN;
    const token_t* const tokens = TOKEN_DATA(p);
    while (n-- > N) {
      const token_t token = tokens[n];
      if (!(token & FIXED_PROBA_BIT)) {
        Record((token >> 15) & 1, stats + (token & 0x3fffu));
      }
    }
    p = p->next_;
  }
}

#endif // 0

//------------------------------------------------------------------------------
// Final coding pass, with known probabilities

int VP8EmitTokens(VP8TBuffer* const b, VP8BitWriter* const bw, const uint8_t* const probas, int final_pass) {
    const VP8Tokens* p = b->pages_;
    StopProfilingWatch stop_watch;
    StartProfiling(&stop_watch);
    assert(!b->error_);
    while (p != NULL) {
        const VP8Tokens* const next = p->next_;
        // printf("%s p:%d next:%d\n", __FUNCTION__, p, next);
        const int N = (next == NULL) ? b->left_ : 0;
        int n = b->page_size_;
        const token_t* const tokens = TOKEN_DATA(p);
        while (n-- > N) {
            const token_t token = tokens[n];
            const int bit = (token >> 15) & 1;
            if (token & FIXED_PROBA_BIT) {
                VP8PutBit(bw, bit, token & 0xffu); // constant proba
            } else {
                VP8PutBit(bw, bit, probas[token & 0x3fffu]);
            }
        }
        if (final_pass) WebPSafeFree((void*)p);
        p = next;
    }
    if (final_pass) b->pages_ = NULL;
    StopProfiling(&stop_watch, &timeVP8EmitTokens, &countVP8EmitTokens);
    return 1;
}

// Size estimation
size_t VP8EstimateTokenSize(VP8TBuffer* const b, const uint8_t* const probas) {
    size_t size = 0;
    const VP8Tokens* p = b->pages_;
    assert(!b->error_);
    while (p != NULL) {
        const VP8Tokens* const next = p->next_;
        const int N = (next == NULL) ? b->left_ : 0;
        int n = b->page_size_;
        const token_t* const tokens = TOKEN_DATA(p);
        while (n-- > N) {
            const token_t token = tokens[n];
            const int bit = token & (1 << 15);
            if (token & FIXED_PROBA_BIT) {
                size += VP8BitCost(bit, token & 0xffu);
            } else {
                size += VP8BitCost(bit, probas[token & 0x3fffu]);
            }
        }
        p = next;
    }
    return size;
}

//------------------------------------------------------------------------------

#else // DISABLE_TOKEN_BUFFER

void VP8TBufferInit(VP8TBuffer* const b) {
    (void)b;
}
void VP8TBufferClear(VP8TBuffer* const b) {
    (void)b;
}

#endif // !DISABLE_TOKEN_BUFFER
