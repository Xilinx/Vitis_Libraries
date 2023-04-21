..
   Copyright (C) 2019-2022, Xilinx, Inc.
   Copyright (C) 2022-2023, Advanced Micro Devices, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


.. _DDS_MIXER:

===========
DDS / Mixer
===========

The DSPLib contains two different DDS/Mixer implementations. These will be referred to as the dds_mixer and dds_mixer_lut in this document. They both differ in their implementations of the DDS and hence don't have identical outputs. The dds_mixer solution is available only on AIE while the dds_mixer_lut is available on both AIE and AIE-ML.
The dds_mixer_lut has modes that enable higher SFDR figures for the signal generator than the dds_mixer. Both implementations have three different modes of operation.
The mode of the component is driven by template parameter ``TP_MIXER_MODE``.

In DDS Only mode (``MIXER_MODE_0``), there is a single output port that contains the sin/cosine components corresponding to the programmed phase increment. The phase increment is a fixed uint32 value provided as a constructor argument, where 2^31 corresponds to Pi (180 degrees phase increment). The number of samples sent through the output port is determined by the TP_INPUT_WINDOW_SIZE parameter. The output port can be a window interface or a stream interface depending on the use of ``TP_API``.

Mixer inputs are enabled with the ``TP_MIXER_MODE`` template parameter. There are two modes that have the mixer functionality enabled.

- In ``MIXER_MODE_1``, a single input port is exposed and the input samples are complex multiplied by the DDS output for the given phase increment.
- In ``MIXER_MODE_2``, two input ports are exposed for multi-carrier operation, with the first behaving as in ``MIXER_MODE_1``, and the second input port getting complex multiplied with the complex conjugate of the DDS signal then accumulated to the result of the first complex multiply operation.

.. toctree::
   :maxdepth: 1

   DDS Mixer <func-dds_mixer_intrinsic.rst>
   DDS Mixer using lookup tables <func-dds_mixer_lut.rst>


.. |image1| image:: ./media/image1.png
.. |image2| image:: ./media/image2.png
.. |image3| image:: ./media/image4.png
.. |image4| image:: ./media/image2.png
.. |image6| image:: ./media/image2.png
.. |image7| image:: ./media/image5.png
.. |image8| image:: ./media/image6.png
.. |image9| image:: ./media/image7.png
.. |image10| image:: ./media/image2.png
.. |image11| image:: ./media/image2.png
.. |image12| image:: ./media/image2.png
.. |image13| image:: ./media/image2.png
.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:



