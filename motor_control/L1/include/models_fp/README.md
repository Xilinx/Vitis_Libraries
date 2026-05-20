# models_fp — Floating-Point Motor Control Models

This directory provides **two complementary implementations** for each of the six
classic Field-Oriented Control (FOC) building blocks. The paired design enables
both HLS synthesis and cross-type precision analysis within a single source tree.

## Functions

| Function | HLS-Synthesizable (`_fp32`) | Golden Reference (template) |
|---|---|---|
| Clarke Direct  | `clarke_direct_fp32.hpp`  | `clarke_direct.hpp`  |
| Clarke Inverse | `clarke_inverse_fp32.hpp` | `clarke_inverse.hpp` |
| Park Direct    | `park_direct_fp32.hpp`    | `park_direct.hpp`    |
| Park Inverse   | `park_inverse_fp32.hpp`   | `park_inverse.hpp`   |
| PI Control     | `pi_control_fp32.hpp`     | `pi_control.hpp`     |
| SVPWM          | `svpwm_fp32.hpp`          | `svpwm.hpp`          |

`compatibility.hpp` is a convenience header that includes all six golden
reference headers in one shot.

## Two Kinds of Models

### `*_fp32.hpp` — HLS-Synthesizable FP32 Implementation

- **Namespace:** `xf::motorcontrol::hls`
- Hardcoded `float` parameters and return types.
- Contains HLS pragmas (`#pragma HLS INLINE`, `#pragma HLS PIPELINE II=1`),
  ready for Vitis HLS synthesis and resource constraint exploration.
- Suitable as the synthesis target wrapped by a non-inline top function
  (see `tests_fp32/*_fp32_tb/src/*_fp32_top.cpp`).

```cpp
#include "clarke_direct_fp32.hpp"
xf::motorcontrol::hls::Clarke_Direct_3p_fp32(ialpha, ibeta, ihomop, ia, ib, ic);
```

### `*.hpp` (no `_fp32` suffix) — Template Golden Reference Model

- **Namespace:** `xf::motorcontrol::golden`
- Template parameter `T` defaults to `float` but accepts any arithmetic type:
  `double`, `ap_fixed<W,I>`, `Q16.15`, or standard integer types.
- Pure C++ — no HLS pragmas — portable to any host compiler.
- When instantiated with `double` or `float`, the result is the highest-precision reference
  available; comparing other data-type instantiations against it reveals the
  numerical error introduced by quantization (e.g., `ap_fixed` vs. `double`).

```cpp
#include "clarke_direct.hpp"

// double-precision golden reference
double a, b, h;
xf::motorcontrol::golden::clarke_direct_golden<double>(a, b, h, ia, ib, ic);

// ap_fixed — same algorithm, measure precision loss vs. double
ap_fixed<32,16> a2, b2, h2;
xf::motorcontrol::golden::clarke_direct_golden<ap_fixed<32,16>>(a2, b2, h2, ia2, ib2, ic2);
```

## Typical Workflow

1. **Synthesize** the `_fp32` variant through Vitis HLS to obtain resource and
   timing estimates on the target FPGA.
2. **Validate** the `_fp32` output against the `double`-instantiated golden
   reference to quantify the fp32 rounding error.
3. **Evaluate alternative types** (e.g., `ap_fixed<24,12>`) by instantiating the
   golden template and comparing to the `double` baseline, enabling a
   precision-vs-resource trade-off analysis without modifying any source.

## Directory Layout

```
models_fp/
├── clarke_direct.hpp          # golden template
├── clarke_direct_fp32.hpp     # HLS-synthesizable fp32
├── clarke_inverse.hpp
├── clarke_inverse_fp32.hpp
├── park_direct.hpp
├── park_direct_fp32.hpp
├── park_inverse.hpp
├── park_inverse_fp32.hpp
├── pi_control.hpp
├── pi_control_fp32.hpp
├── svpwm.hpp
├── svpwm_fp32.hpp
├── compatibility.hpp          # includes all golden headers
└── README.md
```
