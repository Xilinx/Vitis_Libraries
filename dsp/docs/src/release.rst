..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _release_note:

=============
Release Notes
=============

.. toctree::
   :hidden:
   :maxdepth: 1

2025.1
======

The following features have been added to the library in this release:

* **Bitonic Sort** - Added support for Super Sample Rate (SSR).

SSR offers parallel execution which improves performance and allows larger lists to be operated on.

* **Convolution / Correlation** - Added support for Run Time Programmable (RTP) configuration of vector G length to buffer implementation.

Buffer implementation supports vector G length to be passed during runtime.

* **DDS Mixer / DDS Mixer Lut** - Added support for Run Time Programmable (RTP) configuration of Phase Increment, and runtime configuration of Phase Offset and Phase Increment using Buffer port.

Both DDS Mixer and DDS Mixer LUT now support iobuffer input for Phase offset and RTP input for Phase Increment in addition to existing input options, allowing flexible configuration from PS or PL.

* **Euclidean Distance** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Euclidean Distance                    |  xf::dsp::aie::euclidean_distance                                           |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the euclidean distance (ED) algorithm.

Supports AIE and AIE-ML (EA) devices.

* **FIR TDM** - Added support for Run Time Programmable (RTP) configuration of FIR coefficients.

* **Matrix-Vector Multiplication** - Added support for:

      - Run Time Programmable (RTP) configuration of matrix A inputs. Input Matrix A can be configured as a buffer (``inA``) or as an RTP port (``matrixA``).

      - Streaming Interface of vector B inputs. Input ``inB`` can now be configured as a buffer or a stream port.


* **Matrix Multiplication** - Introduced a new template parameter `TT_OUT_DATA` to specify the data type of the output.

* **FFT** - Introduces a new VSS implementation of Super Sample Rate FFTs.

2024.2
======

The following features have been added to the library in this release.

*  **AIE-MLv2** - New device support (EA) is being added to DSPLIB.

Early Access (EA) support for AIE-MLv2 device is being added to the following library elements:

      - Bitonic Sort

      - DDS Mixer LUT

      - DFT

      - FFT

      - FFT Window

      - Filters

      - TDM FIR

      - Hadamard

      - Kronecker

      - Outer Tensor

      - Matrix Mult

      - Matrix Vector

      - Sample Delay

*  **Config Helper** - Console Interface script for configuring DSPLIB IPs.

Config Helper works in conjunction with the DSPLIB metadata for helping users to build legal configurations and generate resulting graphs for DSPLIB AIE-IPs.

*  **Bitonic Sort** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Bitonic Sort                          |  xf::dsp::aie::bitonic_sort                                                 |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the bitonic sorting algorithm.

Supports AIE, AIE-ML and AIE-MLv2 (EA) devices.

*  **FFT** - New features and optimizations.

In this release a new parameter has been added to the FFT: `TT_OUT_DATA`. This parameter allows the output data type
to differ from the input data type. e.g. to allow `TT_DATA` (input) to be cint16 with `TT_OUT_DATA` to be cint32.
Also, various optimizations have been implemented to reduce the memory resource used by some configurations of the FFT.

*  **FFT 2D** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| fft_ifft_2d_graph                     |  xf::dsp::aie::fft::two_d::fft_ifft_2d_graph                                |
+---------------------------------------+-----------------------------------------------------------------------------+

This configurable design library element implements a 2D FFT/IFFT function, decomposing FFT algorithm into AI Engine Tiles and MEM Tiles.

Supports AIE-ML devices.

*  **FIR TDM** - New features and optimizations.

In this release various optimizations have been implemented to optimize throughput and/or reduce memory footprint. In addition, new parameters have been added to the FIR TDM: `TT_OUT_DATA` and `TP_CASC_LEN`. These parameters allow:

      - the output data type to differ from the input data type. e.g. to allow `TT_DATA` (input) to be `cint16` with `TT_OUT_DATA` to be `cint32`.

      - FIR workload can be split into multiple kernels connected through the cascade interface, offering increased throughput at the cost of additional resources.

