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
#ifndef CONTEXT_H
#define CONTEXT_H

#include <cmath>     // std::isinf
#include <algorithm> // std::min

#define MAX_QUAL 254
#define MAX_JACOBIAN_TOLERANCE 8.0
#define JACOBIAN_LOG_TABLE_STEP 0.0001
#define JACOBIAN_LOG_TABLE_INV_STEP (1.0 / JACOBIAN_LOG_TABLE_STEP)
#define JACOBIAN_LOG_TABLE_SIZE ((int)(MAX_JACOBIAN_TOLERANCE / JACOBIAN_LOG_TABLE_STEP) + 1)

template <class NUMBER>
class ContextBase {
   public:
    static NUMBER ph2pr[128];
    static NUMBER INITIAL_CONSTANT;
    static NUMBER LOG10_INITIAL_CONSTANT;
    static NUMBER RESULT_THRESHOLD;

    static bool staticMembersInitializedFlag;
    static NUMBER jacobianLogTable[JACOBIAN_LOG_TABLE_SIZE];
    static NUMBER matchToMatchProb[((MAX_QUAL + 1) * (MAX_QUAL + 2)) >> 1];

    static void initializeStaticMembers() {
        // Order of calls important - Jacobian first, then MatchToMatch
        initializeJacobianLogTable();
        initializeMatchToMatchProb();
    }

    static void deleteStaticMembers() {
        if (staticMembersInitializedFlag) {
            staticMembersInitializedFlag = false;
        }
    }

    // Called only once during library load - don't bother to optimize with single precision fp
    static void initializeJacobianLogTable() {
        for (int k = 0; k < JACOBIAN_LOG_TABLE_SIZE; k++) {
            jacobianLogTable[k] = (NUMBER)(log10(1.0 + pow(10.0, -((double)k) * JACOBIAN_LOG_TABLE_STEP)));
        }
    }

    // Called only once per library load - don't bother optimizing with single fp
    static void initializeMatchToMatchProb() {
        double LN10 = log(10);
        double INV_LN10 = 1.0 / LN10;
        for (int i = 0, offset = 0; i <= MAX_QUAL; offset += ++i)
            for (int j = 0; j <= i; j++) {
                double log10Sum = approximateLog10SumLog10(-0.1 * i, -0.1 * j);
                double matchToMatchLog10 = log1p(-std::min(1.0, pow(10, log10Sum))) * INV_LN10;
                matchToMatchProb[offset + j] = (NUMBER)(pow(10, matchToMatchLog10));
            }
    }
    // Called during computation - use single precision where possible
    static int fastRound(NUMBER d) { return (d > ((NUMBER)0.0)) ? (int)(d + ((NUMBER)0.5)) : (int)(d - ((NUMBER)0.5)); }
    // Called during computation - use single precision where possible
    static NUMBER approximateLog10SumLog10(NUMBER small, NUMBER big) {
        // make sure small is really the smaller value
        if (small > big) {
            NUMBER t = big;
            big = small;
            small = t;
        }

        if (std::isinf(small) == -1 || std::isinf(big) == -1) return big;

        NUMBER diff = big - small;
        if (diff >= ((NUMBER)MAX_JACOBIAN_TOLERANCE)) return big;

        // OK, so |y-x| < tol: we use the following identity then:
        // we need to compute log10(10^x + 10^y)
        // By Jacobian logarithm identity, this is equal to
        // max(x,y) + log10(1+10^-abs(x-y))
        // we compute the second term as a table lookup with integer quantization
        // we have pre-stored correction for 0,0.1,0.2,... 10.0
        int ind = fastRound((NUMBER)(diff * ((NUMBER)JACOBIAN_LOG_TABLE_INV_STEP))); // hard rounding
        return big + jacobianLogTable[ind];
    }
};

template <class NUMBER>
class Context : public ContextBase<NUMBER> {};

template <>
class Context<double> : public ContextBase<double> {
   public:
    Context() : ContextBase<double>() {
        if (!staticMembersInitializedFlag) {
            initializeStaticMembers();

            for (int x = 0; x < 128; x++) {
                ph2pr[x] = pow(10.0, -((double)x) / 10.0);
            }

            INITIAL_CONSTANT = ldexp(1.0, 1020.0);
            LOG10_INITIAL_CONSTANT = log10(INITIAL_CONSTANT);
            RESULT_THRESHOLD = 0.0;

            staticMembersInitializedFlag = true;
        }
    }

    double LOG10(double v) { return log10(v); }
    inline double POW(double b, double e) { return pow(b, e); }

    static double _(double n) { return n; }
    static double _(float n) { return ((double)n); }

    inline double set_mm_prob(int insQual, int delQual) {
        int minQual = delQual;
        int maxQual = insQual;
        if (insQual <= delQual) {
            minQual = insQual;
            maxQual = delQual;
        }

        return MAX_QUAL < maxQual ? 1.0 - POW(10.0, approximateLog10SumLog10(-0.1 * minQual, -0.1 * maxQual))
                                  : matchToMatchProb[((maxQual * (maxQual + 1)) >> 1) + minQual];
    }
};

template <>
class Context<float> : public ContextBase<float> {
   public:
    Context() : ContextBase<float>() {
        if (!staticMembersInitializedFlag) {
            initializeStaticMembers();

            for (int x = 0; x < 128; x++) {
                ph2pr[x] = powf(10.f, -((float)x) / 10.f);
            }

            INITIAL_CONSTANT = ldexpf(1.f, 120.f);
            LOG10_INITIAL_CONSTANT = log10f(INITIAL_CONSTANT);
            RESULT_THRESHOLD = ldexpf(1.f, -110.f);

            staticMembersInitializedFlag = true;
        }
    }

    float LOG10(float v) { return log10f(v); }
    inline float POW(float b, float e) { return powf(b, e); }

    static float _(double n) { return ((float)n); }
    static float _(float n) { return n; }

    inline float set_mm_prob(int insQual, int delQual) {
        int minQual = delQual;
        int maxQual = insQual;
        if (insQual <= delQual) {
            minQual = insQual;
            maxQual = delQual;
        }

        return MAX_QUAL < maxQual ? 1.0f - POW(10.0f, approximateLog10SumLog10(-0.1f * minQual, -0.1f * maxQual))
                                  : matchToMatchProb[((maxQual * (maxQual + 1)) >> 1) + minQual];
    }
};

template <typename NUMBER>
NUMBER ContextBase<NUMBER>::ph2pr[128];
template <typename NUMBER>
NUMBER ContextBase<NUMBER>::INITIAL_CONSTANT;
template <typename NUMBER>
NUMBER ContextBase<NUMBER>::LOG10_INITIAL_CONSTANT;
template <typename NUMBER>
NUMBER ContextBase<NUMBER>::RESULT_THRESHOLD;
template <typename NUMBER>
bool ContextBase<NUMBER>::staticMembersInitializedFlag = false;
template <typename NUMBER>
NUMBER ContextBase<NUMBER>::jacobianLogTable[JACOBIAN_LOG_TABLE_SIZE];
template <typename NUMBER>
NUMBER ContextBase<NUMBER>::matchToMatchProb[((MAX_QUAL + 1) * (MAX_QUAL + 2)) >> 1];

#endif
