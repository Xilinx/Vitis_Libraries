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


.. _WIDGETS:

===============
Widget API Cast
===============

The DSPLib contains a Widget API Cast solution, which provides flexibilty when connecting other kernels. This component is able to change the stream interface to window interface and vice-versa. It may be configured to read two input stream interfaces and interleave data onto an output window interface. In addition, multiple copies of output window may be configured to allow extra flexibility when connecting to further kernels.

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::widget::api_cast::widget_api_cast_graph

~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~

The widget API cast supports int16, cint16, int32, cint32, float and cfloat types as selected by template parameter TT_DATA. This data type is used
by both input and output ports.

~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

To see details on the template parameters for the Widget API Cast, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

To see details on the access functions for the Widget API Cast, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~
To see details on the ports for the Widget API Cast, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

The widget_api_cast library element serves multiple purposes. Firstly, it can convert from window to stream or vice versa. Secondly, it can perform limited broadcast of windows. Thirdly it can perform various patterns of interlace when there are 2 streams in or out.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Code Example including constraints
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following code example shows how the widget_api_cast graph class may be used within a user super-graph, including how to set the runtime<ratio> of internal kernels. This example shows the widget configured to interlace two input streams on a sample-by-sample basis, with the output written to a window.

.. code-block::
.. literalinclude:: ../../../../L2/examples/docs_examples/test_widget_api_cast.hpp
    :language: cpp
    :lines: 15-




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



