..
    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2026, Advanced Micro Devices, Inc.

    `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DSP_BENCHMARK:

=============
Benchmark/QoR
=============

This section provides the L2 performance benchmarks and Quality of Results (QoR) for the AI Engine digital signal processing (DSP) library elements with various configurations. The results are extracted from hardware-emulation-based simulations.

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

If there are no input ports included in the design (such as DDS-only mode) then latency figures will be reported as ``N/A``.

Bitonic Sort
============

The following table gives results for the Bitonic Sort with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_BITONIC_SORT`.

:download:`bitonic_sort_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/bitonic_sort_benchmark.csv>`

Convolution/Correlation
=======================

The following table gives results for Convolution/Correlation with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_CONV_CORR`.

:download:`conv_corr_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/conv_corr_benchmark.csv>`

Cumulative Sum
==============

The following table gives the results for the Cumulative Sum with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_CUMSUM`.

:download:`cumsum_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/cumsum_benchmark.csv>`

DDS/Mixer
=========

The following table gives the results for the DDS/Mixer with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_DDS_MIXER`.

:download:`dds_mixer_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/dds_mixer_benchmark.csv>`

DDS/Mixer LUT
=============

The following table gives the results for the DDS/Mixer LUT with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_DDS_MIXER`.

:download:`dds_mixer_lut_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/dds_mixer_lut_benchmark.csv>`

DFT
===

The following table gives results for the DFT function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_DFT`.

:download:`dft_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/dft_benchmark.csv>`

Euclidean Distance
==================

The following table gives results for the Euclidean Distance with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_EUCLIDEAN_DISTANCE`.

:download:`euclidean_distance_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/euclidean_distance_benchmark.csv>`

FFT IFFT DIT 1CH
================

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for the FFT/IFFT function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_ifft_dit_1ch_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fft_ifft_dit_1ch_benchmark.csv>`

FFT IFFT 2D
===========

The following table gives results for the FFT IFFT 2D function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_ifft_2d_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fft_ifft_2d_benchmark.csv>`

FFT Window
==========

The following table gives results for the FFT Window function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_window_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fft_window_benchmark.csv>`

FIR Decimate Asymmetric
=======================

The following table gives results for the FIR Decimate Asymmetric filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_decimate_asym_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_decimate_asym_benchmark.csv>`

FIR Decimate Halfband
=====================

The following table gives results for the FIR Decimate Halfband filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_decimate_hb_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_decimate_hb_benchmark.csv>`

FIR Decimate Symmetric
======================

The following table gives results for the FIR Decimate Symmetric filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_decimate_sym_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_decimate_sym_benchmark.csv>`

FIR Interpolate Asymmetric
==========================

The following table gives results for the FIR Interpolate Asymmetric filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_interpolate_asym_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_interpolate_asym_benchmark.csv>`

FIR Interpolate Halfband
========================

The following table gives results for the FIR Interpolate Halfband filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_interpolate_hb_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_interpolate_hb_benchmark.csv>`

FIR Resampler
=============

The following table gives results for the FIR Resampler filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_resampler_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_resampler_benchmark.csv>`

FIR Single Rate Symmetric
=========================

The following table gives results for the FIR Single Rate Symmetric filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_sr_sym_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_sr_sym_benchmark.csv>`

FIR Single Rate Asymmetric
==========================

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for the FIR Single Rate Asymmetric filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_sr_asym_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_sr_asym_benchmark.csv>`

FIR TDM
=======

The following table gives results for the TDM FIR filter with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_tdm_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/fir_tdm_benchmark.csv>`

Function Approximation
======================

The following table gives results for the Function Approximation with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_FUNC_APPROX`.

:download:`func_approx_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/func_approx_benchmark.csv>`

Hadamard Product
================

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for the Hadamard Product with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_HADAMARD`.

:download:`hadamard_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/hadamard_benchmark.csv>`

Kronecker
=========

The following table gives results for the Kronecker with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_KRONECKER`.

:download:`kronecker_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/kronecker_benchmark.csv>`

Matrix Multiply
===============

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for the Matrix Multiply function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_GEMM`.

:download:`matrix_mult_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/matrix_mult_benchmark.csv>`

Matrix Vector Multiply
======================

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for the Matrix Vector Multiply function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_GEMV`.

:download:`matrix_vector_mul_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/matrix_vector_mul_benchmark.csv>`

Mixed Radix FFT
===============

The following table gives results for the Mixed Radix FFT function with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_MRFFT`.

:download:`mixed_radix_fft_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/mixed_radix_fft_benchmark.csv>`

Outer Tensor
============

QoR for this library element can be estimated using the ``config_qor_helper`` tool, located at ``dsp/L2/meta/scripts/qor_helper/``. Refer to the README in ``dsp/L2/meta/scripts/qor_helper/`` for usage instructions.

The following table gives results for the Outer Tensor with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_OUTER_TENSOR`.

:download:`outer_tensor_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/outer_tensor_benchmark.csv>`

Sample Delay
============

The following table gives results for the Sample Delay with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_SAMPLE_DELAY`.

:download:`sample_delay_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/sample_delay_benchmark.csv>`

Transpose
=========

The following table gives results for the Transpose with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_TRANSPOSE`.

:download:`transpose_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/transpose_benchmark.csv>`

Widget Real to Complex
======================

The following table gives results for the Widget Real to Complex with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_WIDGETS`.

:download:`widget_real2complex_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/widget_real2complex_benchmark.csv>`

Widget API Cast
===============

The following table gives results for the Widget API Cast with a wide variety of supported parameters, which are defined in: :ref:`DSP_CONFIGURATION_PARAMETERS_WIDGETS`.

:download:`widget_api_cast_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2026.2/dsp/docs/src/csv_data_files/L2/widget_api_cast_benchmark.csv>`
