.. 
   .. Copyright © 2019-2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, RSA, Cryptography
   :description: RSA is a public-key cryptosystem. Its encryption key is public and different from decryption key. 
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

****************
RSA Cryptography
****************

.. toctree::
   :maxdepth: 1

RSA is a public-key cryptosystem. Its encryption key is public and different from decryption key. 
RSA cryptosystem includes key generation, key distribution, encryption/decryption and padding schemes. In this release, the encryption/decryption part is provided.

Implementation
==============

RSA key pair has three parts: modulus :math:`n` , encryption key exponent :math:`e`, decryption key exponent :math:`d`. 
Plain text is an unsigned integer :math:`m`, not larger than modulus and Cipher text is also an unsigned integer :math:`c`.

In Encryption,:

.. math::
   c = m^{e} \mod{n}

In Decryption we have:

.. math::
   m = c^{d} \mod{n}

Optimized Implementation on FPGA
=================================

Encryption is divided into two parts: updateKey and process. Each time you get a message with a new RSA key, call updateKey before getting into encryption/decryption. If you process messages with the same key continuously, only updateKey needs be called once at the beginning. 

Two implementations of function keydateKey are provided. One of them has two inputs: modulus and exponent. The other one has three inputs: modulus, exponent, and rMod. The extract argument "rMod" is actually 2^(2*N) mod modulus. This is a key parameter in the encryption/decryption calculation. If you have pre-calculated the arguments, call the second updateKey and set it up directly. If you do not have it, call the first one and do the calculation on chip with an extract resource.

RSA encryption and decryption are basically the same calculations: big integer modulus exponential calculation.
Instead of straight calculation, convert the big integer into its Montgomery field and do an exponential calculation. Finally, convert the result back to normal representation. In such cases, avoid most integer division and multiplication to save resource and have higher frequency.

Reference
========

Peter Montgomery. "Modular Multiplication Without Trial Division", Mathematics of Computation, vol. 44 no. 170, pp. 519–521, April 1985.

"Efficient architectures for implementing montgomery modular multiplication and RSA modular exponentiation" by Alan Daly, William Marnane
