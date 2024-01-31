.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FILTERS:

=======
Filters
=======

The DSPLib contains several variants of finite impulse response (FIR) filters. These include single-rate FIRs, half-band interpolation/decimation FIRs, as well as integer and fractional interpolation/decimation FIRs for AI Engine (AIE). AIE-ML devices have support for only the single rate FIRs. Differences in hardware lead to different constraints and implementations for FIRs in each device.

.. toctree::
   :maxdepth: 1

   FIRs on AIE <func-fir-filtersAIE.rst>
   FIRs on AIE-ML <func-fir-filters-aieml.rst>