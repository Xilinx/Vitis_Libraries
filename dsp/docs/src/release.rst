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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

2022.2
------

The below features have been added to the library in this release.

*  **FFT Window** - new library element

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| FFT Window                            |  xf::dsp::aie::fft::windowfn::fft_window_graph                              |
+---------------------------------------+-----------------------------------------------------------------------------+

FFT Window is a utility to apply a windowing (scaling) function such as Hamming to a frame of data samples. 

*  **FFT/iFFT**

FFT Dynamic Point Size, i.e. run-time point size determination is now supported with parallelized configurations.

*  **FIR Filters**

All FIR library elements (with the exception of FIR Resampler) now support Super Sample Rate operation for higher throughput. 
To minimize latency, Super Sample Rate operation is implemented using streaming interfaces. 

In addition, usage of Input Window Size (TP_INPUT_WINDOW_VSIZE) parameter has been consolidated across library.
TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph in a single iteration run. 

Reloadable coefficients within the Super Sample Rate configurations are now supported on all FIR variants that support SSR operation. 


2022.1
------

The below features have been added to the library in this release.

*  **DDS / Mixer**

The DDS/ Mixer library element now has extended type support. It now supports cfloat and cint32 for TT_DATA when configured as a mixer. When configured as a DDS, cfloat is now supported for TT_DATA.
Additionally, the DDS/Mixer now supports Super Sample Rate operation for higher throughput.

*  **FFT/iFFT**

FFT point size support has been extended to 65536.
Performance has been improved approximately 10% for cases using PARALLEL_POWER>1 which were previously supported.

*  **FIR Filters**

All FIR library elements now support streaming interfaces as well as window interfaces.
The single rate asymmetric FIR variant now support Super Sample Rate operation for higher throughput.
The FIR resampler library element has been added which performs fractional decimation. This supersedes the existing FIR interpolate fractional library unit.
All FIR variants now support a larger maximum value for FIR_LEN, up to 8k depending on variant, data/coefficient type and API choice.

2021.2
------

The below features have been added to the library in this release.

*  **DDS / Mixer** - new library element

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| DDS / Mixer                           |  xf::dsp::aie::mixer::dds_mixer                                             |
+---------------------------------------+-----------------------------------------------------------------------------+

This component may be configured to one of three modes.
The first mode is a DDS only.
The second mode is a single channel mixer.
The third mode is a symmetrical mixer, taking two input channels and mixing each with the DDS output and the conjugate of DDS output respectively, combining the result to one output channel.
DDS/Mixer supports window input/output interface, as well as streaming interface.

*  **FIR Filters**

Single rate FIRs now support streaming interfaces as well as to window interfaces.

*  **FFT/iFFT**

FFT now supports streaming interfaces as well as to window interfaces.
In addition, FFT now offers improved performance and greater point size support with parallelization.

2021.1
------

The AI Engine DSP Library contains common parameterizable DSP functions used in many advanced signal processing applications. All functions currently support window interfaces with streaming interface support planned for future releases.

.. note:: Namespace aliasing can be utilized to shorten instantiations: ``namespace dsplib = xf::dsp::aie;``


*  **FIR Filters**

The DSPLib contains several variants of Finite Impulse Response (FIR) filters.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Single rate, asymmetrical             |  dsplib::fir::sr_asym::fir_sr_asym_graph                                    |
+---------------------------------------+-----------------------------------------------------------------------------+
| Single rate, symmetrical              |  dsplib::fir::sr_sym::fir_sr_sym_graph                                      |
+---------------------------------------+-----------------------------------------------------------------------------+
| Interpolation asymmetrical            |  dsplib::fir::interpolate_asym::fir_interpolate_asym_graph                  |
+---------------------------------------+-----------------------------------------------------------------------------+
| Decimation, halfband                  |  dsplib::fir::decimate_hb::fir_decimate_hb_graph                            |
+---------------------------------------+-----------------------------------------------------------------------------+
| Interpolation, halfband               |  dsplib::fir::interpolate_hb::fir_interpolate_hb_graph                      |
+---------------------------------------+-----------------------------------------------------------------------------+
| Decimation, asymmetric                |  dsplib::fir::decimate_asym::fir_decimate_asym_graph                        |
+---------------------------------------+-----------------------------------------------------------------------------+
| Interpolation, fractional, asymmetric |  dsplib::fir::interpolate_fract_asym:: fir_interpolate_fract_asym_graph     |
+---------------------------------------+-----------------------------------------------------------------------------+
| Decimation, symmetric                 |  dsplib::fir::decimate_sym::fir_decimate_sym_graph                          |
+---------------------------------------+-----------------------------------------------------------------------------+

All FIR filters can be configured for various types of data and coefficients. These types can be int16, int32, or float and also real or complex.
Both FIR length and cascade length can also be configured for all FIR variants.

*  **FFT/iFFT**

The DSPLib contains one FFT/iFFT solution. This is a single channel, single kernel decimation in time, (DIT), implementation with configurable point size, complex data types, cascade length and FFT/iFFT function.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Single Channel FFT/iFFT               |  dsplib::fft::fft_ifft_dit_1ch_graph                                        |
+---------------------------------------+-----------------------------------------------------------------------------+


*  **Matrix Multiply (GeMM)**

The DSPLib contains one Matrix Multiply/GEMM (GEneral Matrix Multiply) solution. This supports the Matrix Multiplication of 2 Matrices A and B with configurable input data types resulting in a derived output data type.


+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Matrix Mult / GeMM                    |  dsplib::blas::matrix_mult::matrix_mult_graph                               |
+---------------------------------------+-----------------------------------------------------------------------------+

*  **Widget Utilities**

These widgets support converting between window and streams on the input to the DSPLib function and between streams to windows on the output of the DSPLib function where desired and additional widget for converting between real and complex data-types.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Stream to Window / Window to Stream   |  dsplib::widget::api_cast::widget_api_cast_graph                            |
+---------------------------------------+-----------------------------------------------------------------------------+
| Real to Complex / Complex to Real     |  dsplib:widget::real2complex::widget_real2complex_graph                     |
+---------------------------------------+-----------------------------------------------------------------------------+


*  **AIE DSP in Model Composer**

DSP Library functions are supported in Vitis Model Composer, enabling users to easily plug these functions into the Matlab/Simulink environment to ease AI Engine DSP Library evaluation and overall AI Engine ADF graph development.



2020.2
------

Revised the APIs to fully support Vitis HLS.



2020.1
------

The 1.0 release introduces L1 HLS primitives for Discrete Fourier Transform for 1-D and 2-D input data.
