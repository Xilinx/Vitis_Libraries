..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _BENCHMARK:

=============
Benchmark/QoR
=============

This section provides the L2 performance benchmarks and Quality of Results (QoR) for the AI Engine (AIE) digital signal processing (DSP) library elements with various configurations. The results are extracted from a hardware emulation based simulations.

The device used for AIE benchmarking is the xcvc1902-vsva2197-2MP-e-S, and the device used for AIE-ML is the xcve2802-vsvh1760-2MP-e-S.
The benchmark results are obtained using these devices wth an AIE clock frequency of 1.25 GHz and 64-bit PLIOs at 625 MHz.

The metrics reported for each case are:

- **Latency**: The time delay between the first input sample and the first output sample. If there are multiple ports, the latency is recorded from the first input and first output port.
- **Throughput**: Input throughput calculated based on the number of samples per iteration and the time between each consecutive iteration.
- **NUM_BANKS**: Number of memory banks used by the design.
- **NUM_AIE**: Number of AIE tiles used by the design.
- **DATA_MEMORY**: Total data memory in bytes used by the design.
- **PROGRAM_MEMORY**: Program memory in bytes used by each kernel.

The AIE_VARIANT parameter refers to the type of AI Engine that is used for each particular case in the benchmark results, this may be AIE or AIE-ML.

The PROGRAM_MEMORY metrics are harvested for each kernel the design consists of. For example, a finite impulse response (FIR) configured to be implemented on two tiles (CASC_LEN=2) will have two sets of figures displayed in the following table (space delimited).

Latency and Throughput
======================

The latency and throughput values are calculated using the input and output timestamp feature of the aiesimulator. The simulator will create files for the input and output PLIO ports containing the data stream and the timestamp information. Each line of data is preceded by a line containing a timestamp of when it was read or written by the graph port. This feature can be enabled using the option:

.. code-block::

    aiesimulator --graph-latency

The latency value for each iteration is found by calculating the difference between the first input timestamp and the first output timestamp. Throughput is calculated from the number of samples in an iteration divided by the time difference between the first input timestamp of two consecutive iterations.

The latency and throughput values, as reported for each library element in the following tables, are representative of the function operating at a stable rate. In this context, stability is assumed once the latency is consistent across consecutive iterations. Following power-on, systems typically take several iterations before a stable rate is achieved. This is due to the buffers being initially empty and other such effects. The figures reported are from when the system has reached a steady state after this initial transient phase.

In the case where there are multiple input ports and/or multiple output ports, the timestamps from the first of these ports are used as these are the ports that contain the first timestamped sample of each iteration.

Furthermore, if there are no input ports included in the design (such as DDS only mode), then the throughput will be measured using the timestamped data on the output port. In such a case, the latency figures can be marked as invalid with the value reported as ``-1``.

Bitonic Sort
============

The following table gives results for the Bitonic Sort with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_BITONIC_SORT`.

:download:`bitonic_sort_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/bitonic_sort_benchmark.csv>`

.. csv-table:: Bitonic Sort benchmark
   :file: ../../csv_data_files/L2/bitonic_sort_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Convolution / Correlation
=========================

The following table gives results for the Convolution / Correlation with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_CONV_CORR`.

:download:`conv_corr_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/conv_corr_benchmark.csv>`

.. csv-table:: Convolution / Correlation benchmark
   :file: ../../csv_data_files/L2/conv_corr_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

DDS/Mixer
=========

The following table gives the results for the DDS/Mixer and DDS/Mixer LUT with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_DDS_MIXER`.

:download:`dds_mixer_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/dds_mixer_benchmark.csv>`

.. csv-table:: DDS/Mixer benchmark
   :file: ../../csv_data_files/L2/dds_mixer_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

DFT
===

The following table gives results for the DFT function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_DFT`.

:download:`dft_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/dft_benchmark.csv>`

.. csv-table:: DFT benchmark
   :file: ../../csv_data_files/L2/dft_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

FFT IFFT DIT 1CH
================

The following table gives results for the FFT/IFFT function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_ifft_dit_1ch_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/fft_ifft_dit_1ch_benchmark.csv>`

.. csv-table:: FFT IFFT DIT 1CH benchmark
   :file: ../../csv_data_files/L2/fft_ifft_dit_1ch_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

FFT IFFT 2D
===========

The following table gives results for the FFT IFFT 2D function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_ifft_2d_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/fft_ifft_2d_benchmark.csv>`

