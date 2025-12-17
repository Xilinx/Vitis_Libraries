..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_BENCHMARK:

=============
Benchmark/QoR
=============

This section provides L2 performance benchmarks and Quality of Results (QoR) for the AI Engine solver library elements across various configurations. The results are extracted from hardware-emulation-based simulations.

The devices used for benchmarking are:

- AIE: xcvc1902-vsva2197-2MP-e-S
- AIE-ML: xcve2802-vsvh1760-2MP-e-S
- AIE-MLv2: xc2ve3858-ssva2112-2LP-e-S

The benchmark results are obtained with an AI Engine clock frequency of 1.25 GHz (AIE and AIE-ML devices) or 1.05 GHz (AIE-MLv2), and 64-bit PLIOs at 625 MHz.

The metrics reported for each case are:

- **Latency**: The time delay between the first input sample and the first output sample. If there are multiple ports, the latency is recorded from the first input and first output port.
- **Throughput**: Input throughput calculated based on the number of samples per iteration and the time between each consecutive iteration.
- **NUM_BANKS**: Number of memory banks used by the design.
- **NUM_AIE**: Number of AI Engine tiles used by the design.
- **DATA_MEMORY**: Total data memory in bytes used by the design.
- **PROGRAM_MEMORY**: Program memory in bytes used by each kernel.

The ``AIE_VARIANT`` parameter indicates the AI Engine type used for each case in the benchmark results: AIE, AIE-ML, or AIE-MLv2.

The ``PROGRAM_MEMORY`` metric is collected for each kernel in the design. For example, a QR decomposition (QRD) configured to run on two tiles (``CASC_LEN=2``) will have two sets of figures displayed in the following table (space-delimited).

Latency and Throughput
======================

The latency and throughput values are calculated using the input and output timestamp feature of the aiesimulator. The simulator will create files for the input and output PLIO ports containing the data stream and the timestamp information. Each line of data is preceded by a line containing a timestamp of when it was read or written by the graph port. This feature can be enabled using the option:

.. code-block::

    aiesimulator --graph-latency

The latency value for each iteration is found by calculating the difference between the first input timestamp and the first output timestamp. Throughput is calculated from the number of samples in an iteration divided by the time difference between the first input timestamp of two consecutive iterations.

The latency and throughput values, as reported for each library element in the following tables, are representative of the function operating at a stable rate. In this context, stability is assumed once the latency is consistent across consecutive iterations. Following power-on, systems typically take several iterations before a stable rate is achieved. This is due to the buffers being initially empty and other such effects. The figures reported are from when the system has reached a steady state after this initial transient phase.

In the case where there are multiple input ports and/or multiple output ports, the timestamps from the first of these ports are used as these are the ports that contain the first timestamped sample of each iteration.

Furthermore, if there are no input ports included in the design, then the throughput will be measured using the timestamped data on the output port. In such a case, the latency figures can be marked as invalid with the value reported as ``-1``.

Cholesky
============

The following table provides results for Cholesky across a wide range of supported parameters, defined in :ref:`SOLVER_CONFIGURATION_PARAMETERS_CHOLESKY`.

:download:`cholesky_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2025.2/solver/docs/src/csv_data_files/L2/cholesky_benchmark.csv>`

QRD
============

The following table provides results for QRD across a wide range of supported parameters, defined in :ref:`SOLVER_CONFIGURATION_PARAMETERS_QRD`.

:download:`qrd_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2025.2/solver/docs/src/csv_data_files/L2/qrd_benchmark.csv>`
