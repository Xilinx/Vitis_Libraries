/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

/**
 * This file is part of Vitis Data Analytics Library.
 */
#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_PARSE_DOUBLE_V2_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_PARSE_DOUBLE_V2_HPP
#include "hls_stream.h"
#include "ap_int.h"
#include "utils.hpp"
#include "converter.hpp"
#include <limits>

#define XF_UINT64_C2(high32, low32) ((static_cast<uint64_t>(high32) << 32) | static_cast<uint64_t>(low32))

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {
class DiyFp {
   public:
    DiyFp() : f_(), e_() {}
    DiyFp(uint64_t fp, int exp) : f_(fp), e_(exp) {}

    DiyFp(double d) {
        ap_uint<64> in_64 = doubleToBits(d);
        ap_int<16> bias_e = (in_64 & kDpExponentMask) >> kDpSignificandSize;
        ap_uint<64> significand = (in_64 & kDpSignificandMask);
        if (bias_e != 0) {
            f_ = significand + kDpHiddenBit;
            e_ = bias_e - kDpExponentBias;
        } else {
            // de-normal
            f_ = significand;
            e_ = kDpMinExponent + 1;
        }
    }

    inline void mul(const DiyFp& rhs) {
#pragma HLS inline
        ap_uint<128> r = f_ * rhs.f_;
        ap_uint<64> h_r, l_r;
        // get the high 32bit and low 32bit
        h_r.range(63, 0) = r.range(127, 64);
        l_r.range(63, 0) = r.range(63, 0);
        // rounding
        if (l_r[63]) h_r++;
        f_ = h_r;
        e_ = e_ + rhs.e_ + 64;
    }

    void normalize() {
#pragma HLS inline
        ap_uint<6> j = 0;
        // find the leading zero
        for (int i = 0; i < 64; ++i) {
#pragma HLS unroll
            if (f_[63 - i] != 0) {
                j = i;
                break;
            }
        }
        f_ <<= j;
        e_ -= j;
    }
    double toDouble() const {
#pragma HLS inline
        assert(f_ <= kDpHiddenBit + kDpSignificandMask);
        // Underflow.
        if (e_ < kDpDenormalExponent) {
            return 0.0;
        }
        // Overflow.
        if (e_ >= kDpMaxExponent) {
            return std::numeric_limits<double>::infinity();
        }

        ap_uint<64> r_exp = 0;
        // de-normal
        if (e_ == kDpDenormalExponent && (f_ & kDpHiddenBit) == 0) r_exp = 0;
        // normal
        else
            r_exp = e_ + kDpExponentBias;
        ap_uint<64> out = (f_ & kDpSignificandMask) | (r_exp << kDpSignificandSize);
        return bitsToDouble(out);
    }

    static const int kDiySignificandSize = 64;
    static const int kDpSignificandSize = 52;
    static const int kDpExponentBias = 0x3FF + kDpSignificandSize;
    static const int kDpMaxExponent = 0x7FF - kDpExponentBias;
    static const int kDpMinExponent = -kDpExponentBias;
    static const int kDpDenormalExponent = -kDpExponentBias + 1;
    static const uint64_t kDpExponentMask = XF_UINT64_C2(0x7FF00000, 0x00000000);
    static const uint64_t kDpSignificandMask = XF_UINT64_C2(0x000FFFFF, 0xFFFFFFFF);
    static const uint64_t kDpHiddenBit = XF_UINT64_C2(0x00100000, 0x00000000);

   public:
    ap_uint<64> f_;
    ap_int<16> e_;
};
// 10^-348, 10^-340, ..., 10^340
static const uint64_t kCachedPow10_F[] = {
    XF_UINT64_C2(0xfa8fd5a0, 0x081c0288), XF_UINT64_C2(0xbaaee17f, 0xa23ebf76), XF_UINT64_C2(0x8b16fb20, 0x3055ac76),
    XF_UINT64_C2(0xcf42894a, 0x5dce35ea), XF_UINT64_C2(0x9a6bb0aa, 0x55653b2d), XF_UINT64_C2(0xe61acf03, 0x3d1a45df),
    XF_UINT64_C2(0xab70fe17, 0xc79ac6ca), XF_UINT64_C2(0xff77b1fc, 0xbebcdc4f), XF_UINT64_C2(0xbe5691ef, 0x416bd60c),
    XF_UINT64_C2(0x8dd01fad, 0x907ffc3c), XF_UINT64_C2(0xd3515c28, 0x31559a83), XF_UINT64_C2(0x9d71ac8f, 0xada6c9b5),
    XF_UINT64_C2(0xea9c2277, 0x23ee8bcb), XF_UINT64_C2(0xaecc4991, 0x4078536d), XF_UINT64_C2(0x823c1279, 0x5db6ce57),
    XF_UINT64_C2(0xc2109436, 0x4dfb5637), XF_UINT64_C2(0x9096ea6f, 0x3848984f), XF_UINT64_C2(0xd77485cb, 0x25823ac7),
    XF_UINT64_C2(0xa086cfcd, 0x97bf97f4), XF_UINT64_C2(0xef340a98, 0x172aace5), XF_UINT64_C2(0xb23867fb, 0x2a35b28e),
    XF_UINT64_C2(0x84c8d4df, 0xd2c63f3b), XF_UINT64_C2(0xc5dd4427, 0x1ad3cdba), XF_UINT64_C2(0x936b9fce, 0xbb25c996),
    XF_UINT64_C2(0xdbac6c24, 0x7d62a584), XF_UINT64_C2(0xa3ab6658, 0x0d5fdaf6), XF_UINT64_C2(0xf3e2f893, 0xdec3f126),
    XF_UINT64_C2(0xb5b5ada8, 0xaaff80b8), XF_UINT64_C2(0x87625f05, 0x6c7c4a8b), XF_UINT64_C2(0xc9bcff60, 0x34c13053),
    XF_UINT64_C2(0x964e858c, 0x91ba2655), XF_UINT64_C2(0xdff97724, 0x70297ebd), XF_UINT64_C2(0xa6dfbd9f, 0xb8e5b88f),
    XF_UINT64_C2(0xf8a95fcf, 0x88747d94), XF_UINT64_C2(0xb9447093, 0x8fa89bcf), XF_UINT64_C2(0x8a08f0f8, 0xbf0f156b),
    XF_UINT64_C2(0xcdb02555, 0x653131b6), XF_UINT64_C2(0x993fe2c6, 0xd07b7fac), XF_UINT64_C2(0xe45c10c4, 0x2a2b3b06),
    XF_UINT64_C2(0xaa242499, 0x697392d3), XF_UINT64_C2(0xfd87b5f2, 0x8300ca0e), XF_UINT64_C2(0xbce50864, 0x92111aeb),
    XF_UINT64_C2(0x8cbccc09, 0x6f5088cc), XF_UINT64_C2(0xd1b71758, 0xe219652c), XF_UINT64_C2(0x9c400000, 0x00000000),
    XF_UINT64_C2(0xe8d4a510, 0x00000000), XF_UINT64_C2(0xad78ebc5, 0xac620000), XF_UINT64_C2(0x813f3978, 0xf8940984),
    XF_UINT64_C2(0xc097ce7b, 0xc90715b3), XF_UINT64_C2(0x8f7e32ce, 0x7bea5c70), XF_UINT64_C2(0xd5d238a4, 0xabe98068),
    XF_UINT64_C2(0x9f4f2726, 0x179a2245), XF_UINT64_C2(0xed63a231, 0xd4c4fb27), XF_UINT64_C2(0xb0de6538, 0x8cc8ada8),
    XF_UINT64_C2(0x83c7088e, 0x1aab65db), XF_UINT64_C2(0xc45d1df9, 0x42711d9a), XF_UINT64_C2(0x924d692c, 0xa61be758),
    XF_UINT64_C2(0xda01ee64, 0x1a708dea), XF_UINT64_C2(0xa26da399, 0x9aef774a), XF_UINT64_C2(0xf209787b, 0xb47d6b85),
    XF_UINT64_C2(0xb454e4a1, 0x79dd1877), XF_UINT64_C2(0x865b8692, 0x5b9bc5c2), XF_UINT64_C2(0xc83553c5, 0xc8965d3d),
    XF_UINT64_C2(0x952ab45c, 0xfa97a0b3), XF_UINT64_C2(0xde469fbd, 0x99a05fe3), XF_UINT64_C2(0xa59bc234, 0xdb398c25),
    XF_UINT64_C2(0xf6c69a72, 0xa3989f5c), XF_UINT64_C2(0xb7dcbf53, 0x54e9bece), XF_UINT64_C2(0x88fcf317, 0xf22241e2),
    XF_UINT64_C2(0xcc20ce9b, 0xd35c78a5), XF_UINT64_C2(0x98165af3, 0x7b2153df), XF_UINT64_C2(0xe2a0b5dc, 0x971f303a),
    XF_UINT64_C2(0xa8d9d153, 0x5ce3b396), XF_UINT64_C2(0xfb9b7cd9, 0xa4a7443c), XF_UINT64_C2(0xbb764c4c, 0xa7a44410),
    XF_UINT64_C2(0x8bab8eef, 0xb6409c1a), XF_UINT64_C2(0xd01fef10, 0xa657842c), XF_UINT64_C2(0x9b10a4e5, 0xe9913129),
    XF_UINT64_C2(0xe7109bfb, 0xa19c0c9d), XF_UINT64_C2(0xac2820d9, 0x623bf429), XF_UINT64_C2(0x80444b5e, 0x7aa7cf85),
    XF_UINT64_C2(0xbf21e440, 0x03acdd2d), XF_UINT64_C2(0x8e679c2f, 0x5e44ff8f), XF_UINT64_C2(0xd433179d, 0x9c8cb841),
    XF_UINT64_C2(0x9e19db92, 0xb4e31ba9), XF_UINT64_C2(0xeb96bf6e, 0xbadf77d9), XF_UINT64_C2(0xaf87023b, 0x9bf0ee6b),
    // pow10(0, 1, 2, 3, 4, 5, 6, 7)
    XF_UINT64_C2(0x80000000, 0x00000000), XF_UINT64_C2(0xa0000000, 0x00000000), XF_UINT64_C2(0xc8000000, 0x00000000),
    XF_UINT64_C2(0xfa000000, 0x00000000), XF_UINT64_C2(0x9c400000, 0x00000000), XF_UINT64_C2(0xc3500000, 0x00000000),
    XF_UINT64_C2(0xf4240000, 0x00000000), XF_UINT64_C2(0x98968000, 0x00000000)};

static const int16_t kCachedPow10_E[] = {
    -1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007, -980, -954, -927, -901, -874, -847, -821,
    -794,  -768,  -741,  -715,  -688,  -661,  -635,  -608,  -582,  -555, -529, -502, -475, -449, -422, -396,
    -369,  -343,  -316,  -289,  -263,  -236,  -210,  -183,  -157,  -130, -103, -77,  -50,  -24,  3,    30,
    56,    83,    109,   136,   162,   189,   216,   242,   269,   295,  322,  348,  375,  402,  428,  455,
    481,   508,   534,   561,   588,   614,   641,   667,   694,   720,  747,  774,  800,  827,  853,  880,
    907,   933,   960,   986,   1013,  1039,  1066,  -63,   -60,   -57,  -54,  -50,  -47,  -44,  -40};

inline DiyFp getCachedPow10(ap_int<16> exp, ap_int<16>& outExp) {
    // 10^8  = 10^8 * 10^0;
    // 10^9  = 10^8 * 10^1;
    // 10^10 = 10^8 * 10^2;
    // 10^11 = 10^8 * 10^3;
    // 10^12 = 10^8 * 10^4;
    // 10^13 = 10^8 * 10^5;
    // 10^14 = 10^8 * 10^6;
    // 10^15 = 10^8 * 10^7;
    assert(exp >= -348);
    ap_uint<16> tmp = exp + 348;
    ap_uint<16> idx = tmp >> 3;
    ap_uint<16> idx_c = idx << 3;
    outExp = -348 + idx_c;
    return DiyFp(kCachedPow10_F[idx], kCachedPow10_E[idx]);
}
inline ap_int<16> getEffectSignidSize(ap_int<16> order) {
    if (order >= -1021)
        return 53;
    else if (order <= -1074)
        return 0;
    else
        return order + 1074;
}
// static const uint64_t pow10Buff[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
//                                     100000000, 1000000000, 100000000000, 100000000000, 10000000000000,
//                                     10000000000000,
//                                     100000000000000, 1000000000000000, 10000000000000000, 100000000000000000,
//                                     1000000000000000000, 10000000000000000000};
static const DiyFp kPow10[] = {
    DiyFp(UINT64_C2(0xa0000000, 0x00000000), -60), // 10^1
    DiyFp(UINT64_C2(0xc8000000, 0x00000000), -57), // 10^2
    DiyFp(UINT64_C2(0xfa000000, 0x00000000), -54), // 10^3
    DiyFp(UINT64_C2(0x9c400000, 0x00000000), -50), // 10^4
    DiyFp(UINT64_C2(0xc3500000, 0x00000000), -47), // 10^5
    DiyFp(UINT64_C2(0xf4240000, 0x00000000), -44), // 10^6
    DiyFp(UINT64_C2(0x98968000, 0x00000000), -40)  // 10^7
};

/**
 *
 * @brief add one float date type output.
 *
 * @param i_strm input stream which concated end flag and data stream together.
 * @param o_d_strm double output stream.
 * @param o_f_strm float output stream.
 *
 */
static void addFloat(hls::stream<ap_uint<65> >& i_strm,
                     hls::stream<ap_uint<64> >& o_d_strm,
                     hls::stream<ap_uint<32> >& o_f_strm) {
    ap_uint<65> in = i_strm.read();
    bool e = in[0];
    while (!e) {
#pragma HLS pipeline II = 1
        bool nb = i_strm.read_nb(in);
        if (nb) {
            e = in[0];
            ap_uint<64> dt = in.range(64, 1);
            if (!e) {
                float out = xf::dataframe::doubleToFloat(bitsToDouble(dt));
                o_d_strm.write(dt);
                o_f_strm.write(floatToBits(out));
            }
        }
    }
}

struct nb_dp {
    ap_uint<64> s_;
    ap_int<16> ex_;
    ap_uint<16> ln_;
    ap_uint<16> rm_;
    bool nf_;
    bool sg_;
    bool e_;
};
/**
 *
 * @brief based on the input stream generate double output.
 * The input includes significant, exponential, length of string, remaining length of string, flag of nan for inf, sign
 * flag and end flag.
 *
 * @param pk_strm input stream which includes all packed data.
 * @param o_strm output stream.
 *
 */

static void strtodDiyFp( // hls::stream<bool>& e_strm,
    hls::stream<nb_dp>& pk_strm,
    hls::stream<ap_uint<65> >& o_strm) {
    nb_dp pk_dt = pk_strm.read();
    bool e = pk_dt.e_; // e_strm.read();
    double out = 0;

    ap_uint<65> out_65;
    out_65[0] = false;
    o_strm.write(out_65);
    while (!e) {
#pragma HLS pipeline II = 1
        // non-blocking read.
        bool nb = pk_strm.read_nb(pk_dt);
        if (nb) {
            ap_uint<64> significand = pk_dt.s_;
            ap_uint<16> d_len = pk_dt.ln_;
            ap_int<16> d_exp = pk_dt.ex_;
            ap_uint<16> remaining = pk_dt.rm_;
            bool isNaNInf = pk_dt.nf_;
            bool is_neg = pk_dt.sg_;
            e = pk_dt.e_;
            if (!e) {
                if (isNaNInf) out = bitsToDouble(significand);
                // Zero
                else if (d_len == 0)
                    out = 0.0;
                else {
                    // Trim right-most digits
                    const ap_uint<16> kMaxDecimalDigit = 767 + 1;
                    ap_int<16> d_trim = d_len - kMaxDecimalDigit;
                    if (d_len > kMaxDecimalDigit) {
                        d_exp += d_trim;
                        remaining -= d_trim;
                        d_len = kMaxDecimalDigit;
                    }
                    ap_int<16> sum = d_len + d_exp;
                    // underflow, Any x <= 10^-324 is interpreted as zero.
                    if (sum <= -324) out = 0.0;
                    // overflow Any x >= 10^309 is interpreted as +infinity.
                    else if (sum > 309)
                        out = std::numeric_limits<double>::infinity();
                    else {
                        // int remaining = dLen - i;
                        const int kUlpShift = 3;
                        const ap_uint<4> kUlp = 1 << kUlpShift;
                        ap_uint<64> error = 0;
                        if (remaining > 0) error = 4;

                        DiyFp v(significand, 0);
                        v.normalize();
                        error <<= -v.e_;

                        d_exp += remaining;

                        ap_int<16> act_exp;
                        DiyFp cachedPower = getCachedPow10(d_exp, act_exp);
                        ap_uint<16> adjustment = d_exp - act_exp;
                        assert(adjustment >= 0 && adjustment < 8);
                        // add bias
                        DiyFp pow_10 = DiyFp(kCachedPow10_F[adjustment + 87], kCachedPow10_E[adjustment + 87]);
                        v.mul(pow_10); // kPow10[adjustment - 1];
                        if (act_exp != d_exp &&
                            d_len + adjustment > 19) { // has more digits than decimal digits in 64-bit
                            error += kUlp / 2;
                        }
                        v.mul(cachedPower);
                        error += kUlp;
                        if (error != 0) error++;

                        ap_int<16> old_exp = v.e_;
                        v.normalize();
                        error <<= old_exp - v.e_;

                        ap_int<16> efft_sg_sz = getEffectSignidSize(64 + v.e_);
                        ap_int<16> prec_sz = 64 - efft_sg_sz;
                        ap_int<16> prec_sz_c = prec_sz;
                        ap_int<16> scale_exp = (prec_sz + kUlpShift) - 63;
                        if (scale_exp > 0) {
                            error = (error >> scale_exp) + 1 + kUlp;
                            prec_sz -= scale_exp;
                        }

                        DiyFp rounded(v.f_ >> prec_sz_c, v.e_ + prec_sz_c);
                        ap_uint<64> tmp = 1;
                        tmp <<= prec_sz;
                        tmp -= 1;
                        ap_uint<64> prec_bits = v.f_ & tmp;
                        prec_bits <<= 3;
                        ap_uint<64> half_w = tmp << 3;
                        half_w += error;
                        // const uint64_t halfWay = (uint64_t(1) << (precisionSize - 1)) * kUlp;
                        // if (precisionBits >= halfWay + static_cast<unsigned>(error)) {
                        if (prec_bits >= half_w) {
                            rounded.f_++;
                            if (rounded.f_ & (DiyFp::kDpHiddenBit << 1)) { // rounding overflows mantissa (issue #340)
                                rounded.f_ >>= 1;
                                rounded.e_++;
                            }
                        }

                        out = rounded.toDouble();
                    }
                    if (is_neg) out = -out;

                    // return halfWay - static_cast<unsigned>(error) >= precisionBits ||
                    //      precisionBits >= halfWay + static_cast<unsigned>(error);
                }
                out_65.range(64, 1) = doubleToBits(out);
                out_65[0] = false;
                o_strm.write(out_65);
                // o_e_strm.write(false);
            }
        }
    }
    out_65[0] = true;
    o_strm.write(out_65);
    // o_e_strm.write(true);
}

/**
 *
 * @brief parse the string into interger part, fraction part and exponential part.
 * The output is packed in single stream. It includes significant, exponential, length of string, remaining length of
 * string, flag of nan for inf, sign flag and end flag.
 *
 * @param i_strm input stream of string.
 * @param i_e_strm input end flag of i_strm.
 * @param vld_strm input valid flag of i_strm.
 * @param o_strm output packed stream.
 *
 */

static void parseStr2Dec(hls::stream<ap_uint<8> >& i_strm,
                         hls::stream<bool>& i_e_strm,
                         hls::stream<bool>& vld_strm,
                         hls::stream<nb_dp>& o_strm) {
    // init
    ap_uint<64> significand = 0;
    bool is_nan = false;
    bool is_neg = false;
    bool is_inf = false;

    bool is_first = true;
    bool exp_minus = false;

    bool is_exp = false;
    bool is_fract = false;
    bool reach_max = false;

    ap_int<16> exp_frac = 0;
    ap_int<16> exp = 0;
    ap_uint<16> d_len = 0;
    // int trail_zero = 0;
    ap_uint<16> ld_zero = 0;
    ap_uint<16> remaining = 0;

    // bool e = i_e_strm.read();
    bool e = false;
    bool nb_1 = false;
    bool nb_2 = false;
    bool nb_3 = false;
    bool vld = true;
    bool is_ld_0 = true;
    // o_e_strm.write(false);
    nb_dp out;
    out.e_ = false;
    // write out one dummy data
    o_strm.write(out);
    while (!e) {
#pragma HLS pipeline II = 1
        if (!vld && nb_1 && nb_2 && nb_3) {
            ap_uint<64> s = 0;
            ap_int<16> p = 0;
            bool isNanInf = is_nan || is_inf;
            bool isPosNeg = is_neg;
            // o_nf_strm.write(isNanInf);
            // o_sg_strm.write(is_neg);
            if (is_nan) {
                // s[63] = 0;
                // s.range(62, 51) = 0xfff;
                // s.range(50, 0) = 0;
                s = doubleToBits(std::numeric_limits<double>::quiet_NaN());
            } else if (is_inf) {
                // s[63] = is_neg;
                // s.range(62, 52) = 0xfff;
                // s.range(51, 0) = 0;
                s = doubleToBits(std::numeric_limits<double>::infinity());
            } else {
                if (exp_minus)
                    p = -exp + exp_frac;
                else
                    p = exp + exp_frac;
                // if(trail_zero < d_len && trail_zero !=0) trail_zero = d_len - trail_zero;
                // else trail_zero = 0;
                s = significand; /// pow10Buff[trail_zero];
                // p += trail_zero;
                d_len = d_len - ld_zero; // - trail_zero;
            }
            out.s_ = s;
            out.ex_ = p;
            out.ln_ = d_len;
            out.rm_ = remaining;
            out.nf_ = isNanInf;
            out.sg_ = is_neg;
            out.e_ = false;
            o_strm.write(out);
            // reset
            significand = 0;
            exp = 0;
            exp_frac = 0;
            ld_zero = 0;
            // trail_zero = 0;
            d_len = 0;

            is_first = true;
            is_nan = false;
            is_neg = false;
            is_inf = false;
            is_exp = false;
            is_fract = false;
            exp_minus = false;
            reach_max = false;
            is_ld_0 = true;
        }
        ap_uint<8> in;
        {
#pragma HLS latency min = 0 max = 0
            nb_1 = i_e_strm.read_nb(e);
            nb_2 = vld_strm.read_nb(vld);
            nb_3 = i_strm.read_nb(in);
        }
        // std::cout << std::hex << in << " " << vld << " " << e << " " << std::dec;
        if (vld && nb_1 && nb_2 && nb_3) {
            if (is_first && in == '-')
                is_neg = true;
            else if (is_first && in == 'N') {
                is_nan = true;
            } else if (is_first && in == 'I') {
                is_inf = true;
            } else {
                if (in == '.') {
                    is_fract = true;
                } else if (in == 'e' || in == 'E') {
                    is_exp = true;
                } else if (in == '-') {
                    exp_minus = true;
                } else if (in >= '0' && in <= '9') {
                    if (is_exp)
                        exp = exp * 10 + (in - '0');
                    else {
                        d_len++;
                        // if(in != '0') trail_zero = d_len;
                        if (!reach_max && (significand > UINT64_C2(0x19999999, 0x99999999) ||
                                           (significand == UINT64_C2(0x19999999, 0x99999999) && in > '5'))) {
                            reach_max = true;
                            if (in >= '5') significand++;
                        } else if (!reach_max) {
                            significand = significand * 10 + (in - '0');
                        }
                        if (in != '0' && is_ld_0)
                            is_ld_0 = false;
                        else if (is_ld_0)
                            ld_zero++;
                        if (is_fract) exp_frac--;
                        if (reach_max) remaining++;
                        // if (reach_max && !is_fract) exp_frac++;
                        // else if (!reach_max && is_fract) exp_frac--;
                    }
                }
            }
            is_first = false;
        }
    }
    out.e_ = true;
    o_strm.write(out);
};
}
}
}
}
#undef XF_UINT64_C2
#endif
