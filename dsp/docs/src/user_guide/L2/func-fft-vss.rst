..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _VSS_FFT:

============
VSS FFT/IFFT
============

This library element implements a single-channel DIT FFT using both AI Engine tiles and programmable logic to extract higher performance for larger point sizes. The VSS offers two modes of implementing the FFT: Mode 1 and Mode 2. Mode 1 performs more computation on AIE tiles compared to Mode 2. It also allows more fine-tuning on the value of SSR. Mode 2 uses more PL compute resources compared to mode 1. It also user lesser PL memory than Mode 1. The two modes thus offer different trade-offs between performance and resource utilization.

Entry Point
===========

The entry points for the VSS are the ``vss_fft_ifft_pararms.cfg`` and ``vss_fft_ifft_1d.mk`` file present in the L2/include/vss/vss_fft_ifft_1d/ directory in the DSP library. The ``vss_fft_ifft_1d.mk`` takes in a user configurable file, say "vss_fft_ifft_pararms.cfg" as input and generates a .vss object as an output after performing all the intermediate steps like generating the necessary AI Engine graph and PL products and stitching them together. The user can then integrate this .vss object into their larger design. See Vitis documentation on "Vitis Subsystems" for details on how to include a .vss object into your design.

Please edit the parameters in the ``cfg`` file and provide it as input to the ``vss_fft_ifft_1d.mk`` file. An example of how to create a vss and include a .vss object in your design is also provided in L2/examples/vss_fft_ifft_1d/example.mk. It creates a .vss object, links it to a larger system to create an xclbin and runs hardware emulation of the full design.

Device Support
==============

The VSS FFT can generate VSS products for AIE, AIE-ML and AIE-MLv2. The VSS is generated for the ``part`` that the user provides in the input cfg file. All features are supported on the two variants with the following differences:

- ``DATA_TYPE`` and ``TWIDDLE_TYPE``. AIE-ML does not support cfloat type.
- ``TWIDDLE_TYPE``: AIE supports cint32. AIE-ML does not.
- ``ROUND_MODE``: Supported round modes are the same for AIE-ML and AIE-MLv2 devices, but differ from those for AIE devices as for all library elements.

Supported Parameters
====================

The complete list of required parameters for the VSS FFT is shown in L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_params.cfg. Please edit the parameters in this file to configure the VSS FFT. The user can add to the [aie] section of the cfg file for other options that they want to pass directly to the aiecompiler. Please see API reference on vss_fft_ifft_1d_graph.hpp for details on the AI Engine configurable parameters for VSS Mode 1 and see the vss_fft_ifft_1d_front_only_graph.hpp for the parameters for VSS Mode 2.

+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [Category] Parameter             | Description                                                                                                                                |
+==================================+============================================================================================================================================+
| part                             | Name of the part that the VSS should compile for                                                                                           |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| freqhz                           | Frequency of the internal PL components of the VSS (In Hz)                                                                                 |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [aie] enable_partition           | Configuration of the range of columns that you want to place the compiled AIE kernels. Please do not change the name of the aie partition. |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] DATA_TYPE           | Used to set TT_DATA described in API Reference                                                                                             |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] TWIDDLE_TYPE        | Used to set TT_TWIDDLE described in API Reference                                                                                          |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] POINT SIZE          | Used to set TP_POINT_SIZE described in API Reference                                                                                       |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] FFT_NIFFT           | Used to set TP_FFT_NIFFT described in API Reference                                                                                        |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] SHIFT               | Used to set TP_SHIFT described in API reference                                                                                            |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] API_IO              | Used to set TP_API described in API reference                                                                                              |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] ROUND_MODE          | Used to set TP_RND described in API reference                                                                                              |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] SAT_MODE            | Used to set TP_SAT described in API reference                                                                                              |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] Twiddle Mode        | Used to set TP_TWIDDLE_MODE described in API reference                                                                                     |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] SSR                 | Used to set TP_SSR described in API reference                                                                                              |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] AIE_PLIO_WIDTH      | Sets the PLIO width of the AIE-PL interface                                                                                                |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] VSS_MODE            | Sets the mode of decomposition of the VSS. Choose between 1 and 2.                                                                         |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] ADD_FRONT_TRANSPOSE | Use to indicate whether to include a data rearrangement block at the input side of the VSS. Refer to design notes for more details.        |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] ADD_BACK_TRANSPOSE  | Use to indicate whether to include a data rearrangement block at the output side of the VSS. Refer to design notes for more details.       |
+----------------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+


