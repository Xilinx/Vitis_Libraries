.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Finance, Library, Vitis Quantitative Finance Library, fintech
   :description: Vitis Quantitative Finance library licensing, trademark notice and hardware & software requirements.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


.. _overview:

.. toctree::
      :hidden:

Requirements
------------

Software Platform
~~~~~~~~~~~~~~~~~~~

This library is designed to work with AMD Vitis |trade| 2021.1, and therefore inherits the system requirements of Vitis and XRT.

Supported operating systems are RHEL/CentOS 7.4, 7.5 and Ubuntu 16.04.4 LTS, 18.04.1 LTS.
With CentOS/RHEL 7.4 and 7.5, C++11/C++14 should be enabled via
`devtoolset-6 <https://www.softwarecollections.org/en/scls/rhscl/devtoolset-6/>`_.

PCIE Accelerator Card
~~~~~~~~~~~~~~~~~~~~~~~

Hardware modules and kernels are designed to work with Xilinx Alveo cards. Requirements for specific engines or demonstrations will be documented in the associated subdirectory. Hardware builds for Alveo board targets require package installs as:
* `Alveo U200 <https://www.xilinx.com/products/boards-and-kits/alveo/u200.html#gettingStarted>`_
* `Alveo U250 <https://www.xilinx.com/products/boards-and-kits/alveo/u250.html#gettingStarted>`_
* `Alveo U50 <https://www.xilinx.com/products/boards-and-kits/alveo/u50.html#gettingStarted>`_ 

License
-------

Licensed using the `Apache 2.0 license <https://www.apache.org/licenses/LICENSE-2.0>`_.



Trademark Notice
----------------

    AMD, the AMD logo, Artix, ISE, Kintex, Spartan, Virtex, Zynq, and
    other designated brands included herein are trademarks of AMD in the
    United States and other countries.
    
    All other trademarks are the property of their respective owners.


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim: