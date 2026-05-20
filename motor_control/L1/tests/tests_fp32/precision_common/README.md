# Shared precision / HLS test utilities

Headers and sources shared by HLS testbenches under **`L1/tests/tests_fp32/*_fp32_tb/tb/`** and **`L1/tests/tests_fp32/*_apfixed_tb/tb/`** (include path in each `description.json`, `hls_config*.tmpl`, and `run_hls*.tcl`).

Contents include `precision_analyzer.hpp` / `precision_analyzer.cpp`, quantization helpers, and stimulus utilities. Golden templates remain in **`L1/include/models_fp/`**.

Formerly under `L1/tests/precision_common/` (next to `tests_fp32`); co-located under **`tests_fp32/`** so all precision-related test support lives in one tree.
