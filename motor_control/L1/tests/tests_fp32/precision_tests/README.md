# Precision validation (documentation stub)

Float32 **golden** reference math lives only under **`L1/include/models_fp/`** (see `compatibility.hpp`).

**HLS validation** (csim / csynth) runs from each sibling directory under **`L1/tests/tests_fp32/`**, e.g. `clarke_direct_fp32_tb`, `*_apfixed_tb`. Each has its own `Makefile` and `description.json`.

This folder previously held legacy CSV samples and a no-op `Makefile`; those were removed. No standalone build is required here.
