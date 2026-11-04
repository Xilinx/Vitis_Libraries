# vitis_fft_variant: Self-contained 1D FFT (radix-2 Stockham DIT, radix-4 Cooley–Tukey DIT)

Header-only libraries for float 1D FFT: shared types/tokens, radix-2 multi-stage Stockham DIT, radix-4 DIT, and optional twiddle ROM tables for synthesis. All symbols in `xf::dsp::fft` namespace.

## Contents

- **fft_1d_variant_types.hpp** -- `complex_t`, `struct_fft_ssr2`, `struct_fft_ssr4`, `struct_fft_ssr<N>`, token type and constants.
- **fft1d_variant_r2_utils.hpp** -- `calculate_radix2()` butterfly.
- **fft1d_variant_r2_stages.hpp** -- Multi-stage: `stage_bitreverse`, `stage_stockham_dit`, `stage_stockham_dit_final`, `stockham_fft_top` (parameterized by `NUM_FFT`).
- **fft1d_variant_r4_utils.hpp** -- `calculate_radix4_dit()` butterfly.
- **fft1d_variant_r4_stages.hpp** -- `stockham_radix4_fft_top` (requires `NUM_FFT` a **power of 4**).
- **twiddle_rom_4096.hpp**, **twiddle_rom_16384.hpp** -- Precomputed half-period cos/sin ROMs.

## Usage

- Add include path: `-I <path-to>/L1/include/hw/vitis_fft/float/vitis_fft_variant`.
- Define `NUM_FFT` at compile time (`-DNUM_FFT=...`):
  - Radix-2: power of 2 (default 4096 in headers).
  - Radix-4: **power of 4** (e.g. 64, 256, 1024, 4096, 16384).
- For synthesis, add a `twiddle_rom<N>` specialization for sizes not covered by ROM.

## Tests

- **Radix-2**: `L1/tests/hw/1dfft/float/fft_1d_variant_r2/`
- **Radix-4**: `L1/tests/hw/1dfft/float/fft_1d_variant_r4/`

## License

Copyright (C) 2026 Advanced Micro Devices, Inc.

Terms and Conditions <https://www.amd.com/en/corporate/copyright>