*  **Function Approximation** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Function Approximation                |  xf::dsp::aie::func_approx                                                  |
+---------------------------------------+-----------------------------------------------------------------------------+

This element provides a vectorized linear approximation of a function, f(x), for a given input data, x, using a configured lookup table of slope and offset values that describe the function.

* **Mixed Radix FFT** - New Features

In this release the dynamic point size has been added to the IP, selected by setting the new parameter `TP_DYN_PT_SIZE` to 1.
This IP now supports cint32 and cfloat for `TT_DATA` and cint32 for `TT_TWIDDLE`.

*  **VSS FFT/IFFT 1CH (AI Engine + PL)** - New library element.

In this release a VSS (Vitis Sub-System) FFT/IFFT has been added to the DSPLIB.
This configurable design element implements a single-channel DIT FFT/IFFT, decomposing FFT algorithm into AIE Tiles and PL (programmable logic).

Supports AIE and AIE-ML devices.

2024.1
======

The following features have been added to the library in this release.

*  **TDM FIR** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| TDM FIR                               |  xf::dsp::aie::fir::tdm::fir_tdm_graph                                      |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the Time-Division Multiplexing (TDM) variant of finite impulse response (FIR) filter.

Supports AIE and AIE-ML devices.

*  **Convolution / Correlation** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and class name**                                                |
+=======================================+=============================================================================+
| Convolution / Correlation             |  xf::dsp::aie::conv_corr::conv_corr_graph                                   |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the Convolution and Correlation, depending on the specified FUNCT_TYPE template
parameter.

Supports AIE and AIE-ML devices.

*  **Hadamard Product** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Hadamard Product                      |  xf::dsp::aie::hadamard::hadamard_graph                                     |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the Hadamard Product.

Supports AIE and AIE-ML devices.

*  **Outer Tensor Product** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Outer Tensor Product                  |  xf::dsp::aie::outer_tensor::outer_tensor_graph                             |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the Outer Tensor Product.

Supports AIE devices.

*  **Kronecker Matrix Product** - New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Kronecker Matrix Product              |  xf::dsp::aie::kronecker::kronecker_graph                                   |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the  Kronecker Matrix Product.

Supports AIE devices.

*  **Example design of FFT on AI Engine and PL**

This element illustrates the implementation of the Fast Fourier transform that spans across the AI Engine array and Programmable Logic.

Supports AIE and AIE-ML devices.

*  **Matrix-Vector Multiplication**

Added support for AIE-ML devices.

Added Super Sample Rate (SSR) feature. Allows the matrix-vector multiplication to be computed using parallel paths for increased throughput.

*  **Matrix Multiplication**

Added Super Sample Rate (SSR) feature. Allows the matrix multiplication to be computed using parallel paths for increased throughput.

*  **DFT**

Added Super Sample Rate (SSR) feature. Allows the Discrete Fourier Transform (DFT) to be computed using parallel paths for increased throughput.

*  **FFT IFFT**

This element now supports cint32 type twiddles for integer data types. This element also now supports a new parameter
TP_TWIDDLE_MODE.

* **Mixed-Radix FFT**

This element now supports AIE-ML devices.


2023.2
======

The following features have been added to the library in this release.
* **DFT**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| DFT                                   |  xf::dsp::aie::fft::dtf::dft_graph                                          |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the Discrete Fourier transform (DFT).

Supports both AIE and AIE-ML devices.

* **Mixed-Radix FFT**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Mixed Radix FFT                       |  xf::dsp::aie::fft::mixed_radix_fft::mixed_radix_fft_graph                  |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of a single-channel, decimation-in-time, fixed point size fast Fourier transform (FFT) that includes radix3, radix4, and/or radix5 stages.

Supports AIE devices.

* **GeMV**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| GeMV                                  |  xf::dsp::aie::blas::matrix_vector_mul::matrix_mult_graph                   |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of the General Matrix Vector Multiplier (GeMV).

Supports AIE devices.

* **Vectorized Sample Delay**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Sample Delay                          |  xf::dsp::aie::sample_delay::sample_delay_graph                             |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds an implementation of a delay filter for introducing delay into a time series.