Design Notes
============

.. _VSS_SSR_OPERATION:

Super Sample Rate
------------------

The VSS FFT can be configured for Super Sample Rate operation to achieve higher throughput. The design generates TP_SSR number of input and output ports.

The input data to the SSR input ports of the VSS FFT are expected to be distributed evenly in a "card-dealing" fashion. For example,

Port Number 1 gets samples S_1, S_SSR+1, S_2*SSR+1, ...
Port Number 2 gets samples S_2, S_SSR+2, S_2*SSR+2, ...
...
Port Number SSR gets samples S_SSR, S_SSR+SSR, S_2*SSR+SSR, ...



.. _SSR_POINTSIZE_CONSTRAINTS:

Padding Input Data based on Super Sample Rate and Point Size
------------------------------------------------------------

If the point size is a multiple of SSR, then the inputs can be passed as is to the FFT. Otherwise, every point size number of samples needs to be padded with zeros to the closest multiple of SSR before giving as input to the FFT. The output data also contains Point size number of valid data samples padded to the closest multiple of SSR.

.. _ADD_FRONT_TRANSPOSE:

VSS Mode 1 creates a transpose block at its input that is used to rearrange data that arrives in the natural SSR order described in :ref:`VSS_SSR_OPERATION` into an order needed by the first set of compute units within the VSS. This block uses buffers in the PL to rearrange the data. If the user wants to input data directly in the form needed by the compute units, they can save on memory resources by setting the ADD_FRONT_TRANSPOSE flag to 0.
If the front transpose is removed, ensure that the data arriving in each port satisfies the formula:

S[PORT_IDX][SAMP_IDX] = (PORT_IDX + ((SAMP_IDX % D1) * D2) + ((int)(SAMP_IDX / D1) * SSR))

where :
- PORT_IDX ranges from 0 to SSR - 1
- D1 = D2 = square root of the point size when point sizes are a perfect square
- D1 = square root of (point size * 2); D2 = square root of (point size/2) for other point sizes

For example, for a point size of 512 and SSR of 4
- Stream 0 carries sample: SI_0, SI_16, SI_32, SI_48, ... SI_496, SI_4, SI_20, SI_36, ... SI_500, ...
- Stream 1 carries sample: SI_1, SI_17, SI_33, SI_49, ... SI_497, SI_5, SI_21, SI_37, ... SI_501, ...
- Stream 2 carries sample: SI_2, SI_18, SI_34, SI_50, ... SI_498, SI_6, SI_22, SI_38, ... SI_502, ...
- Stream 3 carries sample: SI_3, SI_19, SI_35, SI_51, ... SI_499, SI_7, SI_23, SI_39, ... SI_503, ...


.. _ADD_BACK_TRANSPOSE:

Both VSS Mode 1 and 2 Include a transpose block after all their compute units to rearrange data in a form in the SSR form as described in section :ref:`VSS_SSR_OPERATION`. This block uses buffers in the PL to rearrange the data. If the user has downstream blocks that can directly accept the data in the form given out by the compute units, they can save on memory resources by setting the ADD_BACK_TRANSPOSE flag to 0.
If the back transpose is removed, data arriving in each output port will be different between the 2 VSS modes for the same SSR and point size.

For VSS mode 1, the samples at the output of the VSS without the back transpose would satisfy the formula
S[PORT_IDX][SAMP_IDX] = (PORT_IDX + ((SAMP_IDX % D2) * D1) + ((int)(SAMP_IDX / D2) * SSR))

where:
- PORT_IDX ranges from 0 to SSR - 1
- D1 = D2 = square root of the point size when point sizes are a perfect square
- D1 = square root of (point size * 2); D2 = square root of (point size/2) for other point sizes

For VSS Mode 2,
the samples at the output of the VSS without the back transpose would satisfy the formula
S[PORT_IDX][SAMP_IDX] = (PORT_IDX + (SAMP_IDX % SSR) * D1)

where:
- PORT_IDX ranges from 0 to SSR - 1
- D1 = point size / SSR