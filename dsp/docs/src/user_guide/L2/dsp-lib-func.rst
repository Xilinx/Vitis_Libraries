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



.. _DSP_LIB_FUNC:

*********************
DSP Library Functions
*********************

| The Xilinx |reg| digital signal processing library (DSPLib) is a configurable library of elements that can be used to develop applications on Versal |reg| ACAP AI Engines. This is an Open Source library for DSP applications.
| The user entry point for each function in this library is a graph (L2 level).
| Each entry point graph class will contain one or more L1 level kernels and may contain one or more graph objects. Direct use of kernel classes (L1 level) or any other graph class not identified as an entry point is not recommended as this may bypass legality checking.

The DSPLib consists the following DSP elements:

.. toctree::
   :maxdepth: 2

   DDS / Mixer <func-dds.rst>
   FFT/iFFT <func-fft.rst>
   FFT Window <func-fft_window.rst>
   Filters <func-fir-filters.rst>
   Matrix Multiply <func-matmul.rst>
   Widget API Cast <func-widget-apicast.rst>
   Widget Real to Complex <func-widget-real2comp.rst>

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


