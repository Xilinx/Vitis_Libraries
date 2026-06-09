/*
 * Copyright (C) 2025-2026, Advanced Micro Devices, Inc.
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
 * Stockham DIT radix-2 FFT stages — fully templated by FFT size N.
 *
 * Token protocol (ap_uint<2>):
 *   01 = first (write RAM only), 11 = normal (write + read), 10 = last (read only), 00 = quit.
 * RAM: ram[2][N] ping-pong; pp_state toggles each batch.
 *
 * Template hierarchy:
 *   stockham_fft_top<N, T>            — top-level DATAFLOW pipeline
 *     stage_bitreverse<T, N>          — bit-reverse input permutation
 *     stockham_chain<T, N, 0, L-1>    — recursive chain of butterfly stages
 *       stage_stockham_dit<T, N, s>   — generic Stockham DIT stage (s = 0..L-2)
 *       stage_stockham_dit_final<T,N,L-1> — final stage with natural-order output
 *
 * To support a new FFT size in synthesis, specialize twiddle_rom<N> with a
 * hardcoded cos/sin ROM (see twiddle_rom<64> below).  The generic version
 * computes twiddles via cosf/sinf, which works for C-simulation but maps to
 * floating-point cores in synthesis.
 */
#ifndef FFT1D_VARIANT_R2_STAGES_HPP
#define FFT1D_VARIANT_R2_STAGES_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <cmath>

#ifndef NUM_FFT
#define NUM_FFT 4096
#endif

#include "fft1d_variant_r2_utils.hpp"

namespace xf {
namespace dsp {
namespace fft {

#ifndef __SYNTHESIS__
namespace dut_debug {
inline std::vector<std::vector<complex_t> >& stage_output(int stage_id) {
    static std::vector<std::vector<complex_t> > arr[16];
    return arr[stage_id];
}
}
#endif

// ============================================================================
// Compile-time log2.  N must be a power of 2.
// ============================================================================
template <int N>
struct Log2 {
    static const int value = 1 + Log2<N / 2>::value;
};
template <>
struct Log2<1> {
    static const int value = 0;
};

// ============================================================================
// Twiddle factor ROM: W_N^k = exp(-j*2*pi*k/N), k = 0 .. N/2-1
// ============================================================================

// Generic version: runtime cosf/sinf.  Works for CSIM; for synthesis add a
// specialization with hardcoded ROM entries (see N=64 below).
template <int N>
struct twiddle_rom {
    static const int ROM_SIZE = N / 2;
    static inline void get(int idx, float& c, float& s) {
#pragma HLS INLINE
        float angle = (float)(-2.0 * 3.14159265358979323846 * idx / N);
        c = cosf(angle);
        s = sinf(angle);
    }
};

// N=64 specialization: 32-entry hardcoded ROM for optimal HLS synthesis.
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
        c = COS[idx];
        s = SIN[idx];
    }
};

// N=4096: precomputed half-period ROM.  Without this, cosf/sinf synthesize to
// HOTBM cores (~2k LUT + DSP each) and are duplicated per stage — hundreds of
// DSPs and tens of thousands of LUTs.  ROM maps to BRAM/URAM with a single read.
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

// N=16384: full specialization in twiddle_rom_16384.hpp (8192-entry COS/SIN ROMs).
#include "twiddle_rom_16384.hpp"

// DIT twiddle lookup: STRIDE = 1 << (log2(N)-1-STAGE_ID), simple->complex.
template <int N, int STAGE_ID>
struct twiddle_dit {
    static const int LOG2N = Log2<N>::value;
    static const int STRIDE = 1 << (LOG2N - 1 - STAGE_ID);
    static const int MASK = (N / 2 / STRIDE) - 1;

