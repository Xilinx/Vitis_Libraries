..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

========================
Vitis DSP Library
========================

The AMD Vitis |trade| digital signal processing library (DSPLib) provides an implementation of different L1/L2/L3 elements for digital signal processing.

The DSPLib contains:

- :ref:`INTRODUCTION_PL`.

- :ref:`INTRODUCTION_AIE`.

.. _INTRODUCTION_PL:

PL DSP Library
==============

The AMD Vitis Programmable Logic (PL) DSP library consists of an implementation of a Discrete Fourier Transform using a fast Fourier transform (FFT) algorithm for acceleration on AMD Adaptive Engineering FPGAs. The library is planned to provide three types of implementations, namely L1 PL primitives, L2 PL kernels, and L3 software APIs. Those implementations are organized in hardware (hw) sub-directories of the corresponding L1, L2, and L3.

The L1 PL primitives can be leveraged by developers working on hardware design implementation or designing hardware kernels for acceleration. They are particularly suitable for hardware designers. The L2 PL kernels are HLS-based predesigned kernels that can be directly used for FPGA acceleration of different applications on integration with XRT. The L3 provides software APIs in C, C++, and Python which allow software developers to offload FFT calculation to FPGAs for acceleration. Before an FPGA can perform the FFT computation, the FPGA needs to be configured with a particular image called an overlay.

The Vitis PL DSP Library provides a fully synthesizable PL based Super Sample data Rate (SSR) FFT, as well as a 2-Dimensional FFT version. For detailed documentation, refer to :ref:`L1_1DFFT_OVERVIEW` and  :ref:`L1_2DFFT_OVERVIEW`.

.. _INTRODUCTION_AIE:

AI Engine DSP Library
=====================

The AMD Vitis AIE DSP library consists of designs of various DSP algorithms, optimized to take full advantage of the processing power of AMD Versal |trade| Adaptive SoC devices, which contain an array of AI Engines high-performance vector processors.

The library is organized into three parts:

- L1 AIE kernels
- L2 AIE graphs and VSS Makefiles
- L3 software APIs

Currently, there are no L3 software APIs, and the recommended entry point for all library elements is an L2 AIE graph.

For more information, refer to :ref:`INTRODUCTION`.

The Vitis AIE DSP Library provides a SSR FFT implementation targeting AIE, as well as various SSR Finite Impulse Response (FIR) filters, SSR Direct Digital Synthesis (DDS), and General Matrix Multiply (GeMM) implementation. For a full list of available DSP functions, refer to :ref:`DSP_LIB_FUNC`.

.. toctree::
   :caption: Introduction
   :maxdepth: 1

   Overview <overview.rst>
   Release Note <release.rst>

.. toctree::
   :caption: L1 PL DSP Library User Guide
   :maxdepth: 2

   1-Dimensional(Line) SSR FFT L1 FPGA Module <user_guide/L1.rst>
   2-Dimensional(Matrix) SSR FFT L1 FPGA Module <user_guide/L1_2dfft.rst>

.. toctree::
   :caption: L2 AIE DSP Library User Guide
   :maxdepth: 2

   Introduction <user_guide/L2/introduction.rst>
   DSP Library Functions <user_guide/L2/dsp-lib-func.rst>
   Configuration <user_guide/L2/configuration.rst>
   Compiling and Simulating <user_guide/L2/compiling-and-simulating.rst>
   Benchmark/QoR <user_guide/L2/benchmark.rst>

.. toctree::
   :caption: API Reference

   API Reference Overview <user_guide/L2/api-reference.rst>
   Bitonic Sort <rst/group_bitonic_sort.rst>
   Convolution / Correlation <rst/group_conv_corr_graph.rst>
   DDS Mixer <rst/group_dds_graph.rst>
   DFT <rst/group_dft_graph.rst>
   FFT IFFT <rst/group_fft_graphs.rst>
   2D FFT IFFT <rst/group_fft_ifft_2dgraphs.rst>
   FFT Window <rst/group_fft_window.rst>
   FIRs <rst/group_fir_graphs.rst>
   Function Approximation <rst/group_func_approx.rst>
   GeMM <rst/group_gemm_graph.rst>
   GeMV <rst/group_matrix_vector_mul_graph.rst>
   Graph utils <rst/group_graph_utils.rst>
   Hadamard Product <rst/group_hadamard_graph.rst>
   Kronecker <rst/group_kronecker_graph.rst>
   Outer Tensor <rst/group_outer_tensor_graph.rst>
   Sample Delay <rst/group_sample_delay_graph.rst>
   Widgets <rst/group_widget_graph.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