Supports both AIE and AIE-ML devices.

* **FIR Filters**

The support for AIE-ML devices has been rolled out to all FIR variants.

* **FFT IFFT**

Added a performance optimization that will extract some SSR FFT features onto widget kernels and map these kernels in separate tiles, achieving better performance at a high AI Engine usage cost.

* **All Library Elements**

All library now offer selectable saturation mode, as well as rounding modes.

Test harnesses for all library elements have been expanded to allow parameter configuration through JSON files.

2023.1
======

The following features have been added to the library in this release.

* **DDS Mixer LUT**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| DDS Mixer LUT                         |  xf::dsp::aie::mixer::dds_mixer::dds_mixer_lut_graph                        |
+---------------------------------------+-----------------------------------------------------------------------------+

This element adds a second implementation of a DDS/Mixer that provides higher SFDR figures than the existing DDS/Mixer solution.

Supports both AIE and AIE-ML devices (EA).

* **DDS Mixer**

The usage of Input Window Size parameter (TP_INPUT_WINDOW_VSIZE) has been changed in the DDS and DDS Mixer LUT elements to be consistent with other library elements. TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph in a single iteration run.

* **FFT Window**

FFT Window now supports AIE-ML devices (EA), in addition to AIE devices.

* **FFT/iFFT**

FFT/iFFT now supports AIE-ML devices (EA), in addition to AIE devices.

* **FIR Filters**

Single Rate FIRs now support AIE-ML devices (EA) with the 16-bit data types and 16-bit coeff types which are as follows:

+-------------------------------+------------------------------------+
|                               |     **Data        Type**           |
|                               +------------------+-----------------+
|                               | **Int16**        | **Cint16**      |
+----------------------+--------+------------------+-----------------+
| **Coefficient type** | Int16  | Supported        | Supported       |
|                      +--------+------------------+-----------------+
|                      | Cint16 | note 1           | Supported       |
+----------------------+--------+------------------+-----------------+
| 1. Complex coefficients are not supported for real-only data types.|
+--------------------------------------------------------------------+

All FIR library elements now support AIE devices with 16-bit data types and 32-bit coefficients which are as follows:

+-------------------------------+------------------------------------+
|                               |     **Data        Type**           |
|                               +------------------+-----------------+
|                               | **Int16**        | **Cint16**      |
+----------------------+--------+------------------+-----------------+
| **Coefficient type** | Int32  | Supported        | Supported       |
|                      +--------+------------------+-----------------+
|                      | Cint32 | note 1           | Supported       |
+----------------------+--------+------------------+-----------------+
| 1. Complex coefficients are not supported for real-only data types.|
+--------------------------------------------------------------------+

The FIR Resampler now supports Super Sample Rate operation for higher throughput, through polyphase decomposition.

2022.2
======

The following features have been added to the library in this release.

* **FFT Window**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| FFT Window                            |  xf::dsp::aie::fft::windowfn::fft_window_graph                              |
+---------------------------------------+-----------------------------------------------------------------------------+

FFT Window is a utility to apply a windowing (scaling) function such as hamming to a frame of data samples.

* **FFT/iFFT**

FFT Dynamic Point Size, i.e., runtime point size determination is now supported with parallelized configurations.

* **FIR Filters**

All FIR library elements (with the exception of FIR Resampler) now support the Super Sample Rate operation for higher throughput. To minimize latency, the Super Sample Rate operation is implemented using streaming interfaces.

In addition, usage of the Input Window Size (TP_INPUT_WINDOW_VSIZE) parameter has been consolidated across library. TP_INPUT_WINDOW_VSIZE describes the number of samples processed by the graph in a single iteration run.

Reloadable coefficients within the Super Sample Rate configurations are now supported on all FIR variants that support SSR operation.

2022.1
======

The following features have been added to the library in this release.

* **DDS/Mixer**

The DDS/Mixer library element now has extended type support. It now supports cfloat and cint32 for TT_DATA when configured as a mixer. When configured as a direct digital synthesizer (DDS), cfloat is now supported for TT_DATA. Additionally, the DDS/Mixer now supports the Super Sample Rate operation for higher throughput.