    static inline complex_t W(int cnt) {
#pragma HLS INLINE
        int idx = (cnt & MASK) * STRIDE;
        float c, s;
        twiddle_rom<N>::get(idx, c, s);
        return complex_t(c, s);
    }
};

// ============================================================================
// Combinational bit-reverse: pure wire permutation, synthesizes to zero logic.
// ============================================================================
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

// ============================================================================
// Bit-reverse permutation stage (no butterfly)
// Write: data_in[0] -> ram[bitrev(cnt*2)], data_in[1] -> ram[bitrev(cnt*2+1)]
// Read:  sequential pairs ram[cnt*2], ram[cnt*2+1].
// ============================================================================
template <typename T, int N>
void stage_bitreverse(hls::stream<T>& strm_in,
                      hls::stream<T>& strm_out,
                      hls::stream<ap_uint<2> >& strm_token_in,
                      hls::stream<ap_uint<2> >& strm_token_out) {
    static const int LOG2N = Log2<N>::value;

    complex_t ram[2][N];
#pragma HLS ARRAY_PARTITION variable = ram dim = 1
#pragma HLS ARRAY_PARTITION variable = ram dim = 2 off = true
#pragma HLS BIND_STORAGE variable = ram type = RAM_T2P impl = URAM

    char pp_state = 0;
    char token = strm_token_in.read();
    unsigned short cnt = 0;
    bool isRead, isWrite, isLast;

    while (token) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 32 max = 65536 avg = 2048
        isRead = (token == 1 || token == 3);
        isWrite = (token == 2 || token == 3);
        isLast = (cnt == (N / 2 - 1));

        T data_in;
        if (isRead) {
            strm_in.read(data_in);
            unsigned short wr0 = bitrev_compute<LOG2N>((unsigned short)(cnt * 2));
            unsigned short wr1 = bitrev_compute<LOG2N>((unsigned short)(cnt * 2 + 1));
            ram[pp_state][wr0] = data_in[0];
            ram[pp_state][wr1] = data_in[1];
#if !defined(__SYNTHESIS__) && (NUM_FFT <= 256)
            printf("BITREV cnt %2d: x[%d]->ram[%d], x[%d]->ram[%d]\n", cnt, cnt * 2, wr0, cnt * 2 + 1, wr1);
#endif
        }

        T data_out;
        data_out[0] = ram[1 - pp_state][cnt * 2];
        data_out[1] = ram[1 - pp_state][cnt * 2 + 1];

        if (isWrite) {
            strm_out.write(data_out);
        }

        if (isLast) {
#ifndef __SYNTHESIS__
            dut_debug::stage_output(16 - 1).clear();
#endif
            strm_token_out.write(token);
            if (isRead)
                token = strm_token_in.read();
            else if (token == 2)
                token = 0;
            cnt = 0;
            pp_state = 1 - pp_state;
        } else {
            cnt++;
        }
    }
}

// ============================================================================
// Bank mapping for II=1: two write addresses differ in bit (STAGE_ID+1).
// Partition by that bit so the two writes go to different banks (1 write/cycle/bank).
// Linear addr in [0,N-1] -> bank in {0,1}, local in [0, N/2-1].
// ============================================================================
template <int N, int STAGE_ID>
void addr_to_bank_local(unsigned short addr, unsigned short& bank, unsigned short& local) {
#pragma HLS INLINE
    static const int LOG2N = Log2<N>::value;
    unsigned short bit_pos = (1 << (STAGE_ID + 1));
    bank = (addr >> (STAGE_ID + 1)) & 1u;
    unsigned short low = addr & (bit_pos - 1);
    unsigned short high = (addr >> (STAGE_ID + 2));
    local = low | (high << (STAGE_ID + 1));
}

