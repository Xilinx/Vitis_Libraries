..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DSP_LIB_FUNC:

=====================
DSP Library Functions
=====================

The AMD Vitis |trade| digital signal processing library (DSPLib) is a configurable library of elements that can be used to develop applications on AMD Versal |trade| AI Engines. This is an Open Source library for DSP applications. The user entry point for each function in this library is a graph (L2 level). Each entry point graph class will contain one or more L1 level kernels and can contain one or more graph objects. Direct use of kernel classes (L1 level) or any other graph class not identified as an entry point is not recommended as this might bypass legality checking.

The DSPLib consists of the following DSP elements:

.. toctree::
   :maxdepth: 2

   Bitonic Sort <func-bitonic-sort.rst>
   Convolution / Correlation <func-conv-corr.rst>
   DDS / Mixer <func-dds.rst>
   DFT <func-dft.rst>
   ED <func-ed.rst>
   FFT/iFFT <func-fft.rst>
   2D FFT/iFFT <func-fft-2d.rst>
   FFT Window <func-fft_window.rst>
   Filters <func-fir-filtersAIE.rst>
   FIR TDM <func-fir-TDM.rst>
   Function Approximation <func-func-approx.rst>
   Hadamard Product <func-hadamard.rst>
   Kronecker <func-kronecker.rst>
   Matrix Multiply <func-matmul.rst>
   Matrix-Vector Multiply <func-matrix_vector_mul.rst>
   Mixed Radix FFT <func-mixed_radix_fft.rst>
   Outer Tensor <func-outer-tensor.rst>
   Sample Delay <func-sample_delay.rst>
   Widget API Cast <func-widget-apicast.rst>
   Widget Real to Complex <func-widget-real2comp.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
