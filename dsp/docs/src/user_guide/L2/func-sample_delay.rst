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

_SAMPLE_DELAY:

======================
Sample Delay
======================

Sample Delay a circular buffer based implementation of a delay filter for introducing delay into a time series.

~~~~~~~~~~~
Entry Point
~~~~~~~~~~~

The graph entry point is the following:

.. code-block::

    xf::dsp::aie::sample_delay

~~~~~~~~~~~~~~~
Supported Types
~~~~~~~~~~~~~~~
The sample_delay supports unit8, int8, int16, cint16, int32, cint32, float and cfloat data types on the input.

~~~~~~~~~~~~~~~~~~~
Template Parameters
~~~~~~~~~~~~~~~~~~~

To see details on the template parameters for the sample_delay, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~~~~~
Access functions
~~~~~~~~~~~~~~~~

To see details on the access functions for the sample_delay, see :ref:`API_REFERENCE`.

~~~~~
Ports
~~~~~

To see details on the ports for the sample_delay, see :ref:`API_REFERENCE`.

~~~~~~~~~~~~
Design Notes
~~~~~~~~~~~~

Sample Delay introduces delay into the input data which is often a time series. The unit of delay is 'number of samples' which is passed on Run Time Parameter (RTP) port sampleDelayValue. The legal range of sampleDelayValue is [0, MAX_DELAY-1].
As far as the functionality is concerned, it is a delay filter, however implementation employs a vectorised circular buffer. The delay passed on sampleDelayValye RTP port is applied by converting it into two address offsets: vector offset and element offset.



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



