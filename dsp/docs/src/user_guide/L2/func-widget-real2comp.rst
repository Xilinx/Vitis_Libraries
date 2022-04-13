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

.. _WIDGET_REAL2CMPLX:

======================
Widget Real to Complex
======================

The DSPLib contains a Widget Real to Complex solution, which provides a utility to convert real data to complex or vice versa.

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::widget::real2complex::widget_real2complex_graph

~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~
The widget_real2complex supports int16, cint16, int32, cint32, float and cfloat on input. The corresponding TT_OUT_DATA must be set to the
real or complex partner of the input type, e.g. if TT_DATA = int16, TT_OUT_DATA must be set to cint16.

~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

To see details on the template parameters for the Widget Real to Complex, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

To see details on the access functions for the Widget Real to Complex, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~

To see details on the ports for the Widget Real to Complex, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

The widget_real2complex library element converts real to complex or complex to real. An example of its use is that it can be used to enable real-only FFT operation despite the fact that the FFT currently supports only complex data types.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Code Example including constraints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following code example shows how the widget_real2complex graph class may be used within a user super-graph, including how to set the runtime<ratio> of internal kernels. This example shows the widget configured to convert a window of int16 samples to cint16 samples.

.. code-block::

  #include <adf.h>
  #include "widget_real2complex_graph.hpp"
  #define DATA_TYPE int16
  #define DATA_OUT_TYPE cint16
  #define WINDOW_VSIZE 1024

  class myWidget : public adf::graph
  {
  public:
    adf::port<input> in;
    adf::port<output> out;
    xf::dsp::aie::widget::real2complex::widget_real2complex_graph<DATA_TYPE, DATA_OUT_TYPE, WINDOW_VSIZE> widget;
    myWidget()
    {
      adf::connect<> net0(in , widget.in);
      adf::connect<> net1(widget.out , out);
      adf::kernel *kernels = widget.getKernels();
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



