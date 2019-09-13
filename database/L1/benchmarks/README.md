# Key L1 Module Benchmark Test

The Vitis projects in this folder are for benchmarking the performance of key L1 modules,
whose proformance depends dynamically on input and cannot be simply rated via clock frequency.

Since L1 modules cannot run alone on Alveo cards, the target modules are combined with
minimal supportive L1 modules into Vitis kernels.
