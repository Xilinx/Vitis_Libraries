
.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, python, setup
   :description: Python environment setup guide.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials

Python Environment Setup Guide
===============================

**1. Installing Anaconda3**

1) Download Anaconda3.

.. code-block:: bash

  $ wget https://repo.anaconda.com/archive/Anaconda3-2019.03-Linux-x86_64.sh

2) Run the installer (the installer requires bzip; install it if you do not have it).

.. code-block:: bash

  $ bash ./Anaconda3-2019.03-Linux-x86_64.sh

Choose "Yes" for the question, "Do you wish the installer to initialize Anaconda3 by running conda init?". More information about Anaconda can be found in the `Anaconda Documentation`_.

.. _Anaconda Documentation: https://docs.anaconda.com/anaconda/

3) Add Anaconda3 to PATH, for example:

.. code-block:: bash

  $ export PATH=/home/<user>/anaconda3/bin:$PATH
  $ . /home/<user>/anaconda3/etc/profile.d/conda.sh


**2. Setting Up the xf_blas Environment to Include All Conda Packages Used by the xf_blas L1 Primitive Testing Infrastructure**

Run following command under the ``xf_blas/`` directory. 

.. code-block:: bash

  $ conda config --add channels anaconda
  $ conda env create -f environment.yml
  $ conda activate xf_blas
  $ conda install --file requirements.txt

**3. Deactivate the xf_blas Environment After Testing**

.. note:: Do not complete this step if you intend to run the L1 primitives' testing process. You only take it after you have finished all testing.

.. code-block:: bash

  $ conda deactivate
