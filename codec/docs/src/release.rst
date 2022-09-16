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

.. _release_note:

Release Note
============

.. toctree::
   :hidden:
   :maxdepth: 1

Codec Library is an open-sourced library written in C/C++ for accelerating image coding, decoding and related processing algorithms. It now covers a level of acceleration: the module level(L1) and the pre-defined kernel level(L2). Currently 6 kinds of algorithms are accelerated. 

2022.1
----

The 2022.1 release provides a range of algorithms, includes:

1. JPEG decoding: one L2 API is provided for accelerating entire JPEG decoding process, which supports the ‘Sequential DCT-based mode’ of ISO/IEC 10918-1 standard. It can process 1 Huffman token and create up to 8 DCT coefficients within one cycle. It is also an easy-to-use decoder as it can directly parse the JPEG file header without help of software functions. In addition, L1 API is provided for Huffman decoding.
2. Pik encoding: 3 L2 APIs are provided for accelerating about 90% workload of lossy compression in Google’s pik. The pikEnc used the ‘fast mode’ of pik encoder which can provide better encoding efficiency than most of other still image encoding methods.
3. WebP encoding: 2 L2 APIs are provided for accelerating about 90% workload of lossy compression in WebP which is a popular image format developed by Google and supported in Chrome, Opera and Android, that is optimized to enable faster and smaller images on the Web.
4. Lepton encoding: the API ‘jpegDecLeptonEnc’ can be used for accelerating the encoding process for a new image format 'Lepton' developed by Dropbox. The format can save about 22% size of JPEG images losslessly.
5. JPEG-XL encoding: 3 L2 APIs are provided for accelerating the lossy encoding process of the JPEG XL Image Coding System (ISO/IEC 18181). Currently, not all computing intensive modules are offloaded, and more accelerating APIs will be available in feature.
6. Bicubic resizing: the L2 APIs 'resizeTop' is based on bicubic algorithm, which can take 1 or 8 input samples per cycle. When taking 8 samples, it can process 80 8K images per second. Although resizing is not a coding or encoding algorithm, it is widely used with image codecs in image transcoding applications.
