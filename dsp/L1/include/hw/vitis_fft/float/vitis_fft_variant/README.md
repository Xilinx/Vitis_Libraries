# vitis_fft_variant: Self-contained radix-2 1D FFT (Stockham DIT)

Header-only library for radix-2 1D FFT: types, butterfly utils, multi-stage Stockham DIT stages, and optional twiddle ROM tables for synthesis. All symbols in `xf::dsp::fft` namespace.

## Contents

- **fft_1d_variant_types.hpp** -- `complex_t`, `struct_fft_ssr2`, `struct_fft_ssr<N>`, token type and constants.
- **fft1d_variant_r2_utils.hpp** -- `calculate_radix2()` butterfly.
- **fft1d_variant_r2_stages.hpp** -- Multi-stage: `stage_bitreverse`, `stage_stockham_dit`, `stage_stockham_dit_final`, `stockham_fft_top` (parameterized by `NUM_FFT`).
- **twiddle_rom_4096.hpp**, **twiddle_rom_16384.hpp** -- Precomputed half-period cos/sin ROMs.

## Usage

- Add include path: `-I <path-to>/L1/include/hw/vitis_fft/float/vitis_fft_variant`.
- Define `NUM_FFT` (power of 2) before including `fft1d_variant_r2_stages.hpp` if not using the default; for synthesis, add a `twiddle_rom<N>` specialization for sizes not covered by ROM.

## Tests

- **Multi-stage**: `L1/tests/hw/1dfft/float/fft_1d_variant_r2/`