.. csv-table:: FFT IFFT 2D benchmark
   :file: ../../csv_data_files/L2/fft_ifft_2d_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

FFT Window
==========

The following table gives results for the FFT Window function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_window_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/fft_window_benchmark.csv>`

.. csv-table:: FFT Window benchmark
   :file: ../../csv_data_files/L2/fft_window_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Filters
=======

The following table gives results for FIR filter with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/fir_benchmark.csv>`

.. csv-table:: FIR benchmark
   :file: ../../csv_data_files/L2/fir_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

FIR TDM
=======

The following table gives results for TDM FIR filter with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FILTERS`.

:download:`fir_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/fir_tdm_benchmark.csv>`

.. csv-table:: FIR benchmark
   :file: ../../csv_data_files/L2/fir_tdm_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Function Approximation
======================

The following table gives results for the Function Approximation with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FUNC_APPROX`

:download:`func_approx_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/func_approx_benchmark.csv>`

.. csv-table:: Function Approximation benchmark
   :file: ../../csv_data_files/L2/func_approx_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Hadamard Product
================

The following table gives results for the Hadamard Product with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_HADAMARD`

:download:`hadamard_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/hadamard_benchmark.csv>`

.. csv-table:: Hadamard benchmark
   :file: ../../csv_data_files/L2/hadamard_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Kronecker
=========

The following table gives results for the Kronecker with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_KRONECKER`.

:download:`kronecker_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/kronecker_benchmark.csv>`

.. csv-table:: Kronecker benchmark
   :file: ../../csv_data_files/L2/kronecker_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Matrix Multiply
===============

The following table gives results for the Matrix Multiply function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_GEMM`.

:download:`matrix_mult_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/matrix_mult_benchmark.csv>`

.. csv-table:: Matrix Multiply benchmark
   :file: ../../csv_data_files/L2/matrix_mult_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Matrix Vector Multiply
======================

The following table gives results for the Matrix Vector Multiply function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_GEMV`.

:download:`matrix_vector_mul_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/matrix_vector_mul_benchmark.csv>`

.. csv-table:: Matrix Vector Multiply benchmark
   :file: ../../csv_data_files/L2/matrix_vector_mul_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Mixed Radix FFT
===============

The following table gives results for the Mixed Radix FFT function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_MRFFT`.

:download:`mixed_radix_fft_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/mixed_radix_fft_benchmark.csv>`

.. csv-table:: Mixed Radix FFT benchmark
   :file: ../../csv_data_files/L2/mixed_radix_fft_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Outer Tensor
============

The following table gives results for the Outer Tensor with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_OUTER_TENSOR`.

:download:`outer_tensor_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/outer_tensor_benchmark.csv>`

.. csv-table:: Outer Tensor benchmark
   :file: ../../csv_data_files/L2/outer_tensor_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Sample Delay
============

The following table gives results for the Sample Delay with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_SAMPLE_DELAY`.

:download:`sample_delay_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/sample_delay_benchmark.csv>`

.. csv-table:: Sample Delay benchmark
   :file: ../../csv_data_files/L2/sample_delay_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Widgets
=======

The following table gives results for the Widgets with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_WIDGETS`.

:download:`widget_benchmark.csv <https://github.com/Xilinx/Vitis_Libraries/blob/2024.2/dsp/docs/src/csv_data_files/L2/widget_benchmark.csv>`

.. csv-table:: Widgets benchmark
   :file: ../../csv_data_files/L2/widget_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto


.. |image1| image:: ./media/image1.png
.. |image2| image:: ./media/image2.png
.. |image3| image:: ./media/image4.png
.. |image4| image:: ./media/image2.png
.. |image5| image:: ./media/image2.png
.. |image6| image:: ./media/image2.png
.. |image7| image:: ./media/image5.png
.. |image8| image:: ./media/image6.png
.. |image9| image:: ./media/image7.png
.. |image10| image:: ./media/image2.png
.. |image11| image:: ./media/image2.png
.. |image12| image:: ./media/image2.png
.. |image13| image:: ./media/image2.png