* **FFT/iFFT**

FFT point size support has been extended to 65536. Performance has been improved approximately 10% for cases using PARALLEL_POWER>1, which were previously supported.

* **FIR Filters**

All FIR library elements now support streaming interfaces as well as window interfaces. The single rate asymmetric FIR variant now support the Super Sample Rate operation for higher throughput. The FIR resampler library element has been added which performs fractional decimation. This supersedes the existing FIR interpolate fractional library unit. All FIR variants now support a larger maximum value for FIR_LEN, up to 8k depending on variant, data/coefficient type, and API choice.

2021.2
======

The following features have been added to the library in this release.

* **DDS / Mixer**: New library element.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| DDS / Mixer                           |  xf::dsp::aie::mixer::dds_mixer                                             |
+---------------------------------------+-----------------------------------------------------------------------------+

This component may be configured to one of three modes. The first mode is a DDS only. The second mode is a single channel mixer. The third mode is a symmetrical mixer, taking two input channels and mixing each with the DDS output and the conjugate of DDS output respectively, combining the result to one output channel. DDS/Mixer supports window input/output interface, as well as streaming interface.

* **FIR Filters**

Single rate FIRs now support streaming interfaces as well as to window interfaces.

* **FFT/iFFT**

FFT now supports streaming interfaces as well as to window interfaces. In addition, FFT now offers improved performance and greater point size support with parallelization.

2021.1
======

The AI Engine DSP Library contains common parameterizable DSP functions used in many advanced signal processing applications. All functions currently support window interfaces with streaming interface support planned for future releases.

.. note:: Namespace aliasing can be utilized to shorten instantiations: ``namespace dsplib = xf::dsp::aie;``

* **FIR Filters**

The DSPLib contains several variants of Finite Impulse Response (FIR) filters.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
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

All FIR filters can be configured for various types of data and coefficients. These types can be int16, int32, or float and also real or complex. Both the FIR length and cascade length can also be configured for all FIR variants.

* **FFT/iFFT**

The DSPLib contains one FFT/iFFT solution. This is a single channel, single kernel decimation in time (DIT), implementation with configurable point size, complex data types, cascade length, and FFT/iFFT function.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Single Channel FFT/iFFT               |  dsplib::fft::fft_ifft_dit_1ch_graph                                        |
+---------------------------------------+-----------------------------------------------------------------------------+


* **Matrix Multiply (GeMM)**

The DSPLib contains one Matrix Multiply/GEMM (GEneral Matrix Multiply) solution. This supports the Matrix Multiplication of two Matrices A and B with configurable input data types resulting in a derived output data type.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Matrix Mult / GeMM                    |  dsplib::blas::matrix_mult::matrix_mult_graph                               |
+---------------------------------------+-----------------------------------------------------------------------------+

* **Widget Utilities**

These widgets support converting between window and streams on the input to the DSPLib function and between streams to windows on the output of the DSPLib function where desired and an additional widget for converting between real and complex data-types.

+---------------------------------------+-----------------------------------------------------------------------------+
| **Function**                          | **Namespace and Class Name**                                                |
+=======================================+=============================================================================+
| Stream to Window / Window to Stream   |  dsplib::widget::api_cast::widget_api_cast_graph                            |
+---------------------------------------+-----------------------------------------------------------------------------+
| Real to Complex / Complex to Real     |  dsplib:widget::real2complex::widget_real2complex_graph                     |
+---------------------------------------+-----------------------------------------------------------------------------+

* **AIE DSP in Model Composer**

DSP Library functions are supported in the AMD Vitis|trade| Model Composer, enabling you to easily plug these functions into the Matlab/Simulink environment to ease AI Engine DSP Library evaluation and overall AI Engine ADF graph development.

2020.2
======

Revised the APIs to fully support Vitis HLS.

2020.1
======

The 1.0 release introduces L1 HLS primitives for Discrete Fourier Transform for 1-D and 2-D input data.
