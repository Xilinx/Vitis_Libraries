.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*****************************
HMAC Algorithms
*****************************

.. toctree::
   :maxdepth: 1

HMAC is a message authentication code (MAC) using a hash function. It combines with any cryptographic hash function, for example, md5, sha1, sha256.
Hash function is wrapped to a class as one template parameter in HMAC and the wrapper class only has a static function involving the hash function.
HMAC uses the wrapper's hash function directly inside. The design makes combination HMAC algorithm with hash function more flexible. 
In xf_security lib, the key width (keyW), message width (msgW), key and message length width (lenW), hash value width (hshW) and block size of each hash function are list as below.

Configuration
=================================

============ ============ ============= ============ ============ ==================
    name      keyW(bits)    msgW(bits)   lenW(bits)   hshW(bits)   blockSize(bytes)
------------ ------------ ------------- ------------ ------------ ------------------
    md4         32             32           64           128            64
------------ ------------ ------------- ------------ ------------ ------------------
    md5         32             32           64           128            64
------------ ------------ ------------- ------------ ------------ ------------------
    sha1        32             32           64           160            64
------------ ------------ ------------- ------------ ------------ ------------------
    sha224      32             32           64           224            64
------------ ------------ ------------- ------------ ------------ ------------------
    sha256      32             32           64           256            64
------------ ------------ ------------- ------------ ------------ ------------------
    sha384      64             64           128          384            128
------------ ------------ ------------- ------------ ------------ ------------------
    sha512      64             64           128          512            128
============ ============ ============= ============ ============ ==================



Implementation
=======================

There are two implementations: in sequence and in parallel. If the macro `XF_SECURITY_DECRYPT_HMAC_DATAFLOW` is not defined (by default), HMAC will call the sequence version.
HMAC consists of 3 parts: compute kipad and kopad, `mhsh=hash(kipad+msg)`, `hash(kopad+msh)`.
kipad and kopad are derived from the input key. When the length of key is greater than hash's block size, `K=hash(key)`. kipad is `K XOR kip` while kopad is `K XOR kop`, in which kip is a constant consisting of repeated bytes valued 0x36 block size times and kop is repeating 0x5c.

.. image:: /images/hmac_detail.png
   :alt: hmac
   :width: 100%
   :align: center



Performance (Device: U250)
=================================

============ ===== ====== ====== ======= ====== ===== ====== ====== =======
    name      II    CLB    LUT     FF     DSP   BRAM   SRL   URAM   CP(ns)
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+md4     16    1639   7054   8663     0     8      3      0     3.211
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+md5     16    1560   7576   9524     0     8      3      0     2.934
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+sha1    16    1818   7171   12918    0     8      3      0     2.914
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+sha224  16    1881   7875   13622    0     8      3      0     3.080
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+sha256  16    1939   7971   13909    0     8      3      0     3.013 
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+sha384  16    3516   14577  25835    0     16     3      0     3.198 
------------ ----- ------ ------ ------- ------ ----- ------ ------ -------
 hmac+sha512  16    3537   14970  26995    0     16     3      0     3.176 
============ ===== ====== ====== ======= ====== ===== ====== ====== =======
