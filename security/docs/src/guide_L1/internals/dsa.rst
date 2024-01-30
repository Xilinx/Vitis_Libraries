.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, RSA, Cryptography
   :description: RSA is a public-key cryptosystem. Its encryption key is public and different from decryption key. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

****************************
Digital Signature Algorithm
****************************

.. toctree::
   :maxdepth: 1

Digital Signature Algorithm (DSA) is a public-key cryptosystem. It is used to generate and verify the digital signature.
Details of DSA could be found in FIPS.186-4, section 4.

Implementation
==============

DSA has two pair of functions: updateSigningParam and sign, updateVerifyingParam and verify. You also have two implementation for updateSigningParam and updateVerifyingParam, a trivial one and one with extra arguments. The extra argument is actually 2^(2*N) mod P. If you have the arguments pre-calculated, call the one with this argument. Otherwise, call the one without the arguments and calculate it on chip with an extra resource.

Optimized Implementation on FPGA
=================================

Like RSA, DSA also relies on modular exponential calculation. Adopt the same method and do the expensive modular exponential calculation on the Montgomery field and then convert it back to normal representation. In this way, you can eliminate most integer division and multiplication to save resource and have higher frequency.

Reference
========

Peter Montgomery. "Modular Multiplication Without Trial Division", Mathematics of Computation, vol. 44 no. 170, pp. 519–521, April 1985.

"Efficient architectures for implementing montgomery modular multiplication and RSA modular exponentiation" by Alan Daly, William Marnane

FIPS.186-4, section 4
