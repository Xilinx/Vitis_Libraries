# FFT 1D HLS Radix-4 test (vitis_fft_variant)

Cooley–Tukey radix-4 DIT FFT using **`stockham_radix4_fft_top<NUM_FFT>`** from `L1/include/hw/vitis_fft/float/vitis_fft_variant/fft1d_variant_r4_stages.hpp`.

Top function: **`fft1d_variant_r4_stages_top`**.

## Constraints

- **`NUM_FFT` must be a power of 4** (e.g. 64, 256, 1024, 4096, 16384).
- Data path: **`struct_fft_ssr4`** (four complex samples per AXI beat).
- Same **2-bit token** protocol as the radix-2 variant (`fft_1d_variant_r2`).
- Default HLS clock is **2.5 ns** (400 MHz).

## Run (Vitis HLS)

```bash
cd L1/tests/hw/1dfft/float/fft_1d_variant_r4
# default NUM_FFT=4096
make run TARGET=csim

NUM_FFT=256 make run TARGET=csim
```

The testbench uses a **radix-4 DIT software reference** (`goldenFFT_radix4_quiet`, same indexing as the DUT) as the primary golden, and prints a one-time **radix-2 vs radix-4** cross-check on a probe vector.

## Implementation notes

The stage pipeline follows the radix-2 variant organization: bit-reverse, recursive DATAFLOW stage chain, and token-driven ping-pong storage.  The radix-4 stages use explicit BRAM banking so each cycle can service four complex lanes without RAM port conflicts.

For `NUM_FFT=4096` on `xcvc1902-vsva2197-2MP-e-S`, focused validation showed the main stage loops at **II=1**, estimated Fmax about **403 MHz**, and post-route timing met at **2.5 ns**.  Routed resources were approximately LUT 15148, FF 24768, DSP 240, BRAM 552, URAM 0.

## License

Copyright (C) 2026 Advanced Micro Devices, Inc.

Terms and Conditions <https://www.amd.com/en/corporate/copyright>
