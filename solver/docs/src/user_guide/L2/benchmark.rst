..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _SOLVER_BENCHMARK:

=============
Benchmark/QoR
=============

This section provides the L2 performance benchmarks and Quality of Results (QoR) for the AI Engine solver library elements with various configurations. The results are extracted from hardware-emulation-based simulations.

The devices used for benchmarking are:

- AIE: xcvc1902-vsva2197-2MP-e-S
- AIE-ML: xcve2802-vsvh1760-2MP-e-S
- AIE-MLv2: xc2ve3858-ssva2112-2MP-e-S

The benchmark results are obtained using these devices with an AI Engine clock frequency of 1.25 GHz on AIE, AIE-ML and AIE-MLv2 devices.

The QoR metrics which may be reported are:

- **first shortest latency (us)**: latency from the first input sample to the first output sample, measured on the first iteration.
- **first critical latency (us)**: worst-case latency from the first input sample to the first output sample of the slowest output port, measured on the first iteration.
- **steady shortest latency (us)**: same as **first shortest latency (us)**, but measured once the design has reached a steady, repeating rate.
- **steady critical latency (us)**: same as **first critical latency (us)**, but measured once the design has reached a steady, repeating rate.
- **initiation interval (us)**: time between consecutive output iterations once the design reaches a steady rate.
- **input throughput (MSa/s)**: input samples processed per second, in mega-samples/second.
- **output throughput (MSa/s)**: output samples processed per second, in mega-samples/second.
- **frame rate (kHz)**: algorithm frames processed per second, in kHz.

The resource metrics which are reported are:

- **num aie**: number of AIE tiles used by the config for data processing.
- **num banks**: number of memory banks used by a config.
- **num tiles**: number of AIE tiles used by the config, data processing and memory bank inclusive.
- **data memory**: total data memory in bytes used by the design.

The ``AIE_VARIANT`` parameter refers to the type of AI Engine used for each particular case in the benchmark results; this may be AIE, AIE-ML, or AIE-MLv2.

Latency and Throughput
======================

QoR values are derived using the input and output timestamp feature of the ``aiesimulator``. This feature can be enabled using the option:

.. code-block::

     aiesimulator --graph-latency

All QoR metrics besides **first shortest latency (us)** and **first critical latency (us)** are representative of the function operating at a stable rate. Following power-on, systems typically take several iterations before a stable rate is achieved. This is due to the buffers being initially empty and other such effects. The figures reported are from when the system has reached a steady state after this initial transient phase. If steady state has not been achieved when the simulation has terminated, the value is reported as ``-1``.

If there are no input ports included in the design then latency figures will be reported as ``N/A``.

Cholesky
========

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for Cholesky with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_CHOLESKY`.

:download:`cholesky_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/cholesky_benchmark.csv>`

Eigenvalue
==========

The following table gives results for Eigenvalue with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_EIGENVALUE`.

:download:`eigenvalue_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/eigenvalue_benchmark.csv>`

LS Cholesky Solver
==================

The following table gives results for LS_Cholesky_Solver with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_LS_CHOLESKY_SOLVER`.

:download:`ls_cholesky_solver_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/ls_cholesky_solver_benchmark.csv>`

MP Pseudoinverse
================

The following table gives results for MP_Pseudoinverse with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_MP_PSEUDOINVERSE_SVD`.

:download:`mp_pseudoinverse_svd_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/mp_pseudoinverse_svd_benchmark.csv>`

QRD
===

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for QRD with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_QRD`.

:download:`qrd_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/qrd_benchmark.csv>`

QRD HH
======

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for QRD_HH with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_QRD_HH`.

:download:`qrd_hh_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/qrd_hh_benchmark.csv>`

SVD
===

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for SVD with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_SVD`.

:download:`svd_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/svd_benchmark.csv>`

Substitution
============

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for Substitution with a wide variety of supported parameters, which are defined in: :ref:`SOLVER_CONFIGURATION_PARAMETERS_SUBSTITUTION`.

:download:`substitution_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/solver/docs/src/csv_data_files/L2/substitution_benchmark.csv>`
