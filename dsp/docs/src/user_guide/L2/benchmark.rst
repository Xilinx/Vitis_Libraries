..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _BENCHMARK:

=============
Benchmark/QoR
=============

This section provides the L2 performance benchmarks and QoR (Quality of Results) for AIE DSP library elements with various configurations. The results are extracted from hardware emulation based simulations. The device used for AIE benchmarking is the xcvc1902-vsva2197-1MP-e-S which has a 1GHz clock, and the device used for AIE-ML is the xcve2802-vsvh1760-1MP-e-S-es1 which has a 1.15GHz clock. Some devices and speed grades available clock the array at other frequencies (e.g. 1.25GHz for the xcvc1902-vsva2197-2MP-e-S ). Since the library elements measured here are contained entirely within the AIE array and subject to one clock alone, it is fair to scale throughput figures seen here by the ratio of clock speeds to get the throughput figures for devices where the AIE array is clocked at a different frequency.

The metrics reported for each case are:

- Latency               - the time delay between the first input sample and the first output sample. If there are multiple ports, the latency is recorded from the first input and first output port
- Throughput            - input throughput calculated based on the number of samples per iteration and the time between each consecutive iteration
- NUM_BANKS             - number of memory banks used by the design
- NUM_AIE               - number of AIE tiles used by the design
- DATA_MEMORY           - total data memory in Bytes used by the design
- PROGRAM_MEMORY        - program memory in Bytes used by each kernel

The parameter, AIE_VARIANT, refers to the type of AI Engine that is used for each particular case in the benchmark results. A value of 1 denotes the AIE, and a value of 2 denotes the AIE-ML.

The PROGRAM_MEMORY metrics are harvested for each kernel the design consists of. For example a FIR configured to be implemented on two tiles (CASC_LEN=2) will have two sets of figures displayed in the table below (space delimited).

Latency and Throughput
~~~~~~~~~~~~~~~~~~~~~~

The latency and throughput values are calculated using the input and output timestamp feature of the aiesimulator. The simulator will create files for the input and output PLIO ports containing the data stream and the timestamp information. Each line of data is preceded by a line containing a timestamp of when it was read or written by the graph port. This feature can be enabled using the option:

.. code-block::

    aiesimulator --graph-latency

The latency value for each iteration is found by calculating the difference between the first input timestamp and the first output timestamp.
Throughput is calculated from the number of samples in an iteration divided by the time difference between the first input timestamp of two consecutive iterations.

The latency and throughput values, as reported for each library element in the tables below, are representative of the function operating at a stable rate. In this context, stability is assumed once the latency is consistent across consecutive iterations.
Following power-on, systems typically take several iterations before a stable rate is achieved. This is due to the buffers being initially empty and other such effects. The figures reported are from when the system has reached a steady state after this initial transient phase.

In the case where there are multiple input ports and/or multiple output ports, the timestamps from the first of these ports are used as these are the ports that contain the first timestamped sample of each iteration.
Furthermore, if there are no input ports included in the design (such as DDS only mode) then the throughput will be measured using the timestamped data on the output port.

DDS/Mixer
~~~~~~~~~

Following table gives results for the DDS/Mixer and DDS/Mixer LUT with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_DDS_MIXER`.

:download:`dds_mixer_benchmark.csv <../../csv_data_files/L2/dds_mixer_benchmark.csv>`

.. csv-table:: DDS/Mixer benchmark
   :file: ../../csv_data_files/L2/dds_mixer_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

FFT IFFT
~~~~~~~~

Following table gives results for the FFT/IFFT function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_ifft_benchmark.csv <../../csv_data_files/L2/fft_ifft_benchmark.csv>`

.. csv-table:: FFT IFFT benchmark
   :file: ../../csv_data_files/L2/fft_ifft_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto


FFT Window
~~~~~~~~~~

Following table gives results for the FFT Window function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FFT`.

:download:`fft_window_benchmark.csv <../../csv_data_files/L2/fft_window_benchmark.csv>`

.. csv-table:: FFT Window benchmark
   :file: ../../csv_data_files/L2/fft_window_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto

Filters
~~~~~~~

Following table gives results for FIR filter with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_FILTERS`

:download:`fir_benchmark.csv <../../csv_data_files/L2/fir_benchmark.csv>`

.. csv-table:: FIR benchmark
   :file: ../../csv_data_files/L2/fir_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: 10, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4


Matrix Multiply
~~~~~~~~~~~~~~~

Following table gives results for the Matrix Multiply function with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_GEMM`.

:download:`matrix_mult_benchmark.csv <../../csv_data_files/L2/matrix_mult_benchmark.csv>`

.. csv-table:: Matrix Multiply benchmark
   :file: ../../csv_data_files/L2/matrix_mult_benchmark.csv
   :align: center
   :header-rows: 1
   :widths: auto


Widgets
~~~~~~~

Following table gives results for the Widgets with a wide variety of supported parameters, which are defined in: :ref:`CONFIGURATION_PARAMETERS_WIDGETS`.

:download:`widget_benchmark.csv <../../csv_data_files/L2/widget_benchmark.csv>`

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