// ============================================================================
// Generic Stockham DIT stage: butterfly + rotate-left write addressing.
// RAM split into 2 banks by bit (STAGE_ID+1) so both butterfly writes hit different banks -> II=1.
// ============================================================================
template <typename T, int N, int STAGE_ID>
void stage_stockham_dit(hls::stream<T>& strm_in,
                        hls::stream<T>& strm_out,
                        hls::stream<ap_uint<2> >& strm_token_in,
                        hls::stream<ap_uint<2> >& strm_token_out) {
    static const int HALF = N / 2;
    complex_t ram[2][2][HALF];
#pragma HLS ARRAY_PARTITION variable = ram dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 2 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 3 off
#pragma HLS BIND_STORAGE variable = ram type = RAM_T2P impl = URAM

    char pp_state = 0;
    char token = strm_token_in.read();
    unsigned short cnt = 0;
    bool isRead, isWrite, isLast;

#ifndef __SYNTHESIS__
    std::vector<complex_t> dbg_out;
#endif

    while (token) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 32 max = 65536 avg = 2048
        isRead = (token == 1 || token == 3);
        isWrite = (token == 2 || token == 3);
        isLast = (cnt == (N / 2 - 1));

        // Stockham write addressing: rotate-left lower (STAGE_ID+1) bits, insert 0/1 at bit (STAGE_ID+1).
        unsigned short lower = cnt & ((1 << (STAGE_ID + 1)) - 1);
        unsigned short rotated = ((lower << 1) | (lower >> STAGE_ID)) & ((1 << (STAGE_ID + 1)) - 1);
        unsigned short upper = cnt >> (STAGE_ID + 1);
        unsigned short addr0_2 = (upper << (STAGE_ID + 2)) | rotated;
        unsigned short addr1_2 = (upper << (STAGE_ID + 2)) | (1 << (STAGE_ID + 1)) | rotated;

        unsigned short b0, l0, b1, l1;
        addr_to_bank_local<N, STAGE_ID>(addr0_2, b0, l0);
        addr_to_bank_local<N, STAGE_ID>(addr1_2, b1, l1);

        T data_in;
        complex_t out0, out1;
        if (isRead) {
            strm_in.read(data_in);
            complex_t twiddle = twiddle_dit<N, STAGE_ID>::W(cnt);
            calculate_radix2(data_in[0], data_in[1], twiddle, out0, out1);
            ram[pp_state][b0][l0] = out0;
            ram[pp_state][b1][l1] = out1;
        }

        unsigned short rb0, rl0, rb1, rl1;
        addr_to_bank_local<N, STAGE_ID>((unsigned short)(cnt * 2), rb0, rl0);
        addr_to_bank_local<N, STAGE_ID>((unsigned short)(cnt * 2 + 1), rb1, rl1);
        T data_out;
        data_out[0] = ram[1 - pp_state][rb0][rl0];
        data_out[1] = ram[1 - pp_state][rb1][rl1];

#if !defined(__SYNTHESIS__) && (NUM_FFT <= 256)
        if (isRead) {
            printf("STAGE %d cnt %2d: in(%7.3f,%7.3f)(%7.3f,%7.3f) -> out(%7.3f,%7.3f)(%7.3f,%7.3f) wr[%d,%d]\n",
                   STAGE_ID, cnt, data_in[0].real(), data_in[0].imag(), data_in[1].real(), data_in[1].imag(),
                   out0.real(), out0.imag(), out1.real(), out1.imag(), (unsigned short)addr0_2,
                   (unsigned short)addr1_2);
        }
#endif
        if (isWrite) {
            strm_out.write(data_out);
#ifndef __SYNTHESIS__
            dbg_out.push_back(data_out[0]);
            dbg_out.push_back(data_out[1]);
#endif
        }

        if (isLast) {
#ifndef __SYNTHESIS__
            if (isWrite && (int)dbg_out.size() == N) {
                dut_debug::stage_output(STAGE_ID).push_back(dbg_out);
                dbg_out.clear();
            }
#endif
            strm_token_out.write(token);
            if (isRead)
                token = strm_token_in.read();
            else if (token == 2)
                token = 0;
            cnt = 0;
            pp_state = 1 - pp_state;
        } else {
            cnt++;
        }
    }
}

// Final stage: write to (cnt, cnt+N/2) -> differ in MSB (bit LOG2N-1). Bank = addr >> (LOG2N-1), local = addr &
// (N/2-1).
template <int N>
void addr_to_bank_local_final(unsigned short addr, unsigned short& bank, unsigned short& local) {
#pragma HLS INLINE
    static const int LOG2N = Log2<N>::value;
    static const int HALF = N / 2;
    bank = (addr >> (LOG2N - 1)) & 1u;
    local = addr & (HALF - 1);
}

