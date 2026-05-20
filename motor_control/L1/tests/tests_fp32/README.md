# tests_fp32 — Precision Testbenches for FP32 and ap_fixed Implementations

This directory contains **12 independent HLS testbenches** (6 functions x 2 data
types) designed to measure numerical accuracy across different implementations of
the core FOC building blocks. Every testbench employs **multiple categories of
test vectors** to maximize input-space coverage and expose precision weaknesses
that a single stimulus class would miss.

## Test Matrix

Each function is tested in two variants:

| Function | `_fp32_tb` (float32 vs golden) | `_apfixed_tb` (ap_fixed vs golden) |
|---|---|---|
| Clarke Direct  | `clarke_direct_fp32_tb`  | `clarke_direct_apfixed_tb`  |
| Clarke Inverse | `clarke_inverse_fp32_tb` | `clarke_inverse_apfixed_tb` |
| Park Direct    | `park_direct_fp32_tb`    | `park_direct_apfixed_tb`    |
| Park Inverse   | `park_inverse_fp32_tb`   | `park_inverse_apfixed_tb`   |
| PI Control     | `pi_control_fp32_tb`     | `pi_control_apfixed_tb`     |
| SVPWM          | `svpwm_fp32_tb`          | `svpwm_apfixed_tb`          |

- **`_fp32_tb`**: HLS-synthesizable `float` implementation (`xf::motorcontrol::hls`)
  compared against the template golden model (`xf::motorcontrol::golden`).
- **`_apfixed_tb`**: Existing `ap_fixed` implementation (`hw/`) compared against the
  same golden model, with automatic quantize / dequantize conversion so errors are
  reported in the original float domain.

## Diverse Test Vectors — Why Coverage Matters

A single stimulus category (e.g., random only) tends to cluster around a narrow
region of the input space and cannot reliably uncover errors at boundaries, near
saturation, or under dynamically changing conditions. This framework combines
**multiple complementary stimulus classes** per testbench:

| Stimulus Category | Purpose | Used By |
|---|---|---|
| **Random** | Uniform coverage of the full input range; statistically reveals average-case error. | Clarke, Park, SVPWM |
| **Sinusoidal (3-phase)** | Physically realistic balanced rotating vector; catches errors that appear only under smooth periodic excitation. | Clarke, Park |
| **Angle sweep** | Sweeps electrical angle 0–360° at fine resolution; ensures all quadrants and all six SVPWM sectors are covered. | Park, SVPWM |
| **Rotating vector** | Full 360° rotation in α-β frame; validates inverse transforms end-to-end. | Clarke Inverse, SVPWM |
| **Boundary / corner cases** | Zero inputs, near-saturation, single-phase, unbalanced loads, near-LSB values, asymmetric large amplitudes. | All |
| **Step response** | Closed-loop transient with setpoint step; exposes integral accumulator overflow and mode-change reset behaviour. | PI Control |
| **Sine tracking** | Sinusoidal setpoint; tests dynamic tracking accuracy of the controller over time. | PI Control |
| **Mode change** | Toggles `mode_change` flag mid-run; verifies that the integral state resets correctly. | PI Control (fp32) |
| **Sector coverage** | Explicitly targets all 6 SVPWM sectors with balanced voltages; ensures no sector-boundary discontinuity. | SVPWM |

Each testbench runs **2–3 categories** from the table above, typically producing
**1 000+ random + 200–500 structured + 8–15 boundary** test points per function.

## Error Metrics

Precision results are reported per output channel using `PrecisionAnalyzer`
(`precision_common/precision_analyzer.hpp`):

| Metric | Description |
|---|---|
| MAE | Mean Absolute Error — average-case accuracy |
| RMSE | Root Mean Square Error — penalizes large outliers |
| Max Error | Worst-case single-point deviation |
| SNR (dB) | Signal-to-Noise Ratio — overall signal quality |
| Max / Mean Relative Error | Percentage-based error relative to reference magnitude |

For `_apfixed_tb` tests, point-by-point errors are also exported to CSV files
for offline analysis or plotting.

## Shared Test Infrastructure (`precision_common/`)

| File | Role |
|---|---|
| `stimulus_generator.hpp` | Generates random, sinusoidal, step, and rotating-vector stimuli with configurable amplitude, frequency, and phase. |
| `quantization_utils.hpp` | `float ↔ ap_fixed` conversion with Q-format metadata (range, LSB). |
| `precision_analyzer.hpp/.cpp` | Computes MAE, RMSE, max error, SNR, relative error; pass/fail against configurable thresholds. |
| `hls_test_utils.hpp` | HLS testbench helpers. |

## How to Run

Each testbench directory contains a `Makefile` supporting standard Vitis HLS
targets:

```bash
# C simulation — runs all test vectors, prints precision report
make run TARGET=csim XPART=<part>

# C synthesis — resource & timing estimation
make all TARGET=csynth XPART=<part>
```

To run the full matrix (all 12 testbenches) automatically:

```bash
python3 run_fp32_matrix.py
```

This generates a `REGRESSION_SYNTH_TABLE.md` summarising csim pass/fail, csynth
resource usage (DSP, FF, LUT), estimated clock, latency, and initiation interval
for each testbench.

## Directory Layout

```
tests_fp32/
├── clarke_direct_apfixed_tb/     # ap_fixed<32,16> vs golden
│   ├── src/                      #   HLS top wrapper
│   └── tb/                       #   testbench (random + sine + boundary)
├── clarke_direct_fp32_tb/        # float32 vs golden
│   ├── src/                      #   HLS top wrapper
│   └── tb/                       #   testbench (random + sine + boundary)
├── clarke_inverse_apfixed_tb/
├── clarke_inverse_fp32_tb/
├── park_direct_apfixed_tb/
├── park_direct_fp32_tb/
├── park_inverse_apfixed_tb/
├── park_inverse_fp32_tb/
├── pi_control_apfixed_tb/
├── pi_control_fp32_tb/
├── svpwm_apfixed_tb/
├── svpwm_fp32_tb/
├── precision_common/             # shared test utilities
├── precision_tests/              # additional precision experiments
├── run_fp32_matrix.py            # batch regression runner
└── README.md
```
