..
   Copyright 2022 Xilinx, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

========================
Vitis DSP Library
========================

The Vitis |trade| digital signal processing library (DSPLib) provides an implementation of different L1/L2/L3 elements for digital signal processing.

The DSPLib contains:

- :ref:`INTRODUCTION_PL`.

- :ref:`INTRODUCTION_AIE`.


.. _INTRODUCTION_PL:

****************
PL DSP library
****************

The current PL (Programmable Logic) library consists of an implementation of a Discrete Fourier Transform using a Fast
Fourier Transform (FFT) algorithm for acceleration on Xilinx |reg| FPGAs. The library is planned
to provide three types of implementations, namely L1 PL primitives, L2 PL kernels, and L3 software APIs.
Those implementations are organized in hardware (hw) sub-directories of the corresponding L1, L2, and L3.

The L1 PL primitives can be leveraged by developers working on harware design
implementation or designing hardware kernels for acceleration. They are particularly
suitable for hardware designers. The L2 PL kernels are HLS-based predesigned kernels
that can be directly used for FPGA acceleration of different applications on integration with
the Xilinx Runtime (XRT). The L3 provides software APIs in C, C++, and Python which
allow software developers to offload FFT calculation to FPGAs for acceleration. Before
an FPGA can perform the FFT computation, the FPGA needs to be configured with a particular image
called an overlay.

Vitis |trade| PL DSP Library provides a fully synthesizable PL based Super Sample data Rate (SSR) FFT, as well as a 2-Dimensional FFT version. For detailed documentation, plese refer to: :ref:`L1_1DFFT_OVERVIEW` and  :ref:`L1_2DFFT_OVERVIEW`.


.. _INTRODUCTION_AIE:

*********************
AI Engine DSP library
*********************

AIE DSP library consists of designs of various DSP algorithms, optimized to take full advantage of the processing power of the Xilinx |reg| Versal |reg| Adaptive Computing Acceleration Platform (ACAP) devices, which contain an array of Xilinx |reg| AI Engines - high-performance vector processors.

The library is organized into three part, namely:

- L1 AIE kernels,

- L2 AIE graphs, and

- L3 software APIs,

though at present there are no L3 software APIs and the recommended entry point for all library elements is an L2 AIE graph.

Please refer to :ref:`INTRODUCTION` for more information.

Vitis |trade| AIE DSP Library provides a SSR FFT implementation targeting AIE, as well as various SSR Finite Impulse Response (FIR) filters, SSR Direct Digital Synthesis (DDS), General Matrix Multiply (GeMM) impementation. For a full list of available DSP functions, please refer to :ref:`DSP_LIB_FUNC`.


.. toctree::
   :caption: Introduction
   :maxdepth: 4

   Overview <overview.rst>
   Release Note <release.rst>

.. toctree::
   :caption: L1 PL DSP Library User Guide
   :maxdepth: 4

   1-Dimensional(Line) SSR FFT L1 FPGA Module <user_guide/L1.rst>
   2-Dimensional(Matrix) SSR FFT L1 FPGA Module <user_guide/L1_2dfft.rst>

.. toctree::
   :caption: L2 AIE DSP Library User Guide
   :maxdepth: 4

   Introduction <user_guide/L2/introduction.rst>
   DSP Library Functions <user_guide/L2/dsp-lib-func.rst>
   Compiling and Simulating Using the Makefile <user_guide/L2/compiling-and-simulating.rst>
   Benchmark/QoR <user_guide/L2/benchmark.rst>

.. toctree::
   :caption: API Reference
   :maxdepth: 4

   API Reference Overview <user_guide/L2/api-reference.rst>
   DDS Mixer <rst/group_dds_graph.rst>
   Graph utils <rst/group_graph_utils.rst>
   FFT IFFT <rst/group_fft_graphs.rst>
   FFT Window <rst/group_fft_window.rst>
   FIRs <rst/group_fir_graphs.rst>
   GeMM <rst/group_gemm_graph.rst>
   Widgets <rst/group_widget_graph.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:



