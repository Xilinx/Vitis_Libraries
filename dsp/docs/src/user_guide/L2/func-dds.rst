..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _DDS_MIXER:

=========
DDS/Mixer
=========

The DSPLib contains two different DDS/Mixer implementations. These will be referred to as the dds_mixer and dds_mixer_lut. They both differ in their implementations of the DDS and hence do not have identical outputs. The dds_mixer solution is available only on AIE, while the dds_mixer_lut is available on AIE, AIE-ML and AIE-MLv2. The dds_mixer_lut has modes that enable higher SFDR figures for the signal generator than the dds_mixer. Both implementations have three different modes of operation. The mode of the component is driven by template parameter ``TP_MIXER_MODE``.

In DDS Only mode (``MIXER_MODE_0``), there is a single output port that contains the sin/cosine components corresponding to the programmed phase increment. The phase increment is a fixed uint32 value provided as a constructor argument, where 2^31 corresponds to Pi (180 degrees phase increment). The number of samples sent through the output port is determined by the ``TP_INPUT_WINDOW_SIZE`` parameter. The output port can be a window interface or a stream interface depending on the use of ``TP_API``.

Mixer inputs are enabled with the ``TP_MIXER_MODE`` template parameter. There are two modes that have the mixer functionality enabled.

- In ``MIXER_MODE_1``, a single input port is exposed, and the input samples are complex multiplied by the DDS output for the given phase increment.
- In ``MIXER_MODE_2``, two input ports are exposed for multi-carrier operation, with the first behaving as in ``MIXER_MODE_1``, and the second input port getting complex multiplied with the complex conjugate of the DDS signal then accumulated to the result of the first complex multiply operation.

.. toctree::
   :maxdepth: 1

   DDS Mixer <func-dds_mixer_intrinsic.rst>
   DDS Mixer using lookup tables <func-dds_mixer_lut.rst>

.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: