..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _FFT_IFFT:

========
FFT/iFFT
========

The DSPLib contains two different FFT/iFFT solutions. The first AIE-only implementation is recommended for point sizes less than or equal to 4096, or for configurations which do not require Super Sample Rate performance. The second AIE+PL implementation is recommended for larger point sizes and Super Sample Rate performance. While both support super-sample-rate, the AIE+PL uses resources more efficiently. 

.. toctree::
   :maxdepth: 1

   FFT/IFFT 1CH (AIE-only) <func-fft-ifft-aie-only.rst>
   VSS FFT/IFFT 1CH (AIE + PL) <func-fft-vss.rst>


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
