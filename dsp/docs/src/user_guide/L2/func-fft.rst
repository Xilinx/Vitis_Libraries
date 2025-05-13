..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FFT_IFFT:

========
FFT/iFFT
========

The DSPLib contains two different FFT/iFFT solutions. The first AI Engine-only implementation is recommended for point sizes less than or equal to 4096, or for configurations which do not require Super Sample Rate performance. The second AI Engine + PL implementation is recommended for larger point sizes and Super Sample Rate performance. While both support super-sample-rate, the AI Engine+PL uses resources more efficiently.

.. toctree::
   :maxdepth: 1

   FFT/IFFT 1CH (AI Engine-only) <func-fft-ifft-aie-only.rst>
   VSS FFT/IFFT 1CH (AI Engine + PL) <func-fft-vss.rst>


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:
