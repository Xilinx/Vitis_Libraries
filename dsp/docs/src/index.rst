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

The library is organized into three types of AIE designs, namely:

- L1 AIE kernels,

- L2 AIE graphs, and

- L3 software APIs.

Please refer to :ref:`INTRODUCTION` for more information.

Vitis |trade| AIE DSP Library provides a SSR FFT implementation targeting AIE, as well as various SSR Finite Impulse Response (FIR) filters, SSR Direct Digital Synthesis (DDS), General Matrix Multiply (GeMM) impementation. For a full list of available DSP functions, please refer to :ref:`DSP_LIB_FUNC`.


.. toctree::
   :caption: Library Overview
   :maxdepth: 4

   overview.rst
   release.rst

.. toctree::
   :caption: L1 PL DSP Library User Guide
   :maxdepth: 4
   :numbered:

   user_guide/L1.rst
   user_guide/L1_2dfft.rst

.. toctree::
   :caption: L2 DSP Library User Guide
   :maxdepth: 4
   :numbered:


   user_guide/L2/introduction.rst
   user_guide/L2/dsp-lib-func.rst
   user_guide/L2/compiling-and-simulating.rst
   user_guide/L2/benchmark.rst


.. toctree::
   :maxdepth: 4
   :caption: API Reference
   :hidden:

   API Reference Overview <user_guide/L2/api-reference>
   fir_sr_asym_graph <rst/class_xf_dsp_aie_fir_sr_asym_fir_sr_asym_graph>
   fir_sr_sym_graph <rst/class_xf_dsp_aie_fir_sr_sym_fir_sr_sym_graph>
   fir_interpolate_asym_graph <rst/class_xf_dsp_aie_fir_interpolate_asym_fir_interpolate_asym_graph>
   fir_decimate_hb_graph <rst/class_xf_dsp_aie_fir_decimate_hb_fir_decimate_hb_graph>
   fir_interpolate_hb_graph <rst/class_xf_dsp_aie_fir_interpolate_hb_fir_interpolate_hb_graph>
   fir_decimate_asym_graph <rst/class_xf_dsp_aie_fir_decimate_asym_fir_decimate_asym_graph>
   fir_interpolate_fract_asym_graph <rst/class_xf_dsp_aie_fir_interpolate_fract_asym_fir_interpolate_fract_asym_graph>
   fir_resampler_graph <rst/class_xf_dsp_aie_fir_resampler_fir_resampler_graph>
   fir_decimate_sym_graph <rst/class_xf_dsp_aie_fir_decimate_sym_fir_decimate_sym_graph>
   matrix_mult_graph <rst/class_xf_dsp_aie_blas_matrix_mult_matrix_mult_graph>
   FFT Graphs <rst/group_fft_graphs>
   widget_api_cast_graph <rst/class_xf_dsp_aie_widget_api_cast_widget_api_cast_graph>
   widget_real2complex_graph <rst/class_xf_dsp_aie_widget_real2complex_widget_real2complex_graph>
   dds_mixer_graph <rst/class_xf_dsp_aie_mixer_dds_mixer_dds_mixer_graph>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:



