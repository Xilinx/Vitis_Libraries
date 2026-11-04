/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
 * Cooley-Tukey radix-4 DIT FFT stages with token-driven streaming.
 *
 * This variant follows the radix-2 structural pattern:
 *   - stage_bitreverse_r4
 *   - recursive stage chain (one stage per radix-4 level)
 *   - top-level DATAFLOW region
 *
 * Notes:
 *   - N must be a power of 4.
 *   - Interface beat is SSR=4 (struct_fft_ssr4).
 *   - Token protocol is identical to radix-2 variant.
 */
#ifndef FFT1D_VARIANT_R4_STAGES_HPP
#define FFT1D_VARIANT_R4_STAGES_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <cmath>

#ifndef NUM_FFT
#define NUM_FFT 4096
#endif

#include "fft1d_variant_r4_utils.hpp"

namespace xf {
namespace dsp {
namespace fft {

template <int N>
struct Log2 {
    static const int value = 1 + Log2<N / 2>::value;
};
template <>
struct Log2<1> {
    static const int value = 0;
};

// ============================================================================
// Twiddle ROM (same contract as radix-2 variant)
// ============================================================================
template <int N>
struct twiddle_rom {
    static const int ROM_SIZE = N / 2;
    static inline void get(int idx, float& c, float& s) {
#pragma HLS INLINE
        float angle = (float)(-2.0 * 3.14159265358979323846 * (float)(idx % N) / (float)N);
        c = cosf(angle);
        s = sinf(angle);
    }
};

template <>
struct twiddle_rom<64> {
    static const int ROM_SIZE = 32;
    static inline void get(int idx, float& c, float& s) {
#pragma HLS INLINE
        static const float COS[32] = {
            1.0f,          0.995184726f,  0.980785280f,  0.956940336f,  0.923879533f,  0.881921264f,  0.831469612f,
            0.773010453f,  0.707106781f,  0.634393284f,  0.555570233f,  0.471396737f,  0.382683432f,  0.290284677f,
            0.195090322f,  0.098017140f,  0.0f,          -0.098017140f, -0.195090322f, -0.290284677f, -0.382683432f,
            -0.471396737f, -0.555570233f, -0.634393284f, -0.707106781f, -0.773010453f, -0.831469612f, -0.881921264f,
            -0.923879533f, -0.956940336f, -0.980785280f, -0.995184726f};
        static const float SIN[32] = {
            0.0f,          -0.098017140f, -0.195090322f, -0.290284677f, -0.382683432f, -0.471396737f, -0.555570233f,
            -0.634393284f, -0.707106781f, -0.773010453f, -0.831469612f, -0.881921264f, -0.923879533f, -0.956940336f,
            -0.980785280f, -0.995184726f, -1.0f,         -0.995184726f, -0.980785280f, -0.956940336f, -0.923879533f,
            -0.881921264f, -0.831469612f, -0.773010453f, -0.707106781f, -0.634393284f, -0.555570233f, -0.471396737f,
            -0.382683432f, -0.290284677f, -0.195090322f, -0.098017140f};
        int k = idx & 31;
        c = COS[k];
        s = SIN[k];
    }
};

#include "twiddle_rom_4096.hpp"
template <>
struct twiddle_rom<4096> {
    static const int ROM_SIZE = 2048;
    static inline void get(int idx, float& c, float& s) {
#pragma HLS INLINE
        int k = idx & 2047;
        c = TWIDDLE_COS_4096[k];
        s = TWIDDLE_SIN_4096[k];
    }
};

#include "twiddle_rom_16384.hpp"

template <int N>
inline complex_t twiddle_W_N(int k) {
#pragma HLS INLINE
    k %= N;
    if (k < 0) k += N;
    if (k == 0) return complex_t(1.0f, 0.0f);
    if (k == N / 2) return complex_t(-1.0f, 0.0f);
    float c, s;
    if (k < N / 2) {
        twiddle_rom<N>::get(k, c, s);
    } else {
        twiddle_rom<N>::get(N - k, c, s);
        s = -s;
    }
    return complex_t(c, s);
}

template <int LOG2N>
inline unsigned short bitrev_compute(unsigned short val) {
#pragma HLS INLINE
    unsigned short rev = 0;
    for (int b = 0; b < LOG2N; b++) {
#pragma HLS UNROLL
        rev = (rev << 1) | (val & 1);
        val >>= 1;
    }
    return rev;
}

template <int N>
inline unsigned short addr_to_edge_local_r4(unsigned short addr) {
#pragma HLS INLINE
    return (addr >> 2) & (N / 16 - 1);
}

template <int N, int STAGE_ID>
inline unsigned short addr_to_stage_local_r4(unsigned short addr) {
#pragma HLS INLINE
    static const int SHIFT = 2 * (STAGE_ID + 1);
    const unsigned short low_mid = (addr >> 2) & ((1u << (SHIFT - 2)) - 1u);
    const unsigned short high = addr >> (SHIFT + 2);
    return low_mid | (high << (SHIFT - 2));
}

// ============================================================================
// Bit reverse stage (SSR=4)
// ============================================================================
template <int N>
void stage_bitreverse_r4(hls::stream<struct_fft_ssr4>& strm_in,
                         hls::stream<struct_fft_ssr4>& strm_out,
                         hls::stream<ap_uint<2> >& strm_token_in,
                         hls::stream<ap_uint<2> >& strm_token_out) {
    static const int LOG2N = Log2<N>::value;
    static const int NUM_BEATS = N / 4;
    complex_t ram[2][4][4][N / 16];
#pragma HLS ARRAY_PARTITION variable = ram dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 2 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 3 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 4 off
#pragma HLS BIND_STORAGE variable = ram type = RAM_T2P impl = BRAM

    char pp = 0;
    char token = strm_token_in.read();
    unsigned short cnt = 0;

    while (token) {
#pragma HLS PIPELINE II = 1
        const bool has_input = (token == 1 || token == 3);
        const bool has_output = (token == 2 || token == 3);
        const bool is_last = (cnt == (NUM_BEATS - 1));

        if (has_input) {
            struct_fft_ssr4 din;
            strm_in.read(din);
            const unsigned short wr0 = bitrev_compute<LOG2N>((unsigned short)(cnt * 4));
            const unsigned short wr_low = wr0 & 3u;
            const unsigned short wr_local = addr_to_edge_local_r4<N>(wr0);
            ram[pp][wr_low][0][wr_local] = din[0];
            ram[pp][wr_low][2][wr_local] = din[1];
            ram[pp][wr_low][1][wr_local] = din[2];
            ram[pp][wr_low][3][wr_local] = din[3];
        }

        if (has_output) {
            struct_fft_ssr4 dout;
            const unsigned short rd = (unsigned short)(cnt * 4);
            const unsigned short rd_high = (rd >> (LOG2N - 2)) & 3u;
            const unsigned short rd_local = addr_to_edge_local_r4<N>(rd);
            dout[0] = ram[1 - pp][0][rd_high][rd_local];
            dout[1] = ram[1 - pp][1][rd_high][rd_local];
            dout[2] = ram[1 - pp][2][rd_high][rd_local];
            dout[3] = ram[1 - pp][3][rd_high][rd_local];
            strm_out.write(dout);
        }

        if (is_last) {
            strm_token_out.write(token);
            if (has_input) {
                token = strm_token_in.read();
            } else if (token == 2) {
                token = 0;
            } else {
                token = strm_token_in.read();
            }
            cnt = 0;
            pp = 1 - pp;
        } else {
            cnt++;
        }
    }
}

template <int N, int STAGE_ID>
void stage_stockham_dit_r4(hls::stream<struct_fft_ssr4>& strm_in,
                           hls::stream<struct_fft_ssr4>& strm_out,
                           hls::stream<ap_uint<2> >& strm_token_in,
                           hls::stream<ap_uint<2> >& strm_token_out) {
    static const int NUM_BEATS = N / 4;
    static const int K_BITS = 2 * STAGE_ID;
    static const int K_MASK = (K_BITS == 0) ? 0 : ((1 << K_BITS) - 1);
    static const int STEP = 1 << (Log2<N>::value - 2 * (STAGE_ID + 1));
    static const int SEG_BITS = 2 * (STAGE_ID + 1);
    static const unsigned short SEG_MASK = (1u << SEG_BITS) - 1u;

    complex_t ram[2][4][4][N / 16];
#pragma HLS ARRAY_PARTITION variable = ram dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 2 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 3 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 4 off
#pragma HLS BIND_STORAGE variable = ram type = RAM_T2P impl = BRAM

    char pp = 0;
    char token = strm_token_in.read();
    unsigned short cnt = 0;

    while (token) {
#pragma HLS PIPELINE II = 1
        const bool has_input = (token == 1 || token == 3);
        const bool has_output = (token == 2 || token == 3);
        const bool is_last = (cnt == (NUM_BEATS - 1));

        if (has_input) {
            struct_fft_ssr4 din;
            strm_in.read(din);

            const int k = cnt & K_MASK;
            const complex_t w1 = twiddle_W_N<N>(k * STEP);
            const complex_t w2 = twiddle_W_N<N>(2 * k * STEP);
            const complex_t w3 = twiddle_W_N<N>(3 * k * STEP);

            complex_t o0, o1, o2, o3;
            calculate_radix4_dit(din[0], din[1], din[2], din[3], w1, w2, w3, o0, o1, o2, o3);

            // Stockham write addressing:
            //   rotate-left lower (STAGE_ID+1) radix-4 digits by 1 digit,
            //   then insert lane digit in front of the rotated block.
            unsigned short lower = cnt & SEG_MASK;
            unsigned short rotated = ((lower << 2) | (lower >> (SEG_BITS - 2))) & SEG_MASK;
            unsigned short upper = cnt >> SEG_BITS;
            unsigned short base = (upper << (SEG_BITS + 2)) | rotated;
            unsigned short wr_low = base & 3u;
            unsigned short wr_local = addr_to_stage_local_r4<N, STAGE_ID>(base);
            ram[pp][wr_low][0][wr_local] = o0;
            ram[pp][wr_low][1][wr_local] = o1;
            ram[pp][wr_low][2][wr_local] = o2;
            ram[pp][wr_low][3][wr_local] = o3;
        }

        if (has_output) {
            struct_fft_ssr4 dout;
            const unsigned short rd = (unsigned short)(cnt * 4);
            const unsigned short rd_stage = (rd >> (2 * (STAGE_ID + 1))) & 3u;
            const unsigned short rd_local = addr_to_stage_local_r4<N, STAGE_ID>(rd);
            dout[0] = ram[1 - pp][0][rd_stage][rd_local];
            dout[1] = ram[1 - pp][1][rd_stage][rd_local];
            dout[2] = ram[1 - pp][2][rd_stage][rd_local];
            dout[3] = ram[1 - pp][3][rd_stage][rd_local];
            strm_out.write(dout);
        }

        if (is_last) {
            strm_token_out.write(token);
            if (has_input) {
                token = strm_token_in.read();
            } else if (token == 2) {
                token = 0;
            } else {
                token = strm_token_in.read();
            }
            cnt = 0;
            pp = 1 - pp;
        } else {
            cnt++;
        }
    }
}

template <int N, int STAGE_ID>
void stage_stockham_dit_r4_final(hls::stream<struct_fft_ssr4>& strm_in,
                                 hls::stream<struct_fft_ssr4>& strm_out,
                                 hls::stream<ap_uint<2> >& strm_token_in,
                                 hls::stream<ap_uint<2> >& strm_token_out) {
    static const int NUM_BEATS = N / 4;
    static const int K_BITS = 2 * STAGE_ID;
    static const int K_MASK = (K_BITS == 0) ? 0 : ((1 << K_BITS) - 1);
    static const int STEP = 1 << (Log2<N>::value - 2 * (STAGE_ID + 1));

    complex_t ram[2][4][4][N / 16];
#pragma HLS ARRAY_PARTITION variable = ram dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 2 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 3 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 4 off
#pragma HLS BIND_STORAGE variable = ram type = RAM_T2P impl = BRAM

    char pp = 0;
    char token = strm_token_in.read();
    unsigned short cnt = 0;

    while (token) {
#pragma HLS PIPELINE II = 1
        const bool has_input = (token == 1 || token == 3);
        const bool has_output = (token == 2 || token == 3);
        const bool is_last = (cnt == (NUM_BEATS - 1));

        if (has_input) {
            struct_fft_ssr4 din;
            strm_in.read(din);

            const int k = cnt & K_MASK;
            const complex_t w1 = twiddle_W_N<N>(k * STEP);
            const complex_t w2 = twiddle_W_N<N>(2 * k * STEP);
            const complex_t w3 = twiddle_W_N<N>(3 * k * STEP);

            complex_t o0, o1, o2, o3;
            calculate_radix4_dit(din[0], din[1], din[2], din[3], w1, w2, w3, o0, o1, o2, o3);

            const unsigned short wr_low = cnt & 3u;
            const unsigned short wr_local = addr_to_edge_local_r4<N>((unsigned short)cnt);
            ram[pp][wr_low][0][wr_local] = o0;
            ram[pp][wr_low][1][wr_local] = o1;
            ram[pp][wr_low][2][wr_local] = o2;
            ram[pp][wr_low][3][wr_local] = o3;
        }

        if (has_output) {
            struct_fft_ssr4 dout;
            const unsigned short rd = (unsigned short)(cnt * 4);
            const unsigned short rd_high = (rd >> (Log2<N>::value - 2)) & 3u;
            const unsigned short rd_local = addr_to_edge_local_r4<N>(rd);
            dout[0] = ram[1 - pp][0][rd_high][rd_local];
            dout[1] = ram[1 - pp][1][rd_high][rd_local];
            dout[2] = ram[1 - pp][2][rd_high][rd_local];
            dout[3] = ram[1 - pp][3][rd_high][rd_local];
            strm_out.write(dout);
        }

        if (is_last) {
            strm_token_out.write(token);
            if (has_input) {
                token = strm_token_in.read();
            } else if (token == 2) {
                token = 0;
            } else {
                token = strm_token_in.read();
            }
            cnt = 0;
            pp = 1 - pp;
        } else {
            cnt++;
        }
    }
}

template <int N, int STAGE_ID, int LAST_STAGE_ID>
struct stockham_chain_r4 {
    static void run(hls::stream<struct_fft_ssr4>& in,
                    hls::stream<struct_fft_ssr4>& out,
                    hls::stream<ap_uint<2> >& tin,
                    hls::stream<ap_uint<2> >& tout) {
#pragma HLS DATAFLOW
        static hls::stream<struct_fft_ssr4> d_mid("d_mid_r4");
        static hls::stream<ap_uint<2> > t_mid("t_mid_r4");
        stage_stockham_dit_r4<N, STAGE_ID>(in, d_mid, tin, t_mid);
        stockham_chain_r4<N, STAGE_ID + 1, LAST_STAGE_ID>::run(d_mid, out, t_mid, tout);
    }
};

template <int N, int LAST_STAGE_ID>
struct stockham_chain_r4<N, LAST_STAGE_ID, LAST_STAGE_ID> {
    static void run(hls::stream<struct_fft_ssr4>& in,
                    hls::stream<struct_fft_ssr4>& out,
                    hls::stream<ap_uint<2> >& tin,
                    hls::stream<ap_uint<2> >& tout) {
        stage_stockham_dit_r4_final<N, LAST_STAGE_ID>(in, out, tin, tout);
    }
};

// ============================================================================
// Top: bit-reverse stage -> radix-4 stage chain
// ============================================================================
template <int N>
void stockham_radix4_fft_top(hls::stream<struct_fft_ssr4>& strm_in,
                             hls::stream<struct_fft_ssr4>& strm_out,
                             hls::stream<ap_uint<2> >& strm_token_in,
                             hls::stream<ap_uint<2> >& strm_token_out) {
    static const int LOG2N = Log2<N>::value;
    static const int L4 = LOG2N / 2;

#ifndef __SYNTHESIS__
    static_assert((N & (N - 1)) == 0 && N >= 4, "N must be power of 2 and >= 4");
    static_assert((LOG2N % 2) == 0, "radix-4 variant: N must be power of 4");
#endif

#pragma HLS DATAFLOW
    static hls::stream<struct_fft_ssr4> d_br("d_br_r4");
    static hls::stream<ap_uint<2> > t_br("t_br_r4");
    stage_bitreverse_r4<N>(strm_in, d_br, strm_token_in, t_br);
    stockham_chain_r4<N, 0, L4 - 1>::run(d_br, strm_out, t_br, strm_token_out);
}

} // namespace fft
} // namespace dsp
} // namespace xf

#endif // FFT1D_VARIANT_R4_STAGES_HPP