// ============================================================================
// Final Stockham DIT stage: butterfly -> write to (cnt, cnt+N/2) for natural order.
// RAM split into 2 banks by MSB so both writes hit different banks -> II=1.
// ============================================================================
template <typename T, int N, int STAGE_ID>
void stage_stockham_dit_final(hls::stream<T>& strm_in,
                              hls::stream<T>& strm_out,
                              hls::stream<ap_uint<2> >& strm_token_in,
                              hls::stream<ap_uint<2> >& strm_token_out) {
    static const int HALF = N / 2;
    complex_t ram[2][2][HALF];
#pragma HLS ARRAY_PARTITION variable = ram dim = 1 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 2 complete
#pragma HLS ARRAY_PARTITION variable = ram dim = 3 off
#pragma HLS BIND_STORAGE variable = ram type = RAM_T2P impl = URAM

    char pp_state = 0;
    char token = strm_token_in.read();
    unsigned short cnt = 0;
    bool isRead, isWrite, isLast;

#ifndef __SYNTHESIS__
    std::vector<complex_t> dbg_out;
#endif

    while (token) {
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_TRIPCOUNT min = 32 max = 65536 avg = 2048
        isRead = (token == 1 || token == 3);
        isWrite = (token == 2 || token == 3);
        isLast = (cnt == (HALF - 1));

        unsigned short wb0, wl0, wb1, wl1;
        addr_to_bank_local_final<N>(cnt, wb0, wl0);
        addr_to_bank_local_final<N>((unsigned short)(cnt + HALF), wb1, wl1);
        T data_in;
        if (isRead) {
            strm_in.read(data_in);
            complex_t out0, out1;
            complex_t twiddle = twiddle_dit<N, STAGE_ID>::W(cnt);
            calculate_radix2(data_in[0], data_in[1], twiddle, out0, out1);
            ram[pp_state][wb0][wl0] = out0;
            ram[pp_state][wb1][wl1] = out1;
#if !defined(__SYNTHESIS__) && (NUM_FFT <= 256)
            printf(
                "STAGE %d cnt %2d: in(%7.3f,%7.3f)(%7.3f,%7.3f) tw(%7.3f,%7.3f) -> out(%7.3f,%7.3f)(%7.3f,%7.3f) "
                "wr[%d,%d]\n",
                STAGE_ID, cnt, data_in[0].real(), data_in[0].imag(), data_in[1].real(), data_in[1].imag(),
                twiddle.real(), twiddle.imag(), out0.real(), out0.imag(), out1.real(), out1.imag(), cnt, cnt + HALF);
#endif
        }

        unsigned short rb0, rl0, rb1, rl1;
        addr_to_bank_local_final<N>((unsigned short)(cnt * 2), rb0, rl0);
        addr_to_bank_local_final<N>((unsigned short)(cnt * 2 + 1), rb1, rl1);
        T data_out;
        data_out[0] = ram[1 - pp_state][rb0][rl0];
        data_out[1] = ram[1 - pp_state][rb1][rl1];

        if (isWrite) {
            strm_out.write(data_out);
#ifndef __SYNTHESIS__
            dbg_out.push_back(data_out[0]);
            dbg_out.push_back(data_out[1]);
#endif
        }

        if (isLast) {
#ifndef __SYNTHESIS__
            if (isWrite && (int)dbg_out.size() == N) {
                dut_debug::stage_output(STAGE_ID).push_back(dbg_out);
                dbg_out.clear();
            }
#endif
            strm_token_out.write(token);
            if (isRead)
                token = strm_token_in.read();
            else if (token == 2)
                token = 0;
            cnt = 0;
            pp_state = 1 - pp_state;
        } else {
            cnt++;
        }
    }
}

// ============================================================================
// Recursive stage chain (nested DATAFLOW regions).
//   Chains stages STAGE_ID through LAST_STAGE_ID.
//   Generic stages use stage_stockham_dit; the final uses stage_stockham_dit_final.
//   Each level creates a DATAFLOW region: current stage + recursive tail.
// ============================================================================
template <typename T, int N, int STAGE_ID, int LAST_STAGE_ID>
struct stockham_chain {
    static void run(hls::stream<T>& in,
                    hls::stream<T>& out,
                    hls::stream<ap_uint<2> >& tin,
                    hls::stream<ap_uint<2> >& tout) {
#pragma HLS DATAFLOW
        static hls::stream<T> d_mid;
        static hls::stream<ap_uint<2> > t_mid;
        stage_stockham_dit<T, N, STAGE_ID>(in, d_mid, tin, t_mid);
        stockham_chain<T, N, STAGE_ID + 1, LAST_STAGE_ID>::run(d_mid, out, t_mid, tout);
    }
};

// Base case: single final stage (no DATAFLOW needed).
template <typename T, int N, int LAST_STAGE_ID>
struct stockham_chain<T, N, LAST_STAGE_ID, LAST_STAGE_ID> {
    static void run(hls::stream<T>& in,
                    hls::stream<T>& out,
                    hls::stream<ap_uint<2> >& tin,
                    hls::stream<ap_uint<2> >& tout) {
        stage_stockham_dit_final<T, N, LAST_STAGE_ID>(in, out, tin, tout);
    }
};

// ============================================================================
// Top-level Stockham DIT FFT template
// Pipeline: bit-reverse -> (LOG2N-1) generic stages -> 1 final stage
// Total stages = LOG2N + 1 (including bit-reverse).
//
// Usage:
//   stockham_fft_top<64, struct_fft_ssr2>(in, out, tok_in, tok_out);
// ============================================================================
template <int N, typename T>
void stockham_fft_top(hls::stream<T>& strm_in,
                      hls::stream<T>& strm_out,
                      hls::stream<ap_uint<2> >& strm_token_in,
                      hls::stream<ap_uint<2> >& strm_token_out) {
#pragma HLS DATAFLOW

    static const int LOG2N = Log2<N>::value;

    static hls::stream<T> d_br("d_br");
    static hls::stream<ap_uint<2> > t_br("t_br");

    stage_bitreverse<T, N>(strm_in, d_br, strm_token_in, t_br);
    stockham_chain<T, N, 0, LOG2N - 1>::run(d_br, strm_out, t_br, strm_token_out);
}

} // namespace fft
} // namespace dsp
} // namespace xf

#endif // FFT1D_VARIANT_R2_STAGES_HPP
