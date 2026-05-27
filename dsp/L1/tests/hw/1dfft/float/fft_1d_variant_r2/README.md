# FFT 1D HLS Radix-2 test (vitis_fft_variant)

Stockham DIT radix-2 FFT using **`stockham_fft_top<NUM_FFT, struct_fft_ssr2>`** from `L1/include/hw/vitis_fft/float/vitis_fft_variant/` (`fft1d_variant_r2_stages.hpp`).

Top function: **`fft1d_variant_r2_stages_top`**.

## Design highlights

### 1. Compact recursive synthesizable code

The entire LOG2N-stage pipeline is generated from a single C++ template through compile-time recursion (`stockham_chain<T, N, STAGE_ID, LAST_STAGE_ID>`). Each stage instantiation shares the same `stage_stockham_dit` source, with twiddle strides, bank-split bit positions, and address logic fully resolved at compile time — no runtime overhead. Adding support for a new FFT size requires only a twiddle ROM specialization; the stage logic itself is unchanged.

### 2. Logic area nearly independent of FFT size

DSP and LUT consumption is dominated by a single radix-2 butterfly (`calculate_radix2`) reused across all stages at II=1. Increasing N mainly grows **URAM** (ping-pong storage per stage: 2 banks x N/2 complex floats) while DSP, LUT, and FF stay nearly constant. This makes the design highly scalable for large point sizes.

### 3. Token-driven continuous streaming

A 2-bit token protocol (`ap_uint<2>`) flows alongside data through every stage, enabling back-to-back multi-vector processing with no pipeline flush between vectors.

| Token | Binary | Constant | Read input | Write output | Description |
|-------|--------|----------|:----------:|:------------:|-------------|
| FIRST | `01` | `TOKEN_FIRST` | yes | no | Prime the pipeline: write first vector into ping-pong RAM, no output yet |
| NORMAL | `11` | `TOKEN_NORMAL` | yes | yes | Steady state: read new vector while outputting previous result |
| LAST | `10` | `TOKEN_LAST` | no | yes | Drain the pipeline: output last result, no new input |
| IDLE | `00` | `TOKEN_IDLE` | no | no | Terminate: exit the processing loop |

Bit encoding: `bit[0]` = read enable, `bit[1]` = write enable. The `while(token)` loop naturally exits on `TOKEN_IDLE (00)`.

**Throughput:** for K vectors of N points (SSR=2, so N/2 beats per vector), the first output appears after 1 vector latency; the remaining K-1 outputs stream at full rate. Total cycles ≈ (K+1) x N/2.

## FFT size

Set **`NUM_FFT`** (power of 2) at build time. Default in the Makefile is **4096**.

```bash
# 4096-point (default)
make run TARGET=csim

# Other sizes, e.g. 64 or 16384
NUM_FFT=64 make run TARGET=csim
NUM_FFT=16384 make run TARGET=csim
```

`hls_config.tmpl` passes `-DNUM_FFT=${NUM_FFT}` to the DUT and testbench. The Makefile **always regenerates** `hls_config.cfg` from the template on each run (so changing `NUM_FFT` on the command line cannot accidentally reuse a stale `-DNUM_FFT` from an earlier build).

**Twiddle ROMs (synthesis):** the library provides **`twiddle_rom`** specializations for **64, 4096, 16384** (see `twiddle_rom_4096.hpp`, `twiddle_rom_16384.hpp` next to `fft1d_variant_r2_stages.hpp`). Other sizes fall back to `cosf`/`sinf` in HLS (high LUT/DSP); add a ROM for production use.

## Build and run

Prerequisite: `source` your Vitis/Vivado environment (e.g. `env.sh`).

```bash
cd L1/tests/hw/1dfft/float/fft_1d_variant_r2

# C simulation (no FPGA part required for config; needs vitis-run)
make run TARGET=csim

# Post-implementation (use separate WORK_DIR to keep builds)
WORK_DIR=hls_4096 NUM_FFT=4096 make run TARGET=vivado_impl PLATFORM=vck190

make clean
```

## Testbench

**`test_fft1d_variant_r2_tb.cpp`** runs **eight** stimulus types back-to-back (impulse, DC, sines, square, triangular, counter, random), compares DUT output to a **golden radix-2 DIT FFT** using SNR and optional max-error thresholds. Large `NUM_FFT` uses relaxed tolerance.

## Files

- **fft1d_variant_r2_top.cpp** — HLS top `fft1d_variant_r2_stages_top`; calls `stockham_fft_top<NUM_FFT, …>`.
- **test_fft1d_variant_r2_tb.cpp** — Multi-vector regression (SNR / optional point-wise).
- **vector_factory.hpp** — Waveforms, `goldenFFT`, compare/print helpers.
- **hls_config.tmpl** — Vitis HLS config (`XF_PROJ_ROOT`, `-DNUM_FFT`).
- **Makefile** — `NUM_FFT ?= 4096`, `export NUM_FFT`; `csim` builds config only (no Vivado required for that step).

Library headers in `L1/include/hw/vitis_fft/float/vitis_fft_variant/`:

- **fft_1d_variant_types.hpp** — `complex_t`, `struct_fft_ssr2`, `struct_fft_ssr<N>`, token type and constants.
- **fft1d_variant_r2_utils.hpp** — `calculate_radix2()` butterfly.
- **fft1d_variant_r2_stages.hpp** — Multi-stage pipeline, `stockham_fft_top` template.
- **twiddle_rom_4096.hpp**, **twiddle_rom_16384.hpp** — Precomputed half-period cos/sin ROMs.

## Resource comparison (N=16384, SSR=2, xcvc1902, clock 2 ns)

| Configuration | FFT Size | SSR | DSP | BRAM | URAM | LUT | FF | Max Freq (MHz) |
|---|---|---|---|---|---|---|---|---|
| fft_1d_snr_ssr_2 (existing) | 16384 | 2 | 273 | 96 | 4 | 85050 | 56049 | 500 |
| fft_1d_variant_r2 (this design) | 16384 | 2 | 111 | 288 | 120 | 10408 | 16044 | 458.9 |

**Synthesis note:** stages use `BIND_STORAGE` with URAM for ping-pong RAM; without block memory, dual reads can cost large LUT (muxes). Reports: `WORK_DIR/hls/syn/report/`.
