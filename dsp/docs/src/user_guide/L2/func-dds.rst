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


.. _DDS_MIXER:

===========
DDS / Mixer
===========

The DSPLib contains a DDS and Mixer solution.

In DDS Only mode, there is a single output port that contains the sin/cosine components corresponding to the programmed phase increment. The phase increment is a fixed uint32 value provided as a constructor argument, where 2^31 corresponds to Pi (180 degrees phase increment). The number of samples sent through the output port is determined by the TP_INPUT_WINDOW_SIZE parameter. The output port can be a window interface or a stream interface depending on the use of TP_API.

Mixer inputs are enabled with the TP_MIXER_MODE template parameter. There are two modes that have the mixer functionality enabled. In MIXER_MODE_1, a single input port is exposed and the input samples are complex multiplied by the DDS output for the given phase increment. In MIXER_MODE_2, two input ports are exposed for multi-carrier operation, with the first behaving as in MIXER_MODE_1, and the second input port getting complex multiplied with the complex conjugate of the DDS signal then accumulated to the result of the first complex multiply operation.

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::mixer::dds_mixer::dds_mixer_graph

~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~

The DDS/Mixer supports input types of cint16, cint32 and cfloat as selected by TT_DATA. Input is only required when TP_MIXER_MODE is
set to 1 (simple mixer) or 2 (dual conjugate mixer).
The output type is also set by TT_DATA. When TP_MIXER_MODE is set to 0 (DDS mode), TT_DATA types of cint16 or cfloat only are supported.

~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

To see details on the template parameters for the DDS/Mixer, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

To see details on the access functions for the DDS/Mixer, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~

To see details on the ports for the DDS/Mixer, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

Scaling
-------

When configured as a DDS (TP_MIXER_MODE=0) the output of the DDS is intended to be the components of a unit vector. For TT_DATA = cfloat, this means that the outputs will be in the range -1.0 to +1.0. For TT_DATA=cint16 the output is scaled by 2 to the power 15 such that the binary point follows the most significant bit of the output. Therefore, if the DDS output is used to multiply/mix, you must account for this 15 bit shift.

Super Sample Rate Operation
---------------------------

While the term Super Sample Rate strictly means the processing of more than one sample per clock cycle, in the AIE context it is taken to mean an implementation using parallel kernels to improve performance at the expense of additional resource use.
In the DDS/Mixer, SSR operation is controlled by the template parameter TP_SSR. Quite simply, the number of kernels used is set by TP_SSR and the performance increases proportionately.

Super Sample Rate Sample to Port Mapping
////////////////////////////////////////

When Super Sample Rate operation is used, data is input (where applicable) and output using multiple ports. These multiple ports on input or output act as one channel. The mapping of samples to ports is that each successive sample should be passed to a different port in a round-robin fashion, e.g. with TP_SSR set to 3, sample 0 should be sent to input port 0, sample 1 to input port 1, sample 2 to input port 2, sample 3 to input port 0 and so on.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Code Example including constraints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following code example shows how the DDS/Mixer graph class may be used within a user super-graph to use an instance configured as a mixer.

.. code-block::

  #include <adf.h>
  #include "dds_mixer_graph.hpp"
  #define DATA_TYPE cint16
  #define INPUT_WINDOW_VSIZE 1024
  #define MIXER_MODE 1
  #define API 0
  #define SSR 1

  class myMixer : public adf::graph
  {
  public:
    adf::port<input> in;
    adf::port<output> out;
    static constexpr unsigned int phaseInc = 0x12345678;
    static constexpr unsigned int initialPhaseOffset = 0x00001000;
    myMixer()
    {
      xf::dsp::aie::mixer::dds_mixer::dds_mixer_graph<DATA_TYPE, INPUT_WINDOW_VSIZE,
                                                      MIXER_MODE, API, SSR>
                                                      mixer(phaseInc, initialPhaseOffset);

      adf::connect<> net0(in , mixer.in1[0]);
      adf::connect<> net1(mixer.out[0] , out);
      adf::kernel *kernels = mixer.getKernels();
      adf::runtime<ratio>(*kernels) = 0.5;
    }
  };


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



