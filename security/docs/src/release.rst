.. 
   .. Copyright © 2019–2026 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Vitis, Security, Library, Vitis Security Library, release, notes, release notes
   :description: Vitis Security Library release notes.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Known Issue
------------

2026.2
------

In this release, support for the following security-sensitive APIs is withdrawn:

* ChaCha20 / XChaCha20 stream cipher
* Poly1305 message authentication
* RC4 stream cipher
* RSA public-key encryption (``asymmetric.hpp``)
* DSA digital signature
* ``modularExp()`` in ``modular.hpp`` (secret-dependent exponentiation). ``addMod`` and ``monProduct`` remain available.
* Ed25519 (EdDSA) sign/verify
* Poseidon hash
* Verifiable Delay Function (VDF) evaluate/verify
* Host-side msgpack helper (``msgpack.hpp``)
* AES-GCM authenticated encryption (``gcm.hpp``): encrypt/decrypt for AES-128/192/256
* AES-GMAC message authentication (``gmac.hpp``): AES-128/192/256
* SM3 hash and SM4 block cipher (``sm234.hpp``): ``sm3()`` and ``sm4`` class

Other AES modes (CBC, CTR, OFB, XTS, CCM, etc.), SHA, and HMAC remain available.

2026.1
------

In this release, support for the following security-sensitive APIs is withdrawn:

* EC/ECDSA implementations for secp256k1, NIST P-256, and NIST P-384
* Generic ECC helper APIs
* BLS curve/signature support
* SM2 sign/verify support

SM3 and SM4 support remains available.

2022.2
------

In this release, support for the following functions is added:

* Poseidon Hash function
* ECDSA NIST-P384

2021.2
------

The 2021.2 release provides support for:

* KECCAK-256
* CRC32C
* NIST P-256

2021.1
------

The 2021.1 release provides support for:

* RIPEMD160
* Initial support for BLS (not complete)

2020.2
------

The 2020.2 release provides support for signatures algorithms and VDF functions.

* DSA
* ECC
* ECDSA
* EDDSA
* VDF

2019.2
------

The 2019.2 release provides a range of HLS primitives for:

* Symmetric Block Cipher
* Symmetric Stream Cipher
* Cipher Operation Mode
* Message Authentication code(MAC)
* Hash Function
* Asymmetric Cryptography
