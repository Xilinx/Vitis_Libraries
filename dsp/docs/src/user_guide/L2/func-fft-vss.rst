..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _VSS_FFT:

============
VSS FFT/IFFT
============

This library element implements a single-channel DIT FFT using both AIE and programmable logic to extract higher performance for larger point sizes.

Entry Point
===========

The entry points for the VSS are the ``vss_fft_ifft_pararms.cfg`` and ``vss_fft_ifft_1d.mk`` file present in the L2/include/vss/vss_fft_ifft_1d/ directory in the DSP library. The ``vss_fft_ifft_1d.mk`` takes in a user configurable file, say "vss_fft_ifft_pararms.cfg" as input and generates a .vss object as an output after performing all the intermediate steps like generating the necessary AIE and PL products and stitching them together. The user can then integrate this .vss object into their larger design. See Vitis documentation on "Vitis Subsystems" for details on how to include a .vss object into your design. 

Please edit the parameters in the ``cfg`` file and provide it as input to the ``vss_fft_ifft_1d.mk`` file. An example of how to create a vss and include a .vss object in your design is also provided in L2/examples/vss_fft_ifft_1d/example.mk. It creates a vss object, links it to a larger system to create an xclbin and runs hardware emulation of the full design.

Device Support
==============

The VSS FFT can generate VSS products for both AIE and AIE-ML devices. The VSS is generated for the ``part`` that the user provides in the input cfg file. All features are supported on the two variants with the following differences:

- ``DATA_TYPE`` and ``TWIDDLE_TYPE``. AIE-ML does not support cfloat type.
- ``TWIDDLE_TYPE``: AIE supports cint32. AIE-ML does not.
- ``ROUND_MODE``: Supported round modes differ between AIE and AIE-ML devices as for all library elements.

Supported Parameters
====================

The complete list of supported parameters for the VSS FFT is shown in the L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_pararms.cfg. Please edit only the parameters in this file to configure the VSS FFT. Please see API reference on vss_fft_ifft_1d_graph.hpp for details on the AIE configurable parameters.

+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [Category] Parameter        | Description                                                                                                                                |
+=============================+============================================================================================================================================+
| part                        | Name of the part that the VSS should compile for                                                                                           |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| freqhz                      | Frequency of the internal PL components of the VSS (In Hz)                                                                                 |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [aie] enable_partition      | Configuration of the range of columns that you want to place the compiled AIE kernels. Please do not change the name of the aie partition. |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [aie] pl-freq               | Frequency of the PLIO kernels of the AIE-PL interface. (In MHz)                                                                            |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] DATA_TYPE      | Used to set TT_DATA described in API Reference                                                                                             |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] TWIDDLE_TYPE   | Used to set TT_TWIDDLE described in API Reference                                                                                          |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] POINT SIZE     | Used to set TP_POINT_SIZE described in API Reference                                                                                       |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] FFT_NIFFT      | Used to set TP_FFT_NIFFT described in API Reference                                                                                        |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] SHIFT          | Used to set TP_SHIFT described in API reference                                                                                            |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] API_IO         | Used to set TP_API described in API reference                                                                                              |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] ROUND_MODE     | Used to set TP_RND described in API reference                                                                                              |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] SAT_MODE       | Used to set TP_SAT described in API reference                                                                                              |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] Twiddle Mode   | Used to set TP_TWIDDLE_MODE described in API reference                                                                                     |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] SSR            | Used to set TP_SSR described in API reference                                                                                              |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+
| [APP_PARAMS] AIE_PLIO_WIDTH | Sets the PLIO width of the AIE-PL interface                                                                                                |
+-----------------------------+--------------------------------------------------------------------------------------------------------------------------------------------+

Design Notes
============

.. _VSS_SSR_OPERATION:

Super Sample Rate
------------------

The VSS FFT can be configured for Super Sample Rate operation to achieve higher throughputs. The design generates TP_SSR number of input and output ports.

The input data to the SSR input ports of the VSS FFT are expected to be distributed evenly in a "card-dealing" fashion. For example,

Port Number 1 gets samples 1, SSR+1, 2*SSR+1, ...
Port Number 2 gets samples 2, SSR+2, 2*SSR+2, ...
...
Port Number SSR gets samples SSR, SSR+SSR, 2*SSR+SSR, ...

.. _SSR_POINTSIZE_CONSTRAINTS:

Padding Input Data based on Super Sample Rate and Point Size
------------------------------------------------------------

If the point size is a multiple of SSR, then the inputs can be passed as is to the FFT. Otherwise every point size number of samples needs to be padded with zeros to the closest multiple of SSR before giving as input to the FFT. The output data also contains Point size number of valid data samples padded to the closest multiple of SSR.

